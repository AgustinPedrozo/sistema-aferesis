<?php
// POST /api/whatsapp_enviar_ahora.php   (sin cuerpo)
// Ejecuta ahora el mismo proceso que corre solo cada pocos minutos: envía los recordatorios que correspondan.
// Sirve para probar y para demostrar el sistema sin esperar al programador de tareas.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/whatsapp_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_WHATSAPP);

    $r = procesar_recordatorios($pdo);
    $r['ok'] = true;
    $r['modo'] = parametro_texto($pdo, 'whatsapp_modo', 'simulado');
    $r['mensaje'] = $r['revisados'] === 0
        ? 'No hay turnos que necesiten recordatorio en este momento.'
        : 'Recordatorios enviados: ' . $r['enviados'] . '. Con error: ' . $r['con_error'] . '.';
    responder($r);
} catch (Throwable $e) {
    error_log('whatsapp_enviar_ahora.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
