<?php
// GET /api/configuracion_listar.php
// Parámetros del sistema que la Jefa de Área/Gerente puede ajustar, con su valor actual y su rango:
// primero los generales y después los límites de cada donante para cada tipo de donación.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/configuracion_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_CONFIGURACION);

    $actual = configuracion_actual($pdo);
    $lista = [];
    foreach (PARAMETROS_EDITABLES as $clave => $def) {
        $lista[] = [
            'clave' => $clave,
            'etiqueta' => $def['etiqueta'],
            'ayuda' => $def['ayuda'],
            'unidad' => $def['unidad'],
            'min' => $def['min'],
            'max' => $def['max'],
            'valor' => $actual[$clave],
        ];
    }
    foreach (configuracion_tipos($pdo) as $id => $tipo) {
        foreach (CAMPOS_TIPO as $campo => $def) {
            $lista[] = [
                'clave' => "tipo.$id.$campo",
                'etiqueta' => $tipo['nombre'] . ': ' . $def['etiqueta'],
                'ayuda' => $def['ayuda'],
                'unidad' => $def['unidad'],
                'min' => $def['min'],
                'max' => $def['max'],
                'valor' => $tipo[$campo],
            ];
        }
    }
    responder(['ok' => true, 'parametros' => $lista]);
} catch (Throwable $e) {
    error_log('configuracion_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
