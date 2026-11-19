<?php
// POST /api/turno_estado.php   Cuerpo: {"id_turno": 1, "estado": "Cancelado"}  (o "Ausente")
// Solo se pueden cancelar o marcar como ausentes los turnos pendientes.
// "Completado" lo marca el sistema cuando se registra la donación, no se hace desde acá.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/turnos_comun.php';

solo_post();

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $entrada = leer_json();
    $id = (int)($entrada['id_turno'] ?? 0);
    $nuevo = (string)($entrada['estado'] ?? '');
    if ($id <= 0 || !in_array($nuevo, ['Cancelado', 'Ausente'], true)) {
        responder(['ok' => false, 'error' => 'Datos incompletos: se necesita id_turno y estado (Cancelado o Ausente).'], 400);
    }

    $turno = obtener_turno($pdo, $id);
    if ($turno === null) {
        responder(['ok' => false, 'error' => 'No se encontró el turno.'], 404);
    }
    if ($turno['estado'] !== 'Pendiente') {
        responder(['ok' => false, 'error' => 'Solo se puede modificar un turno pendiente.'], 409);
    }

    if ($nuevo === 'Ausente') {
        $st = $pdo->prepare('SELECT CAST(:fh AS DATETIME) <= NOW()');
        $st->execute([':fh' => $turno['fecha_hora']]);
        if ((int)$st->fetchColumn() === 0) {
            responder(['ok' => false, 'error' => 'Todavía no pasó la hora del turno: no se puede marcar como ausente.'], 409);
        }
    }

    $pdo->prepare('UPDATE turnos SET estado = :e WHERE id_turno = :id')
        ->execute([':e' => $nuevo, ':id' => $id]);

    $mensaje = $nuevo === 'Cancelado' ? 'Turno cancelado.' : 'Turno marcado como ausente.';
    responder(['ok' => true, 'mensaje' => $mensaje, 'turno' => obtener_turno($pdo, $id)]);
} catch (Throwable $e) {
    error_log('turno_estado.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
