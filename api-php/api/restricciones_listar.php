<?php
// GET /api/restricciones_listar.php
// Catálogo de restricciones (causas de diferimiento) activas, para elegir en la entrevista.
declare(strict_types=1);
require __DIR__ . '/auth.php';

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $filas = $pdo->query(
        'SELECT id_restriccion, nombre, tipo, dias_espera_minimos, dias_espera_maximos,
                requiere_reevaluacion, observaciones
         FROM restricciones_medicas WHERE activa = TRUE ORDER BY nombre'
    )->fetchAll();

    foreach ($filas as &$f) {
        $f['id_restriccion'] = (int)$f['id_restriccion'];
        $f['dias_espera_minimos'] = $f['dias_espera_minimos'] !== null ? (int)$f['dias_espera_minimos'] : null;
        $f['dias_espera_maximos'] = $f['dias_espera_maximos'] !== null ? (int)$f['dias_espera_maximos'] : null;
        $f['requiere_reevaluacion'] = (bool)$f['requiere_reevaluacion'];
    }
    unset($f);

    responder(['ok' => true, 'restricciones' => $filas]);
} catch (Throwable $e) {
    error_log('restricciones_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
