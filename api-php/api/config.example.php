<?php
// COPIAR este archivo como config.php y completar los datos reales (config.php no se sube a Git).
// Configuración común de la API: conexión a la base y utilidades de respuesta.
declare(strict_types=1);

ini_set('display_errors', '0');   // un error de PHP nunca debe ensuciar el JSON de respuesta

const DB_HOST = '127.0.0.1';
const DB_NOMBRE = 'bancosangre';
const DB_USUARIO = 'bancosangre_app';
const DB_CLAVE = 'cambiar_esta_clave';

const HORAS_SESION = 8;       // duración del token
const MAX_INTENTOS = 5;       // intentos fallidos antes de bloquear
const MINUTOS_BLOQUEO = 15;   // duración del bloqueo

function conectar(): PDO
{
    static $pdo = null;
    if ($pdo === null) {
        $dsn = 'mysql:host=' . DB_HOST . ';dbname=' . DB_NOMBRE . ';charset=utf8mb4';
        $pdo = new PDO($dsn, DB_USUARIO, DB_CLAVE, [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false,
        ]);
    }
    return $pdo;
}

/** Envía la respuesta JSON y termina el script. */
function responder(array $datos, int $codigo = 200)
{
    http_response_code($codigo);
    header('Content-Type: application/json; charset=utf-8');
    echo json_encode($datos, JSON_UNESCAPED_UNICODE);
    exit;
}

/** Lee el cuerpo de la petición como JSON (devuelve [] si no es válido). */
function leer_json(): array
{
    $crudo = file_get_contents('php://input');
    $datos = json_decode($crudo === false ? '' : $crudo, true);
    return is_array($datos) ? $datos : [];
}

function solo_post(): void
{
    if (($_SERVER['REQUEST_METHOD'] ?? '') !== 'POST') {
        responder(['ok' => false, 'error' => 'Método no permitido'], 405);
    }
}
