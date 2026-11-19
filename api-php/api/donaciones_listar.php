<?php
// GET /api/donaciones_listar.php?id_donante=1
// Historial de donaciones del donante (últimas 100), con quién extrajo y si fue una excepción.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/donaciones_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_VER_DONACION);

    $idDonante = (int)($_GET['id_donante'] ?? 0);
    if ($idDonante <= 0) {
        responder(['ok' => false, 'error' => 'Falta el donante (id_donante).'], 400);
    }
    if (obtener_donante($pdo, $idDonante) === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }

    $st = $pdo->prepare(
        'SELECT d.id_donacion, d.numero_donacion, d.fecha_donacion, t.nombre AS tipo_donacion,
                d.cantidad_plaquetas, d.cantidad_plasma, d.peso_donante, d.incidencias,
                d.es_excepcion, d.motivo_excepcion,
                ue.nombre_completo AS extractora, ua.nombre_completo AS autorizada_por
         FROM donaciones d
         JOIN tipos_donacion t ON t.id_tipo_donacion = d.id_tipo_donacion
         JOIN usuarios ue ON ue.id_usuario = d.id_usuario_extractora
         LEFT JOIN usuarios ua ON ua.id_usuario = d.id_usuario_autoriza
         WHERE d.id_donante = :d
         ORDER BY d.fecha_donacion DESC, d.id_donacion DESC
         LIMIT 100'
    );
    $st->execute([':d' => $idDonante]);
    $filas = $st->fetchAll();
    foreach ($filas as &$f) {
        $f['id_donacion'] = (int)$f['id_donacion'];
        $f['cantidad_plaquetas'] = (int)$f['cantidad_plaquetas'];
        $f['cantidad_plasma'] = (int)$f['cantidad_plasma'];
        $f['peso_donante'] = $f['peso_donante'] !== null ? (float)$f['peso_donante'] : null;
        $f['es_excepcion'] = (bool)$f['es_excepcion'];
    }
    unset($f);

    responder(['ok' => true, 'donaciones' => $filas]);
} catch (Throwable $e) {
    error_log('donaciones_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
