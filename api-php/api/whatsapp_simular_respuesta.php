<?php
// POST /api/whatsapp_simular_respuesta.php   Cuerpo: {"id_notificacion": 3, "texto": "No puedo ir, estoy resfriado"}
// SOLO en modo simulado: hace de cuenta que el donante respondió el mensaje, para probar y demostrar la pantalla
// de respuestas. Con un servicio real, las respuestas llegan solas y esto queda deshabilitado.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/whatsapp_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_WHATSAPP);

    if (parametro_texto($pdo, 'whatsapp_modo', 'simulado') !== 'simulado') {
        responder(['ok' => false, 'error' => 'La simulación solo está disponible en modo simulado.'], 409);
    }
    $in = leer_json();
    $id = (int)($in['id_notificacion'] ?? 0);
    $texto = mb_substr(trim((string)($in['texto'] ?? '')), 0, 500);
    if ($id <= 0 || $texto === '') {
        responder(['ok' => false, 'error' => 'Se necesita id_notificacion y el texto de la respuesta.'], 400);
    }
    if (!registrar_respuesta($pdo, $id, $texto)) {
        responder(['ok' => false, 'error' => 'No existe un recordatorio enviado con ese número.'], 404);
    }
    responder(['ok' => true, 'mensaje' => 'Respuesta registrada: queda pendiente de atender.']);
} catch (Throwable $e) {
    error_log('whatsapp_simular_respuesta.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
