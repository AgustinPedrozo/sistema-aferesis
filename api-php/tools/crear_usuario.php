<?php
// Solo línea de comandos. Uso (Windows + XAMPP):
//   C:\xampp\php\php.exe crear_usuario.php cc "Cecilia Casola" 1234 "Administrador"
declare(strict_types=1);

if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    exit;
}
require __DIR__ . '/../api/config.php';

if ($argc !== 5) {
    fwrite(STDERR, "Uso: php crear_usuario.php <usuario> \"<Nombre completo>\" <clave> \"<Rol>\"\n");
    exit(1);
}
[, $usuario, $nombre, $clave, $rol] = $argv;

$pdo = conectar();
$st = $pdo->prepare('SELECT id_rol FROM roles WHERE nombre = :r');
$st->execute([':r' => $rol]);
$idRol = $st->fetchColumn();
if ($idRol === false) {
    fwrite(STDERR, "El rol '$rol' no existe.\n");
    exit(1);
}

$pdo->prepare(
    'INSERT INTO usuarios (nombre_usuario, clave_hash, nombre_completo, id_rol)
     VALUES (:u, :h, :n, :r)'
)->execute([
    ':u' => $usuario,
    ':h' => password_hash($clave, PASSWORD_DEFAULT),
    ':n' => $nombre,
    ':r' => $idRol,
]);
echo "Usuario '$usuario' creado con rol '$rol'.\n";
