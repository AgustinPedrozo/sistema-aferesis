<?php
// POST /api/usuario_guardar.php   (solo Administrador)
// Alta:         { "nombre_usuario":"dc", "nombre_completo":"Ana Perez", "clave":"1234", "id_rol":1 }
// Modificación: { "id_usuario":7, "nombre_completo":"...", "id_rol":2, "activo":true, "clave":"" }
//               "clave" vacía o ausente = no se cambia. Si se cambia, también se desbloquea la cuenta.
declare(strict_types=1);
require __DIR__ . '/auth.php';

solo_post();

function falla(string $mensaje, int $codigo = 422): void
{
    responder(['ok' => false, 'error' => $mensaje], $codigo);
}

try {
    $pdo = conectar();
    $admin = requerir_sesion($pdo);
    requerir_rol($admin, ['Administrador']);

    $d = leer_json();
    $id = (int)($d['id_usuario'] ?? 0);
    $nombreCompleto = trim((string)($d['nombre_completo'] ?? ''));
    $clave = trim((string)($d['clave'] ?? ''));
    $idRol = (int)($d['id_rol'] ?? 0);

    if (mb_strlen($nombreCompleto) < 3 || mb_strlen($nombreCompleto) > 100) {
        falla('El nombre completo debe tener entre 3 y 100 caracteres.');
    }
    $st = $pdo->prepare('SELECT nombre FROM roles WHERE id_rol = :r');
    $st->execute([':r' => $idRol]);
    $nombreRol = $st->fetchColumn();
    if ($nombreRol === false) {
        falla('Elija un rol de la lista.');
    }
    if ($clave !== '' && !preg_match('/^\d{4,10}$/', $clave)) {
        falla('La clave debe ser un número de 4 a 10 dígitos.');
    }

    // ---------------- ALTA ----------------
    if ($id === 0) {
        $nombreUsuario = strtolower(trim((string)($d['nombre_usuario'] ?? '')));
        if (!preg_match('/^[a-z0-9]{2,20}$/', $nombreUsuario)) {
            falla('El usuario debe tener de 2 a 20 letras o números, sin espacios ni acentos.');
        }
        if ($clave === '') {
            falla('Para crear un usuario hay que indicar una clave.');
        }
        $st = $pdo->prepare('SELECT COUNT(*) FROM usuarios WHERE nombre_usuario = :u');
        $st->execute([':u' => $nombreUsuario]);
        if ((int)$st->fetchColumn() > 0) {
            falla('Ya existe un usuario con esas iniciales. Elija otras.', 409);
        }
        $pdo->prepare(
            'INSERT INTO usuarios (nombre_usuario, clave_hash, nombre_completo, id_rol)
             VALUES (:u, :h, :n, :r)'
        )->execute([
            ':u' => $nombreUsuario,
            ':h' => password_hash($clave, PASSWORD_DEFAULT),
            ':n' => $nombreCompleto,
            ':r' => $idRol,
        ]);
        error_log("usuario_guardar.php: {$admin['nombre_usuario']} creó a $nombreUsuario ($nombreRol)");
        responder(['ok' => true, 'mensaje' => "Usuario '$nombreUsuario' creado."], 201);
    }

    // ---------------- MODIFICACION ----------------
    $activo = array_key_exists('activo', $d) ? (bool)$d['activo'] : true;

    $pdo->beginTransaction();
    try {
        // Se bloquean todos los administradores para que dos cambios a la vez no dejen el sistema sin ninguno
        $admins = $pdo->query(
            "SELECT u.id_usuario FROM usuarios u JOIN roles r ON r.id_rol = u.id_rol
             WHERE r.nombre = 'Administrador' AND u.activo = TRUE FOR UPDATE"
        )->fetchAll(PDO::FETCH_COLUMN);

        $st = $pdo->prepare(
            'SELECT u.id_usuario, u.nombre_usuario, u.id_rol, u.activo, r.nombre AS rol
             FROM usuarios u JOIN roles r ON r.id_rol = u.id_rol
             WHERE u.id_usuario = :id FOR UPDATE'
        );
        $st->execute([':id' => $id]);
        $actual = $st->fetch();
        if (!$actual) {
            $pdo->rollBack();
            falla('Ese usuario no existe.', 404);
        }

        $esYo = (int)$actual['id_usuario'] === (int)$admin['id_usuario'];
        if ($esYo && (!$activo || $idRol !== (int)$actual['id_rol'])) {
            $pdo->rollBack();
            falla('No puede desactivar su propio usuario ni cambiarse el rol. Hágalo con otro administrador.');
        }
        $eraAdminActivo = $actual['rol'] === 'Administrador' && (bool)$actual['activo'];
        $seguiraAdminActivo = $nombreRol === 'Administrador' && $activo;
        if ($eraAdminActivo && !$seguiraAdminActivo && count($admins) <= 1) {
            $pdo->rollBack();
            falla('Tiene que quedar al menos un administrador activo.');
        }

        $sql = 'UPDATE usuarios SET nombre_completo = :n, id_rol = :r, activo = :a';
        $par = [':n' => $nombreCompleto, ':r' => $idRol, ':a' => $activo ? 1 : 0, ':id' => $id];
        if ($clave !== '') {
            $sql .= ', clave_hash = :h, intentos_fallidos = 0, bloqueado_hasta = NULL';
            $par[':h'] = password_hash($clave, PASSWORD_DEFAULT);
        }
        $pdo->prepare($sql . ' WHERE id_usuario = :id')->execute($par);

        // Si cambió la clave, el rol o se desactivó, se cierran sus sesiones abiertas (menos la actual si es uno mismo)
        $cambioSeguridad = $clave !== '' || !$activo || $idRol !== (int)$actual['id_rol'];
        if ($cambioSeguridad) {
            $sqlSes = 'DELETE FROM sesiones WHERE id_usuario = :id';
            $parSes = [':id' => $id];
            if ($esYo) {
                $sqlSes .= ' AND token_hash <> :h';
                $parSes[':h'] = hash('sha256', $_SERVER['HTTP_X_AUTH_TOKEN'] ?? '');
            }
            $pdo->prepare($sqlSes)->execute($parSes);
        }
        $pdo->commit();
    } catch (Throwable $e) {
        if ($pdo->inTransaction()) {
            $pdo->rollBack();
        }
        throw $e;
    }

    error_log("usuario_guardar.php: {$admin['nombre_usuario']} modificó a {$actual['nombre_usuario']}");
    responder(['ok' => true, 'mensaje' => "Usuario '{$actual['nombre_usuario']}' actualizado."]);
} catch (Throwable $e) {
    error_log('usuario_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
