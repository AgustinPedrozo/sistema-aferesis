<?php
// Funciones compartidas por los endpoints de WhatsApp (no se llama directamente desde la app).
declare(strict_types=1);
require_once __DIR__ . '/config.php';
require_once __DIR__ . '/entrevistas_comun.php';   // parametro_int()

// Quiénes ven y gestionan los mensajes
const ROLES_WHATSAPP = ['Doctora', 'Entrevistadora/Administrativo', 'Jefa de Área/Gerente', 'Administrador'];

function parametro_texto(PDO $pdo, string $clave, string $porDefecto): string
{
    $st = $pdo->prepare('SELECT valor FROM parametros_configuracion WHERE clave = :c');
    $st->execute([':c' => $clave]);
    $v = $st->fetchColumn();
    return ($v === false || $v === null || $v === '') ? $porDefecto : (string)$v;
}

/**
 * Teléfono argentino -> formato internacional de WhatsApp (549 + área + número, 13 dígitos).
 * Acepta "223 15 456-7890", "(0223) 456 7890", "+54 9 223 456 7890", etc. Devuelve null si no se puede interpretar.
 */
function normalizar_telefono_ar(string $crudo): ?string
{
    $d = (string)preg_replace('/\D+/', '', $crudo);
    if ($d === '') {
        return null;
    }
    if (str_starts_with($d, '00')) {
        $d = substr($d, 2);
    }
    if (str_starts_with($d, '54')) {
        $d = substr($d, 2);
        if (str_starts_with($d, '9')) {
            $d = substr($d, 1);
        }
    } elseif (str_starts_with($d, '0')) {
        $d = substr($d, 1);
    }
    if (strlen($d) === 12) {   // con el "15" del celular entre el área y el número
        for ($a = 2; $a <= 4; $a++) {
            $num = substr($d, $a + 2);
            if (substr($d, $a, 2) === '15' && strlen($num) >= 6 && strlen($num) <= 8) {
                $d = substr($d, 0, $a) . $num;
                break;
            }
        }
    }
    return strlen($d) === 10 ? '549' . $d : null;
}

function armar_mensaje_recordatorio(array $t): string
{
    $f = DateTime::createFromFormat('Y-m-d H:i:s', (string)$t['fecha_hora']);
    $tipo = (string)$t['tipo'] === 'Ambos' ? 'plaquetas y plasma' : mb_strtolower((string)$t['tipo']);
    $lugar = trim((string)($t['lugar'] ?? '')) !== '' ? trim((string)$t['lugar']) : 'Hemocentro Regional de Mar del Plata';
    return 'Hola ' . $t['nombre'] . ', le recordamos su turno para donar ' . $tipo . ' el '
        . ($f ? $f->format('d/m/Y') . ' a las ' . $f->format('H:i') : (string)$t['fecha_hora'])
        . ' en ' . $lugar . '. Si no puede asistir o tiene alguna consulta, responda a este mensaje '
        . 'y el personal se comunicará con usted.';
}

/**
 * Único punto de salida de mensajes. Devuelve [enviado (bool), modo, detalle].
 * Hoy solo existe el modo "simulado". Para conectar un servicio real (API de WhatsApp Business)
 * se agrega otro caso en este switch y nada más cambia en el sistema.
 */
function enviar_whatsapp(PDO $pdo, string $telefono, string $mensaje): array
{
    $modo = parametro_texto($pdo, 'whatsapp_modo', 'simulado');
    switch ($modo) {
        case 'simulado':
            return [true, 'simulado', 'Simulado: no se envió ningún mensaje real.'];
        default:
            return [false, $modo, "El modo de envío '$modo' todavía no está disponible."];
    }
}

/**
 * Busca los turnos pendientes que caen dentro de las próximas N horas (parámetro horas_aviso_turno)
 * y todavía no tienen recordatorio, y envía uno a cada donante. Seguro de ejecutar muchas veces:
 * la clave única (id_turno, fecha_hora_turno) impide duplicados aunque dos ejecuciones se crucen.
 * Si el turno se reprograma, corresponde un recordatorio nuevo.
 */
function procesar_recordatorios(PDO $pdo): array
{
    $horas = parametro_int($pdo, 'horas_aviso_turno', 24);

    $st = $pdo->prepare(
        "SELECT t.id_turno, t.id_donante, t.fecha_hora, t.lugar, d.nombre, d.apellido, d.telefono, td.nombre AS tipo
         FROM turnos t
         JOIN donantes d ON d.id_donante = t.id_donante
         JOIN tipos_donacion td ON td.id_tipo_donacion = t.id_tipo_donacion
         WHERE t.estado = 'Pendiente' AND d.activo = TRUE
           AND t.fecha_hora > NOW() AND t.fecha_hora <= DATE_ADD(NOW(), INTERVAL :h HOUR)
           AND NOT EXISTS (SELECT 1 FROM notificaciones_whatsapp n
                           WHERE n.id_turno = t.id_turno AND n.fecha_hora_turno = t.fecha_hora)
         ORDER BY t.fecha_hora"
    );
    $st->execute([':h' => $horas]);
    $turnos = $st->fetchAll();

    $resumen = ['revisados' => count($turnos), 'enviados' => 0, 'con_error' => 0, 'detalle' => []];

    foreach ($turnos as $t) {
        $telefono = normalizar_telefono_ar((string)$t['telefono']);
        $mensaje = armar_mensaje_recordatorio($t);

        // Se registra primero: si dos ejecuciones se cruzan, la clave única deja pasar solo a una
        try {
            $pdo->prepare(
                "INSERT INTO notificaciones_whatsapp
                    (id_donante, id_turno, fecha_hora_turno, mensaje_enviado, estado_envio, telefono_destino)
                 VALUES (:d, :t, :fh, :m, 'Pendiente', :tel)"
            )->execute([':d' => $t['id_donante'], ':t' => $t['id_turno'], ':fh' => $t['fecha_hora'],
                        ':m' => $mensaje, ':tel' => $telefono]);
        } catch (PDOException $e) {
            if ($e->getCode() === '23000') {
                continue;   // otra ejecución ya se ocupó de este turno
            }
            throw $e;
        }
        $idNotif = (int)$pdo->lastInsertId();

        if ($telefono === null) {
            $estado = 'Error';
            $modo = null;
            $detalle = 'El teléfono registrado no es válido: ' . mb_substr((string)$t['telefono'], 0, 40);
        } else {
            [$ok, $modo, $detalle] = enviar_whatsapp($pdo, $telefono, $mensaje);
            $estado = $ok ? 'Enviado' : 'Error';
        }

        $pdo->prepare(
            'UPDATE notificaciones_whatsapp SET estado_envio = :e, modo_envio = :mo, detalle_envio = :de, fecha_envio = NOW()
             WHERE id_notificacion = :id'
        )->execute([':e' => $estado, ':mo' => $modo, ':de' => mb_substr((string)$detalle, 0, 255), ':id' => $idNotif]);

        $estado === 'Enviado' ? $resumen['enviados']++ : $resumen['con_error']++;
        $resumen['detalle'][] = [
            'donante' => $t['apellido'] . ', ' . $t['nombre'],
            'turno' => $t['fecha_hora'],
            'estado' => $estado,
            'detalle' => $detalle,
        ];
    }
    return $resumen;
}

/**
 * Registra la respuesta de un donante a un recordatorio y deja el mensaje marcado
 * como "pendiente de atender" para el personal. La usan hoy el simulador y mañana el servicio real.
 * Devuelve false si el recordatorio no existe o no fue enviado.
 */
function registrar_respuesta(PDO $pdo, int $idNotificacion, string $texto): bool
{
    $st = $pdo->prepare(
        "UPDATE notificaciones_whatsapp
         SET respuesta_recibida = CONCAT_WS('\n', respuesta_recibida, :t),
             fecha_respuesta = NOW(), requiere_atencion_medica = TRUE, atendido = FALSE, id_usuario_atiende = NULL
         WHERE id_notificacion = :id AND estado_envio = 'Enviado'"
    );
    $st->execute([':t' => $texto, ':id' => $idNotificacion]);
    return $st->rowCount() > 0;
}
