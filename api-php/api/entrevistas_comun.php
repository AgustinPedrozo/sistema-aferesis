<?php
// Funciones compartidas por los endpoints de entrevistas y restricciones (no se llama directamente desde la app).
declare(strict_types=1);
require_once __DIR__ . '/config.php';
require_once __DIR__ . '/turnos_comun.php';   // validar_fecha()

// Roles que pueden hacer y consultar entrevistas médicas
const ROLES_ENTREVISTA = ['Doctora', 'Entrevistadora/Administrativo', 'Jefa de Área/Gerente', 'Administrador'];

/** Valor entero de un parámetro de configuración (o el valor por defecto si no existe). */
function parametro_int(PDO $pdo, string $clave, int $porDefecto): int
{
    $st = $pdo->prepare('SELECT valor FROM parametros_configuracion WHERE clave = :c');
    $st->execute([':c' => $clave]);
    $valor = $st->fetchColumn();
    return ($valor === false || !is_numeric($valor)) ? $porDefecto : (int)$valor;
}

/** Lee un número opcional (acepta coma o punto). Devuelve [valor|null, error|null]. */
function leer_numero_opcional($crudo, float $min, float $max, string $texto): array
{
    $t = str_replace(',', '.', trim((string)$crudo));
    if ($t === '') {
        return [null, null];
    }
    if (!is_numeric($t) || (float)$t < $min || (float)$t > $max) {
        return [null, "$texto debe ser un número entre $min y $max."];
    }
    return [round((float)$t, 2), null];
}

/** Lee el recuento de plaquetas por mm³. Acepta "150000" o "150.000". Devuelve [valor|null, error|null]. */
function leer_plaquetas($crudo): array
{
    $t = (string)preg_replace('/[\s.,]/', '', trim((string)$crudo));
    if ($t === '') {
        return [null, null];
    }
    if (!ctype_digit($t) || (int)$t < 1000 || (int)$t > 2000000) {
        return [null, 'El recuento de plaquetas debe ser un número completo entre 1000 y 2000000 (por mm³), por ejemplo 210000.'];
    }
    return [(int)$t, null];
}

/** Normaliza una restricción de un donante para enviarla a la aplicación. */
function normalizar_restriccion_donante(array $f): array
{
    $f['id_donante_restriccion'] = (int)$f['id_donante_restriccion'];
    $f['id_restriccion'] = (int)$f['id_restriccion'];
    $f['hasta_nueva_evaluacion'] = (bool)$f['hasta_nueva_evaluacion'];
    $f['vigente'] = (bool)$f['vigente'];
    return $f;
}

/**
 * Restricciones de un donante (todas, o solo las vigentes).
 * Una restricción está vigente si no fue levantada y todavía no llegó su fecha de habilitación
 * (o no tiene fecha: queda hasta una nueva evaluación).
 */
function listar_restricciones_donante(PDO $pdo, int $idDonante, bool $soloVigentes): array
{
    $sql = 'SELECT dr.id_donante_restriccion, dr.id_restriccion, rm.nombre, rm.tipo,
                   dr.fecha_inicio_restriccion, dr.fecha_habilitacion_calculada,
                   dr.hasta_nueva_evaluacion, dr.observaciones, dr.fecha_registro,
                   ur.nombre_completo AS registrada_por,
                   dr.fecha_levantamiento, ul.nombre_completo AS levantada_por,
                   (dr.fecha_levantamiento IS NULL
                    AND (dr.fecha_habilitacion_calculada IS NULL
                         OR dr.fecha_habilitacion_calculada > CURDATE())) AS vigente
            FROM donante_restricciones dr
            JOIN restricciones_medicas rm ON rm.id_restriccion = dr.id_restriccion
            JOIN usuarios ur ON ur.id_usuario = dr.id_usuario_registro
            LEFT JOIN usuarios ul ON ul.id_usuario = dr.id_usuario_levanta
            WHERE dr.id_donante = :d';
    if ($soloVigentes) {
        $sql .= ' AND dr.fecha_levantamiento IS NULL
                  AND (dr.fecha_habilitacion_calculada IS NULL OR dr.fecha_habilitacion_calculada > CURDATE())';
    }
    $sql .= ' ORDER BY dr.fecha_registro DESC, dr.id_donante_restriccion DESC';

    $st = $pdo->prepare($sql);
    $st->execute([':d' => $idDonante]);
    return array_map('normalizar_restriccion_donante', $st->fetchAll());
}

/** Texto corto de una restricción vigente para mensajes: "Tatuaje (hasta 28/01/2027)". */
function describir_restriccion(array $r): string
{
    if (!empty($r['fecha_habilitacion_calculada'])) {
        $f = DateTime::createFromFormat('Y-m-d', (string)$r['fecha_habilitacion_calculada']);
        return $r['nombre'] . ' (hasta ' . ($f ? $f->format('d/m/Y') : $r['fecha_habilitacion_calculada']) . ')';
    }
    return $r['nombre'] . ' (hasta nueva evaluación)';
}
