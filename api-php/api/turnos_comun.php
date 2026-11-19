<?php
// Funciones compartidas por los endpoints de turnos (no se llama directamente desde la app).
declare(strict_types=1);
require_once __DIR__ . '/config.php';

const ESTADOS_TURNO = ['Pendiente', 'Completado', 'Cancelado', 'Ausente'];

// Consulta base: un turno con los datos del donante y el tipo de donación.
const SELECT_TURNO = 'SELECT t.id_turno, t.id_donante, d.dni, d.apellido, d.nombre, d.telefono,
        t.id_tipo_donacion, td.nombre AS tipo_donacion, t.fecha_hora, t.lugar, t.estado
    FROM turnos t
    JOIN donantes d ON d.id_donante = t.id_donante
    JOIN tipos_donacion td ON td.id_tipo_donacion = t.id_tipo_donacion';

/** Convierte una fila de la base al formato que se envía a la aplicación. */
function normalizar_turno(array $f): array
{
    $f['id_turno'] = (int)$f['id_turno'];
    $f['id_donante'] = (int)$f['id_donante'];
    $f['id_tipo_donacion'] = (int)$f['id_tipo_donacion'];
    // fecha_hora llega como "2026-10-21 09:30:00": se separa para que la pantalla lo use fácil
    $f['fecha'] = substr((string)$f['fecha_hora'], 0, 10);
    $f['hora'] = substr((string)$f['fecha_hora'], 11, 5);
    return $f;
}

/** Devuelve el turno o null si no existe. */
function obtener_turno(PDO $pdo, int $id): ?array
{
    $st = $pdo->prepare(SELECT_TURNO . ' WHERE t.id_turno = :id');
    $st->execute([':id' => $id]);
    $fila = $st->fetch();
    return $fila ? normalizar_turno($fila) : null;
}

/** Valida "AAAA-MM-DD HH:MM". Devuelve el DateTime o null si no es válido. */
function validar_fecha_hora(string $valor): ?DateTime
{
    $dt = DateTime::createFromFormat('Y-m-d H:i', $valor);
    return ($dt !== false && $dt->format('Y-m-d H:i') === $valor) ? $dt : null;
}

/** Valida "AAAA-MM-DD". */
function validar_fecha(string $valor): bool
{
    $d = DateTime::createFromFormat('Y-m-d', $valor);
    return $d !== false && $d->format('Y-m-d') === $valor;
}
