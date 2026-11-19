package com.hemocentro.aferesis

import android.graphics.Color
import android.os.Bundle
import android.view.WindowManager
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityHemo3HistorialBinding
import kotlinx.coroutines.launch

class Hemo3HistorialActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"   // entrada: el donante (como JSON)
    }

    private lateinit var binding: ActivityHemo3HistorialBinding
    private lateinit var donante: Donante

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        val json = intent.getStringExtra(EXTRA_DONANTE)
        if (json == null || !Sesion.puedeVerHemo3) {
            finish()
            return
        }
        donante = Gson().fromJson(json, Donante::class.java)

        // Contenido confidencial: se bloquean capturas y grabaciones de pantalla
        window.setFlags(WindowManager.LayoutParams.FLAG_SECURE, WindowManager.LayoutParams.FLAG_SECURE)

        binding = ActivityHemo3HistorialBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtDonante.text = "${donante.apellido}, ${donante.nombre} · DNI ${donante.dni}"
        binding.btnVolver.setOnClickListener { finish() }

        cargar()
    }

    private fun cargar() {
        binding.txtHistorial.setTextColor(Color.DKGRAY)
        binding.txtHistorial.text = "Cargando..."

        lifecycleScope.launch {
            val r = ApiClient.hemo3Listar(donante.id_donante)
            if (revisarSesion()) return@launch

            if (!r.ok) {
                binding.txtHistorial.setTextColor(Color.parseColor("#B00020"))
                binding.txtHistorial.text = r.error ?: "No se pudo cargar el historial."
                return@launch
            }
            val lista = r.formularios ?: emptyList()
            if (lista.isEmpty()) {
                binding.txtHistorial.text = "Todavía no tiene formularios HEMO 3."
                return@launch
            }
            binding.txtHistorial.text = lista.joinToString("\n\n————————————\n\n") { f ->
                val texto = StringBuilder()
                texto.append("${fechaHoraCorta(f.fecha_registro)}  ·  Cargó: ${f.registrado_por}")
                texto.append(
                    if (f.numero_donacion != null) "\nVinculado a la donación N.º ${f.numero_donacion}"
                    else "\nSin extracción vinculada"
                )
                if (f.alguna_afirmativa) texto.append("\nCon respuestas afirmativas")
                for (x in f.respuestas) {
                    texto.append("\n\n• ${x.pregunta}\n   → ${x.respuesta}")
                }
                if (f.observaciones.isNotBlank()) texto.append("\n\nObservaciones: ${f.observaciones}")
                texto.toString()
            }
        }
    }
}