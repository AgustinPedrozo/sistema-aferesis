package com.hemocentro.aferesis
// Lo que mandamos a login.php
data class LoginRequest(
    val usuario: String,
    val clave: String
)

// El usuario que devuelve el servidor (los nombres son iguales a los del JSON)
data class Usuario(
    val id: Int,
    val nombre_usuario: String,
    val nombre_completo: String,
    val rol: String
)

// Lo que contesta login.php
data class LoginResponse(
    val ok: Boolean = false,
    val token: String? = null,
    val usuario: Usuario? = null,
    val error: String? = null
)

// Un donante, con los mismos nombres que devuelve el servidor
data class Donante(
    val id_donante: Int,
    val dni: String,
    val nombre: String,
    val apellido: String,
    val fecha_nacimiento: String,   // "AAAA-MM-DD"
    val sexo: String,               // F, M o X
    val domicilio: String?,
    val telefono: String,
    val email: String?,
    val peso: Double?,
    val altura: Double?,
    val activo: Boolean,
    val migrado_desde_papel: Boolean
)

// Lo que contesta donantes_buscar.php
data class DonantesResponse(
    val ok: Boolean = false,
    val donantes: List<Donante>? = null,
    val error: String? = null
)


// Lo que mandamos a donante_estado.php
data class EstadoDonanteRequest(
    val id_donante: Int,
    val activo: Boolean
)


// Lo que contestan donante_estado.php y donante_guardar.php
data class DonanteResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val donante: Donante? = null,
    val error: String? = null,
    val campos: Map<String, String>? = null   // campo -> mensaje de error (códigos 422 y 409)
)

// Lo que mandamos a donante_guardar.php.
// id_donante = null -> alta (Gson no manda los campos nulos); con número -> modificación.
// Peso y altura viajan como texto: el servidor acepta coma o punto.
data class GuardarDonanteRequest(
    val id_donante: Int?,
    val dni: String,
    val nombre: String,
    val apellido: String,
    val fecha_nacimiento: String,
    val sexo: String,
    val domicilio: String,
    val telefono: String,
    val email: String,
    val peso: String,
    val altura: String
)

// Un turno, con los mismos nombres que devuelve el servidor
data class Turno(
    val id_turno: Int,
    val id_donante: Int,
    val dni: String,
    val apellido: String,
    val nombre: String,
    val telefono: String,
    val id_tipo_donacion: Int,
    val tipo_donacion: String,
    val fecha_hora: String,   // "2026-10-21 09:30:00"
    val lugar: String?,
    val estado: String,       // Pendiente, Completado, Cancelado o Ausente
    val fecha: String,        // "2026-10-21"
    val hora: String          // "09:30"
)

// Lo que contesta turnos_listar.php
data class TurnosResponse(
    val ok: Boolean = false,
    val turnos: List<Turno>? = null,
    val error: String? = null
)

// Lo que mandamos a turno_estado.php
data class EstadoTurnoRequest(
    val id_turno: Int,
    val estado: String        // "Cancelado" o "Ausente"
)

// Lo que contestan turno_estado.php y turno_guardar.php
data class TurnoResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val turno: Turno? = null,
    val error: String? = null,
    val campos: Map<String, String>? = null
)

// Un tipo de donación (para la lista desplegable)
data class TipoDonacion(
    val id_tipo_donacion: Int,
    val nombre: String
)

// Lo que contesta tipos_donacion_listar.php
data class TiposResponse(
    val ok: Boolean = false,
    val tipos: List<TipoDonacion>? = null,
    val error: String? = null
)

// Lo que mandamos a turno_guardar.php.
// id_turno = null -> turno nuevo; con número -> reprogramar.
// Los campos null no viajan (Gson los omite) y el servidor responde "Elija un donante", etc.
data class GuardarTurnoRequest(
    val id_turno: Int?,
    val id_donante: Int?,
    val id_tipo_donacion: Int?,
    val fecha_hora: String,     // "2026-10-26 09:30"
    val lugar: String
)

// Una entrevista médica del historial
data class Entrevista(
    val id_entrevista: Int,
    val fecha_entrevista: String,
    val apto: Boolean,
    val recuento_plaquetas: Int?,
    val peso_kg: Double?,
    val altura_cm: Double?,
    val observaciones: String?,
    val entrevistador: String
)

// Lo que contesta entrevistas_listar.php
data class EntrevistasResponse(
    val ok: Boolean = false,
    val entrevistas: List<Entrevista>? = null,
    val error: String? = null
)

// Una restricción (causa de diferimiento) de un donante
data class RestriccionDonante(
    val id_donante_restriccion: Int,
    val id_restriccion: Int,
    val nombre: String,
    val tipo: String?,
    val fecha_inicio_restriccion: String?,
    val fecha_habilitacion_calculada: String?,   // null = hasta nueva evaluación
    val hasta_nueva_evaluacion: Boolean,
    val observaciones: String?,
    val registrada_por: String?,
    val fecha_levantamiento: String?,
    val levantada_por: String?,
    val vigente: Boolean
)

// Lo que contesta donante_restricciones_listar.php
data class RestriccionesDonanteResponse(
    val ok: Boolean = false,
    val restricciones: List<RestriccionDonante>? = null,
    val error: String? = null
)

// Una causa de diferimiento del catálogo (para elegir en la entrevista)
data class RestriccionCatalogo(
    val id_restriccion: Int,
    val nombre: String,
    val tipo: String?,
    val dias_espera_minimos: Int?,
    val dias_espera_maximos: Int?,
    val requiere_reevaluacion: Boolean,
    val observaciones: String?
)

// Lo que contesta restricciones_listar.php
data class CatalogoRestriccionesResponse(
    val ok: Boolean = false,
    val restricciones: List<RestriccionCatalogo>? = null,
    val error: String? = null
)

// Una causa de diferimiento nueva dentro de la entrevista
data class NuevaRestriccion(
    val id_restriccion: Int,
    val fecha_inicio: String,    // "2026-10-24"
    val observaciones: String
)

// Lo que mandamos a entrevista_guardar.php.
// apto = null (no se eligió nada) no viaja, y el servidor contesta "Indique si el donante es apto o no."
data class GuardarEntrevistaRequest(
    val id_donante: Int,
    val apto: Boolean?,
    val peso: String,
    val altura: String,
    val recuento_plaquetas: String,
    val observaciones: String,
    val restricciones: List<NuevaRestriccion>,
    val levantar: List<Int>
)

// Lo que contesta entrevista_guardar.php
data class EntrevistaResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val error: String? = null,
    val campos: Map<String, String>? = null,   // errores por campo (422)
    val bloqueos: List<String>? = null         // motivos por los que no puede ser apto (409)
)

// Entrevista de hoy que usa el servidor para decidir
data class EntrevistaHoy(
    val id_entrevista: Int = 0,
    val apto: Boolean = false,
    val peso_kg: Double? = null,
    val altura_cm: Double? = null
)

// Conteos y fechas para mostrar cuánto lleva donado
data class DatosDonacion(
    val ultima_donacion: String? = null,
    val habilitado_desde: String? = null,
    val donaciones_semana: Int = 0,
    val limite_semanal: Int = 0,
    val donaciones_anio: Int = 0,
    val limite_anual: Int = 0
)

// Lo que contesta donacion_verificar.php
data class VerificarDonacionResponse(
    val ok: Boolean = false,
    val puede_donar: Boolean = false,
    val requiere_excepcion: Boolean = false,
    val bloqueos: List<String>? = null,
    val violaciones: List<String>? = null,
    val avisos: List<String>? = null,
    val entrevista: EntrevistaHoy? = null,
    val datos: DatosDonacion? = null,
    val error: String? = null
)

// Una donación del historial
data class Donacion(
    val id_donacion: Int,
    val numero_donacion: String,
    val fecha_donacion: String,
    val tipo_donacion: String,
    val cantidad_plaquetas: Int,
    val cantidad_plasma: Int,
    val peso_donante: Double?,
    val incidencias: String?,
    val es_excepcion: Boolean,
    val motivo_excepcion: String?,
    val extractora: String,
    val autorizada_por: String?
)

// Lo que contesta donaciones_listar.php
data class DonacionesResponse(
    val ok: Boolean = false,
    val donaciones: List<Donacion>? = null,
    val error: String? = null
)

// Datos técnicos de la planilla HEMO 2. Todo viaja como texto: el servidor valida y convierte.
data class DetalleHemo2(
    val hto: String,
    val hb: String,
    val ta: String,
    val pulso: String,
    val temperatura: String,
    val tipo_bolsa: String,
    val marca_bolsa: String,
    val lote_bolsa: String,
    val nro_tubuladura: String,
    val vencimiento_bolsa: String,   // "2027-03-01" o vacío
    val anticoagulante: String,
    val brazo: String,
    val dificultades: Boolean,
    val peso_bolsa: String,
    val hora_inicio: String,         // "09:00" o vacío
    val hora_fin: String
)

// Autorización de una extracción fuera de los límites.
// Si quien registra es Jefa o Administrador, usuario y clave quedan en null y no viajan.
data class ExcepcionRequest(
    val motivo: String,
    val usuario: String?,
    val clave: String?
)

// Lo que mandamos a donacion_guardar.php. Si excepcion es null, no viaja.
data class GuardarDonacionRequest(
    val id_donante: Int,
    val id_tipo_donacion: Int,
    val numero_donacion: String,
    val cantidad_plaquetas: String,
    val cantidad_plasma: String,
    val peso: String,
    val altura: String,
    val incidencias: String,
    val detalle: DetalleHemo2,
    val excepcion: ExcepcionRequest? = null
)

// Lo que contesta donacion_guardar.php
data class DonacionResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val error: String? = null,
    val campos: Map<String, String>? = null,   // errores por campo (422)
    val bloqueos: List<String>? = null,        // motivos médicos que impiden extraer (409)
    val requiere_excepcion: Boolean = false,   // 409: hace falta autorización
    val violaciones: List<String>? = null,     // qué límites se superan
    val avisos: List<String>? = null,
    val turno_completado: Int? = null
)

// Una pregunta del HEMO 3 (el texto lo define el servidor)
data class PreguntaHemo3(
    val clave: String,
    val texto: String
)

// Lo que contesta hemo3_campos.php
data class Hemo3CamposResponse(
    val ok: Boolean = false,
    val preguntas: List<PreguntaHemo3>? = null,
    val existe_hoy: Boolean = false,   // ya hay uno cargado hoy (sin mostrar su contenido)
    val error: String? = null
)

// Lo que mandamos a hemo3_guardar.php.
// respuestas: clave de la pregunta -> true (Sí) / false (No). Las sin responder no viajan.
data class GuardarHemo3Request(
    val id_donante: Int,
    val respuestas: Map<String, Boolean>,
    val observaciones: String
)

// Lo que contesta hemo3_guardar.php
data class Hemo3GuardarResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val error: String? = null,
    val campos: Map<String, String>? = null,
    val ya_existia_hoy: Boolean = false
)

// Una respuesta ya guardada
data class RespuestaHemo3(
    val pregunta: String,
    val respuesta: String   // "Sí" o "No"
)

// Un formulario ya guardado
data class FormularioHemo3(
    val id_formulario: Int,
    val fecha_registro: String,
    val registrado_por: String,
    val id_entrevista: Int?,
    val id_donacion: Int?,
    val numero_donacion: String?,
    val respuestas: List<RespuestaHemo3>,
    val alguna_afirmativa: Boolean,
    val observaciones: String
)

// Lo que contesta hemo3_listar.php
data class Hemo3ListaResponse(
    val ok: Boolean = false,
    val formularios: List<FormularioHemo3>? = null,
    val error: String? = null
)

// Un mensaje de WhatsApp (recordatorio enviado, con o sin respuesta del donante)
data class MensajeWhatsapp(
    val id_notificacion: Int,
    val id_donante: Int,
    val id_turno: Int?,
    val apellido: String,
    val nombre: String,
    val telefono: String,
    val fecha_hora_turno: String?,
    val mensaje_enviado: String?,
    val estado_envio: String?,      // Pendiente, Enviado o Error
    val fecha_envio: String?,
    val modo_envio: String?,
    val detalle_envio: String?,
    val respuesta_recibida: String?,
    val fecha_respuesta: String?,
    val atendido: Boolean,
    val atendido_por: String?
) {
    // Tiene una respuesta del donante que nadie atendió todavía
    val pendienteDeAtender: Boolean get() = respuesta_recibida != null && !atendido
}

// Lo que contesta whatsapp_respuestas_listar.php
data class MensajesResponse(
    val ok: Boolean = false,
    val total_pendientes: Int = 0,
    val mensajes: List<MensajeWhatsapp>? = null,
    val error: String? = null
)

// Lo que mandamos a whatsapp_atender.php
data class AtenderRequest(
    val id_notificacion: Int
)

// Lo que mandamos a whatsapp_simular_respuesta.php
data class SimularRespuestaRequest(
    val id_notificacion: Int,
    val texto: String
)

// Lo que contestan whatsapp_atender.php y whatsapp_simular_respuesta.php
data class WhatsappAccionResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val error: String? = null
)

// Un recordatorio generado por whatsapp_enviar_ahora.php
data class EnvioDetalle(
    val donante: String,
    val turno: String,
    val estado: String,
    val detalle: String?
)

// Lo que contesta whatsapp_enviar_ahora.php
data class EnviarAhoraResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val modo: String? = null,
    val revisados: Int = 0,
    val enviados: Int = 0,
    val con_error: Int = 0,
    val detalle: List<EnvioDetalle>? = null,
    val error: String? = null
)

data class ItemSexo(val sexo: String, val cantidad: Int)
data class ItemEdad(val rango: String, val cantidad: Int)
data class ItemTipo(val tipo: String, val cantidad: Int)
data class ItemMes(val mes: Int, val nombre: String, val donaciones: Int, val plaquetas: Int, val plasma: Int)
data class ItemCausa(val causa: String, val cantidad: Int)
data class ItemEstado(val estado: String, val cantidad: Int)

data class ResumenDonantes(
    val total: Int,
    val activos: Int,
    val inactivos: Int,
    val nuevos_en_anio: Int,
    val por_sexo: List<ItemSexo>,
    val por_edad: List<ItemEdad>
)

data class ResumenDonaciones(
    val total: Int,
    val donantes_distintos: Int,
    val promedio_por_donante: Double?,   // null si no hubo donaciones
    val unidades_plaquetas: Int,
    val unidades_plasma: Int,
    val por_tipo: List<ItemTipo>,
    val por_mes: List<ItemMes>
)

data class ResumenDiferimientos(
    val donantes_diferidos_vigentes: Int,
    val causas_mas_frecuentes: List<ItemCausa>
)

data class ResumenEntrevistas(val total: Int, val aptos: Int, val no_aptos: Int)

data class ResumenTurnos(
    val por_estado: List<ItemEstado>,
    val ausentismo_porcentaje: Double?   // null si no hubo turnos completados ni ausentes
)

data class ResumenControl(val excepciones_autorizadas: Int)

// Lo que contesta estadisticas_resumen.php
data class EstadisticasResponse(
    val ok: Boolean = false,
    val anio: Int = 0,
    val generado: String? = null,
    val donantes: ResumenDonantes? = null,
    val donaciones: ResumenDonaciones? = null,
    val diferimientos: ResumenDiferimientos? = null,
    val entrevistas: ResumenEntrevistas? = null,
    val turnos: ResumenTurnos? = null,
    val control: ResumenControl? = null,
    val error: String? = null
)

// Un parámetro editable (el servidor manda todo, incluido el rango permitido)
data class Parametro(
    val clave: String,       // "edad_maxima" o "tipo.1.limite_anual"
    val etiqueta: String,
    val ayuda: String,
    val unidad: String,
    val min: Int,
    val max: Int,
    val valor: Int
)

// Lo que contesta configuracion_listar.php
data class ParametrosResponse(
    val ok: Boolean = false,
    val parametros: List<Parametro>? = null,
    val error: String? = null
)

// Lo que mandamos a configuracion_guardar.php: solo los cambios (clave -> valor como texto)
data class GuardarConfiguracionRequest(
    val cambios: Map<String, String>
)

// Lo que contesta configuracion_guardar.php
data class ConfiguracionGuardarResponse(
    val ok: Boolean = false,
    val mensaje: String? = null,
    val error: String? = null,
    val campos: Map<String, String>? = null   // clave del parámetro -> mensaje de error (422)
)