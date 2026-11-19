<?php
// Funciones compartidas por los endpoints de donantes (no se llama directamente desde la app).
declare(strict_types=1);
require_once __DIR__ . '/config.php';

const CAMPOS_DONANTE = 'id_donante, dni, nombre, apellido, fecha_nacimiento, sexo, domicilio, telefono, email, peso, altura, activo, migrado_desde_papel';

/** Convierte una fila de la base al formato que se envía a la aplicación. */
function normalizar_donante(array $f): array
{
    $f['id_donante'] = (int)$f['id_donante'];
    $f['peso'] = $f['peso'] !== null ? (float)$f['peso'] : null;
    $f['altura'] = $f['altura'] !== null ? (float)$f['altura'] : null;
    $f['activo'] = (bool)$f['activo'];
    $f['migrado_desde_papel'] = (bool)$f['migrado_desde_papel'];
    return $f;
}

/** Devuelve el donante o null si no existe. */
function obtener_donante(PDO $pdo, int $id): ?array
{
    $st = $pdo->prepare('SELECT ' . CAMPOS_DONANTE . ' FROM donantes WHERE id_donante = :id');
    $st->execute([':id' => $id]);
    $fila = $st->fetch();
    return $fila ? normalizar_donante($fila) : null;
}

/**
 * Valida y limpia los datos de un donante.
 * Devuelve [datos_limpios, errores]. Si hay errores, no se debe guardar nada.
 * Los errores son un mapa campo => mensaje, para que la pantalla marque cada campo.
 */
function validar_donante(array $in): array
{
    $errores = [];
    $d = [];

    // DNI: 7 u 8 dígitos (se aceptan puntos y espacios, y se limpian)
    $d['dni'] = (string)preg_replace('/[\s.]/', '', (string)($in['dni'] ?? ''));
    if (!preg_match('/^\d{7,8}$/', $d['dni'])) {
        $errores['dni'] = 'El DNI debe tener 7 u 8 números.';
    }

    foreach (['nombre' => 'el nombre', 'apellido' => 'el apellido'] as $campo => $texto) {
        $d[$campo] = trim((string)($in[$campo] ?? ''));
        if ($d[$campo] === '' || mb_strlen($d[$campo]) > 100) {
            $errores[$campo] = "Escriba $texto (hasta 100 letras).";
        }
    }

    // Fecha de nacimiento: AAAA-MM-DD, real y no futura
    $d['fecha_nacimiento'] = trim((string)($in['fecha_nacimiento'] ?? ''));
    $fecha = DateTime::createFromFormat('Y-m-d', $d['fecha_nacimiento']);
    if ($fecha === false
        || $fecha->format('Y-m-d') !== $d['fecha_nacimiento']
        || $fecha > new DateTime('today')
        || $fecha < new DateTime('1900-01-01')) {
        $errores['fecha_nacimiento'] = 'La fecha de nacimiento no es válida (formato AAAA-MM-DD).';
    }

    $d['sexo'] = strtoupper(trim((string)($in['sexo'] ?? '')));
    if (!in_array($d['sexo'], ['F', 'M', 'X'], true)) {
        $errores['sexo'] = 'Elija el sexo (F, M o X).';
    }

    // Domicilio: opcional
    $d['domicilio'] = trim((string)($in['domicilio'] ?? ''));
    if (mb_strlen($d['domicilio']) > 200) {
        $errores['domicilio'] = 'El domicilio es demasiado largo.';
    }
    if ($d['domicilio'] === '') {
        $d['domicilio'] = null;
    }

    // Teléfono: obligatorio (se usa para los recordatorios de WhatsApp)
    $d['telefono'] = trim((string)($in['telefono'] ?? ''));
    if (!preg_match('/^[0-9+\-\s()]{8,30}$/', $d['telefono'])) {
        $errores['telefono'] = 'Escriba un teléfono válido (números, espacios, + - y paréntesis).';
    }

    // Email: opcional
    $d['email'] = trim((string)($in['email'] ?? ''));
    if ($d['email'] === '') {
        $d['email'] = null;
    } elseif (mb_strlen($d['email']) > 120 || !filter_var($d['email'], FILTER_VALIDATE_EMAIL)) {
        $errores['email'] = 'El email no es válido.';
    }

    // Peso (kg) y altura (cm): opcionales, numéricos y con un rango razonable
    $numero = function (string $campo, float $min, float $max, string $texto) use ($in, &$d, &$errores): void {
        $crudo = str_replace(',', '.', trim((string)($in[$campo] ?? '')));
        if ($crudo === '') {
            $d[$campo] = null;
            return;
        }
        if (!is_numeric($crudo) || (float)$crudo < $min || (float)$crudo > $max) {
            $errores[$campo] = "$texto debe ser un número entre $min y $max.";
            return;
        }
        $d[$campo] = round((float)$crudo, 2);
    };
    $numero('peso', 20, 300, 'El peso (kg)');
    $numero('altura', 100, 250, 'La altura (cm)');

    return [$d, $errores];
}
