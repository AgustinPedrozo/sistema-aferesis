<?php
// GET /api/estadisticas_resumen.php            (año actual)
// GET /api/estadisticas_resumen.php?anio=2026
// Resumen para la Jefa de Área/Gerente. Solo lectura y solo para Jefa de Área/Gerente y Administrador.
// Donantes y diferimientos vigentes son "al día de hoy"; donaciones, entrevistas, turnos y causas son del año pedido.
declare(strict_types=1);
require __DIR__ . '/auth.php';

const ROLES_ESTADISTICAS = ['Jefa de Área/Gerente', 'Administrador'];

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_ESTADISTICAS);

    $anio = (int)($_GET['anio'] ?? date('Y'));
    if ($anio < 2000 || $anio > 2100) {
        responder(['ok' => false, 'error' => 'El año debe estar entre 2000 y 2100.'], 400);
    }

    $uno = function (string $sql, array $p = []) use ($pdo): array {
        $st = $pdo->prepare($sql);
        $st->execute($p);
        return $st->fetch() ?: [];
    };
    $varias = function (string $sql, array $p = []) use ($pdo): array {
        $st = $pdo->prepare($sql);
        $st->execute($p);
        return $st->fetchAll();
    };

    // ---------------- Donantes (hoy) ----------------
    $tot = $uno('SELECT COUNT(*) AS total, COALESCE(SUM(activo = TRUE), 0) AS activos FROM donantes');

    $nombresSexo = ['F' => 'Femenino', 'M' => 'Masculino', 'X' => 'Otro'];
    $sexoFilas = $varias('SELECT sexo, COUNT(*) AS n FROM donantes WHERE activo = TRUE GROUP BY sexo');
    $sexo = [];
    foreach ($nombresSexo as $codigo => $nombre) {
        $n = 0;
        foreach ($sexoFilas as $f) {
            if ($f['sexo'] === $codigo) {
                $n = (int)$f['n'];
            }
        }
        $sexo[] = ['sexo' => $nombre, 'cantidad' => $n];
    }

    $rangos = ['Menos de 18' => 0, '18 a 25' => 0, '26 a 35' => 0, '36 a 45' => 0, '46 a 55' => 0, '56 a 65' => 0, 'Más de 65' => 0];
    foreach ($varias('SELECT TIMESTAMPDIFF(YEAR, fecha_nacimiento, CURDATE()) AS edad FROM donantes WHERE activo = TRUE') as $f) {
        $e = (int)$f['edad'];
        $clave = $e < 18 ? 'Menos de 18' : ($e <= 25 ? '18 a 25' : ($e <= 35 ? '26 a 35' : ($e <= 45 ? '36 a 45'
            : ($e <= 55 ? '46 a 55' : ($e <= 65 ? '56 a 65' : 'Más de 65')))));
        $rangos[$clave]++;
    }
    $edades = [];
    foreach ($rangos as $rango => $n) {
        $edades[] = ['rango' => $rango, 'cantidad' => $n];
    }

    $nuevos = (int)$uno('SELECT COUNT(*) AS n FROM donantes WHERE YEAR(fecha_registro) = :a', [':a' => $anio])['n'];

    // ---------------- Donaciones (año) ----------------
    $don = $uno(
        'SELECT COUNT(*) AS total, COUNT(DISTINCT id_donante) AS donantes,
                COALESCE(SUM(cantidad_plaquetas), 0) AS plaquetas, COALESCE(SUM(cantidad_plasma), 0) AS plasma
         FROM donaciones WHERE YEAR(fecha_donacion) = :a',
        [':a' => $anio]
    );
    $porTipo = array_map(fn($f) => ['tipo' => $f['nombre'], 'cantidad' => (int)$f['n']], $varias(
        'SELECT t.nombre, COUNT(d.id_donacion) AS n
         FROM tipos_donacion t
         LEFT JOIN donaciones d ON d.id_tipo_donacion = t.id_tipo_donacion AND YEAR(d.fecha_donacion) = :a
         WHERE t.activo = TRUE
         GROUP BY t.id_tipo_donacion, t.nombre ORDER BY t.id_tipo_donacion',
        [':a' => $anio]
    ));

    $meses = ['Enero', 'Febrero', 'Marzo', 'Abril', 'Mayo', 'Junio', 'Julio', 'Agosto', 'Septiembre', 'Octubre', 'Noviembre', 'Diciembre'];
    $porMes = [];
    foreach ($meses as $i => $nombre) {
        $porMes[$i + 1] = ['mes' => $i + 1, 'nombre' => $nombre, 'donaciones' => 0, 'plaquetas' => 0, 'plasma' => 0];
    }
    foreach ($varias(
        'SELECT MONTH(fecha_donacion) AS m, COUNT(*) AS n, SUM(cantidad_plaquetas) AS pq, SUM(cantidad_plasma) AS pl
         FROM donaciones WHERE YEAR(fecha_donacion) = :a GROUP BY MONTH(fecha_donacion)',
        [':a' => $anio]
    ) as $f) {
        $porMes[(int)$f['m']]['donaciones'] = (int)$f['n'];
        $porMes[(int)$f['m']]['plaquetas'] = (int)$f['pq'];
        $porMes[(int)$f['m']]['plasma'] = (int)$f['pl'];
    }

    $donantesQueDonaron = (int)$don['donantes'];
    $totalDon = (int)$don['total'];

    // ---------------- Diferimientos ----------------
    $vigentes = (int)$uno(
        'SELECT COUNT(DISTINCT id_donante) AS n FROM donante_restricciones
         WHERE fecha_levantamiento IS NULL
           AND (fecha_habilitacion_calculada IS NULL OR fecha_habilitacion_calculada > CURDATE())'
    )['n'];
    $causas = array_map(fn($f) => ['causa' => $f['nombre'], 'cantidad' => (int)$f['n']], $varias(
        'SELECT rm.nombre, COUNT(*) AS n
         FROM donante_restricciones dr JOIN restricciones_medicas rm ON rm.id_restriccion = dr.id_restriccion
         WHERE YEAR(dr.fecha_registro) = :a
         GROUP BY rm.id_restriccion, rm.nombre ORDER BY n DESC, rm.nombre LIMIT 8',
        [':a' => $anio]
    ));

    // ---------------- Entrevistas (año) ----------------
    $ent = $uno(
        'SELECT COUNT(*) AS total, COALESCE(SUM(apto = TRUE), 0) AS aptos
         FROM entrevistas_medicas WHERE YEAR(fecha_entrevista) = :a',
        [':a' => $anio]
    );

    // ---------------- Turnos (año) ----------------
    $estados = ['Pendiente' => 0, 'Completado' => 0, 'Cancelado' => 0, 'Ausente' => 0];
    foreach ($varias('SELECT estado, COUNT(*) AS n FROM turnos WHERE YEAR(fecha_hora) = :a GROUP BY estado', [':a' => $anio]) as $f) {
        $estados[$f['estado']] = (int)$f['n'];
    }
    $base = $estados['Completado'] + $estados['Ausente'];
    $porEstado = [];
    foreach ($estados as $estado => $n) {
        $porEstado[] = ['estado' => $estado, 'cantidad' => $n];
    }

    // ---------------- Control ----------------
    $exc = (int)$uno('SELECT COUNT(*) AS n FROM donaciones WHERE es_excepcion = TRUE AND YEAR(fecha_donacion) = :a', [':a' => $anio])['n'];

    responder([
        'ok' => true,
        'anio' => $anio,
        'generado' => date('Y-m-d H:i'),
        'donantes' => [
            'total' => (int)$tot['total'],
            'activos' => (int)$tot['activos'],
            'inactivos' => (int)$tot['total'] - (int)$tot['activos'],
            'nuevos_en_anio' => $nuevos,
            'por_sexo' => $sexo,
            'por_edad' => $edades,
        ],
        'donaciones' => [
            'total' => $totalDon,
            'donantes_distintos' => $donantesQueDonaron,
            'promedio_por_donante' => $donantesQueDonaron > 0 ? round($totalDon / $donantesQueDonaron, 1) : null,
            'unidades_plaquetas' => (int)$don['plaquetas'],
            'unidades_plasma' => (int)$don['plasma'],
            'por_tipo' => $porTipo,
            'por_mes' => array_values($porMes),
        ],
        'diferimientos' => [
            'donantes_diferidos_vigentes' => $vigentes,
            'causas_mas_frecuentes' => $causas,
        ],
        'entrevistas' => [
            'total' => (int)$ent['total'],
            'aptos' => (int)$ent['aptos'],
            'no_aptos' => (int)$ent['total'] - (int)$ent['aptos'],
        ],
        'turnos' => [
            'por_estado' => $porEstado,
            'ausentismo_porcentaje' => $base > 0 ? round($estados['Ausente'] * 100 / $base, 1) : null,
        ],
        'control' => [
            'excepciones_autorizadas' => $exc,
        ],
    ]);
} catch (Throwable $e) {
    error_log('estadisticas_resumen.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
