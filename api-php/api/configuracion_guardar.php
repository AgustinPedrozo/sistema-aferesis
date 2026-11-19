<?php
// POST /api/configuracion_guardar.php
// Cuerpo: { "cambios": { "edad_maxima": 60, "tipo.1.limite_anual": 20 } }   (solo lo que cambia; claves de configuracion_listar.php)
// Todo o nada: si un valor no es válido no se guarda ninguno.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/configuracion_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_CONFIGURACION);

    $cambios = leer_json()['cambios'] ?? null;
    if (!is_array($cambios) || !$cambios) {
        responder(['ok' => false, 'error' => 'No hay cambios para guardar.'], 400);
    }

    $tipos = configuracion_tipos($pdo);
    $errores = [];
    $nuevos = [];        // parámetros generales: clave => valor
    $nuevosTipos = [];   // límites por tipo: id => [campo => valor]

    foreach ($cambios as $clave => $valor) {
        $clave = (string)$clave;
        $def = null;
        $idTipo = null;
        $campo = null;
        $nombre = '';

        if (isset(PARAMETROS_EDITABLES[$clave])) {
            $def = PARAMETROS_EDITABLES[$clave];
            $nombre = $def['etiqueta'];
        } elseif (preg_match('/^tipo\.(\d+)\.([a-z_]+)$/', $clave, $m)
            && isset($tipos[(int)$m[1]]) && isset(CAMPOS_TIPO[$m[2]])) {
            $idTipo = (int)$m[1];
            $campo = $m[2];
            $def = CAMPOS_TIPO[$campo];
            $nombre = $tipos[$idTipo]['nombre'] . ': ' . $def['etiqueta'];
        } else {
            $errores[$clave] = 'Ese parámetro no se puede modificar.';
            continue;
        }

        $texto = trim((string)$valor);
        if (!ctype_digit($texto) || strlen($texto) > 7) {
            $errores[$clave] = "$nombre: escriba un número entero.";
            continue;
        }
        $n = (int)$texto;
        if ($n < $def['min'] || $n > $def['max']) {
            $errores[$clave] = "$nombre: debe estar entre " . $def['min'] . ' y ' . $def['max'] . ' (' . $def['unidad'] . ').';
            continue;
        }
        if ($idTipo === null) {
            $nuevos[$clave] = $n;
        } else {
            $nuevosTipos[$idTipo][$campo] = $n;
        }
    }
    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay valores para corregir.', 'campos' => $errores], 422);
    }

    // Coherencia entre parámetros, mirando cómo quedaría todo después del cambio
    $final = $nuevos + configuracion_actual($pdo);
    $incoherencias = [];
    if ($final['edad_minima'] >= $final['edad_maxima']) {
        $incoherencias['edad_maxima'] = 'La edad máxima tiene que ser mayor que la mínima.';
    }
    if ($final['limite_diario_donaciones'] > $final['limite_semanal_donaciones']) {
        $incoherencias['limite_diario_donaciones'] = 'Los turnos por día no pueden superar a los de la semana.';
    }
    if ($final['limite_semanal_donaciones'] > $final['limite_mensual_donaciones']) {
        $incoherencias['limite_semanal_donaciones'] = 'Los turnos por semana no pueden superar a los del mes.';
    }
    foreach ($nuevosTipos as $idTipo => $campos) {
        $semanal = $campos['limite_semanal'] ?? $tipos[$idTipo]['limite_semanal'];
        $anual = $campos['limite_anual'] ?? $tipos[$idTipo]['limite_anual'];
        if ($semanal > $anual) {
            $incoherencias["tipo.$idTipo.limite_semanal"] = $tipos[$idTipo]['nombre'] . ': las donaciones por semana no pueden superar a las del año.';
        }
    }
    if ($incoherencias) {
        responder(['ok' => false, 'error' => 'Hay valores para corregir.', 'campos' => $incoherencias], 422);
    }

    $pdo->beginTransaction();
    try {
        $st = $pdo->prepare(
            'INSERT INTO parametros_configuracion (clave, valor) VALUES (:c, :v)
             ON DUPLICATE KEY UPDATE valor = VALUES(valor)'
        );
        foreach ($nuevos as $clave => $n) {
            $st->execute([':c' => $clave, ':v' => (string)$n]);
        }
        foreach ($nuevosTipos as $idTipo => $campos) {
            foreach ($campos as $campo => $n) {
                // $campo viene de la lista CAMPOS_TIPO (ya validado), por eso se puede usar como nombre de columna
                $pdo->prepare("UPDATE tipos_donacion SET $campo = :v WHERE id_tipo_donacion = :id")
                    ->execute([':v' => $n, ':id' => $idTipo]);
            }
        }
        $pdo->commit();
    } catch (Throwable $e) {
        if ($pdo->inTransaction()) {
            $pdo->rollBack();
        }
        throw $e;
    }

    $total = count($nuevos) + array_sum(array_map('count', $nuevosTipos));
    error_log('configuracion_guardar.php: ' . $usuario['nombre_usuario'] . ' cambió ' . json_encode([$nuevos, $nuevosTipos]));
    responder([
        'ok' => true,
        'mensaje' => $total === 1 ? 'Se guardó 1 cambio.' : "Se guardaron $total cambios.",
        'cambiados' => $total,
    ]);
} catch (Throwable $e) {
    error_log('configuracion_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
