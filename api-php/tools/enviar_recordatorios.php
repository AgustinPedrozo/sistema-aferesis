<?php
// Solo línea de comandos. Envía los recordatorios de turnos que correspondan. Está pensado para correr solo,
// cada 10 minutos, con el Programador de tareas de Windows. Prueba manual (Windows + XAMPP):
//   C:\xampp\php\php.exe C:\xampp\htdocs\bancosangre\tools\enviar_recordatorios.php
declare(strict_types=1);

if (PHP_SAPI !== 'cli') {
    http_response_code(403);
    exit;
}
require __DIR__ . '/../api/whatsapp_comun.php';

try {
    $pdo = conectar();
    $r = procesar_recordatorios($pdo);
    echo date('Y-m-d H:i:s') . ' - revisados: ' . $r['revisados'] . ', enviados: ' . $r['enviados']
        . ', con error: ' . $r['con_error'] . PHP_EOL;
    foreach ($r['detalle'] as $d) {
        echo '  ' . $d['estado'] . ' | ' . $d['donante'] . ' | turno ' . $d['turno'] . ' | ' . $d['detalle'] . PHP_EOL;
    }
} catch (Throwable $e) {
    fwrite(STDERR, date('Y-m-d H:i:s') . ' - ERROR: ' . $e->getMessage() . PHP_EOL);
    exit(1);
}
