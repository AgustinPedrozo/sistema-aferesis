package com.hemocentro.aferesis

import retrofit2.Response
import retrofit2.http.Body
import retrofit2.http.GET
import retrofit2.http.POST
import retrofit2.http.Query

interface ApiService {
    @POST("login.php")
    suspend fun login(@Body pedido: LoginRequest): Response<LoginResponse>

    // inactivos = 1 incluye los dados de baja; si es null, no se manda
    @GET("donantes_buscar.php")
    suspend fun buscarDonantes(
        @Query("q") q: String,
        @Query("inactivos") inactivos: Int?
    ): Response<DonantesResponse>

    @POST("donante_estado.php")
    suspend fun cambiarEstadoDonante(@Body pedido: EstadoDonanteRequest): Response<DonanteResponse>

    @POST("donante_guardar.php")
    suspend fun guardarDonante(@Body pedido: GuardarDonanteRequest): Response<DonanteResponse>

    // estado = null -> no se manda y el servidor devuelve todos
    @GET("turnos_listar.php")
    suspend fun listarTurnos(
        @Query("fecha") fecha: String,
        @Query("estado") estado: String?
    ): Response<TurnosResponse>

    @POST("turno_estado.php")
    suspend fun cambiarEstadoTurno(@Body pedido: EstadoTurnoRequest): Response<TurnoResponse>

    @GET("tipos_donacion_listar.php")
    suspend fun listarTipos(): Response<TiposResponse>

    @POST("turno_guardar.php")
    suspend fun guardarTurno(@Body pedido: GuardarTurnoRequest): Response<TurnoResponse>

    @GET("entrevistas_listar.php")
    suspend fun listarEntrevistas(
        @Query("id_donante") idDonante: Int
    ): Response<EntrevistasResponse>

    // solo_vigentes = 1 trae solo las que siguen vigentes; si es null trae todas
    @GET("donante_restricciones_listar.php")
    suspend fun listarRestriccionesDonante(
        @Query("id_donante") idDonante: Int,
        @Query("solo_vigentes") soloVigentes: Int?
    ): Response<RestriccionesDonanteResponse>

    @GET("restricciones_listar.php")
    suspend fun listarCatalogoRestricciones(): Response<CatalogoRestriccionesResponse>

    @POST("entrevista_guardar.php")
    suspend fun guardarEntrevista(@Body pedido: GuardarEntrevistaRequest): Response<EntrevistaResponse>

    @GET("donacion_verificar.php")
    suspend fun verificarDonacion(
        @Query("id_donante") idDonante: Int,
        @Query("id_tipo_donacion") idTipo: Int
    ): Response<VerificarDonacionResponse>

    @GET("donaciones_listar.php")
    suspend fun listarDonaciones(
        @Query("id_donante") idDonante: Int
    ): Response<DonacionesResponse>

    @POST("donacion_guardar.php")
    suspend fun guardarDonacion(@Body pedido: GuardarDonacionRequest): Response<DonacionResponse>

    @GET("hemo3_campos.php")
    suspend fun hemo3Campos(@Query("id_donante") idDonante: Int): Response<Hemo3CamposResponse>

    @POST("hemo3_guardar.php")
    suspend fun hemo3Guardar(@Body pedido: GuardarHemo3Request): Response<Hemo3GuardarResponse>

    @GET("hemo3_listar.php")
    suspend fun hemo3Listar(@Query("id_donante") idDonante: Int): Response<Hemo3ListaResponse>

    // todos = 1 trae los últimos 100 mensajes; si es null, solo las respuestas pendientes
    @GET("whatsapp_respuestas_listar.php")
    suspend fun listarWhatsapp(@Query("todos") todos: Int?): Response<MensajesResponse>

    @POST("whatsapp_atender.php")
    suspend fun atenderWhatsapp(@Body pedido: AtenderRequest): Response<WhatsappAccionResponse>

    @POST("whatsapp_simular_respuesta.php")
    suspend fun simularRespuesta(@Body pedido: SimularRespuestaRequest): Response<WhatsappAccionResponse>

    // Este endpoint no necesita cuerpo
    @POST("whatsapp_enviar_ahora.php")
    suspend fun enviarRecordatoriosAhora(): Response<EnviarAhoraResponse>

    @GET("estadisticas_resumen.php")
    suspend fun estadisticas(@Query("anio") anio: Int): Response<EstadisticasResponse>

    @GET("configuracion_listar.php")
    suspend fun listarConfiguracion(): Response<ParametrosResponse>

    @POST("configuracion_guardar.php")
    suspend fun guardarConfiguracion(@Body pedido: GuardarConfiguracionRequest): Response<ConfiguracionGuardarResponse>

}