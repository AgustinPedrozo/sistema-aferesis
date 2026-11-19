-- =====================================================================
-- Sistema de Gestión de Donación de Plaquetas y Plasma por Aféresis
-- Hemocentro Regional de Mar del Plata  -  Base de datos (versión final)
-- MariaDB (XAMPP), InnoDB.
--
-- CÓMO USARLO (base nueva y vacía):
--   1. phpMyAdmin -> entrar como root -> pestaña SQL.
--   2. Pegar TODO este archivo y ejecutar.
--      (Si phpMyAdmin se queja por el trigger del final, poner $$ en el campo
--       "Delimitador" debajo del cuadro de texto y ejecutar de nuevo.)
--   3. Crear los usuarios del sistema desde la línea de comandos:
--        C:\xampp\php\php.exe C:\xampp\htdocs\bancosangre\tools\crear_usuario.php ad "Administrador" 1234 "Administrador"
--
--     DROP DATABASE bancosangre;
--
-- Este archivo ya incluye todo lo que antes eran scripts aparte
-- (api_setup, entrevistas_setup y whatsapp_setup): NO hay que correrlos de nuevo.
-- =====================================================================

CREATE DATABASE IF NOT EXISTS bancosangre
    CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE bancosangre;

-- ---------------------------------------------------------------------
-- 1. ROLES, PERMISOS, USUARIOS Y SESIONES
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS roles (
    id_rol INT AUTO_INCREMENT PRIMARY KEY,
    nombre VARCHAR(50) UNIQUE NOT NULL
);

CREATE TABLE IF NOT EXISTS permisos (
    id_permiso INT AUTO_INCREMENT PRIMARY KEY,
    clave VARCHAR(60) UNIQUE NOT NULL,      -- ej: 'donaciones.autorizar_excepcion'
    descripcion VARCHAR(200)
);

CREATE TABLE IF NOT EXISTS rol_permisos (
    id_rol INT NOT NULL,
    id_permiso INT NOT NULL,
    PRIMARY KEY (id_rol, id_permiso),
    FOREIGN KEY (id_rol) REFERENCES roles(id_rol),
    FOREIGN KEY (id_permiso) REFERENCES permisos(id_permiso)
);

CREATE TABLE IF NOT EXISTS usuarios (
    id_usuario INT AUTO_INCREMENT PRIMARY KEY,
    nombre_usuario VARCHAR(50) UNIQUE NOT NULL,   -- iniciales, ej: 'cc'
    clave_hash VARCHAR(255) NOT NULL,             -- siempre hash, nunca texto plano
    nombre_completo VARCHAR(100) NOT NULL,
    id_rol INT NOT NULL,
    activo BOOLEAN DEFAULT TRUE,
    intentos_fallidos INT NOT NULL DEFAULT 0,     -- bloqueo por fuerza bruta (claves numéricas)
    bloqueado_hasta DATETIME NULL,
    fecha_creacion DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_rol) REFERENCES roles(id_rol)
);

-- Sesiones de la API (escritorio y móvil). Se guarda el hash del token, nunca el token.
CREATE TABLE IF NOT EXISTS sesiones (
    id_sesion INT AUTO_INCREMENT PRIMARY KEY,
    id_usuario INT NOT NULL,
    token_hash CHAR(64) UNIQUE NOT NULL,
    fecha_creacion DATETIME DEFAULT CURRENT_TIMESTAMP,
    fecha_expiracion DATETIME NOT NULL,
    FOREIGN KEY (id_usuario) REFERENCES usuarios(id_usuario),
    INDEX idx_sesiones_expiracion (fecha_expiracion)
);

-- ---------------------------------------------------------------------
-- 2. TIPOS DE DONACION Y SUS LIMITES (PLAQUETAS, PLASMA, AMBOS)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS tipos_donacion (
    id_tipo_donacion INT AUTO_INCREMENT PRIMARY KEY,
    nombre VARCHAR(50) UNIQUE NOT NULL,
    intervalo_minimo_horas INT NOT NULL DEFAULT 72,        -- obligatorio entre procedimientos
    intervalo_recomendado_min_dias INT NOT NULL DEFAULT 7, -- recomendación oficial (solo aviso)
    intervalo_recomendado_max_dias INT NOT NULL DEFAULT 15,
    limite_semanal INT NOT NULL DEFAULT 2,                 -- por donante
    limite_anual INT NOT NULL DEFAULT 24,                  -- por donante, año calendario
    horas_validez_certificado INT NOT NULL DEFAULT 36,     -- (sin uso: el certificado en PDF quedó fuera del alcance)
    activo BOOLEAN NOT NULL DEFAULT TRUE                   -- habilitado desde el panel de configuración
);

-- ---------------------------------------------------------------------
-- 3. DONANTES
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS donantes (
    id_donante INT AUTO_INCREMENT PRIMARY KEY,
    dni VARCHAR(20) UNIQUE NOT NULL,
    nombre VARCHAR(100) NOT NULL,
    apellido VARCHAR(100) NOT NULL,
    fecha_nacimiento DATE NOT NULL,
    sexo ENUM('F','M','X') NOT NULL,
    domicilio VARCHAR(200),
    telefono VARCHAR(30) NOT NULL,                    -- usado para WhatsApp
    email VARCHAR(120),
    peso DECIMAL(5,2),                                -- kg (último valor registrado)
    altura DECIMAL(5,2),                              -- cm (último valor registrado)
    activo BOOLEAN NOT NULL DEFAULT TRUE,             -- baja lógica
    fecha_registro DATETIME DEFAULT CURRENT_TIMESTAMP,
    migrado_desde_papel BOOLEAN DEFAULT FALSE,
    ficha_papel_foto VARCHAR(255) NULL                -- opcional: foto de la hoja física
);

-- ---------------------------------------------------------------------
-- 4. CATALOGO DE RESTRICCIONES MEDICAS (diferimientos configurables)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS restricciones_medicas (
    id_restriccion INT AUTO_INCREMENT PRIMARY KEY,
    nombre VARCHAR(100) UNIQUE NOT NULL,
    tipo ENUM('Temporal','Definitiva') NOT NULL DEFAULT 'Temporal',
    dias_espera_minimos INT NULL,
    dias_espera_maximos INT NULL,
    requiere_reevaluacion BOOLEAN NOT NULL DEFAULT FALSE,
    observaciones TEXT NULL,
    activa BOOLEAN NOT NULL DEFAULT TRUE
);

-- ---------------------------------------------------------------------
-- 5. TURNOS
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS turnos (
    id_turno INT AUTO_INCREMENT PRIMARY KEY,
    id_donante INT NOT NULL,
    id_tipo_donacion INT NOT NULL,
    fecha_hora DATETIME NOT NULL,
    lugar VARCHAR(150),                               -- se incluye en el recordatorio
    estado ENUM('Pendiente','Completado','Cancelado','Ausente') NOT NULL DEFAULT 'Pendiente',
    id_usuario_registro INT NULL,
    fecha_registro DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_donante) REFERENCES donantes(id_donante),
    FOREIGN KEY (id_tipo_donacion) REFERENCES tipos_donacion(id_tipo_donacion),
    FOREIGN KEY (id_usuario_registro) REFERENCES usuarios(id_usuario),
    INDEX idx_turnos_fecha (fecha_hora)
);

-- ---------------------------------------------------------------------
-- 6. ENTREVISTA MEDICA Y APTITUD
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS entrevistas_medicas (
    id_entrevista INT AUTO_INCREMENT PRIMARY KEY,
    id_donante INT NOT NULL,
    id_usuario_entrevistador INT NOT NULL,            -- doctora o entrevistador calificado
    id_turno INT NULL,
    fecha_entrevista DATETIME DEFAULT CURRENT_TIMESTAMP,
    recuento_plaquetas INT NULL,                      -- plaquetas/mm³ del análisis previo (mínimo 150000)
    peso_kg DECIMAL(5,2) NULL,                        -- kg medidos el día de la entrevista
    altura_cm DECIMAL(5,2) NULL,                      -- cm medidos el día de la entrevista
    apto BOOLEAN NOT NULL,                            -- Apto / No apto (diferido)
    observaciones TEXT,
    FOREIGN KEY (id_donante) REFERENCES donantes(id_donante),
    FOREIGN KEY (id_usuario_entrevistador) REFERENCES usuarios(id_usuario),
    FOREIGN KEY (id_turno) REFERENCES turnos(id_turno),
    INDEX idx_entrevista_donante_fecha (id_donante, fecha_entrevista)
);

-- ---------------------------------------------------------------------
-- 7. RESTRICCIONES APLICADAS A DONANTES (con auditoría)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS donante_restricciones (
    id_donante_restriccion INT AUTO_INCREMENT PRIMARY KEY,
    id_donante INT NOT NULL,
    id_restriccion INT NOT NULL,
    id_entrevista INT NULL,
    id_usuario_registro INT NOT NULL,                 -- auditoría: quién la marcó
    fecha_inicio_restriccion DATE NOT NULL,
    fecha_habilitacion_calculada DATE NULL,           -- la calcula el trigger si no se informa
    hasta_nueva_evaluacion BOOLEAN NOT NULL DEFAULT FALSE,
    observaciones TEXT NULL,
    fecha_registro DATETIME DEFAULT CURRENT_TIMESTAMP, -- auditoría: cuándo
    fecha_levantamiento DATETIME NULL,                -- si se levantó en una nueva evaluación: cuándo,
    id_usuario_levanta INT NULL,                      --   quién
    id_entrevista_levanta INT NULL,                   --   y en qué entrevista
    FOREIGN KEY (id_usuario_levanta) REFERENCES usuarios(id_usuario),
    FOREIGN KEY (id_entrevista_levanta) REFERENCES entrevistas_medicas(id_entrevista),
    FOREIGN KEY (id_donante) REFERENCES donantes(id_donante),
    FOREIGN KEY (id_restriccion) REFERENCES restricciones_medicas(id_restriccion),
    FOREIGN KEY (id_entrevista) REFERENCES entrevistas_medicas(id_entrevista),
    FOREIGN KEY (id_usuario_registro) REFERENCES usuarios(id_usuario),
    INDEX idx_restr_donante_hab (id_donante, fecha_habilitacion_calculada)
);

-- ---------------------------------------------------------------------
-- 8. DONACIONES (una fila por procedimiento de aféresis)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS donaciones (
    id_donacion INT AUTO_INCREMENT PRIMARY KEY,
    -- Trazabilidad: hoy 10 dígitos = AA + localidad (3) + secuencial (5).
    -- Se valida en la aplicación para poder cambiar la regla sin tocar la base.
    numero_donacion VARCHAR(20) UNIQUE NOT NULL,
    id_donante INT NOT NULL,
    id_tipo_donacion INT NOT NULL,
    id_turno INT NULL,
    id_entrevista INT NULL,                           -- entrevista previa (NULL en datos migrados)
    id_usuario_extractora INT NOT NULL,
    fecha_donacion DATETIME DEFAULT CURRENT_TIMESTAMP,
    peso_donante DECIMAL(5,2),                        -- kg el día de la donación (volemia)
    altura_donante DECIMAL(5,2),                      -- cm el día de la donación
    cantidad_plaquetas INT NOT NULL DEFAULT 0,
    cantidad_plasma INT NOT NULL DEFAULT 0,
    incidencias TEXT NULL,
    -- Excepciones a los límites (autoriza Jefa de Área/Gerente)
    es_excepcion BOOLEAN NOT NULL DEFAULT FALSE,
    motivo_excepcion TEXT NULL,
    id_usuario_autoriza INT NULL,
    migrado_desde_papel BOOLEAN DEFAULT FALSE,
    fecha_registro DATETIME DEFAULT CURRENT_TIMESTAMP,
    FOREIGN KEY (id_donante) REFERENCES donantes(id_donante),
    FOREIGN KEY (id_tipo_donacion) REFERENCES tipos_donacion(id_tipo_donacion),
    FOREIGN KEY (id_turno) REFERENCES turnos(id_turno),
    FOREIGN KEY (id_entrevista) REFERENCES entrevistas_medicas(id_entrevista),
    FOREIGN KEY (id_usuario_extractora) REFERENCES usuarios(id_usuario),
    FOREIGN KEY (id_usuario_autoriza) REFERENCES usuarios(id_usuario),
    CONSTRAINT chk_excepcion CHECK (
        es_excepcion = FALSE
        OR (motivo_excepcion IS NOT NULL AND id_usuario_autoriza IS NOT NULL)
    ),
    INDEX idx_don_donante_fecha (id_donante, fecha_donacion)   -- control de límites
);

-- Datos técnicos de la planilla HEMO 2 (carga manual, sin validación).
CREATE TABLE IF NOT EXISTS donaciones_detalle (
    id_donacion INT PRIMARY KEY,
    -- Signos clínicos
    hto DECIMAL(4,1),
    hb DECIMAL(4,1),
    ta VARCHAR(10),                                   -- ej: '120/80'
    pulso SMALLINT,
    temperatura DECIMAL(4,1),
    -- Datos de la bolsa
    tipo_bolsa ENUM('Doble','Triple','Cuádruple'),
    marca_bolsa VARCHAR(50),
    lote_bolsa VARCHAR(50),
    nro_tubuladura VARCHAR(50),
    vencimiento_bolsa DATE,
    anticoagulante VARCHAR(50),
    -- Datos del procedimiento
    brazo ENUM('Derecho','Izquierdo'),
    dificultades BOOLEAN,
    peso_bolsa DECIMAL(7,2),
    hora_inicio TIME,
    hora_fin TIME,
    FOREIGN KEY (id_donacion) REFERENCES donaciones(id_donacion)
);

-- ---------------------------------------------------------------------
-- 9. FORMULARIO CONFIDENCIAL HEMO 3 (solo se almacena, no se valida)
--    Se carga antes de la donación; id_donacion se completa cuando existe.
--    Restringir su consulta a los roles autorizados desde la API.
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS formulario_confidencial (
    id_formulario INT AUTO_INCREMENT PRIMARY KEY,
    id_donante INT NOT NULL,
    id_entrevista INT NULL,
    id_donacion INT NULL,
    id_usuario_registro INT NOT NULL,
    fecha_registro DATETIME DEFAULT CURRENT_TIMESTAMP,
    datos LONGTEXT NOT NULL,
    FOREIGN KEY (id_donante) REFERENCES donantes(id_donante),
    FOREIGN KEY (id_entrevista) REFERENCES entrevistas_medicas(id_entrevista),
    FOREIGN KEY (id_donacion) REFERENCES donaciones(id_donacion),
    FOREIGN KEY (id_usuario_registro) REFERENCES usuarios(id_usuario)
);

-- ---------------------------------------------------------------------
-- 11. RECORDATORIOS Y NOTIFICACIONES DE WHATSAPP
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS notificaciones_whatsapp (
    id_notificacion INT AUTO_INCREMENT PRIMARY KEY,
    id_donante INT NOT NULL,
    id_turno INT NULL,
    fecha_hora_turno DATETIME NULL,                   -- a qué fecha/hora del turno corresponde el aviso
    mensaje_enviado TEXT NOT NULL,
    estado_envio ENUM('Pendiente','Enviado','Error') NOT NULL DEFAULT 'Pendiente',
    fecha_envio DATETIME DEFAULT CURRENT_TIMESTAMP,
    respuesta_recibida TEXT NULL,
    fecha_respuesta DATETIME NULL,
    requiere_atencion_medica BOOLEAN DEFAULT FALSE,
    atendido BOOLEAN DEFAULT FALSE,
    id_usuario_atiende INT NULL,
    telefono_destino VARCHAR(30) NULL,                -- número normalizado al que se envió
    modo_envio VARCHAR(20) NULL,                      -- 'simulado' o el servicio real usado
    detalle_envio VARCHAR(255) NULL,                  -- explicación si hubo error
    UNIQUE KEY uq_notif_turno (id_turno, fecha_hora_turno),   -- un solo recordatorio por turno y horario
    FOREIGN KEY (id_donante) REFERENCES donantes(id_donante),
    FOREIGN KEY (id_turno) REFERENCES turnos(id_turno),
    FOREIGN KEY (id_usuario_atiende) REFERENCES usuarios(id_usuario)
);

-- ---------------------------------------------------------------------
-- 13. PARAMETROS GLOBALES (panel de configuración)
-- ---------------------------------------------------------------------
CREATE TABLE IF NOT EXISTS parametros_configuracion (
    clave VARCHAR(50) PRIMARY KEY,
    valor VARCHAR(100) NOT NULL,
    descripcion VARCHAR(255)
);

-- ---------------------------------------------------------------------
-- 14. VISTA: HISTORIAL DE DONACIONES (sin duplicar datos)
-- ---------------------------------------------------------------------
CREATE OR REPLACE VIEW historial_donaciones AS
SELECT d.id_donacion, d.id_donante, d.numero_donacion, d.fecha_donacion,
       t.nombre AS tipo_donacion, d.cantidad_plaquetas, d.cantidad_plasma,
       d.incidencias, d.es_excepcion
FROM donaciones d
JOIN tipos_donacion t ON t.id_tipo_donacion = d.id_tipo_donacion;

-- ---------------------------------------------------------------------
-- DATOS INICIALES
-- ---------------------------------------------------------------------
INSERT IGNORE INTO roles (nombre) VALUES
('Doctora'),
('Entrevistadora/Administrativo'),
('Extractora'),
('Jefa de Área/Gerente'),
('Administrador');

-- Plasma y Ambos arrancan con los mismos límites que plaquetas:
-- confirmar con el cliente y ajustar desde el panel de configuración.
INSERT IGNORE INTO tipos_donacion (nombre) VALUES
('Plaquetas'),
('Plasma'),
('Ambos');

-- Plazos según el documento de visión (6 meses = 180 días). Si el médico decide un
-- plazo mayor (hasta el máximo), la aplicación informa la fecha de habilitación.
INSERT IGNORE INTO restricciones_medicas
    (nombre, tipo, dias_espera_minimos, dias_espera_maximos, requiere_reevaluacion, observaciones) VALUES
('Tatuaje o piercing', 'Temporal', 180, 365, FALSE, '6 meses; 12 meses según criterio médico'),
('Embarazo / parto', 'Temporal', 180, NULL, FALSE, '6 meses desde el parto'),
('Cirugía', 'Temporal', 180, NULL, FALSE, '6 meses'),
('Medicación antibiótica', 'Temporal', 7, NULL, FALSE, '7 días desde la última toma'),
('Aspirina, diclofenac o ibuprofeno', 'Temporal', 2, 3, FALSE, '48 a 72 horas desde la última toma'),
('Hierro bajo', 'Temporal', NULL, NULL, TRUE, 'Hasta que se normalice; requiere reevaluación'),
('Período de ventana', 'Temporal', NULL, 365, TRUE, 'Hasta 12 meses, según serología'),
('Cuadro gripal', 'Temporal', NULL, NULL, TRUE, 'Plazo a definir con el cliente'),
('Recuento de plaquetas bajo', 'Temporal', NULL, NULL, TRUE, 'Menos de 150.000 plaquetas/mm³: repetir el análisis'),
('Peso insuficiente', 'Temporal', NULL, NULL, TRUE, 'El donante debe pesar más de 50 kg'),
('Edad fuera de rango', 'Temporal', NULL, NULL, TRUE, 'Se acepta entre 18 y 65 años');

INSERT IGNORE INTO parametros_configuracion (clave, valor, descripcion) VALUES
('limite_diario_donaciones', '10', 'Máximo de extracciones diarias en el banco'),
('limite_semanal_donaciones', '50', 'Máximo de extracciones semanales en el banco'),
('limite_mensual_donaciones', '200', 'Máximo de extracciones mensuales en el banco'),
('edad_minima', '18', 'Edad mínima del donante (años)'),
('edad_maxima', '65', 'Edad máxima del donante (años)'),
('peso_minimo_kg', '50', 'El donante debe pesar más que este valor (kg)'),
('recuento_minimo_plaquetas', '150000', 'Plaquetas/mm³ mínimas en el análisis previo'),
('horas_aviso_turno', '24', 'Horas antes del turno para enviar el recordatorio por WhatsApp'),
('whatsapp_modo', 'simulado', 'simulado = no envía nada real (pruebas); el envío real se conecta más adelante');

-- ---------------------------------------------------------------------
-- TRIGGER: cálculo automático de la fecha de habilitación
-- Si no se informa fecha_habilitacion_calculada y la restricción no es
-- "hasta nueva evaluación", se calcula inicio + dias_espera_minimos del catálogo.
-- ---------------------------------------------------------------------
DROP TRIGGER IF EXISTS trg_restriccion_habilitacion;
DELIMITER $$
CREATE TRIGGER trg_restriccion_habilitacion
BEFORE INSERT ON donante_restricciones
FOR EACH ROW
BEGIN
    DECLARE v_dias INT;
    IF NEW.fecha_habilitacion_calculada IS NULL AND NEW.hasta_nueva_evaluacion = FALSE THEN
        SELECT dias_espera_minimos INTO v_dias
        FROM restricciones_medicas
        WHERE id_restriccion = NEW.id_restriccion;
        IF v_dias IS NOT NULL THEN
            SET NEW.fecha_habilitacion_calculada =
                DATE_ADD(NEW.fecha_inicio_restriccion, INTERVAL v_dias DAY);
        END IF;
    END IF;
END$$
DELIMITER ;

-- ---------------------------------------------------------------------
-- USUARIO DE LA BASE QUE USA LA APLICACIÓN (PHP). No usa root.
-- La clave tiene que ser la misma que DB_CLAVE en htdocs/bancosangre/api/config.php.
-- Cambiarla antes de instalar en el banco real.
-- ---------------------------------------------------------------------
CREATE USER IF NOT EXISTS 'bancosangre_app'@'localhost' IDENTIFIED BY 'cambiar_esta_clave';
GRANT SELECT, INSERT, UPDATE, DELETE ON bancosangre.* TO 'bancosangre_app'@'localhost';
FLUSH PRIVILEGES;
