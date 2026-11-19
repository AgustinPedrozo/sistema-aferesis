#pragma once
#include <string>
#include <nlohmann/json.hpp>

// Resultado de una llamada a la API PHP.
struct RespuestaApi {
    long codigoHttp = 0;
    nlohmann::json cuerpo;   // JSON recibido (null si fallo la conexion)
    std::string errorRed;    // se completa si no hubo conexion o la respuesta no era JSON

    bool ok() const { return errorRed.empty() && codigoHttp >= 200 && codigoHttp < 300; }

    // Mensaje de error listo para mostrar al usuario (texto en UTF-8).
    std::string error() const {
        if (!errorRed.empty()) return errorRed;
        if (cuerpo.is_object()) return cuerpo.value("error", std::string("Error desconocido"));
        return "Error desconocido";
    }
};

class ApiClient {
public:
    // urlBase: por ejemplo "http://localhost/bancosangre/api"
    explicit ApiClient(std::string urlBase);
    ~ApiClient();

    // Sesion
    RespuestaApi iniciarSesion(const std::string& usuario, const std::string& clave);
    RespuestaApi obtenerPerfil();
    bool tieneSesion() const { return !token_.empty(); }
    const std::string& token() const { return token_; }   // para abrir la pagina de estadisticas con la sesion iniciada
    // Datos del usuario que inicio sesion (texto en UTF-8).
    const std::string& nombreCompleto() const { return nombreCompleto_; }
    const std::string& rol() const { return rol_; }

    // Donantes
    RespuestaApi buscarDonantes(const std::string& texto, bool incluirInactivos);
    RespuestaApi guardarDonante(const nlohmann::json& datos);       // alta (sin id_donante) o modificacion (con id_donante)
    RespuestaApi cambiarEstadoDonante(int idDonante, bool activo);  // baja (false) o reactivacion (true)

    // Turnos (las fechas van como "AAAA-MM-DD")
    RespuestaApi listarTurnos(const std::string& fechaDesde, const std::string& fechaHasta = std::string());
    RespuestaApi guardarTurno(const nlohmann::json& datos);                       // nuevo (sin id_turno) o reprogramacion (con id_turno)
    RespuestaApi cambiarEstadoTurno(int idTurno, const std::string& estado);     // "Cancelado" o "Ausente"
    RespuestaApi listarTiposDonacion();

    // Entrevista medica y restricciones
    RespuestaApi listarRestricciones();                                           // catalogo de causas de diferimiento
    RespuestaApi listarRestriccionesDonante(int idDonante, bool soloVigentes);
    RespuestaApi listarEntrevistas(int idDonante);
    RespuestaApi guardarEntrevista(const nlohmann::json& datos);

    // Extraccion por afaresis
    RespuestaApi verificarDonacion(int idDonante, int idTipoDonacion);            // puede donar hoy? limites?
    RespuestaApi guardarDonacion(const nlohmann::json& datos);                    // planilla HEMO 2 (+ excepcion opcional)
    RespuestaApi listarDonaciones(int idDonante);

    // Panel de configuracion (solo Jefa de Area/Gerente y Administrador)
    RespuestaApi listarConfiguracion();
    RespuestaApi guardarConfiguracion(const nlohmann::json& cambios);             // {"clave": valor, ...}

    // Usuarios (solo Administrador)
    RespuestaApi listarUsuarios();
    RespuestaApi guardarUsuario(const nlohmann::json& datos);

    // Formulario confidencial HEMO 3
    RespuestaApi obtenerCamposHemo3(int idDonante);                               // preguntas + si ya hay uno cargado hoy (idDonante 0 = solo preguntas)
    RespuestaApi guardarHemo3(const nlohmann::json& datos);
    RespuestaApi listarHemo3(int idDonante);                                      // solo Doctora, Jefa y Administrador

    // Mensajes de WhatsApp (recordatorios de turno y respuestas)
    RespuestaApi enviarRecordatoriosAhora();                                      // corre el mismo proceso automatico, ahora
    RespuestaApi listarMensajes(bool todos);                                      // false = solo respuestas pendientes de atender
    RespuestaApi atenderMensaje(int idNotificacion);
    RespuestaApi simularRespuesta(int idNotificacion, const std::string& texto);  // solo en modo simulado

private:
    // POST con JSON si cuerpo != nullptr; GET si es nullptr.
    RespuestaApi ejecutar(const std::string& endpoint, const nlohmann::json* cuerpo);

    std::string urlBase_;
    std::string token_;
    std::string nombreCompleto_;
    std::string rol_;
};