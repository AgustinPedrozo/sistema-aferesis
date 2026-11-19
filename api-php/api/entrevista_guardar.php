<?php
// POST /api/entrevista_guardar.php
// Registra la entrevista médica previa a la donación y, si el donante queda diferido,
// las restricciones (causas de diferimiento). Todo se guarda junto o no se guarda nada.
//
// Cuerpo (JSON):
// {
//   "id_donante": 1,
//   "id_turno": 3,                      (opcional)
//   "apto": true,                       (true = apto, false = no apto / diferido)
//   "peso": 72.5, "altura": 175,
//   "recuento_plaquetas": 210000,
//   "observaciones": "...",
//   "restricciones": [                  (solo si no es apto)
//      {"id_restriccion": 1, "fecha_inicio": "2026-08-01",
//       "fecha_habilitacion": "2027-02-01",       (opcional: si falta, se calcula con el catálogo)
//       "hasta_nueva_evaluacion": false,           (opcional)
//       "observaciones": "..."}
//   ],
//   "levantar": [5, 6]                  (opcional: ids de restricciones vigentes que esta evaluación levanta)
// }
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/entrevistas_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_ENTREVISTA);

    $in = leer_json();
    $errores = [];
    $hoy = (string)$pdo->query('SELECT CURDATE()')->fetchColumn();

    // ---- Donante ----
    $idDonante = (int)($in['id_donante'] ?? 0);
    $donante = $idDonante > 0 ? obtener_donante($pdo, $idDonante) : null;
    if ($donante === null) {
        $errores['id_donante'] = 'Elija un donante.';
    } elseif (!$donante['activo']) {
        $errores['id_donante'] = 'El donante está dado de baja.';
    }

    // ---- Aptitud y datos de la entrevista ----
    $apto = $in['apto'] ?? null;
    if (!is_bool($apto)) {
        $errores['apto'] = 'Indique si el donante es apto o no.';
    }

    [$peso, $error] = leer_numero_opcional($in['peso'] ?? '', 20, 300, 'El peso (kg)');
    if ($error !== null) {
        $errores['peso'] = $error;
    }
    [$altura, $error] = leer_numero_opcional($in['altura'] ?? '', 100, 250, 'La altura (cm)');
    if ($error !== null) {
        $errores['altura'] = $error;
    }
    [$plaquetas, $error] = leer_plaquetas($in['recuento_plaquetas'] ?? '');
    if ($error !== null) {
        $errores['recuento_plaquetas'] = $error;
    }
    $observaciones = trim((string)($in['observaciones'] ?? ''));
    if (mb_strlen($observaciones) > 2000) {
        $errores['observaciones'] = 'Las observaciones son demasiado largas.';
    }

    $idTurno = (int)($in['id_turno'] ?? 0);
    if ($idTurno > 0 && $donante !== null) {
        $st = $pdo->prepare('SELECT COUNT(*) FROM turnos WHERE id_turno = :t AND id_donante = :d');
        $st->execute([':t' => $idTurno, ':d' => $idDonante]);
        if ((int)$st->fetchColumn() === 0) {
            $errores['id_turno'] = 'El turno no corresponde a este donante.';
        }
    }

    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay datos para corregir.', 'campos' => $errores], 422);
    }

    // ---- Restricciones que esta evaluación levanta ----
    $vigentes = listar_restricciones_donante($pdo, $idDonante, true);
    $idsVigentes = array_column($vigentes, 'id_donante_restriccion');

    $levantar = is_array($in['levantar'] ?? null) ? $in['levantar'] : [];
    $idsLevantar = array_values(array_unique(array_map('intval', $levantar)));
    foreach ($idsLevantar as $idL) {
        if (!in_array($idL, $idsVigentes, true)) {
            $errores['levantar'] = 'Una de las restricciones a levantar no está vigente para este donante.';
        }
    }
    $restantes = array_values(array_filter(
        $vigentes,
        fn(array $v): bool => !in_array($v['id_donante_restriccion'], $idsLevantar, true)
    ));

    // ---- Restricciones nuevas ----
    $nuevas = [];
    $entradaRestr = is_array($in['restricciones'] ?? null) ? $in['restricciones'] : [];
    foreach ($entradaRestr as $r) {
        if (!is_array($r)) {
            $errores['restricciones'] = 'Una de las restricciones no es válida.';
            continue;
        }

        $st = $pdo->prepare(
            'SELECT id_restriccion, nombre, dias_espera_minimos, requiere_reevaluacion
             FROM restricciones_medicas WHERE id_restriccion = :id AND activa = TRUE'
        );
        $st->execute([':id' => (int)($r['id_restriccion'] ?? 0)]);
        $cat = $st->fetch();
        if (!$cat) {
            $errores['restricciones'] = 'Elija las restricciones de la lista.';
            continue;
        }

        // Inicio: por defecto hoy; puede ser pasado (ej.: fecha del tatuaje), nunca futuro
        $inicio = trim((string)($r['fecha_inicio'] ?? ''));
        if ($inicio === '') {
            $inicio = $hoy;
        }
        if (!validar_fecha($inicio) || $inicio > $hoy) {
            $errores['restricciones'] = 'La fecha de inicio de "' . $cat['nombre'] . '" no es válida o es futura.';
            continue;
        }

        // Fecha de habilitación: la indicada, o la calculada con el catálogo, o "hasta nueva evaluación"
        $fin = trim((string)($r['fecha_habilitacion'] ?? ''));
        $hasta = (isset($r['hasta_nueva_evaluacion']) && is_bool($r['hasta_nueva_evaluacion']))
            ? $r['hasta_nueva_evaluacion']
            : (bool)$cat['requiere_reevaluacion'];

        if ($fin !== '') {
            if (!validar_fecha($fin) || $fin < $inicio) {
                $errores['restricciones'] = 'La fecha de habilitación de "' . $cat['nombre'] . '" no es válida.';
                continue;
            }
            $hasta = false;
        } elseif ($hasta) {
            $fin = null;
        } elseif ($cat['dias_espera_minimos'] !== null) {
            $dias = (int)$cat['dias_espera_minimos'];
            $st = $pdo->prepare("SELECT DATE_ADD(CAST(:i AS DATE), INTERVAL $dias DAY)");
            $st->execute([':i' => $inicio]);
            $fin = (string)$st->fetchColumn();
        } else {
            $fin = null;
            $hasta = true;   // sin plazo definido en el catálogo: queda hasta una nueva evaluación
        }

        $nuevas[] = [
            'id' => (int)$cat['id_restriccion'],
            'inicio' => $inicio,
            'fin' => $fin,
            'hasta' => $hasta,
            'obs' => mb_substr(trim((string)($r['observaciones'] ?? '')), 0, 1000),
        ];
    }

    // ---- Coherencia entre aptitud y restricciones ----
    if ($apto === true && $nuevas) {
        $errores['restricciones'] = 'Un donante apto no puede tener restricciones nuevas. Marque "no apto" o quite las restricciones.';
    }
    if ($apto === false && !$nuevas && !$restantes) {
        $errores['restricciones'] = 'Si el donante no es apto, elija al menos una causa de diferimiento.';
    }
    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay datos para corregir.', 'campos' => $errores], 422);
    }

    // ---- Requisitos para ser apto (parámetros configurables) ----
    if ($apto === true) {
        $pesoMin = parametro_int($pdo, 'peso_minimo_kg', 50);
        $plaquetasMin = parametro_int($pdo, 'recuento_minimo_plaquetas', 150000);
        $edadMin = parametro_int($pdo, 'edad_minima', 18);
        $edadMax = parametro_int($pdo, 'edad_maxima', 65);

        $st = $pdo->prepare('SELECT TIMESTAMPDIFF(YEAR, fecha_nacimiento, CURDATE()) FROM donantes WHERE id_donante = :d');
        $st->execute([':d' => $idDonante]);
        $edad = (int)$st->fetchColumn();

        $bloqueos = [];
        if ($peso === null) {
            $bloqueos[] = 'falta registrar el peso';
        } elseif ($peso <= $pesoMin) {
            $bloqueos[] = "el peso ($peso kg) debe ser mayor a $pesoMin kg";
        }
        if ($plaquetas === null) {
            $bloqueos[] = 'falta el recuento de plaquetas del análisis previo';
        } elseif ($plaquetas < $plaquetasMin) {
            $bloqueos[] = 'el recuento de plaquetas (' . number_format($plaquetas, 0, ',', '.')
                . ') es menor al mínimo (' . number_format($plaquetasMin, 0, ',', '.') . ')';
        }
        if ($edad < $edadMin || $edad > $edadMax) {
            $bloqueos[] = "la edad ($edad años) debe estar entre $edadMin y $edadMax años";
        }
        if ($restantes) {
            $bloqueos[] = 'tiene restricciones vigentes: ' . implode(', ', array_map('describir_restriccion', $restantes));
        }

        if ($bloqueos) {
            responder([
                'ok' => false,
                'error' => 'No se puede marcar como apto: ' . implode('; ', $bloqueos) . '.',
                'bloqueos' => $bloqueos,
            ], 409);
        }
    }

    // ---- Guardado (todo junto, o nada) ----
    $pdo->beginTransaction();
    try {
        $pdo->prepare(
            'INSERT INTO entrevistas_medicas
                (id_donante, id_usuario_entrevistador, id_turno, recuento_plaquetas, peso_kg, altura_cm, apto, observaciones)
             VALUES (:d, :u, :t, :plaq, :peso, :altura, :apto, :obs)'
        )->execute([
            ':d' => $idDonante,
            ':u' => $usuario['id_usuario'],
            ':t' => $idTurno > 0 ? $idTurno : null,
            ':plaq' => $plaquetas,
            ':peso' => $peso,
            ':altura' => $altura,
            ':apto' => $apto ? 1 : 0,
            ':obs' => $observaciones === '' ? null : $observaciones,
        ]);
        $idEntrevista = (int)$pdo->lastInsertId();

        foreach ($idsLevantar as $idL) {
            $pdo->prepare(
                'UPDATE donante_restricciones
                 SET fecha_levantamiento = NOW(), id_usuario_levanta = :u, id_entrevista_levanta = :e
                 WHERE id_donante_restriccion = :id AND id_donante = :d AND fecha_levantamiento IS NULL'
            )->execute([':u' => $usuario['id_usuario'], ':e' => $idEntrevista, ':id' => $idL, ':d' => $idDonante]);
        }

        foreach ($nuevas as $n) {
            $pdo->prepare(
                'INSERT INTO donante_restricciones
                    (id_donante, id_restriccion, id_entrevista, id_usuario_registro,
                     fecha_inicio_restriccion, fecha_habilitacion_calculada, hasta_nueva_evaluacion, observaciones)
                 VALUES (:d, :r, :e, :u, :ini, :fin, :hasta, :obs)'
            )->execute([
                ':d' => $idDonante,
                ':r' => $n['id'],
                ':e' => $idEntrevista,
                ':u' => $usuario['id_usuario'],
                ':ini' => $n['inicio'],
                ':fin' => $n['fin'],
                ':hasta' => $n['hasta'] ? 1 : 0,
                ':obs' => $n['obs'] === '' ? null : $n['obs'],
            ]);
        }

        // El peso y la altura medidos hoy quedan como los últimos valores del donante
        $pdo->prepare('UPDATE donantes SET peso = COALESCE(:p, peso), altura = COALESCE(:a, altura) WHERE id_donante = :d')
            ->execute([':p' => $peso, ':a' => $altura, ':d' => $idDonante]);

        $pdo->commit();
    } catch (Throwable $e) {
        if ($pdo->inTransaction()) {
            $pdo->rollBack();
        }
        throw $e;
    }

    $st = $pdo->prepare(
        'SELECT id_entrevista, id_donante, fecha_entrevista, apto, recuento_plaquetas, peso_kg, altura_cm, observaciones
         FROM entrevistas_medicas WHERE id_entrevista = :id'
    );
    $st->execute([':id' => $idEntrevista]);
    $entrevista = $st->fetch();
    $entrevista['id_entrevista'] = (int)$entrevista['id_entrevista'];
    $entrevista['id_donante'] = (int)$entrevista['id_donante'];
    $entrevista['apto'] = (bool)$entrevista['apto'];

    responder([
        'ok' => true,
        'mensaje' => $apto ? 'Entrevista registrada: el donante es apto.' : 'Entrevista registrada: el donante queda diferido.',
        'entrevista' => $entrevista,
        'restricciones_vigentes' => listar_restricciones_donante($pdo, $idDonante, true),
    ], 201);
} catch (Throwable $e) {
    error_log('entrevista_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
