<?php
// POST /api/donacion_guardar.php
// Registra una extracción por aféresis con los datos de la planilla HEMO 2.
// Todo se guarda junto o no se guarda nada: donación, datos técnicos y turno completado.
//
// Cuerpo (JSON):
// {
//   "id_donante": 1,
//   "id_tipo_donacion": 1,              (Plaquetas / Plasma / Ambos)
//   "numero_donacion": "2600100001",    (10 dígitos: AA + localidad 3 + secuencial 5)
//   "id_turno": 3,                      (opcional; si falta se usa el turno pendiente de hoy, si hay uno)
//   "cantidad_plaquetas": 1, "cantidad_plasma": 0,
//   "peso": 72.5, "altura": 175,        (opcionales: si faltan se usan los de la entrevista de hoy)
//   "incidencias": "...",
//   "detalle": { "hto": 42, "hb": 14, "ta": "120/80", "pulso": 72, "temperatura": 36.5,
//                "tipo_bolsa": "Triple", "marca_bolsa": "...", "lote_bolsa": "...", "nro_tubuladura": "...",
//                "vencimiento_bolsa": "2027-03-01", "anticoagulante": "ACD-A",
//                "brazo": "Derecho", "dificultades": false, "peso_bolsa": 250.5,
//                "hora_inicio": "09:00", "hora_fin": "10:15" },
//   "excepcion": {                      (solo si se pasa algún límite)
//      "motivo": "Urgencia extrema: ...",
//      "usuario": "jc", "clave": "1234" (si quien registra no es Jefa/Administrador: datos de quien autoriza)
//   }
// }
//
// Respuestas: 201 guardado | 422 datos para corregir | 409 no puede donar (bloqueos) o pasa límites
// (en ese caso "requiere_excepcion": true y "violaciones") | 403 sin permiso o autorización inválida.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/donaciones_comun.php';

solo_post();

/** Verifica usuario y clave de quien autoriza la excepción. Devuelve su id o corta con 403. */
function verificar_autorizador(PDO $pdo, string $nombre, string $clave): int
{
    $st = $pdo->prepare(
        'SELECT u.id_usuario, u.clave_hash, r.nombre AS rol,
                (u.bloqueado_hasta IS NOT NULL AND u.bloqueado_hasta > NOW()) AS bloqueado
         FROM usuarios u JOIN roles r ON r.id_rol = u.id_rol
         WHERE u.nombre_usuario = :u AND u.activo = TRUE'
    );
    $st->execute([':u' => $nombre]);
    $f = $st->fetch();

    if ($f && (int)$f['bloqueado'] === 1) {
        responder(['ok' => false, 'error' => 'La cuenta de quien autoriza está bloqueada temporalmente por intentos fallidos.'], 429);
    }
    $hash = $f['clave_hash'] ?? password_hash('relleno', PASSWORD_DEFAULT);
    $correcta = password_verify($clave, $hash);

    if (!$f || !$correcta) {
        if ($f) {   // mismo bloqueo por intentos que el login
            $max = (int)MAX_INTENTOS;
            $min = (int)MINUTOS_BLOQUEO;
            $pdo->prepare(
                "UPDATE usuarios
                 SET bloqueado_hasta = IF(intentos_fallidos + 1 >= $max, DATE_ADD(NOW(), INTERVAL $min MINUTE), bloqueado_hasta),
                     intentos_fallidos = IF(intentos_fallidos + 1 >= $max, 0, intentos_fallidos + 1)
                 WHERE id_usuario = :id"
            )->execute([':id' => $f['id_usuario']]);
        }
        responder(['ok' => false, 'error' => 'No se pudo validar la autorización: usuario o clave incorrectos.'], 403);
    }
    if (!in_array($f['rol'], ROLES_AUTORIZAN_EXCEPCION, true)) {
        responder(['ok' => false, 'error' => 'Ese usuario no tiene permiso para autorizar excepciones. Debe hacerlo la Jefa de Área/Gerente.'], 403);
    }
    $pdo->prepare('UPDATE usuarios SET intentos_fallidos = 0, bloqueado_hasta = NULL WHERE id_usuario = :id')
        ->execute([':id' => $f['id_usuario']]);
    return (int)$f['id_usuario'];
}

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_EXTRACCION);

    $in = leer_json();
    $errores = [];

    // ---- Datos de la donación ----
    $idDonante = (int)($in['id_donante'] ?? 0);
    $idTipo = (int)($in['id_tipo_donacion'] ?? 0);
    if ($idDonante <= 0 || $idTipo <= 0) {
        responder(['ok' => false, 'error' => 'Datos incompletos: se necesita id_donante e id_tipo_donacion.'], 400);
    }

    $numero = trim((string)($in['numero_donacion'] ?? ''));
    if (!preg_match('/^\d{10}$/', $numero)) {
        $errores['numero_donacion'] = 'El número de donación debe tener 10 dígitos (año 2 + localidad 3 + secuencial 5).';
    }

    $cantPlaq = trim((string)($in['cantidad_plaquetas'] ?? '0'));
    $cantPlas = trim((string)($in['cantidad_plasma'] ?? '0'));
    $cantPlaq = $cantPlaq === '' ? '0' : $cantPlaq;
    $cantPlas = $cantPlas === '' ? '0' : $cantPlas;
    if (!ctype_digit($cantPlaq) || (int)$cantPlaq > 20) {
        $errores['cantidad_plaquetas'] = 'La cantidad de plaquetas debe ser un número entero (unidades obtenidas).';
    }
    if (!ctype_digit($cantPlas) || (int)$cantPlas > 20) {
        $errores['cantidad_plasma'] = 'La cantidad de plasma debe ser un número entero (unidades obtenidas).';
    }

    [$peso, $err] = leer_numero_opcional($in['peso'] ?? '', 20, 300, 'El peso');
    if ($err) {
        $errores['peso'] = $err;
    }
    [$altura, $err] = leer_numero_opcional($in['altura'] ?? '', 100, 250, 'La altura');
    if ($err) {
        $errores['altura'] = $err;
    }
    $incidencias = leer_texto_opcional($in['incidencias'] ?? '', 2000);

    [$detalle, $erroresDetalle] = leer_detalle_hemo2(is_array($in['detalle'] ?? null) ? $in['detalle'] : []);
    foreach ($erroresDetalle as $campo => $texto) {
        $errores[$campo] = $texto;
    }

    $donante = obtener_donante($pdo, $idDonante);
    if ($donante === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }
    if (!$donante['activo']) {
        responder(['ok' => false, 'error' => 'El donante está dado de baja.'], 409);
    }
    $tipo = obtener_tipo_donacion($pdo, $idTipo);
    if ($tipo === null) {
        responder(['ok' => false, 'error' => 'El tipo de donación no existe o no está habilitado.'], 404);
    }

    // Las cantidades tienen que coincidir con el tipo elegido
    if (!isset($errores['cantidad_plaquetas']) && !isset($errores['cantidad_plasma'])) {
        $cp = (int)$cantPlaq;
        $cs = (int)$cantPlas;
        $nombreTipo = (string)$tipo['nombre'];
        if ($nombreTipo === 'Plaquetas' && ($cp < 1 || $cs !== 0)) {
            $errores['cantidad_plaquetas'] = 'Una donación de Plaquetas debe tener al menos 1 unidad de plaquetas y 0 de plasma.';
        } elseif ($nombreTipo === 'Plasma' && ($cs < 1 || $cp !== 0)) {
            $errores['cantidad_plasma'] = 'Una donación de Plasma debe tener al menos 1 unidad de plasma y 0 de plaquetas.';
        } elseif ($nombreTipo === 'Ambos' && ($cp < 1 || $cs < 1)) {
            $errores['cantidad_plaquetas'] = 'Una donación de Ambos debe tener al menos 1 unidad de plaquetas y 1 de plasma.';
        }
    }

    // Turno (opcional): tiene que ser del mismo donante y estar pendiente
    $idTurno = (int)($in['id_turno'] ?? 0);
    if ($idTurno > 0) {
        $turno = obtener_turno($pdo, $idTurno);
        if ($turno === null) {
            $errores['id_turno'] = 'No se encontró el turno indicado.';
        } elseif ($turno['id_donante'] !== $idDonante) {
            $errores['id_turno'] = 'El turno pertenece a otro donante.';
        } elseif ($turno['estado'] !== 'Pendiente') {
            $errores['id_turno'] = 'El turno indicado ya no está pendiente.';
        }
    }

    // Número de donación repetido
    if (!isset($errores['numero_donacion'])) {
        $st = $pdo->prepare('SELECT 1 FROM donaciones WHERE numero_donacion = :n');
        $st->execute([':n' => $numero]);
        if ($st->fetchColumn()) {
            $errores['numero_donacion'] = 'Ese número de donación ya fue usado en otra extracción.';
        }
    }

    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay datos para corregir.', 'campos' => $errores], 422);
    }

    // ---- Controles médicos y límites ----
    $ev = evaluar_donacion($pdo, $idDonante, $tipo);
    if ($ev['bloqueos']) {
        responder([
            'ok' => false,
            'error' => 'No se puede registrar la extracción: ' . implode('; ', $ev['bloqueos']) . '.',
            'bloqueos' => $ev['bloqueos'],
        ], 409);
    }

    // Si pasa algún límite hace falta una excepción autorizada. Se verifica ANTES de abrir la transacción,
    // para que un intento fallido de clave quede registrado aunque después no se guarde nada.
    $idAutoriza = null;
    $motivo = null;
    if ($ev['violaciones']) {
        $ex = is_array($in['excepcion'] ?? null) ? $in['excepcion'] : null;
        if ($ex === null) {
            responder([
                'ok' => false,
                'error' => 'El donante supera los límites: ' . implode('; ', $ev['violaciones']) . '. Hace falta la autorización de la Jefa de Área/Gerente.',
                'requiere_excepcion' => true,
                'violaciones' => $ev['violaciones'],
            ], 409);
        }
        $motivo = mb_substr(trim((string)($ex['motivo'] ?? '')), 0, 1000);
        if (mb_strlen($motivo) < 10) {
            responder(['ok' => false, 'error' => 'Hay datos para corregir.',
                'campos' => ['motivo_excepcion' => 'Explique el motivo de la excepción (al menos 10 caracteres).']], 422);
        }
        if (in_array($usuario['rol'], ROLES_AUTORIZAN_EXCEPCION, true)) {
            $idAutoriza = (int)$usuario['id_usuario'];
        } else {
            $idAutoriza = verificar_autorizador($pdo, trim((string)($ex['usuario'] ?? '')), (string)($ex['clave'] ?? ''));
        }
    }

    // ---- Guardado ----
    $pdo->beginTransaction();
    try {
        // Se bloquea al donante para que dos extracciones simultáneas no esquiven los límites
        $pdo->prepare('SELECT id_donante FROM donantes WHERE id_donante = :d FOR UPDATE')->execute([':d' => $idDonante]);
        $ev2 = evaluar_donacion($pdo, $idDonante, $tipo);
        if ($ev2['bloqueos'] || ($ev2['violaciones'] && $idAutoriza === null)) {
            $pdo->rollBack();
            responder(['ok' => false, 'error' => 'El estado del donante cambió mientras se cargaba la planilla. Vuelva a verificar.'], 409);
        }
        $esExcepcion = (bool)$ev2['violaciones'];

        $ent = $ev2['entrevista'];
        $pesoFinal = $peso ?? ($ent['peso_kg'] ?? null);
        $alturaFinal = $altura ?? ($ent['altura_cm'] ?? null);

        $pdo->prepare(
            'INSERT INTO donaciones
                (numero_donacion, id_donante, id_tipo_donacion, id_turno, id_entrevista, id_usuario_extractora,
                 peso_donante, altura_donante, cantidad_plaquetas, cantidad_plasma, incidencias,
                 es_excepcion, motivo_excepcion, id_usuario_autoriza)
             VALUES (:n, :d, :t, :tu, :e, :u, :p, :a, :cp, :cs, :i, :x, :m, :au)'
        )->execute([
            ':n' => $numero, ':d' => $idDonante, ':t' => $idTipo,
            ':tu' => null, ':e' => $ent['id_entrevista'] ?? null, ':u' => $usuario['id_usuario'],
            ':p' => $pesoFinal, ':a' => $alturaFinal, ':cp' => (int)$cantPlaq, ':cs' => (int)$cantPlas,
            ':i' => $incidencias, ':x' => $esExcepcion ? 1 : 0,
            ':m' => $esExcepcion ? $motivo : null, ':au' => $esExcepcion ? $idAutoriza : null,
        ]);
        $idDonacion = (int)$pdo->lastInsertId();

        $columnas = array_keys($detalle);
        $pdo->prepare(
            'INSERT INTO donaciones_detalle (id_donacion, ' . implode(', ', $columnas) . ')
             VALUES (:id_donacion, :' . implode(', :', $columnas) . ')'
        )->execute(array_merge([':id_donacion' => $idDonacion], array_combine(
            array_map(fn($c) => ':' . $c, $columnas), array_values($detalle)
        )));

        // Turno: el indicado, o el pendiente de hoy de este donante si hay uno
        $turnoCompletado = null;
        if ($idTurno <= 0) {
            $st = $pdo->prepare(
                "SELECT id_turno FROM turnos
                 WHERE id_donante = :d AND estado = 'Pendiente' AND DATE(fecha_hora) = CURDATE()
                 ORDER BY fecha_hora LIMIT 1"
            );
            $st->execute([':d' => $idDonante]);
            $idTurno = (int)($st->fetchColumn() ?: 0);
        }
        if ($idTurno > 0) {
            $pdo->prepare("UPDATE turnos SET estado = 'Completado' WHERE id_turno = :t AND estado = 'Pendiente'")
                ->execute([':t' => $idTurno]);
            $pdo->prepare('UPDATE donaciones SET id_turno = :t WHERE id_donacion = :id')
                ->execute([':t' => $idTurno, ':id' => $idDonacion]);
            $turnoCompletado = $idTurno;
        }

        // El formulario HEMO 3 que se cargó hoy para este donante queda vinculado a esta donación
        $pdo->prepare(
            'UPDATE formulario_confidencial SET id_donacion = :don
             WHERE id_donante = :d AND id_donacion IS NULL AND DATE(fecha_registro) = CURDATE()'
        )->execute([':don' => $idDonacion, ':d' => $idDonante]);

        $pdo->commit();
    } catch (Throwable $e) {
        if ($pdo->inTransaction()) {
            $pdo->rollBack();
        }
        throw $e;
    }

    $st = $pdo->prepare(
        'SELECT id_donacion, numero_donacion, fecha_donacion, cantidad_plaquetas, cantidad_plasma, es_excepcion
         FROM donaciones WHERE id_donacion = :id'
    );
    $st->execute([':id' => $idDonacion]);
    $don = $st->fetch();
    $don['id_donacion'] = (int)$don['id_donacion'];
    $don['cantidad_plaquetas'] = (int)$don['cantidad_plaquetas'];
    $don['cantidad_plasma'] = (int)$don['cantidad_plasma'];
    $don['es_excepcion'] = (bool)$don['es_excepcion'];

    responder([
        'ok' => true,
        'mensaje' => 'Extracción registrada.' . ($don['es_excepcion'] ? ' Quedó registrada como excepción autorizada.' : ''),
        'donacion' => $don,
        'turno_completado' => $turnoCompletado,
        'avisos' => $ev['avisos'],
    ], 201);
} catch (Throwable $e) {
    error_log('donacion_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
