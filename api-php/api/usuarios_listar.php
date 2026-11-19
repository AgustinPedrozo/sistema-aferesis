<?php
// GET /api/usuarios_listar.php   (solo Administrador)
// Devuelve { ok, usuarios:[{id_usuario, nombre_usuario, nombre_completo, id_rol, rol, activo, bloqueado}], roles:[{id_rol, nombre}] }
// Nunca devuelve claves ni hashes.
declare(strict_types=1);
require __DIR__ . '/auth.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ['Administrador']);

    $usuarios = $pdo->query(
        'SELECT u.id_usuario, u.nombre_usuario, u.nombre_completo, u.id_rol, r.nombre AS rol, u.activo,
                (u.bloqueado_hasta IS NOT NULL AND u.bloqueado_hasta > NOW()) AS bloqueado
         FROM usuarios u
         JOIN roles r ON r.id_rol = u.id_rol
         ORDER BY u.nombre_usuario'
    )->fetchAll();
    foreach ($usuarios as &$u) {
        $u['id_usuario'] = (int)$u['id_usuario'];
        $u['id_rol'] = (int)$u['id_rol'];
        $u['activo'] = (bool)$u['activo'];
        $u['bloqueado'] = (bool)$u['bloqueado'];
    }
    unset($u);

    $roles = $pdo->query('SELECT id_rol, nombre FROM roles ORDER BY id_rol')->fetchAll();
    foreach ($roles as &$r) {
        $r['id_rol'] = (int)$r['id_rol'];
    }
    unset($r);

    responder(['ok' => true, 'usuarios' => $usuarios, 'roles' => $roles]);
} catch (Throwable $e) {
    error_log('usuarios_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
