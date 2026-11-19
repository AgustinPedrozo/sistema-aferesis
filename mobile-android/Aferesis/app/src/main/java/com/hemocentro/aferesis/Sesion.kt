package com.hemocentro.aferesis

// Guarda quién inició sesión y qué puede hacer según su rol
object Sesion {

    var usuario: Usuario? = null

    private fun rol(): String = usuario?.rol?.trim()?.lowercase() ?: ""

    val esAdministrador: Boolean get() = rol() == "administrador"
    val esJefa: Boolean get() = rol().startsWith("jefa")
    val esDoctora: Boolean get() = rol().startsWith("doctora")
    val esExtractora: Boolean get() = rol().startsWith("extractora")
    val esEntrevistadora: Boolean get() = rol().startsWith("entrevistadora")

    // Estadísticas y configuración: solo Jefa de Área y Administrador
    val puedeVerEstadisticas: Boolean get() = esJefa || esAdministrador
    val puedeVerConfiguracion: Boolean get() = esJefa || esAdministrador

    // Baja y reactivación de donantes: solo Jefa de Área y Administrador
    val puedeDarDeBaja: Boolean get() = esJefa || esAdministrador

    // Entrevistas médicas: todos los roles menos la Extractora
    val puedeHacerEntrevista: Boolean get() = esDoctora || esEntrevistadora || esJefa || esAdministrador

    // Ver el estado de extracción y el historial de donaciones
    val puedeVerExtraccion: Boolean get() = esExtractora || esDoctora || esJefa || esAdministrador

    // Registrar una extracción
    val puedeRegistrarExtraccion: Boolean get() = esExtractora || esJefa || esAdministrador

    // Quien registra puede autorizar su propia excepción (el servidor lo vuelve a controlar)
    val puedeAutorizarExcepcion: Boolean get() = esJefa || esAdministrador

    // HEMO 3 (confidencial): cargarlo y verlo tienen permisos distintos
    val puedeCargarHemo3: Boolean get() = esDoctora || esEntrevistadora || esJefa || esAdministrador
    val puedeVerHemo3: Boolean get() = esDoctora || esJefa || esAdministrador

    // Mensajes de WhatsApp: todos los roles menos la Extractora
    val puedeVerMensajes: Boolean get() = esDoctora || esEntrevistadora || esJefa || esAdministrador

    // Herramientas de prueba (enviar recordatorios ahora, simular respuesta): solo el Administrador
    val puedeUsarHerramientasPrueba: Boolean get() = esAdministrador

    fun cerrar() {
        usuario = null
        ApiClient.token = null
    }
}