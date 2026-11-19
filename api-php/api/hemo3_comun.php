<?php
// Funciones compartidas por los endpoints del formulario confidencial HEMO 3 (no se llama directamente desde la app).
// El formulario es CONFIDENCIAL: cargarlo puede casi todo el personal, pero verlo solo Doctora, Jefa y Administrador.
declare(strict_types=1);
require_once __DIR__ . '/config.php';

const ROLES_HEMO3_CARGA = ['Doctora', 'Entrevistadora/Administrativo', 'Jefa de Área/Gerente', 'Administrador'];
const ROLES_HEMO3_VER = ['Doctora', 'Jefa de Área/Gerente', 'Administrador'];

// ---------------------------------------------------------------------------------------------
// CONTENIDO DEL FORMULARIO: las preguntas del HEMO 3 (cada una se responde Sí o No).
// Está todo acá, en un solo lugar: si el formulario cambia, se cambia esta lista y la pantalla
// muestra las preguntas nuevas sin tocar nada más. La clave de cada pregunta no debe cambiar
// una vez que hay formularios cargados.
// ---------------------------------------------------------------------------------------------
const HEMO3_PREGUNTAS = [
    'relaciones_transfundidos' => '¿Ha tenido relaciones sexuales sin preservativo u otros métodos de barrera con personas que reciben transfusiones o tratamiento de diálisis?',
    'relaciones_ocasionales' => '¿Ha tenido relaciones sexuales sin preservativo u otros métodos de barrera con parejas ocasionales?',
    'consumo_sustancias' => '¿Consumió cocaína, paco, éxtasis o tusi?',
];

/** Catálogo para armar la pantalla: preguntas [{clave, texto}, ...] */
function hemo3_catalogo(): array
{
    $preguntas = [];
    foreach (HEMO3_PREGUNTAS as $clave => $texto) {
        $preguntas[] = ['clave' => $clave, 'texto' => $texto];
    }
    return ['preguntas' => $preguntas];
}

/** Interpreta una respuesta Sí/No: true, false o null si no se entiende. */
function hemo3_leer_respuesta($valor): ?bool
{
    if (is_bool($valor)) {
        return $valor;
    }
    $t = mb_strtolower(trim((string)$valor));
    if (in_array($t, ['si', 'sí', 'true', '1'], true)) {
        return true;
    }
    if (in_array($t, ['no', 'false', '0'], true)) {
        return false;
    }
    return null;
}

/** ¿Hay un formulario HEMO 3 de hoy para este donante? (no lee su contenido) */
function hemo3_existe_hoy(PDO $pdo, int $idDonante): bool
{
    $st = $pdo->prepare('SELECT 1 FROM formulario_confidencial WHERE id_donante = :d AND DATE(fecha_registro) = CURDATE() LIMIT 1');
    $st->execute([':d' => $idDonante]);
    return (bool)$st->fetchColumn();
}
