package com.hemocentro.aferesis

import android.content.Intent
import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.view.WindowManager
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityHemo3Binding
import com.hemocentro.aferesis.databinding.ItemPreguntaHemo3Binding
import kotlinx.coroutines.launch

class Hemo3Activity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"   // entrada: el donante (como JSON)
    }

    private lateinit var binding: ActivityHemo3Binding
    private lateinit var donante: Donante

    private var preguntas: List<PreguntaHemo3> = emptyList()
    private val filas = mutableListOf<ItemPreguntaHemo3Binding>()   // una por pregunta, en el mismo orden
    private var existeHoy = false
    private var colorNormal = Color.BLACK

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        val json = intent.getStringExtra(EXTRA_DONANTE)
        if (json == null || !Sesion.puedeCargarHemo3) {
            finish()
            return
        }
        donante = Gson().fromJson(json, Donante::class.java)

        // Formulario confidencial: se bloquean capturas y grabaciones de pantalla
        window.setFlags(WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE)

        binding = ActivityHemo3Binding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtDonante.text = "${donante.apellido}, ${donante.nombre} · DNI ${donante.dni}"

        // "Ver anteriores" solo existe para Doctora, Jefa y Administrador
        binding.btnAnteriores.visibility = if (Sesion.puedeVerHemo3) View.VISIBLE else View.GONE
        binding.btnAnteriores.setOnClickListener {
            val i = Intent(this, Hemo3HistorialActivity::class.java)
            i.putExtra(Hemo3HistorialActivity.EXTRA_DONANTE, Gson().toJson(donante))
            startActivity(i)
        }
        binding.btnGuardar.setOnClickListener { confirmarGuardado() }
        binding.btnCancelar.setOnClickListener { finish() }

        // No se puede guardar hasta que lleguen las preguntas
        binding.btnGuardar.isEnabled = false
        cargar()
    }

    private fun cargar() {
        lifecycleScope.launch {
            val r = ApiClient.hemo3Campos(donante.id_donante)
            if (revisarSesion()) return@launch

            val lista = r.preguntas
            if (r.ok && !lista.isNullOrEmpty()) {
                preguntas = lista
                existeHoy = r.existe_hoy
                armarPreguntas()
                binding.txtAviso.visibility = if (existeHoy) View.VISIBLE else View.GONE
                binding.btnGuardar.isEnabled = true
            } else {
                mostrarErrorGeneral(r.error ?: "No se pudo cargar el formulario.")
            }
        }
    }

    // Una fila con Sí / No por cada pregunta que mandó el servidor
    private fun armarPreguntas() {
        binding.contPreguntas.removeAllViews()
        filas.clear()
        for (p in preguntas) {
            val fila = ItemPreguntaHemo3Binding.inflate(layoutInflater, binding.contPreguntas, false)
            fila.txtPregunta.text = p.texto
            colorNormal = fila.txtPregunta.currentTextColor
            filas.add(fila)
            binding.contPreguntas.addView(fila.root)
        }
    }

    // Un HEMO 3 no se puede modificar después: se confirma antes de enviarlo
    private fun confirmarGuardado() {
        val extra = if (existeHoy) "\n\nYa hay otro cargado hoy para este donante." else ""
        AlertDialog.Builder(this)
            .setTitle("Guardar formulario")
            .setMessage("¿Guardar el formulario HEMO 3? No se puede modificar después.$extra")
            .setPositiveButton("Sí, guardar") { _, _ -> guardar() }
            .setNegativeButton("No", null)
            .show()
    }

    private fun guardar() {
        limpiarErrores()

        // Solo viajan las preguntas respondidas; el servidor avisa cuántas faltan
        val respuestas = linkedMapOf<String, Boolean>()
        preguntas.forEachIndexed { i, p ->
            val f = filas[i]
            when (f.grupo.checkedRadioButtonId) {
                f.rbSi.id -> respuestas[p.clave] = true
                f.rbNo.id -> respuestas[p.clave] = false
                else -> f.txtPregunta.setTextColor(Color.parseColor("#B00020"))   // ayuda visual
            }
        }

        val pedido = GuardarHemo3Request(
            id_donante = donante.id_donante,
            respuestas = respuestas,
            observaciones = binding.edtObs.text.toString().trim()
        )

        binding.btnGuardar.isEnabled = false
        binding.btnGuardar.text = "Guardando..."

        lifecycleScope.launch {
            val r = ApiClient.hemo3Guardar(pedido)

            binding.btnGuardar.isEnabled = true
            binding.btnGuardar.text = "Guardar formulario"

            if (revisarSesion()) return@launch

            if (r.ok) {
                // No se muestra ninguna respuesta: solo el aviso del servidor
                Toast.makeText(this@Hemo3Activity, r.mensaje ?: "Formulario guardado.", Toast.LENGTH_LONG).show()
                finish()
            } else {
                mostrarErrores(r)
            }
        }
    }

    private fun limpiarErrores() {
        binding.txtError.visibility = View.GONE
        binding.txtErrorRespuestas.visibility = View.GONE
        binding.tilObs.error = null
        for (f in filas) f.txtPregunta.setTextColor(colorNormal)
    }

    private fun mostrarErrorGeneral(texto: String) {
        binding.txtError.text = texto
        binding.txtError.visibility = View.VISIBLE
        binding.scroll.smoothScrollTo(0, 0)
    }

    private fun mostrarErrores(r: Hemo3GuardarResponse) {
        mostrarErrorGeneral(
            if (r.campos.isNullOrEmpty()) r.error ?: "No se pudo guardar el formulario."
            else "Hay datos para corregir. Revisá los campos marcados en rojo."
        )
        r.campos?.forEach { (campo, mensaje) ->
            when (campo) {
                "respuestas" -> {
                    binding.txtErrorRespuestas.text = mensaje
                    binding.txtErrorRespuestas.visibility = View.VISIBLE
                }
                "observaciones" -> binding.tilObs.error = mensaje
            }
        }
    }
}