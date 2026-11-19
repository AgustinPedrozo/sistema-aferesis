<?php
// GET /api/donante_restricciones_listar.php?id_donante=1                  (todas, con historial)
// GET /api/donante_restricciones_listar.php?id_donante=1&solo_vigentes=1  (solo las que siguen vigentes)
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/entrevistas_comun.php';

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $idDonante = (int)($_GET['id_donante'] ?? 0);
    if ($idDonante <= 0) {
        responder(['ok' => false, 'error' => 'Falta el donante (id_donante).'], 400);
    }
    if (obtener_donante($pdo, $idDonante) === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }

    $soloVigentes = (($_GET['solo_vigentes'] ?? '') === '1');
    responder(['ok' => true, 'restricciones' => listar_restricciones_donante($pdo, $idDonante, $soloVigentes)]);
} catch (Throwable $e) {
    error_log('donante_restricciones_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
