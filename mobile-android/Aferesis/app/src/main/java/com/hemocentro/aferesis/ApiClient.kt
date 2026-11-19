package com.hemocentro.aferesis

import com.google.gson.Gson
import okhttp3.OkHttpClient
import retrofit2.Response
import retrofit2.Retrofit
import retrofit2.converter.gson.GsonConverterFactory
import java.io.IOException

object ApiClient {

    // Gracias a adb reverse, "localhost:8080" en el celu = Apache de la PC
    private const val BASE_URL = "http://localhost:8080/bancosangre/api/"

    // Token de la sesión; se guarda al hacer login
    var token: String? = null

    // Se pone en true si el servidor dice 401 (token vencido o inválido)
    @Volatile
    var sesionVencida = false

    private val gson = Gson()

    // Cliente HTTP: agrega el token a TODOS los pedidos y detecta el 401
    private val http = OkHttpClient.Builder()
        .addInterceptor { chain ->
            val pedido = chain.request().newBuilder()
            token?.let { pedido.header("X-Auth-Token", it) }
            val respuesta = chain.proceed(pedido.build())

            // En el login un 401 significa "clave incorrecta", no "sesión vencida"
            if (respuesta.code() == 401 && !chain.request().url().encodedPath().endsWith("login.php")) {
                Sesion.cerrar()
                sesionVencida = true
            }
            respuesta
        }
        .build()

    private val servicio: ApiService = Retrofit.Builder()
        .baseUrl(BASE_URL)
        .client(http)
        .addConverterFactory(GsonConverterFactory.create(gson))
        .build()
        .create(ApiService::class.java)

    // Ejecuta cualquier llamada y devuelve SIEMPRE una respuesta usable:
    // si todo sale bien, la del servidor; si falla algo, una con el mensaje de error.
    private suspend fun <T : Any> ejecutar(
        clase: Class<T>,
        conError: (String) -> T,
        llamada: suspend () -> Response<T>
    ): T {
        return try {
            val resp = llamada()
            // Con código 200 el JSON viene en body(); con 400/401/409/422... viene en errorBody()
            val cuerpo = if (resp.isSuccessful) {
                resp.body()
            } else {
                resp.errorBody()?.string()?.let { gson.fromJson(it, clase) }
            }
            cuerpo ?: conError("Respuesta vacía del servidor (código ${resp.code()})")
        } catch (e: IOException) {
            conError("No se pudo conectar con el servidor. Revisá el cable, adb reverse y que Apache esté encendido.")
        } catch (e: Exception) {
            conError("El servidor devolvió una respuesta inesperada.")
        }
    }

    suspend fun login(usuario: String, clave: String): LoginResponse {
        val r = ejecutar(LoginResponse::class.java, { LoginResponse(error = it) }) {
            servicio.login(LoginRequest(usuario, clave))
        }
        if (r.ok) {
            token = r.token
            Sesion.usuario = r.usuario
        }
        return r
    }

    suspend fun buscarDonantes(q: String, incluirInactivos: Boolean): DonantesResponse {
        return ejecutar(DonantesResponse::class.java, { DonantesResponse(error = it) }) {
            servicio.buscarDonantes(q, if (incluirInactivos) 1 else null)
        }
    }

    suspend fun cambiarEstadoDonante(idDonante: Int, activo: Boolean): DonanteResponse {
        return ejecutar(DonanteResponse::class.java, { DonanteResponse(error = it) }) {
            servicio.cambiarEstadoDonante(EstadoDonanteRequest(idDonante, activo))
        }
    }

    suspend fun guardarDonante(pedido: GuardarDonanteRequest): DonanteResponse {
        return ejecutar(DonanteResponse::class.java, { DonanteResponse(error = it) }) {
            servicio.guardarDonante(pedido)
        }
    }

    suspend fun listarTurnos(fecha: String, estado: String?): TurnosResponse {
        return ejecutar(TurnosResponse::class.java, { TurnosResponse(error = it) }) {
            servicio.listarTurnos(fecha, estado)
        }
    }

    suspend fun cambiarEstadoTurno(idTurno: Int, estado: String): TurnoResponse {
        return ejecutar(TurnoResponse::class.java, { TurnoResponse(error = it) }) {
            servicio.cambiarEstadoTurno(EstadoTurnoRequest(idTurno, estado))
        }
    }


    suspend fun listarTipos(): TiposResponse {
        return ejecutar(TiposResponse::class.java, { TiposResponse(error = it) }) {
            servicio.listarTipos()
        }
    }

    suspend fun guardarTurno(pedido: GuardarTurnoRequest): TurnoResponse {
        return ejecutar(TurnoResponse::class.java, { TurnoResponse(error = it) }) {
            servicio.guardarTurno(pedido)
        }
    }

    suspend fun listarEntrevistas(idDonante: Int): EntrevistasResponse {
        return ejecutar(EntrevistasResponse::class.java, { EntrevistasResponse(error = it) }) {
            servicio.listarEntrevistas(idDonante)
        }
    }

    suspend fun listarRestriccionesDonante(idDonante: Int, soloVigentes: Boolean): RestriccionesDonanteResponse {
        return ejecutar(RestriccionesDonanteResponse::class.java, { RestriccionesDonanteResponse(error = it) }) {
            servicio.listarRestriccionesDonante(idDonante, if (soloVigentes) 1 else null)
        }
    }

    suspend fun listarCatalogoRestricciones(): CatalogoRestriccionesResponse {
        return ejecutar(CatalogoRestriccionesResponse::class.java, { CatalogoRestriccionesResponse(error = it) }) {
            servicio.listarCatalogoRestricciones()
        }
    }

    suspend fun guardarEntrevista(pedido: GuardarEntrevistaRequest): EntrevistaResponse {
        return ejecutar(EntrevistaResponse::class.java, { EntrevistaResponse(error = it) }) {
            servicio.guardarEntrevista(pedido)
        }
    }

    suspend fun verificarDonacion(idDonante: Int, idTipo: Int): VerificarDonacionResponse {
        return ejecutar(VerificarDonacionResponse::class.java, { VerificarDonacionResponse(error = it) }) {
            servicio.verificarDonacion(idDonante, idTipo)
        }
    }

    suspend fun listarDonaciones(idDonante: Int): DonacionesResponse {
        return ejecutar(DonacionesResponse::class.java, { DonacionesResponse(error = it) }) {
            servicio.listarDonaciones(idDonante)
        }
    }

    suspend fun guardarDonacion(pedido: GuardarDonacionRequest): DonacionResponse {
        return ejecutar(DonacionResponse::class.java, { DonacionResponse(error = it) }) {
            servicio.guardarDonacion(pedido)
        }
    }

    suspend fun hemo3Campos(idDonante: Int): Hemo3CamposResponse {
        return ejecutar(Hemo3CamposResponse::class.java, { Hemo3CamposResponse(error = it) }) {
            servicio.hemo3Campos(idDonante)
        }
    }

    suspend fun hemo3Guardar(pedido: GuardarHemo3Request): Hemo3GuardarResponse {
        return ejecutar(Hemo3GuardarResponse::class.java, { Hemo3GuardarResponse(error = it) }) {
            servicio.hemo3Guardar(pedido)
        }
    }

    suspend fun hemo3Listar(idDonante: Int): Hemo3ListaResponse {
        return ejecutar(Hemo3ListaResponse::class.java, { Hemo3ListaResponse(error = it) }) {
            servicio.hemo3Listar(idDonante)
        }
    }

    suspend fun listarWhatsapp(todos: Boolean): MensajesResponse {
        return ejecutar(MensajesResponse::class.java, { MensajesResponse(error = it) }) {
            servicio.listarWhatsapp(if (todos) 1 else null)
        }
    }

    suspend fun atenderWhatsapp(idNotificacion: Int): WhatsappAccionResponse {
        return ejecutar(WhatsappAccionResponse::class.java, { WhatsappAccionResponse(error = it) }) {
            servicio.atenderWhatsapp(AtenderRequest(idNotificacion))
        }
    }

    suspend fun simularRespuesta(idNotificacion: Int, texto: String): WhatsappAccionResponse {
        return ejecutar(WhatsappAccionResponse::class.java, { WhatsappAccionResponse(error = it) }) {
            servicio.simularRespuesta(SimularRespuestaRequest(idNotificacion, texto))
        }
    }

    suspend fun enviarRecordatoriosAhora(): EnviarAhoraResponse {
        return ejecutar(EnviarAhoraResponse::class.java, { EnviarAhoraResponse(error = it) }) {
            servicio.enviarRecordatoriosAhora()
        }
    }

    suspend fun estadisticas(anio: Int): EstadisticasResponse {
        return ejecutar(EstadisticasResponse::class.java, { EstadisticasResponse(error = it) }) {
            servicio.estadisticas(anio)
        }
    }

    suspend fun listarConfiguracion(): ParametrosResponse {
        return ejecutar(ParametrosResponse::class.java, { ParametrosResponse(error = it) }) {
            servicio.listarConfiguracion()
        }
    }

    suspend fun guardarConfiguracion(cambios: Map<String, String>): ConfiguracionGuardarResponse {
        return ejecutar(ConfiguracionGuardarResponse::class.java, { ConfiguracionGuardarResponse(error = it) }) {
            servicio.guardarConfiguracion(GuardarConfiguracionRequest(cambios))
        }
    }

}