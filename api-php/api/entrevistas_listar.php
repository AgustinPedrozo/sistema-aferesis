<?php
// GET /api/entrevistas_listar.php?id_donante=1
// Últimas 50 entrevistas médicas del donante (datos médicos: solo roles de entrevista).
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/entrevistas_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_ENTREVISTA);

    $idDonante = (int)($_GET['id_donante'] ?? 0);
    if ($idDonante <= 0) {
        responder(['ok' => false, 'error' => 'Falta el donante (id_donante).'], 400);
    }
    if (obtener_donante($pdo, $idDonante) === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }

    $st = $pdo->prepare(
        'SELECT e.id_entrevista, e.fecha_entrevista, e.apto, e.recuento_plaquetas,
                e.peso_kg, e.altura_cm, e.observaciones, u.nombre_completo AS entrevistador
         FROM entrevistas_medicas e
         JOIN usuarios u ON u.id_usuario = e.id_usuario_entrevistador
         WHERE e.id_donante = :d
         ORDER BY e.fecha_entrevista DESC, e.id_entrevista DESC
         LIMIT 50'
    );
    $st->execute([':d' => $idDonante]);
    $filas = $st->fetchAll();

    foreach ($filas as &$f) {
        $f['id_entrevista'] = (int)$f['id_entrevista'];
        $f['apto'] = (bool)$f['apto'];
        $f['recuento_plaquetas'] = $f['recuento_plaquetas'] !== null ? (int)$f['recuento_plaquetas'] : null;
        $f['peso_kg'] = $f['peso_kg'] !== null ? (float)$f['peso_kg'] : null;
        $f['altura_cm'] = $f['altura_cm'] !== null ? (float)$f['altura_cm'] : null;
    }
    unset($f);

    responder(['ok' => true, 'entrevistas' => $filas]);
} catch (Throwable $e) {
    error_log('entrevistas_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
