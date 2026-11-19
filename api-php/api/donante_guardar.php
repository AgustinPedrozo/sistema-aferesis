<?php
// POST /api/donante_guardar.php
// Sin "id_donante" en el JSON -> ALTA (crea el donante).
// Con "id_donante"            -> MODIFICACIÓN (actualiza los datos personales).
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';

solo_post();

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $entrada = leer_json();
    [$d, $errores] = validar_donante($entrada);
    if ($errores) {
        responder(['ok' => false, 'error' => 'Hay datos para corregir.', 'campos' => $errores], 422);
    }

    $params = [];
    foreach ($d as $campo => $valor) {
        $params[':' . $campo] = $valor;
    }

    $id = (int)($entrada['id_donante'] ?? 0);

    if ($id === 0) {
        $pdo->prepare(
            'INSERT INTO donantes (dni, nombre, apellido, fecha_nacimiento, sexo, domicilio, telefono, email, peso, altura)
             VALUES (:dni, :nombre, :apellido, :fecha_nacimiento, :sexo, :domicilio, :telefono, :email, :peso, :altura)'
        )->execute($params);
        $nuevoId = (int)$pdo->lastInsertId();
        responder(['ok' => true, 'mensaje' => 'Donante registrado.', 'donante' => obtener_donante($pdo, $nuevoId)], 201);
    }

    if (obtener_donante($pdo, $id) === null) {
        responder(['ok' => false, 'error' => 'No se encontró el donante.'], 404);
    }
    $params[':id'] = $id;
    $pdo->prepare(
        'UPDATE donantes SET dni = :dni, nombre = :nombre, apellido = :apellido,
                fecha_nacimiento = :fecha_nacimiento, sexo = :sexo, domicilio = :domicilio,
                telefono = :telefono, email = :email, peso = :peso, altura = :altura
         WHERE id_donante = :id'
    )->execute($params);
    responder(['ok' => true, 'mensaje' => 'Datos del donante actualizados.', 'donante' => obtener_donante($pdo, $id)]);
} catch (PDOException $e) {
    // 1062 = valor repetido en una columna UNIQUE (acá, el DNI)
    if ((int)($e->errorInfo[1] ?? 0) === 1062) {
        responder([
            'ok' => false,
            'error' => 'Ya existe un donante registrado con ese DNI.',
            'campos' => ['dni' => 'DNI ya registrado.'],
        ], 409);
    }
    error_log('donante_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
} catch (Throwable $e) {
    error_log('donante_guardar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
