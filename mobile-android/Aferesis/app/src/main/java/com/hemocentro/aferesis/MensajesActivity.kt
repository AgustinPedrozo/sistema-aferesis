package com.hemocentro.aferesis

import android.graphics.Color
import android.os.Bundle
import android.text.InputFilter
import android.text.InputType
import android.view.View
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.hemocentro.aferesis.databinding.ActivityMensajesBinding
import kotlinx.coroutines.launch

class MensajesActivity : BaseActivity() {

    private lateinit var binding: ActivityMensajesBinding
    private lateinit var adaptador: MensajesAdapter

    // Si se piden varias cargas seguidas, solo se muestra la última
    private var pedidoActual = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        // La Extractora no tiene acceso a los mensajes
        if (!Sesion.puedeVerMensajes) {
            finish()
            return
        }

        binding = ActivityMensajesBinding.inflate(layoutInflater)
        setContentView(binding.root)

        adaptador = MensajesAdapter { mensaje -> mostrarDetalle(mensaje) }
        binding.rvMensajes.layoutManager = LinearLayoutManager(this)
        binding.rvMensajes.adapter = adaptador

        // Herramientas de prueba: solo para el Administrador
        binding.btnEnviarAhora.visibility =
            if (Sesion.puedeUsarHerramientasPrueba) View.VISIBLE else View.GONE
        binding.btnEnviarAhora.setOnClickListener { confirmarEnvioAhora() }

        binding.btnActualizar.setOnClickListener { cargar() }
        binding.chkTodos.setOnCheckedChangeListener { _, _ -> cargar() }
        binding.btnVolver.setOnClickListener { finish() }

        cargar()
    }

    // Pide la lista al servidor: solo pendientes, o los últimos 100 si la casilla está marcada
    private fun cargar() {
        val miPedido = ++pedidoActual
        binding.txtMensaje.text = "Cargando..."
        binding.txtMensaje.visibility = View.VISIBLE

        lifecycleScope.launch {
            val todos = binding.chkTodos.isChecked
            val r = ApiClient.listarWhatsapp(todos)

            if (miPedido != pedidoActual) return@launch   // llegó tarde
            if (revisarSesion()) return@launch

            if (!r.ok) {
                adaptador.mostrar(emptyList())
                binding.txtPendientes.text = ""
                binding.txtMensaje.text = r.error ?: "No se pudieron cargar los mensajes."
                return@launch
            }

            val lista = r.mensajes ?: emptyList()
            adaptador.mostrar(lista)

            binding.txtPendientes.text = when (r.total_pendientes) {
                0 -> "No hay respuestas pendientes de atender."
                1 -> "1 respuesta pendiente de atender."
                else -> "${r.total_pendientes} respuestas pendientes de atender."
            }
            binding.txtPendientes.setTextColor(
                Color.parseColor(if (r.total_pendientes > 0) "#E65100" else "#2E7D32")
            )

            when {
                lista.isEmpty() -> {
                    binding.txtMensaje.text =
                        if (todos) "Todavía no se envió ningún mensaje." else "No hay respuestas pendientes."
                }
                todos && lista.size >= 100 -> {
                    binding.txtMensaje.text = "Se muestran los últimos 100 mensajes."
                }
                else -> binding.txtMensaje.visibility = View.GONE
            }
        }
    }

    private fun textoDetalle(m: MensajeWhatsapp): String {
        val t = StringBuilder()
        t.append("Teléfono: ${m.telefono}")
        t.append("\nTurno: ${fechaHoraCorta(m.fecha_hora_turno)}")
        t.append("\n\nMensaje enviado (${fechaHoraCorta(m.fecha_envio)}):\n${m.mensaje_enviado ?: "—"}")
        t.append("\n\nEstado del envío: ${m.estado_envio ?: "—"}")
        if (!m.detalle_envio.isNullOrBlank()) t.append("\n${m.detalle_envio}")
        if (m.respuesta_recibida != null) {
            t.append("\n\nRespuesta del donante (${fechaHoraCorta(m.fecha_respuesta)}):\n${m.respuesta_recibida}")
            t.append(
                if (m.atendido) "\n\nAtendida por ${m.atendido_por ?: "—"}"
                else "\n\nTodavía nadie la atendió."
            )
        }
        return t.toString()
    }

    private fun mostrarDetalle(m: MensajeWhatsapp) {
        val dialogo = AlertDialog.Builder(this)
            .setTitle("${m.apellido}, ${m.nombre}")
            .setMessage(textoDetalle(m))
            .setNegativeButton("Cerrar", null)

        if (m.pendienteDeAtender) {
            dialogo.setPositiveButton("Marcar como atendida") { _, _ -> confirmarAtender(m) }
        }
        // Solo se puede simular una respuesta sobre un mensaje que se envió
        if (Sesion.puedeUsarHerramientasPrueba && m.estado_envio == "Enviado") {
            dialogo.setNeutralButton("Simular respuesta") { _, _ -> pedirSimulacion(m, "") }
        }
        dialogo.show()
    }

    private fun mostrarAviso(texto: String) {
        AlertDialog.Builder(this)
            .setMessage(texto)
            .setPositiveButton("Aceptar", null)
            .show()
    }

    // ---------- Atender ----------

    private fun confirmarAtender(m: MensajeWhatsapp) {
        AlertDialog.Builder(this)
            .setTitle("Marcar como atendida")
            .setMessage("¿Ya atendiste la respuesta de ${m.apellido}, ${m.nombre}?")
            .setPositiveButton("Sí") { _, _ -> atender(m) }
            .setNegativeButton("No", null)
            .show()
    }

    private fun atender(m: MensajeWhatsapp) {
        lifecycleScope.launch {
            val r = ApiClient.atenderWhatsapp(m.id_notificacion)
            if (revisarSesion()) return@launch

            if (r.ok) {
                Toast.makeText(this@MensajesActivity, r.mensaje ?: "Listo.", Toast.LENGTH_LONG).show()
            } else {
                // Por ejemplo: "...quizás ya fue atendida"
                mostrarAviso(r.error ?: "No se pudo marcar como atendida.")
            }
            cargar()   // en los dos casos se recarga para ver cómo está realmente
        }
    }

    // ---------- Herramientas de prueba (Administrador) ----------

    private fun pedirSimulacion(m: MensajeWhatsapp, textoPrevio: String) {
        val campo = EditText(this)
        campo.hint = "Ej.: No puedo ir, estoy resfriado"
        campo.inputType = InputType.TYPE_CLASS_TEXT or
                InputType.TYPE_TEXT_FLAG_CAP_SENTENCES or InputType.TYPE_TEXT_FLAG_MULTI_LINE
        campo.filters = arrayOf(InputFilter.LengthFilter(500))
        campo.minLines = 2
        campo.textSize = 18f
        campo.setText(textoPrevio)

        val margen = (20 * resources.displayMetrics.density).toInt()
        val contenedor = FrameLayout(this)
        contenedor.setPadding(margen, margen / 2, margen, 0)
        contenedor.addView(campo)

        AlertDialog.Builder(this)
            .setTitle("Simular respuesta")
            .setMessage("Escribí lo que habría contestado ${m.nombre}. Es solo una prueba: no se envía ningún WhatsApp real.")
            .setView(contenedor)
            .setPositiveButton("Registrar") { _, _ ->
                val texto = campo.text.toString().trim()
                if (texto.isEmpty()) {
                    Toast.makeText(this, "Escribí el texto de la respuesta.", Toast.LENGTH_LONG).show()
                    pedirSimulacion(m, "")
                } else {
                    simular(m, texto)
                }
            }
            .setNegativeButton("Cancelar", null)
            .show()
    }

    private fun simular(m: MensajeWhatsapp, texto: String) {
        lifecycleScope.launch {
            val r = ApiClient.simularRespuesta(m.id_notificacion, texto)
            if (revisarSesion()) return@launch

            if (r.ok) {
                Toast.makeText(this@MensajesActivity, r.mensaje ?: "Respuesta registrada.", Toast.LENGTH_LONG).show()
            } else {
                mostrarAviso(r.error ?: "No se pudo registrar la respuesta.")
            }
            cargar()
        }
    }

    private fun confirmarEnvioAhora() {
        AlertDialog.Builder(this)
            .setTitle("Enviar recordatorios ahora")
            .setMessage(
                "Se generan los recordatorios de los turnos pendientes de las próximas horas que todavía no tienen uno. " +
                        "En modo simulado no se manda ningún WhatsApp real, pero los registros quedan guardados."
            )
            .setPositiveButton("Sí, enviar") { _, _ -> enviarAhora() }
            .setNegativeButton("No", null)
            .show()
    }

    private fun enviarAhora() {
        binding.btnEnviarAhora.isEnabled = false
        binding.btnEnviarAhora.text = "Enviando..."

        lifecycleScope.launch {
            val r = ApiClient.enviarRecordatoriosAhora()

            binding.btnEnviarAhora.isEnabled = true
            binding.btnEnviarAhora.text = "Enviar recordatorios ahora (prueba)"

            if (revisarSesion()) return@launch

            if (r.ok) {
                val t = StringBuilder(r.mensaje ?: "Listo.")
                t.append("\nModo: ${r.modo ?: "—"}")
                r.detalle?.forEach { d ->
                    t.append("\n\n• ${d.donante}\n   Turno ${fechaHoraCorta(d.turno)}: ${d.estado}")
                    if (!d.detalle.isNullOrBlank()) t.append("\n   ${d.detalle}")
                }
                mostrarAviso(t.toString())
            } else {
                mostrarAviso(r.error ?: "No se pudieron enviar los recordatorios.")
            }
            cargar()
        }
    }
}