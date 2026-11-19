#include "pch.h"   // en proyectos MFC viejos se llama "stdafx.h": usar el mismo que el resto de los .cpp
#include "ApiClient.h"

#include <curl/curl.h>
#include <cctype>
#include <memory>

namespace {
    size_t escribirRespuesta(char* datos, size_t tam, size_t n, void* destino) {
        static_cast<std::string*>(destino)->append(datos, tam * n);
        return tam * n;
    }

    // Codifica un texto (UTF-8) para poder ponerlo dentro de un enlace: "Perez Juan" -> "Perez%20Juan"
    std::string codificarUrl(const std::string& texto) {
        static const char* hex = "0123456789ABCDEF";
        std::string salida;
        for (unsigned char c : texto) {
            if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
                salida += static_cast<char>(c);
            }
            else {
                salida += '%';
                salida += hex[c >> 4];
                salida += hex[c & 15];
            }
        }
        return salida;
    }
}  // namespace

ApiClient::ApiClient(std::string urlBase) : urlBase_(std::move(urlBase)) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
}

ApiClient::~ApiClient() {
    curl_global_cleanup();
}

RespuestaApi ApiClient::iniciarSesion(const std::string& usuario, const std::string& clave) {
    nlohmann::json datos = { {"usuario", usuario}, {"clave", clave} };
    RespuestaApi r = ejecutar("login.php", &datos);
    if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
        token_ = r.cuerpo.value("token", std::string());
        const nlohmann::json u = r.cuerpo.value("usuario", nlohmann::json::object());
        if (u.is_object()) {
            nombreCompleto_ = u.value("nombre_completo", std::string());
            rol_ = u.value("rol", std::string());
        }
    }
    return r;
}

RespuestaApi ApiClient::obtenerPerfil() {
    return ejecutar("perfil.php", nullptr);
}

RespuestaApi ApiClient::buscarDonantes(const std::string& texto, bool incluirInactivos) {
    std::string endpoint = "donantes_buscar.php?q=" + codificarUrl(texto);
    if (incluirInactivos) {
        endpoint += "&inactivos=1";
    }
    return ejecutar(endpoint, nullptr);
}

RespuestaApi ApiClient::guardarDonante(const nlohmann::json& datos) {
    return ejecutar("donante_guardar.php", &datos);
}

RespuestaApi ApiClient::cambiarEstadoDonante(int idDonante, bool activo) {
    nlohmann::json datos = { {"id_donante", idDonante}, {"activo", activo} };
    return ejecutar("donante_estado.php", &datos);
}

RespuestaApi ApiClient::listarTurnos(const std::string& fechaDesde, const std::string& fechaHasta) {
    std::string endpoint = "turnos_listar.php?fecha=" + codificarUrl(fechaDesde);
    if (!fechaHasta.empty()) {
        endpoint += "&hasta=" + codificarUrl(fechaHasta);
    }
    return ejecutar(endpoint, nullptr);
}

RespuestaApi ApiClient::guardarTurno(const nlohmann::json& datos) {
    return ejecutar("turno_guardar.php", &datos);
}

RespuestaApi ApiClient::cambiarEstadoTurno(int idTurno, const std::string& estado) {
    nlohmann::json datos = { {"id_turno", idTurno}, {"estado", estado} };
    return ejecutar("turno_estado.php", &datos);
}

RespuestaApi ApiClient::listarTiposDonacion() {
    return ejecutar("tipos_donacion_listar.php", nullptr);
}

RespuestaApi ApiClient::listarRestricciones() {
    return ejecutar("restricciones_listar.php", nullptr);
}

RespuestaApi ApiClient::listarRestriccionesDonante(int idDonante, bool soloVigentes) {
    std::string endpoint = "donante_restricciones_listar.php?id_donante=" + std::to_string(idDonante);
    if (soloVigentes) {
        endpoint += "&solo_vigentes=1";
    }
    return ejecutar(endpoint, nullptr);
}

RespuestaApi ApiClient::listarEntrevistas(int idDonante) {
    return ejecutar("entrevistas_listar.php?id_donante=" + std::to_string(idDonante), nullptr);
}

RespuestaApi ApiClient::guardarEntrevista(const nlohmann::json& datos) {
    return ejecutar("entrevista_guardar.php", &datos);
}

RespuestaApi ApiClient::verificarDonacion(int idDonante, int idTipoDonacion) {
    return ejecutar("donacion_verificar.php?id_donante=" + std::to_string(idDonante)
        + "&id_tipo_donacion=" + std::to_string(idTipoDonacion), nullptr);
}

RespuestaApi ApiClient::guardarDonacion(const nlohmann::json& datos) {
    return ejecutar("donacion_guardar.php", &datos);
}

RespuestaApi ApiClient::listarDonaciones(int idDonante) {
    return ejecutar("donaciones_listar.php?id_donante=" + std::to_string(idDonante), nullptr);
}

RespuestaApi ApiClient::listarConfiguracion() {
    return ejecutar("configuracion_listar.php", nullptr);
}

RespuestaApi ApiClient::guardarConfiguracion(const nlohmann::json& cambios) {
    nlohmann::json datos = { {"cambios", cambios} };
    return ejecutar("configuracion_guardar.php", &datos);
}

RespuestaApi ApiClient::listarUsuarios() {
    return ejecutar("usuarios_listar.php", nullptr);
}

RespuestaApi ApiClient::guardarUsuario(const nlohmann::json& datos) {
    return ejecutar("usuario_guardar.php", &datos);
}

RespuestaApi ApiClient::obtenerCamposHemo3(int idDonante) {
    return ejecutar(idDonante > 0 ? "hemo3_campos.php?id_donante=" + std::to_string(idDonante) : std::string("hemo3_campos.php"), nullptr);
}

RespuestaApi ApiClient::guardarHemo3(const nlohmann::json& datos) {
    return ejecutar("hemo3_guardar.php", &datos);
}

RespuestaApi ApiClient::listarHemo3(int idDonante) {
    return ejecutar("hemo3_listar.php?id_donante=" + std::to_string(idDonante), nullptr);
}

RespuestaApi ApiClient::enviarRecordatoriosAhora() {
    const nlohmann::json vacio = nlohmann::json::object();
    return ejecutar("whatsapp_enviar_ahora.php", &vacio);
}

RespuestaApi ApiClient::listarMensajes(bool todos) {
    return ejecutar(todos ? "whatsapp_respuestas_listar.php?todos=1" : "whatsapp_respuestas_listar.php", nullptr);
}

RespuestaApi ApiClient::atenderMensaje(int idNotificacion) {
    nlohmann::json datos = { {"id_notificacion", idNotificacion} };
    return ejecutar("whatsapp_atender.php", &datos);
}

RespuestaApi ApiClient::simularRespuesta(int idNotificacion, const std::string& texto) {
    nlohmann::json datos = { {"id_notificacion", idNotificacion}, {"texto", texto} };
    return ejecutar("whatsapp_simular_respuesta.php", &datos);
}

RespuestaApi ApiClient::ejecutar(const std::string& endpoint, const nlohmann::json* cuerpo) {
    RespuestaApi r;
    std::unique_ptr<CURL, decltype(&curl_easy_cleanup)> curl(curl_easy_init(), curl_easy_cleanup);
    if (!curl) {
        r.errorRed = "No se pudo inicializar libcurl";
        return r;
    }

    std::string respuesta;
    std::string cuerpoTexto;

    curl_slist* cabeceras = nullptr;
    cabeceras = curl_slist_append(cabeceras, "Content-Type: application/json");
    cabeceras = curl_slist_append(cabeceras, "Accept: application/json");
    if (!token_.empty()) {
        cabeceras = curl_slist_append(cabeceras, ("X-Auth-Token: " + token_).c_str());
    }

    const std::string url = urlBase_ + "/" + endpoint;
    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, cabeceras);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, escribirRespuesta);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &respuesta);
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT, 5L);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT, 15L);

    if (cuerpo) {
        cuerpoTexto = cuerpo->dump();
        curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, cuerpoTexto.c_str());
        curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE, static_cast<long>(cuerpoTexto.size()));
    }

    const CURLcode rc = curl_easy_perform(curl.get());
    curl_slist_free_all(cabeceras);

    if (rc != CURLE_OK) {
        r.errorRed = curl_easy_strerror(rc);
        return r;
    }

    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &r.codigoHttp);
    r.cuerpo = nlohmann::json::parse(respuesta, nullptr, false);
    if (r.cuerpo.is_discarded()) {
        r.cuerpo = nlohmann::json();
        r.errorRed = "La respuesta del servidor no es JSON valido";
    }
    return r;
}