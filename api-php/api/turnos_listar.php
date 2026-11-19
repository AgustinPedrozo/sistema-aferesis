<?php
// GET /api/turnos_listar.php?fecha=2026-10-21                      (agenda de un día)
// GET /api/turnos_listar.php?fecha=2026-10-21&hasta=2026-10-25     (rango de días)
// Opcional: &estado=Pendiente|Completado|Cancelado|Ausente
// Sin "fecha" se usa el día de hoy.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/turnos_comun.php';

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $hoy = (string)$pdo->query('SELECT CURDATE()')->fetchColumn();
    $desde = trim((string)($_GET['fecha'] ?? ''));
    if ($desde === '') {
        $desde = $hoy;
    }
    $hasta = trim((string)($_GET['hasta'] ?? ''));
    if ($hasta === '') {
        $hasta = $desde;
    }
    if (!validar_fecha($desde) || !validar_fecha($hasta) || $hasta < $desde) {
        responder(['ok' => false, 'error' => 'Las fechas no son válidas (formato AAAA-MM-DD).'], 400);
    }

    $sql = SELECT_TURNO . ' WHERE t.fecha_hora >= :desde
            AND t.fecha_hora < DATE_ADD(CAST(:hasta AS DATE), INTERVAL 1 DAY)';
    $params = [':desde' => $desde, ':hasta' => $hasta];

    $estado = (string)($_GET['estado'] ?? '');
    if ($estado !== '') {
        if (!in_array($estado, ESTADOS_TURNO, true)) {
            responder(['ok' => false, 'error' => 'Estado no válido.'], 400);
        }
        $sql .= ' AND t.estado = :estado';
        $params[':estado'] = $estado;
    }
    $sql .= ' ORDER BY t.fecha_hora, d.apellido, d.nombre LIMIT 500';

    $st = $pdo->prepare($sql);
    $st->execute($params);
    $turnos = array_map('normalizar_turno', $st->fetchAll());

    responder(['ok' => true, 'desde' => $desde, 'hasta' => $hasta, 'turnos' => $turnos]);
} catch (Throwable $e) {
    error_log('turnos_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
