# Sistema de Gestión de Donación por Aféresis

Sistema para digitalizar la donación de **plaquetas y plasma por aféresis** en un centro de hemoterapia: donantes, turnos, entrevista médica, extracción, límites de donación, formulario confidencial, recordatorios por WhatsApp y estadísticas. Incluye una **aplicación de escritorio (C++/MFC)**, una **app Android (Kotlin)**, una **API en PHP** y una base de datos **MariaDB**.

> **Proyecto de competencia / académico.** Se desarrolló pensando en el Hemocentro Regional como caso de estudio. Todos los datos que aparecen son de prueba. No está pensado para usarse con datos reales sin una revisión de seguridad previa (por ejemplo, HTTPS).

## El desafío
El personal del centro no está acostumbrado a la tecnología más allá de aplicaciones como WhatsApp. El objetivo de diseño fue **cero fricción**: pantallas simples, flujos guiados, mensajes claros, confirmaciones visuales y sonoras, y validaciones suaves que ayudan en lugar de bloquear.

## Capturas
> Agregar imágenes en `docs/capturas/` y enlazarlas acá.

## Funcionalidades
- **Donantes:** alta, baja lógica y modificación, con peso y altura. Validación de DNI, teléfono y fecha de nacimiento realista (16 a 100 años).
- **Turnos:** agenda diaria, cupos por día, semana y mes, reprogramación, cancelación y marcado de ausentes.
- **Recordatorios por WhatsApp:** aviso automático antes del turno, registro de respuestas y atención por el personal. Hoy funciona en **modo simulado** (ver más abajo).
- **Entrevista médica:** aptitud (Apto / No apto) y diferimientos a partir de un catálogo de causas, con cálculo automático de la fecha de habilitación y registro de quién y cuándo los marcó.
- **Extracción por aféresis:** planilla HEMO 2 completa (signos clínicos, datos de la bolsa, brazo, horarios, incidencias).
- **Control de límites:** intervalo mínimo entre donaciones, máximo semanal y anual, por tipo de donación. Se puede saltear solo con una **excepción autorizada** por Jefa de Área/Gerente o Administrador, con motivo, y queda registrada.
- **Formulario confidencial HEMO 3:** se almacena sin juzgarlo y solo lo ven los roles habilitados.
- **Historial del donante** y **estadísticas** para la Jefa de Área (donantes, sexo, edades, donaciones por mes, diferidos y causas, turnos ausentes).
- **Panel de configuración:** límites y requisitos editables sin tocar código.
- **Usuarios y roles:** Doctora, Entrevistadora/Administrativo, Extractora, Jefa de Área/Gerente y Administrador, con permisos distintos.

## Arquitectura
```
 Escritorio (C++ / MFC) ──┐
                          ├──►  API REST (PHP + PDO)  ──►  MariaDB
 App Android (Kotlin)  ───┘      (reglas de negocio)
```
Toda la lógica de negocio vive **en el servidor**. Los clientes muestran y validan de forma amigable, pero la decisión final la toma la API.

## Tecnologías
| Capa | Tecnologías |
|---|---|
| Escritorio | C++, MFC (Visual Studio), libcurl, nlohmann/json |
| Móvil | Kotlin, Android Studio, Retrofit, OkHttp, Gson, corrutinas |
| API | PHP, PDO |
| Base de datos | MariaDB (XAMPP), phpMyAdmin, vistas y triggers |
| Estadísticas | HTML y SVG |
| Automatización | Programador de tareas de Windows (recordatorios) |

## Seguridad
- Autenticación por **token de sesión** (se guarda solo su hash) con vencimiento.
- Contraseñas almacenadas con **hash**, nunca en texto plano.
- **Bloqueo temporal** tras 5 intentos fallidos, también para las credenciales de quien autoriza una excepción.
- **Permisos por rol** validados en el servidor.
- Consultas preparadas (PDO) y operaciones de varios pasos **todo o nada** (transacciones).
- Auditoría de acciones críticas (diferimientos, excepciones, atención de mensajes).

## Estructura del repositorio
```
api-php/          API en PHP, herramientas (tools/) y página de estadísticas
database/         Script SQL (tablas, vista, trigger y datos iniciales)
desktop-mfc/      Aplicación de escritorio (Visual Studio)
mobile-android/   App Android (Android Studio)
```

## Cómo ponerlo en marcha

### 1. Base de datos y API
1. Instalá **XAMPP** y encendé **Apache** y **MySQL**.
2. En phpMyAdmin (usuario `root`), pestaña **SQL**, ejecutá `database/bancosangre.sql`. Si se queja por el trigger, poné `$$` en el campo *Delimitador*.
3. El script crea el usuario de base de datos `bancosangre_app` con una clave de ejemplo (`cambiar_esta_clave`). **Cambiala** por una propia.
4. Copiá el contenido de `api-php/` a `C:\xampp\htdocs\bancosangre\` (carpetas `api` y `tools` y el archivo `estadisticas.html`).
5. En `htdocs\bancosangre\api\`, copiá `config.example.php` como `config.php` y completá `DB_CLAVE` con la clave del paso 3. `config.php` no se sube a Git.
6. Creá el primer usuario:
   ```
   C:\xampp\php\php.exe C:\xampp\htdocs\bancosangre\tools\crear_usuario.php ad "Administrador" 1234 "Administrador"
   ```
   Roles válidos: `Doctora`, `Entrevistadora/Administrativo`, `Extractora`, `Jefa de Área/Gerente`, `Administrador`. Después se pueden crear más desde la pantalla **Usuarios** (solo Administrador). Las claves son numéricas de 4 a 10 dígitos. **Cambiá la de ejemplo.**
7. Prueba rápida: `http://localhost/bancosangre/api/login.php` debe responder con un JSON, aunque sea de error.

### 2. Escritorio (C++ / MFC)
1. Visual Studio 2022 o posterior con la carga de trabajo **Desarrollo para el escritorio con C++** y el componente **MFC y ATL de C++ más recientes**. La solución usa el formato `.slnx`, que requiere una versión reciente.
2. Instalá las librerías con [vcpkg](https://github.com/microsoft/vcpkg):
   ```
   vcpkg integrate install
   vcpkg install curl:x64-windows nlohmann-json:x64-windows
   ```
3. Abrí la solución en `desktop-mfc/`, elegí **Debug** y **x64**, compilá y ejecutá.

### 3. App Android
1. Abrí `mobile-android/Aferesis` con Android Studio.
2. La app apunta a `http://localhost:8080/bancosangre/api/` (constante `BASE_URL` en `ApiClient.kt`). Con el celular conectado por cable, redirigí el puerto hacia el Apache de la PC:
   ```
   adb reverse tcp:8080 tcp:80
   ```
   Ajustá el puerto si tu Apache usa otro.

### 4. Recordatorios automáticos (opcional)
Una tarea programada que ejecuta `tools/enviar_recordatorios.php` cada 10 minutos:
```
schtasks /Create /SC MINUTE /MO 10 /TN "BancoSangre Recordatorios" /TR "C:\xampp\php\php.exe C:\xampp\htdocs\bancosangre\tools\enviar_recordatorios.php"
```
Para probarlo sin esperar, la pantalla de mensajes tiene la opción de enviar los recordatorios ahora.

## Sobre WhatsApp
El envío está en **modo simulado**: el sistema detecta los turnos, arma el mensaje, normaliza el teléfono, registra el envío y las respuestas, pero no entrega mensajes reales. Todo el envío pasa por una única función (`enviar_whatsapp()`), de modo que conectar la **API oficial de WhatsApp Business (Meta Cloud API)** solo requiere agregar un caso nuevo ahí. No se usaron librerías no oficiales para no poner en riesgo el número del centro.

## Alcance y límites actuales
- Fuera de alcance: donación de sangre entera, glóbulos rojos, logística de distribución, interfaz para donantes, emisión de certificados en PDF y migración desde papel.
- Los datos de la planilla HEMO 2 se almacenan sin validación clínica automática.
- La comunicación es por HTTP; para un uso real hace falta HTTPS o una red interna controlada.

## Autor
Agustín Pedrozo
