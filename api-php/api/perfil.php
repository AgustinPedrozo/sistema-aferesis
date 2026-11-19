<?php
// GET /api/perfil.php  (con encabezado X-Auth-Token). Sirve para probar que el token funciona.
declare(strict_types=1);
require __DIR__ . '/auth.php';

try {
    $usuario = requerir_sesion(conectar());
    responder(['ok' => true, 'usuario' => $usuario]);
} catch (Throwable $e) {
    error_log('perfil.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
