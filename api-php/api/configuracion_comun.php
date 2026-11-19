<?php
// Funciones compartidas por los endpoints del panel de configuración (no se llama directamente desde la app).
// Solo se pueden cambiar los parámetros de esta lista, y siempre dentro de su rango.
declare(strict_types=1);
require_once __DIR__ . '/config.php';

const ROLES_CONFIGURACION = ['Jefa de Área/Gerente', 'Administrador'];

const PARAMETROS_EDITABLES = [
    'limite_diario_donaciones' => [
        'etiqueta' => 'Turnos por día', 'unidad' => 'turnos', 'min' => 1, 'max' => 500, 'defecto' => 10,
        'ayuda' => 'Cantidad máxima de turnos que se pueden dar en un mismo día.',
    ],
    'limite_semanal_donaciones' => [
        'etiqueta' => 'Turnos por semana', 'unidad' => 'turnos', 'min' => 1, 'max' => 3000, 'defecto' => 50,
        'ayuda' => 'Cantidad máxima de turnos que se pueden dar en una misma semana (de lunes a domingo).',
    ],
    'limite_mensual_donaciones' => [
        'etiqueta' => 'Turnos por mes', 'unidad' => 'turnos', 'min' => 1, 'max' => 10000, 'defecto' => 200,
        'ayuda' => 'Cantidad máxima de turnos que se pueden dar en un mismo mes.',
    ],
    'edad_minima' => [
        'etiqueta' => 'Edad mínima del donante', 'unidad' => 'años', 'min' => 16, 'max' => 40, 'defecto' => 18,
        'ayuda' => 'El donante no puede ser apto si es menor a esta edad.',
    ],
    'edad_maxima' => [
        'etiqueta' => 'Edad máxima del donante', 'unidad' => 'años', 'min' => 50, 'max' => 90, 'defecto' => 65,
        'ayuda' => 'El donante no puede ser apto si es mayor a esta edad.',
    ],
    'peso_minimo_kg' => [
        'etiqueta' => 'Peso mínimo del donante', 'unidad' => 'kg', 'min' => 40, 'max' => 120, 'defecto' => 50,
        'ayuda' => 'El donante debe pesar más que este valor para ser apto.',
    ],
    'recuento_minimo_plaquetas' => [
        'etiqueta' => 'Recuento mínimo de plaquetas', 'unidad' => 'por mm³', 'min' => 50000, 'max' => 400000, 'defecto' => 150000,
        'ayuda' => 'Plaquetas por mm³ que debe tener el análisis previo para ser apto.',
    ],
    'horas_aviso_turno' => [
        'etiqueta' => 'Aviso de turno por WhatsApp', 'unidad' => 'horas antes', 'min' => 1, 'max' => 168, 'defecto' => 24,
        'ayuda' => 'Con cuántas horas de anticipación se envía el recordatorio del turno.',
    ],
];

/** Valores actuales de los parámetros editables: [clave => int]. Si falta alguno en la base, se usa su valor por defecto. */
function configuracion_actual(PDO $pdo): array
{
    $valores = [];
    foreach ($pdo->query('SELECT clave, valor FROM parametros_configuracion')->fetchAll() as $f) {
        $valores[$f['clave']] = $f['valor'];
    }
    $r = [];
    foreach (PARAMETROS_EDITABLES as $clave => $def) {
        $v = $valores[$clave] ?? null;
        $r[$clave] = ($v !== null && ctype_digit((string)$v)) ? (int)$v : $def['defecto'];
    }
    return $r;
}

// ---------------------------------------------------------------------------------------------
// Límites de cada donante, que son distintos para cada tipo de donación (viven en tipos_donacion).
// En la pantalla aparecen en la misma lista que el resto, con la clave "tipo.<id>.<campo>".
// ---------------------------------------------------------------------------------------------
const CAMPOS_TIPO = [
    'intervalo_minimo_horas' => [
        'etiqueta' => 'horas mínimas entre donaciones', 'unidad' => 'horas', 'min' => 1, 'max' => 720,
        'ayuda' => 'Tiempo que tiene que pasar entre una extracción y la siguiente del mismo donante (72 horas = 3 días).',
    ],
    'limite_semanal' => [
        'etiqueta' => 'donaciones por semana', 'unidad' => 'por donante', 'min' => 1, 'max' => 7,
        'ayuda' => 'Máximo de extracciones de un mismo donante en una semana (de lunes a domingo).',
    ],
    'limite_anual' => [
        'etiqueta' => 'donaciones por año', 'unidad' => 'por donante', 'min' => 1, 'max' => 365,
        'ayuda' => 'Máximo de extracciones de un mismo donante en el año calendario (del 1 de enero al 31 de diciembre).',
    ],
];

/** Tipos de donación con sus límites actuales: [id => ['nombre' => ..., campo => int, ...]] */
function configuracion_tipos(PDO $pdo): array
{
    $r = [];
    foreach ($pdo->query(
        'SELECT id_tipo_donacion, nombre, intervalo_minimo_horas, limite_semanal, limite_anual
         FROM tipos_donacion ORDER BY id_tipo_donacion'
    )->fetchAll() as $f) {
        $r[(int)$f['id_tipo_donacion']] = [
            'nombre' => $f['nombre'],
            'intervalo_minimo_horas' => (int)$f['intervalo_minimo_horas'],
            'limite_semanal' => (int)$f['limite_semanal'],
            'limite_anual' => (int)$f['limite_anual'],
        ];
    }
    return $r;
}
