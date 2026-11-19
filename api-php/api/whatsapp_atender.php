<?php
// POST /api/whatsapp_atender.php   Cuerpo: {"id_notificacion": 3}
// Marca la respuesta de un donante como atendida por el usuario que está usando el sistema.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/whatsapp_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_WHATSAPP);

    $id = (int)(leer_json()['id_notificacion'] ?? 0);
    if ($id <= 0) {
        responder(['ok' => false, 'error' => 'Falta id_notificacion.'], 400);
    }
    $st = $pdo->prepare(
        'UPDATE notificaciones_whatsapp SET atendido = TRUE, id_usuario_atiende = :u
         WHERE id_notificacion = :id AND respuesta_recibida IS NOT NULL AND atendido = FALSE'
    );
    $st->execute([':u' => $usuario['id_usuario'], ':id' => $id]);
    if ($st->rowCount() === 0) {
        responder(['ok' => false, 'error' => 'No hay una respuesta pendiente con ese número (quizás ya fue atendida).'], 409);
    }
    responder(['ok' => true, 'mensaje' => 'Respuesta marcada como atendida.']);
} catch (Throwable $e) {
    error_log('whatsapp_atender.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
