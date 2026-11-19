<?php
// GET /api/hemo3_listar.php?id_donante=1
// Formularios HEMO 3 del donante (últimos 20). CONFIDENCIAL: solo Doctora, Jefa de Área/Gerente y Administrador.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';
require __DIR__ . '/hemo3_comun.php';

try {
    $pdo = conectar();
    $usuario = requerir_sesion($pdo);
    requerir_rol($usuario, ROLES_HEMO3_VER);

    $idDonante = (int)($_GET['id_donante'] ?? 0);
    if ($idDonante <= 0) {
        responder(['ok' => false, 'error' => 'Falta el donante (id_donante).'], 400);
    }
    if (obtener_donante($pdo, $idDonante) === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }

    $st = $pdo->prepare(
        'SELECT f.id_formulario, f.fecha_registro, f.id_entrevista, f.id_donacion, f.datos,
                u.nombre_completo AS registrado_por, d.numero_donacion
         FROM formulario_confidencial f
         JOIN usuarios u ON u.id_usuario = f.id_usuario_registro
         LEFT JOIN donaciones d ON d.id_donacion = f.id_donacion
         WHERE f.id_donante = :d
         ORDER BY f.fecha_registro DESC, f.id_formulario DESC LIMIT 20'
    );
    $st->execute([':d' => $idDonante]);

    $formularios = [];
    foreach ($st->fetchAll() as $f) {
        $datos = json_decode((string)$f['datos'], true);
        $datos = is_array($datos) ? $datos : [];
        $formularios[] = [
            'id_formulario' => (int)$f['id_formulario'],
            'fecha_registro' => $f['fecha_registro'],
            'registrado_por' => $f['registrado_por'],
            'id_entrevista' => $f['id_entrevista'] !== null ? (int)$f['id_entrevista'] : null,
            'id_donacion' => $f['id_donacion'] !== null ? (int)$f['id_donacion'] : null,
            'numero_donacion' => $f['numero_donacion'],
            'respuestas' => array_map(fn($r) => [
                'pregunta' => (string)($r['pregunta'] ?? ''),
                'respuesta' => (string)($r['respuesta'] ?? ''),
            ], is_array($datos['respuestas'] ?? null) ? $datos['respuestas'] : []),
            'alguna_afirmativa' => count(array_filter(
                is_array($datos['respuestas'] ?? null) ? $datos['respuestas'] : [],
                fn($r) => ($r['respuesta'] ?? '') === 'Sí'
            )) > 0,
            'observaciones' => (string)($datos['observaciones'] ?? ''),
        ];
    }
    responder(['ok' => true, 'formularios' => $formularios]);
} catch (Throwable $e) {
    error_log('hemo3_listar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
