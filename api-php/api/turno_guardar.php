<?php
// POST /api/turno_guardar.php
// Sin "id_turno" -> NUEVO turno. Con "id_turno" -> REPROGRAMAR un turno pendiente.
// Cuerpo: {"id_donante":1, "id_tipo_donacion":1, "fecha_hora":"2026-10-21 09:30", "lugar":"..."}
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/turnos_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    $entrada = leer_json();

    $idTurno = (int)($entrada['id_turno'] ?? 0);
    $idDonante = (int)($entrada['id_donante'] ?? 0);
    $idTipo = (int)($entrada['id_tipo_donacion'] ?? 0);
    $fechaHora = trim((string)($entrada['fecha_hora'] ?? ''));
    $lugar = trim((string)($entrada['lugar'] ?? ''));

    // Al reprogramar, el donante es el del turno original y el turno tiene que seguir pendiente
    if ($idTurno > 0) {
        $actual = obtener_turno($pdo, $idTurno);
        if ($actual === null) {
            responder(['ok' => false, 'error' => 'No se encontró el turno.'], 404);
        }
        if ($actual['estado'] !== 'Pendiente') {
            responder(['ok' => false, 'error' => 'Solo se puede reprogramar un turno pendiente.'], 409);
        }
        $idDonante = $actual['id_donante'];
    }

    // ---- Validación de los datos ----
    $errores = [];

    $dt = validar_fecha_hora($fechaHora);
    if ($dt === null) {
        $errores['fecha_hora'] = 'Elija una fecha y una hora válidas.';
    } else {
        // La comparación con "ahora" se hace en la base, que es quien tiene la hora oficial del sistema
        $st = $pdo->prepare('SELECT CAST(:fh AS DATETIME) >= NOW()');
        $st->execute([':fh' => $dt->format('Y-m-d H:i:00')]);
        if ((int)$st->fetchColumn() === 0) {
            $errores['fecha_hora'] = 'La fecha y la hora del turno ya pasaron.';
        }
    }

    if (mb_strlen($lugar) > 150) {
        $errores['lugar'] = 'El lugar es demasiado largo.';
    }

    $donante = $idDonante > 0 ? obtener_donante($pdo, $idDonante) : null;
    if ($donante === null) {
        $errores['id_donante'] = 'Elija un donante.';
    } elseif (!$donante['activo']) {
        $errores['id_donante'] = 'El donante está dado de baja.';
    }

    $st = $pdo->prepare('SELECT COUNT(*) FROM tipos_donacion WHERE id_tipo_donacion = :id AND activo = TRUE');
    $st->execute([':id' => $idTipo]);
    if ((int)$st->fetchColumn() === 0) {
        $errores['id_tipo_donacion'] = 'Elija un tipo de donación habilitado.';
    }

    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay datos para corregir.', 'campos' => $errores], 422);
    }

    // ---- Reglas de agenda ----
    $dia = $dt->format('Y-m-d');
    $excluir = $idTurno > 0 ? ' AND id_turno <> :t' : '';
    $extra = $idTurno > 0 ? [':t' => $idTurno] : [];

    // 1) Un donante no puede tener dos turnos pendientes el mismo día
    $st = $pdo->prepare(
        "SELECT COUNT(*) FROM turnos
         WHERE id_donante = :d AND DATE(fecha_hora) = :dia AND estado = 'Pendiente'" . $excluir
    );
    $st->execute([':d' => $idDonante, ':dia' => $dia] + $extra);
    if ((int)$st->fetchColumn() > 0) {
        responder([
            'ok' => false,
            'error' => 'El donante ya tiene un turno pendiente ese día.',
            'campos' => ['fecha_hora' => 'El donante ya tiene un turno ese día.'],
        ], 409);
    }

    // 2) Cupo diario del banco (parámetro configurable)
    $limite = (int)$pdo->query(
        "SELECT valor FROM parametros_configuracion WHERE clave = 'limite_diario_donaciones'"
    )->fetchColumn();
    if ($limite > 0) {
        $st = $pdo->prepare(
            "SELECT COUNT(*) FROM turnos
             WHERE DATE(fecha_hora) = :dia AND estado IN ('Pendiente', 'Completado')" . $excluir
        );
        $st->execute([':dia' => $dia] + $extra);
        if ((int)$st->fetchColumn() >= $limite) {
            responder([
                'ok' => false,
                'error' => "Ya se completó el cupo de ese día ($limite turnos).",
                'campos' => ['fecha_hora' => 'No hay cupo ese día.'],
            ], 409);
        }
    }

    // 3) Cupos semanal y mensual del banco (parámetros configurables)
    foreach ([
        ['limite_semanal_donaciones', 'YEARWEEK(fecha_hora, 1) = YEARWEEK(:dia, 1)', 'esa semana'],
        ['limite_mensual_donaciones', "DATE_FORMAT(fecha_hora, '%Y-%m') = DATE_FORMAT(:dia, '%Y-%m')", 'ese mes'],
    ] as [$clave, $condicion, $cuando]) {
        $st = $pdo->prepare('SELECT valor FROM parametros_configuracion WHERE clave = :c');
        $st->execute([':c' => $clave]);
        $tope = (int)$st->fetchColumn();
        if ($tope > 0) {
            $st = $pdo->prepare(
                "SELECT COUNT(*) FROM turnos
                 WHERE $condicion AND estado IN ('Pendiente', 'Completado')" . $excluir
            );
            $st->execute([':dia' => $dia] + $extra);
            if ((int)$st->fetchColumn() >= $tope) {
                responder([
                    'ok' => false,
                    'error' => "Ya se completó el cupo de $cuando ($tope turnos).",
                    'campos' => ['fecha_hora' => "No hay cupo $cuando."],
                ], 409);
            }
        }
    }

    // ---- Guardado ----
    $fechaSql = $dt->format('Y-m-d H:i:00');
    $lugarSql = $lugar === '' ? null : $lugar;

    if ($idTurno === 0) {
        $pdo->prepare(
            'INSERT INTO turnos (id_donante, id_tipo_donacion, fecha_hora, lugar, id_usuario_registro)
             VALUES (:d, :tipo, :fh, :lugar, :u)'
        )->execute([
            ':d' => $idDonante, ':tipo' => $idTipo, ':fh' => $fechaSql,
            ':lugar' => $lugarSql, ':u' => $usuario['id_usuario'],
        ]);
        $nuevoId = (int)$pdo->lastInsertId();
        responder(['ok' => true, 'mensaje' => 'Turno asignado.', 'turno' => obtener_turno($pdo, $nuevoId)], 201);
    }

    $pdo->prepare(
        'UPDATE turnos SET id_tipo_donacion = :tipo, fecha_hora = :fh, lugar = :lugar WHERE id_turno = :id'
    )->execute([':tipo' => $idTipo, ':fh' => $fechaSql, ':lugar' => $lugarSql, ':id' => $idTurno]);
    responder(['ok' => true, 'mensaje' => 'Turno reprogramado.', 'turno' => obtener_turno($pdo, $idTurno)]);
} catch (Throwable $e) {
    error_log('turno_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
