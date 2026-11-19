<?php
// GET /api/donacion_verificar.php?id_donante=1&id_tipo_donacion=1
// Antes de cargar la planilla: dice si el donante puede donar hoy, qué lo impide y cuánto lleva donado.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/donaciones_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_VER_DONACION);

    $idDonante = (int)($_GET['id_donante'] ?? 0);
    $idTipo = (int)($_GET['id_tipo_donacion'] ?? 0);
    if ($idDonante <= 0 || $idTipo <= 0) {
        responder(['ok' => false, 'error' => 'Faltan datos: id_donante e id_tipo_donacion.'], 400);
    }
    $donante = obtener_donante($pdo, $idDonante);
    if ($donante === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }
    $tipo = obtener_tipo_donacion($pdo, $idTipo);
    if ($tipo === null) {
        responder(['ok' => false, 'error' => 'El tipo de donación no existe o no está habilitado.'], 404);
    }

    $bloqueos = [];
    if (!$donante['activo']) {
        $bloqueos[] = 'el donante está dado de baja';
    }
    $ev = evaluar_donacion($pdo, $idDonante, $tipo);
    $bloqueos = array_merge($bloqueos, $ev['bloqueos']);

    responder([
        'ok' => true,
        'puede_donar' => !$bloqueos && !$ev['violaciones'],
        'requiere_excepcion' => !$bloqueos && (bool)$ev['violaciones'],
        'bloqueos' => $bloqueos,
        'violaciones' => $ev['violaciones'],
        'avisos' => $ev['avisos'],
        'entrevista' => $ev['entrevista'],
        'datos' => $ev['datos'],
    ]);
} catch (Throwable $e) {
    error_log('donacion_verificar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
