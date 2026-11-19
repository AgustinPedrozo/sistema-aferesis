<?php
// Funciones compartidas por los endpoints de donaciones por aféresis (no se llama directamente desde la app).
declare(strict_types=1);
require_once __DIR__ . '/config.php';
require_once __DIR__ . '/turnos_comun.php';        // validar_fecha()
require_once __DIR__ . '/entrevistas_comun.php';   // leer_numero_opcional(), listar_restricciones_donante(), describir_restriccion()
require_once __DIR__ . '/hemo3_comun.php';         // hemo3_existe_hoy()

// Quiénes registran extracciones, y quiénes pueden consultar su estado
const ROLES_EXTRACCION = ['Extractora', 'Jefa de Área/Gerente', 'Administrador'];
const ROLES_VER_DONACION = ['Extractora', 'Doctora', 'Jefa de Área/Gerente', 'Administrador'];
// Quiénes pueden autorizar una extracción fuera de los límites
const ROLES_AUTORIZAN_EXCEPCION = ['Jefa de Área/Gerente', 'Administrador'];

/** Devuelve el tipo de donación activo o null. */
function obtener_tipo_donacion(PDO $pdo, int $id): ?array
{
    $st = $pdo->prepare('SELECT * FROM tipos_donacion WHERE id_tipo_donacion = :id AND activo = TRUE');
    $st->execute([':id' => $id]);
    $f = $st->fetch();
    return $f ?: null;
}

/**
 * Revisa si el donante puede donar ahora.
 *  - bloqueos:    razones médicas que NO se pueden saltear (sin entrevista apto de hoy, restricciones vigentes).
 *  - violaciones: límites que solo se pueden saltear con autorización de la Jefa de Área/Gerente.
 *  - avisos:      recomendaciones que no impiden nada.
 */
function evaluar_donacion(PDO $pdo, int $idDonante, array $tipo): array
{
    $bloqueos = [];
    $violaciones = [];
    $avisos = [];

    // --- Entrevista de hoy: la última del día manda ---
    $st = $pdo->prepare(
        'SELECT id_entrevista, apto, peso_kg, altura_cm, fecha_entrevista
         FROM entrevistas_medicas
         WHERE id_donante = :d AND DATE(fecha_entrevista) = CURDATE()
         ORDER BY fecha_entrevista DESC, id_entrevista DESC LIMIT 1'
    );
    $st->execute([':d' => $idDonante]);
    $entrevista = $st->fetch() ?: null;

    if ($entrevista === null) {
        $bloqueos[] = 'no tiene una entrevista médica de hoy';
    } elseif (!(bool)$entrevista['apto']) {
        $bloqueos[] = 'la entrevista de hoy lo marcó como NO APTO';
    }

    $vigentes = listar_restricciones_donante($pdo, $idDonante, true);
    if ($vigentes) {
        $bloqueos[] = 'tiene restricciones vigentes: ' . implode(', ', array_map('describir_restriccion', $vigentes));
    }

    // --- Formulario confidencial HEMO 3: no impide la extracción, solo se avisa ---
    if (!hemo3_existe_hoy($pdo, $idDonante)) {
        $avisos[] = 'Todavía no se cargó el formulario confidencial HEMO 3 de hoy.';
    }

    // --- Límites (se cuentan todas las aféresis del donante, sin importar el tipo) ---
    $st = $pdo->prepare(
        'SELECT MAX(fecha_donacion) AS ultima,
                DATE_ADD(MAX(fecha_donacion), INTERVAL :h HOUR) AS habilitado_desde,
                (DATE_ADD(MAX(fecha_donacion), INTERVAL :h2 HOUR) > NOW()) AS en_intervalo,
                (TIMESTAMPDIFF(HOUR, MAX(fecha_donacion), NOW()) < :rec) AS antes_de_lo_recomendado,
                SUM(YEARWEEK(fecha_donacion, 1) = YEARWEEK(NOW(), 1)) AS en_semana,
                SUM(YEAR(fecha_donacion) = YEAR(NOW())) AS en_anio
         FROM donaciones WHERE id_donante = :d'
    );
    $st->execute([
        ':h' => (int)$tipo['intervalo_minimo_horas'],
        ':h2' => (int)$tipo['intervalo_minimo_horas'],
        ':rec' => (int)$tipo['intervalo_recomendado_min_dias'] * 24,
        ':d' => $idDonante,
    ]);
    $c = $st->fetch();
    $enSemana = (int)($c['en_semana'] ?? 0);
    $enAnio = (int)($c['en_anio'] ?? 0);
    $limSemana = (int)$tipo['limite_semanal'];
    $limAnio = (int)$tipo['limite_anual'];

    if ($c['ultima'] !== null && (int)$c['en_intervalo'] === 1) {
        $f = DateTime::createFromFormat('Y-m-d H:i:s', (string)$c['habilitado_desde']);
        $violaciones[] = 'no pasaron las ' . (int)$tipo['intervalo_minimo_horas'] . ' horas mínimas desde la última donación; puede donar desde el '
            . ($f ? $f->format('d/m/Y H:i') : $c['habilitado_desde']);
    } elseif ($c['ultima'] !== null && (int)$c['antes_de_lo_recomendado'] === 1) {
        $avisos[] = 'Se recomienda esperar entre ' . (int)$tipo['intervalo_recomendado_min_dias'] . ' y '
            . (int)$tipo['intervalo_recomendado_max_dias'] . ' días entre donaciones.';
    }
    if ($enSemana >= $limSemana) {
        $violaciones[] = "ya hizo $enSemana donaciones esta semana (máximo $limSemana)";
    }
    if ($enAnio >= $limAnio) {
        $violaciones[] = "ya hizo $enAnio donaciones este año (máximo $limAnio)";
    }

    return [
        'bloqueos' => $bloqueos,
        'violaciones' => $violaciones,
        'avisos' => $avisos,
        'entrevista' => $entrevista ? [
            'id_entrevista' => (int)$entrevista['id_entrevista'],
            'apto' => (bool)$entrevista['apto'],
            'peso_kg' => $entrevista['peso_kg'] !== null ? (float)$entrevista['peso_kg'] : null,
            'altura_cm' => $entrevista['altura_cm'] !== null ? (float)$entrevista['altura_cm'] : null,
        ] : null,
        'datos' => [
            'ultima_donacion' => $c['ultima'],
            'habilitado_desde' => $c['ultima'] !== null ? $c['habilitado_desde'] : null,
            'donaciones_semana' => $enSemana,
            'limite_semanal' => $limSemana,
            'donaciones_anio' => $enAnio,
            'limite_anual' => $limAnio,
        ],
    ];
}

/** Lee una hora "HH:MM". Devuelve [valor|null, error|null]. */
function leer_hora($crudo, string $texto): array
{
    $t = trim((string)$crudo);
    if ($t === '') {
        return [null, null];
    }
    $d = DateTime::createFromFormat('H:i', $t);
    if ($d === false || $d->format('H:i') !== $t) {
        return [null, "$texto debe tener formato HH:MM (por ejemplo 09:30)."];
    }
    return [$t . ':00', null];
}

/** Texto opcional con largo máximo. */
function leer_texto_opcional($crudo, int $max): ?string
{
    $t = trim((string)$crudo);
    return $t === '' ? null : mb_substr($t, 0, $max);
}

/**
 * Datos técnicos de la planilla HEMO 2. Se guardan tal cual los carga el personal: solo se controla
 * que cada dato tenga el tipo correcto (número, hora, fecha) y un rango muy amplio para atajar errores de tipeo.
 * Devuelve [datos, errores].
 */
function leer_detalle_hemo2(array $d): array
{
    $e = [];
    $r = [];

    foreach ([
        'hto' => [0, 100, 'El hematocrito (Hto)'],
        'hb' => [0, 30, 'La hemoglobina (Hb)'],
        'temperatura' => [30, 45, 'La temperatura'],
        'peso_bolsa' => [0, 5000, 'El peso de la bolsa'],
    ] as $campo => [$min, $max, $texto]) {
        [$r[$campo], $err] = leer_numero_opcional($d[$campo] ?? '', (float)$min, (float)$max, $texto);
        if ($err) {
            $e[$campo] = $err;
        }
    }

    $pulso = trim((string)($d['pulso'] ?? ''));
    if ($pulso === '') {
        $r['pulso'] = null;
    } elseif (!ctype_digit($pulso) || (int)$pulso < 20 || (int)$pulso > 250) {
        $r['pulso'] = null;
        $e['pulso'] = 'El pulso debe ser un número entero entre 20 y 250.';
    } else {
        $r['pulso'] = (int)$pulso;
    }

    $ta = trim((string)($d['ta'] ?? ''));
    if ($ta !== '' && !preg_match('/^\d{2,3}\/\d{2,3}$/', $ta)) {
        $e['ta'] = 'La tensión arterial debe tener formato 120/80.';
        $ta = '';
    }
    $r['ta'] = $ta === '' ? null : $ta;

    $bolsa = trim((string)($d['tipo_bolsa'] ?? ''));
    $mapaBolsa = ['doble' => 'Doble', 'triple' => 'Triple', 'cuadruple' => 'Cuádruple', 'cuádruple' => 'Cuádruple'];
    if ($bolsa === '') {
        $r['tipo_bolsa'] = null;
    } elseif (isset($mapaBolsa[mb_strtolower($bolsa)])) {
        $r['tipo_bolsa'] = $mapaBolsa[mb_strtolower($bolsa)];
    } else {
        $r['tipo_bolsa'] = null;
        $e['tipo_bolsa'] = 'El tipo de bolsa debe ser Doble, Triple o Cuádruple.';
    }

    $brazo = trim((string)($d['brazo'] ?? ''));
    $mapaBrazo = ['derecho' => 'Derecho', 'izquierdo' => 'Izquierdo'];
    if ($brazo === '') {
        $r['brazo'] = null;
    } elseif (isset($mapaBrazo[mb_strtolower($brazo)])) {
        $r['brazo'] = $mapaBrazo[mb_strtolower($brazo)];
    } else {
        $r['brazo'] = null;
        $e['brazo'] = 'El brazo debe ser Derecho o Izquierdo.';
    }

    $dif = $d['dificultades'] ?? null;
    $r['dificultades'] = ($dif === null || $dif === '') ? null : (int)filter_var($dif, FILTER_VALIDATE_BOOLEAN);

    $r['marca_bolsa'] = leer_texto_opcional($d['marca_bolsa'] ?? '', 50);
    $r['lote_bolsa'] = leer_texto_opcional($d['lote_bolsa'] ?? '', 50);
    $r['nro_tubuladura'] = leer_texto_opcional($d['nro_tubuladura'] ?? '', 50);
    $r['anticoagulante'] = leer_texto_opcional($d['anticoagulante'] ?? '', 50);

    $venc = trim((string)($d['vencimiento_bolsa'] ?? ''));
    if ($venc !== '' && !validar_fecha($venc)) {
        $e['vencimiento_bolsa'] = 'El vencimiento de la bolsa debe tener formato AAAA-MM-DD.';
        $venc = '';
    }
    $r['vencimiento_bolsa'] = $venc === '' ? null : $venc;

    [$r['hora_inicio'], $err] = leer_hora($d['hora_inicio'] ?? '', 'La hora de inicio');
    if ($err) {
        $e['hora_inicio'] = $err;
    }
    [$r['hora_fin'], $err] = leer_hora($d['hora_fin'] ?? '', 'La hora de finalización');
    if ($err) {
        $e['hora_fin'] = $err;
    }
    if (!isset($e['hora_inicio']) && !isset($e['hora_fin']) && $r['hora_inicio'] && $r['hora_fin'] && $r['hora_fin'] < $r['hora_inicio']) {
        $e['hora_fin'] = 'La hora de finalización no puede ser anterior a la de inicio.';
    }

    return [$r, $e];
}
