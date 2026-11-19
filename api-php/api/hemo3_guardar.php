<?php
// POST /api/hemo3_guardar.php
// Guarda el formulario confidencial HEMO 3 (autoexclusión) que completó el donante. Se almacena tal cual,
// sin juzgar su contenido; solo se controla que las opciones existan.
//
// Cuerpo (JSON):
// { "id_donante": 1,
//   "respuestas": { "relaciones_transfundidos": false, "relaciones_ocasionales": true, "consumo_sustancias": false },
//   "observaciones": "..." }                     (opcional)
// Hay que responder TODAS las preguntas (claves de hemo3_campos.php), cada una con Sí/No (true/false).
// Queda vinculado a la entrevista y a la donación de hoy si existen (la donación se vincula sola al registrarla).
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/hemo3_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_HEMO3_CARGA);

    $in = leer_json();
    $idDonante = (int)($in['id_donante'] ?? 0);
    if ($idDonante <= 0) {
        responder(['ok' => false, 'error' => 'Falta el donante (id_donante).'], 400);
    }
    $donante = obtener_donante($pdo, $idDonante);
    if ($donante === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }
    if (!$donante['activo']) {
        responder(['ok' => false, 'error' => 'El donante está dado de baja.'], 409);
    }

    $errores = [];
    $crudas = $in['respuestas'] ?? null;
    $respuestas = [];
    if (!is_array($crudas)) {
        $errores['respuestas'] = 'Faltan las respuestas del formulario.';
    } else {
        $sinResponder = [];
        foreach (HEMO3_PREGUNTAS as $clave => $texto) {
            $r = array_key_exists($clave, $crudas) ? hemo3_leer_respuesta($crudas[$clave]) : null;
            if ($r === null) {
                $sinResponder[] = $texto;
                continue;
            }
            $respuestas[] = ['clave' => $clave, 'pregunta' => $texto, 'respuesta' => $r ? 'Sí' : 'No'];
        }
        if ($sinResponder) {
            $errores['respuestas'] = 'Falta responder Sí o No en ' . count($sinResponder) . ' pregunta(s).';
        }
    }
    $observaciones = mb_substr(trim((string)($in['observaciones'] ?? '')), 0, 2000);

    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay datos para corregir.', 'campos' => $errores], 422);
    }

    $yaExistia = hemo3_existe_hoy($pdo, $idDonante);

    // Se guarda el texto de cada pregunta junto con su clave: si el formulario cambia, lo ya cargado no se altera
    $datos = json_encode([
        'version' => 2,
        'respuestas' => $respuestas,
        'observaciones' => $observaciones,
    ], JSON_UNESCAPED_UNICODE);

    $st = $pdo->prepare(
        'SELECT id_entrevista FROM entrevistas_medicas
         WHERE id_donante = :d AND DATE(fecha_entrevista) = CURDATE()
         ORDER BY fecha_entrevista DESC, id_entrevista DESC LIMIT 1'
    );
    $st->execute([':d' => $idDonante]);
    $idEntrevista = $st->fetchColumn() ?: null;

    $st = $pdo->prepare(
        'SELECT id_donacion FROM donaciones WHERE id_donante = :d AND DATE(fecha_donacion) = CURDATE()
         ORDER BY fecha_donacion DESC, id_donacion DESC LIMIT 1'
    );
    $st->execute([':d' => $idDonante]);
    $idDonacion = $st->fetchColumn() ?: null;

    $pdo->prepare(
        'INSERT INTO formulario_confidencial (id_donante, id_entrevista, id_donacion, id_usuario_registro, datos)
         VALUES (:d, :e, :don, :u, :datos)'
    )->execute([':d' => $idDonante, ':e' => $idEntrevista, ':don' => $idDonacion,
                ':u' => $usuario['id_usuario'], ':datos' => $datos]);

    responder([
        'ok' => true,
        'mensaje' => 'Formulario HEMO 3 guardado.' . ($yaExistia ? ' Atención: ya había otro cargado hoy para este donante.' : ''),
        'id_formulario' => (int)$pdo->lastInsertId(),
        'ya_existia_hoy' => $yaExistia,
    ], 201);
} catch (Throwable $e) {
    error_log('hemo3_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
