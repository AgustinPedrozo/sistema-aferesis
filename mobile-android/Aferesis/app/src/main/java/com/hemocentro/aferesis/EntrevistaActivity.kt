package com.hemocentro.aferesis

import android.graphics.Color
import android.os.Bundle
import android.widget.Toast
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityEntrevistaBinding
import kotlinx.coroutines.launch
import android.content.Intent
import androidx.activity.result.contract.ActivityResultContracts

class EntrevistaActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"   // entrada: el donante elegido (como JSON)
    }

    private lateinit var binding: ActivityEntrevistaBinding
    private lateinit var donante: Donante
    // Cuando vuelve del formulario con la entrevista guardada, recarga restricciones e historial
    private val abrirFormulario = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) cargar()
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        val json = intent.getStringExtra(EXTRA_DONANTE)
        if (json == null) {
            finish()
            return
        }
        donante = Gson().fromJson(json, Donante::class.java)

        binding = ActivityEntrevistaBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtDonante.text = "${donante.apellido}, ${donante.nombre}\nDNI ${donante.dni}"

        binding.btnNueva.setOnClickListener {
            val i = Intent(this, EntrevistaFormActivity::class.java)
            i.putExtra(EntrevistaFormActivity.EXTRA_DONANTE, Gson().toJson(donante))
            abrirFormulario.launch(i)
        }
        binding.btnVolver.setOnClickListener { finish() }

        cargar()
    }

    // Pide al servidor las restricciones vigentes y el historial, y los muestra
    private fun cargar() {
        binding.txtRestricciones.setTextColor(Color.DKGRAY)
        binding.txtRestricciones.text = "Cargando..."
        binding.txtHistorial.text = ""

        lifecycleScope.launch {
            val restricciones = ApiClient.listarRestriccionesDonante(donante.id_donante, true)
            val entrevistas = ApiClient.listarEntrevistas(donante.id_donante)

            if (revisarSesion()) return@launch

            mostrarRestricciones(restricciones)
            mostrarHistorial(entrevistas)
        }
    }

    private fun mostrarRestricciones(r: RestriccionesDonanteResponse) {
        if (!r.ok) {
            binding.txtRestricciones.setTextColor(Color.parseColor("#B00020"))
            binding.txtRestricciones.text = r.error ?: "No se pudieron cargar las restricciones."
            return
        }
        val lista = r.restricciones ?: emptyList()
        if (lista.isEmpty()) {
            binding.txtRestricciones.setTextColor(Color.parseColor("#2E7D32"))
            binding.txtRestricciones.text = "Sin restricciones vigentes."
            return
        }
        binding.txtRestricciones.setTextColor(Color.parseColor("#B00020"))
        binding.txtRestricciones.text = lista.joinToString("\n\n") { x ->
            val hasta = if (x.fecha_habilitacion_calculada != null)
                "habilitado desde el ${fechaCorta(x.fecha_habilitacion_calculada)}"
            else
                "hasta nueva evaluación"
            "• ${x.nombre}\n   Desde el ${fechaCorta(x.fecha_inicio_restriccion)}, $hasta" +
                    (x.observaciones?.let { "\n   $it" } ?: "")
        }
    }

    private fun mostrarHistorial(r: EntrevistasResponse) {
        if (!r.ok) {
            binding.txtHistorial.setTextColor(Color.parseColor("#B00020"))
            binding.txtHistorial.text = r.error ?: "No se pudo cargar el historial."
            return
        }
        val lista = r.entrevistas ?: emptyList()
        if (lista.isEmpty()) {
            binding.txtHistorial.text = "Todavía no tiene entrevistas."
            return
        }
        binding.txtHistorial.text = lista.joinToString("\n\n") { e ->
            val datos = listOfNotNull(
                e.peso_kg?.let { "Peso ${numeroCorto(it)} kg" },
                e.altura_cm?.let { "Altura ${numeroCorto(it)} cm" },
                e.recuento_plaquetas?.let { "Plaquetas ${miles(it)}" }
            ).joinToString(" · ")

            val titulo = "${fechaCorta(e.fecha_entrevista)}  " +
                    if (e.apto) "✔ Apto" else "✖ No apto"

            titulo +
                    (if (datos.isNotEmpty()) "\n$datos" else "") +
                    "\nEntrevistó: ${e.entrevistador}" +
                    (e.observaciones?.let { "\n$it" } ?: "")
        }
    }
}