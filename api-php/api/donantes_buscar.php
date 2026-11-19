<?php
// GET /api/donantes_buscar.php?q=texto            (solo donantes activos)
// GET /api/donantes_buscar.php?q=texto&inactivos=1 (incluye los dados de baja)
// Si "q" son solo números busca por DNI (empieza con); si no, por apellido y nombre.
declare(strict_types=1);
require __DIR__ . '/auth.php';
require __DIR__ . '/donantes_comun.php';

try {
    $pdo = conectar();
    requerir_sesion($pdo);

    $q = trim((string)($_GET['q'] ?? ''));
    if (mb_strlen($q) < 2) {
        responder(['ok' => false, 'error' => 'Escriba al menos 2 caracteres para buscar.'], 400);
    }
    $filtroActivo = (($_GET['inactivos'] ?? '') === '1') ? '' : ' AND activo = TRUE';

    if (preg_match('/^\d+$/', $q)) {
        $sql = 'SELECT ' . CAMPOS_DONANTE . ' FROM donantes WHERE dni LIKE :q' . $filtroActivo
             . ' ORDER BY apellido, nombre LIMIT 50';
        $params = [':q' => $q . '%'];
    } else {
        // Se escapan % y _ para que el texto escrito se busque tal cual
        $patron = '%' . addcslashes($q, '%_\\') . '%';
        $sql = 'SELECT ' . CAMPOS_DONANTE . ' FROM donantes'
             . ' WHERE (CONCAT(apellido, " ", nombre) LIKE :q1 OR CONCAT(nombre, " ", apellido) LIKE :q2)'
             . $filtroActivo . ' ORDER BY apellido, nombre LIMIT 50';
        $params = [':q1' => $patron, ':q2' => $patron];
    }

    $st = $pdo->prepare($sql);
    $st->execute($params);
    $donantes = array_map('normalizar_donante', $st->fetchAll());

    responder(['ok' => true, 'donantes' => $donantes]);
} catch (Throwable $e) {
    error_log('donantes_buscar.php: ' . $e->getMessage());
    responder(['ok' => false, 'error' => 'Error interno del servidor'], 500);
}
