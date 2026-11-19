<?php
// GET /api/hemo3_campos.php?id_donante=1
// Opciones del formulario HEMO 3 y si el donante ya tiene uno cargado hoy (sin mostrar su contenido).
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/hemo3_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_HEMO3_CARGA);

    $respuesta = ['ok' => true] + hemo3_catalogo();
    $idDonante = (int)($_GET['id_donante'] ?? 0);
    if ($idDonante > 0) {
        $respuesta['existe_hoy'] = hemo3_existe_hoy($pdo, $idDonante);
    }
    responder($respuesta);
} catch (Throwable $e) {
    error_log('hemo3_campos.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
