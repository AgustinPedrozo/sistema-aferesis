<?php
// GET /api/whatsapp_respuestas_listar.php                 -> respuestas pendientes de atender (las más viejas primero)
// GET /api/whatsapp_respuestas_listar.php?todos=1         -> últimos 100 mensajes enviados, con o sin respuesta
// Siempre incluye "total_pendientes" (para el contador de la pantalla principal).
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/whatsapp_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_WHATSAPP);

    $todos = ($_GET['todos'] ?? '') === '1';
    $pendiente = 'n.respuesta_recibida IS NOT NULL AND n.atendido = FALSE';

    $sql = 'SELECT n.id_notificacion, n.id_donante, n.id_turno, d.apellido, d.nombre, d.telefono,
                   n.fecha_hora_turno, n.mensaje_enviado, n.estado_envio, n.fecha_envio, n.modo_envio, n.detalle_envio,
                   n.respuesta_recibida, n.fecha_respuesta, n.atendido, ua.nombre_completo AS atendido_por
            FROM notificaciones_whatsapp n
            JOIN donantes d ON d.id_donante = n.id_donante
            LEFT JOIN usuarios ua ON ua.id_usuario = n.id_usuario_atiende '
        . ($todos ? 'ORDER BY n.fecha_envio DESC, n.id_notificacion DESC LIMIT 100'
                  : "WHERE $pendiente ORDER BY n.fecha_respuesta ASC LIMIT 200");
    $filas = $pdo->query($sql)->fetchAll();
    foreach ($filas as &$f) {
        $f['id_notificacion'] = (int)$f['id_notificacion'];
        $f['id_donante'] = (int)$f['id_donante'];
        $f['id_turno'] = $f['id_turno'] !== null ? (int)$f['id_turno'] : null;
        $f['atendido'] = (bool)$f['atendido'];
    }
    unset($f);

    $total = (int)$pdo->query("SELECT COUNT(*) FROM notificaciones_whatsapp n WHERE $pendiente")->fetchColumn();
    responder(['ok' => true, 'total_pendientes' => $total, 'mensajes' => $filas]);
} catch (Throwable $e) {
    error_log('whatsapp_respuestas_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
