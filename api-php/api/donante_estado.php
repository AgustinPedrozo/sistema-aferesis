<?php
// POST /api/donante_estado.php   Cuerpo: {"id_donante": 1, "activo": false}
// BAJA lógica (activo = false) o reactivación (activo = true). Nunca se borra el donante:
// su historial de donaciones tiene que conservarse.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';

solo_post();

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ['Jefa de Área/Gerente', 'Administrador']);

    $entrada = leer_json();
    $id = (int)($entrada['id_donante'] ?? 0);
    if ($id <= 0 || !isset($entrada['activo']) || !is_bool($entrada['activo'])) {
        responder(['ok' => false, 'error' => 'Datos incompletos: se necesita id_donante y activo (true o false).'], 400);
    }

    if (obtener_donante($pdo, $id) === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }

    $pdo->prepare('UPDATE donantes SET activo = :a WHERE id_donante = :id')
        ->execute([':a' => $entrada['activo'] ? 1 : 0, ':id' => $id]);

    $mensaje = $entrada['activo'] ? 'Donante reactivado.' : 'Donante dado de baja.';
    responder(['ok' => true, 'mensaje' => $mensaje, 'donante' => obtener_donante($pdo, $id)]);
} catch (Throwable $e) {
    error_log('donante_estado.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
