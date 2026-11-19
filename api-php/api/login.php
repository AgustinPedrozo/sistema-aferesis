<?php
// POST /api/login.php   Cuerpo: {"usuario": "cc", "clave": "1234"}
// OK: {"ok": true, "token": "...", "usuario": {...}}
declare(strict_types=1);
require __DIR__ . '/config.php';

solo_post();

$entrada = leer_json();
$usuario = trim((string)($entrada['usuario'] ?? ''));
$clave = (string)($entrada['clave'] ?? '');

if ($usuario === '' || $clave === '') {
    responder(['ok' => false, 'error' => 'Usuario y clave son obligatorios'], 400);
}

try {
    $pdo = conectar();

    $st = $pdo->prepare(
        'SELECT u.id_usuario, u.nombre_usuario, u.clave_hash, u.nombre_completo,
                r.nombre AS rol,
                (u.bloqueado_hasta IS NOT NULL AND u.bloqueado_hasta > NOW()) AS bloqueado
         FROM usuarios u
         JOIN roles r ON r.id_rol = u.id_rol
         WHERE u.nombre_usuario = :u AND u.activo = TRUE'
    );
    $st->execute([':u' => $usuario]);
    $fila = $st->fetch();

    if ($fila && (int)$fila['bloqueado'] === 1) {
        responder(['ok' => false, 'error' => 'Cuenta bloqueada temporalmente por intentos fallidos. Intente más tarde.'], 429);
    }

    // Se verifica siempre contra un hash, exista o no el usuario, para que el tiempo
    // de respuesta no revele qué usuarios existen.
    $hash = $fila['clave_hash'] ?? password_hash('relleno', PASSWORD_DEFAULT);
    $correcta = password_verify($clave, $hash);

    if (!$fila || !$correcta) {
        if ($fila) {
            $max = (int)MAX_INTENTOS;
            $min = (int)MINUTOS_BLOQUEO;
            $pdo->prepare(
                "UPDATE usuarios
                 SET bloqueado_hasta = IF(intentos_fallidos + 1 >= $max, DATE_ADD(NOW(), INTERVAL $min MINUTE), bloqueado_hasta),
                     intentos_fallidos = IF(intentos_fallidos + 1 >= $max, 0, intentos_fallidos + 1)
                 WHERE id_usuario = :id"
            )->execute([':id' => $fila['id_usuario']]);
        }
        responder(['ok' => false, 'error' => 'Usuario o clave incorrectos'], 401);
    }

    // Login correcto: se limpian los intentos y se crea la sesión.
    $pdo->prepare('UPDATE usuarios SET intentos_fallidos = 0, bloqueado_hasta = NULL WHERE id_usuario = :id')
        ->execute([':id' => $fila['id_usuario']]);
    $pdo->exec('DELETE FROM sesiones WHERE fecha_expiracion < NOW()');

    $token = bin2hex(random_bytes(32));
    $horas = (int)HORAS_SESION;
    $pdo->prepare(
        "INSERT INTO sesiones (id_usuario, token_hash, fecha_expiracion)
         VALUES (:id, :h, DATE_ADD(NOW(), INTERVAL $horas HOUR))"
    )->execute([':id' => $fila['id_usuario'], ':h' => hash('sha256', $token)]);

    responder([
        'ok' => true,
        'token' => $token,
        'usuario' => [
            'id' => (int)$fila['id_usuario'],
            'nombre_usuario' => $fila['nombre_usuario'],
            'nombre_completo' => $fila['nombre_completo'],
            'rol' => $fila['rol'],
        ],
    ]);
} catch (Throwable $e) {
    error_log('login.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
