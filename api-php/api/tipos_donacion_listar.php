<?php
// GET /api/tipos_donacion_listar.php
// Tipos de donación habilitados (para llenar la lista desplegable de la pantalla de turnos).
declare(strict_types=1);
require __DIR__ . '/auth.php';

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $filas = $pdo->query(
        'SELECT id_tipo_donacion, nombre FROM tipos_donacion WHERE activo = TRUE ORDER BY id_tipo_donacion'
    )->fetchAll();
    foreach ($filas as &$f) {
        $f['id_tipo_donacion'] = (int)$f['id_tipo_donacion'];
    }
    unset($f);

    responder(['ok' => true, 'tipos' => $filas]);
} catch (Throwable $e) {
    error_log('tipos_donacion_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
