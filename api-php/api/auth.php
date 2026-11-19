<?php
// Funciones para proteger los demás endpoints. Uso:
//   require __DIR__ . '/auth.php';
//   $pdo = conectar();
//   $u = requerir_sesion($pdo);
//   requerir_rol($u, ['Jefa de Área/Gerente']);   // opcional
declare(strict_types=1);
require_once __DIR__ . '/config.php';

/** Valida el token del encabezado X-Auth-Token y devuelve el usuario. Corta con 401 si no sirve. */
function requerir_sesion(PDO $pdo): array
{
    $token = $_SERVER['HTTP_X_AUTH_TOKEN'] ?? '';
    if ($token === '') {
        responder(['ok' => false, 'error' => 'Sesión requerida'], 401);
    }

    $st = $pdo->prepare(
        'SELECT u.id_usuario, u.nombre_usuario, u.nombre_completo, r.nombre AS rol
         FROM sesiones s
         JOIN usuarios u ON u.id_usuario = s.id_usuario
         JOIN roles r ON r.id_rol = u.id_rol
         WHERE s.token_hash = :h AND s.fecha_expiracion > NOW() AND u.activo = TRUE'
    );
    $st->execute([':h' => hash('sha256', $token)]);
    $usuario = $st->fetch();

    if (!$usuario) {
        responder(['ok' => false, 'error' => 'Sesión inválida o vencida'], 401);
    }
    return $usuario;
}

/** Corta con 403 si el rol del usuario no está entre los permitidos. */
function requerir_rol(array $usuario, array $rolesPermitidos): void
{
    if (!in_array($usuario['rol'], $rolesPermitidos, true)) {
        responder(['ok' => false, 'error' => 'No tiene permiso para esta acción'], 403);
    }
}
