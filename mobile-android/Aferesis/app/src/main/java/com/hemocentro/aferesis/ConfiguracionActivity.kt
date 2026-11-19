package com.hemocentro.aferesis

import android.graphics.Color
import android.graphics.Typeface
import android.os.Bundle
import android.view.View
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import com.google.android.material.textfield.TextInputEditText
import com.google.android.material.textfield.TextInputLayout
import com.hemocentro.aferesis.databinding.ActivityConfiguracionBinding
import com.hemocentro.aferesis.databinding.ItemParametroBinding
import kotlinx.coroutines.launch

class ConfiguracionActivity : BaseActivity() {

    // Una fila de la pantalla: el parámetro y sus controles
    private class Fila(val p: Parametro, val til: TextInputLayout, val edt: TextInputEditText)

    private lateinit var binding: ActivityConfiguracionBinding
    private val filas = mutableListOf<Fila>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        // Solo Jefa de Área/Gerente y Administrador
        if (!Sesion.puedeVerConfiguracion) {
            finish()
            return
        }

        binding = ActivityConfiguracionBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnGuardar.isEnabled = false   // hasta que lleguen los parámetros
        binding.btnGuardar.setOnClickListener { guardar() }
        binding.btnVolver.setOnClickListener { finish() }

        cargar()
    }

    private fun cargar() {
        binding.txtMensaje.setTextColor(Color.DKGRAY)
        binding.txtMensaje.text = "Cargando..."
        binding.txtMensaje.visibility = View.VISIBLE

        lifecycleScope.launch {
            val r = ApiClient.listarConfiguracion()
            if (revisarSesion()) return@launch

            val lista = r.parametros
            if (!r.ok || lista.isNullOrEmpty()) {
                binding.txtMensaje.setTextColor(Color.parseColor("#B00020"))
                binding.txtMensaje.text = r.error ?: "No se pudo cargar la configuración."
                return@launch
            }
            binding.txtMensaje.visibility = View.GONE
            armarFilas(lista)
            binding.btnGuardar.isEnabled = true
        }
    }

    // Una fila por parámetro, con un título antes de cada sección
    private fun armarFilas(lista: List<Parametro>) {
        binding.contParametros.removeAllViews()
        filas.clear()

        var seccion = ""
        for (p in lista) {
            val nueva = if (p.clave.startsWith("tipo."))
                "Límites de cada donante, por tipo de donación"
            else
                "Parámetros generales"
            if (nueva != seccion) {
                seccion = nueva
                val titulo = TextView(this)
                titulo.text = seccion
                titulo.textSize = 21f
                titulo.setTypeface(null, Typeface.BOLD)
                titulo.setPadding(0, (28 * resources.displayMetrics.density).toInt(), 0, 0)
                binding.contParametros.addView(titulo)
            }

            val fila = ItemParametroBinding.inflate(layoutInflater, binding.contParametros, false)
            fila.txtEtiqueta.text = p.etiqueta.replaceFirstChar { it.uppercase() }
            fila.txtAyuda.text = p.ayuda
            fila.tilValor.hint = "De ${p.min} a ${p.max} (${p.unidad})"
            fila.edtValor.setText(p.valor.toString())
            binding.contParametros.addView(fila.root)
            filas.add(Fila(p, fila.tilValor, fila.edtValor))
        }
    }

    private fun limpiarErrores() {
        binding.txtError.visibility = View.GONE
        for (f in filas) f.til.error = null
    }

    private fun guardar() {
        limpiarErrores()

        // Solo viajan los valores que cambiaron
        val cambios = linkedMapOf<String, String>()
        val resumen = StringBuilder()
        for (f in filas) {
            val texto = f.edt.text.toString().trim()
            if (texto != f.p.valor.toString()) {
                cambios[f.p.clave] = texto
                resumen.append("\n• ${f.p.etiqueta}: ${f.p.valor} → ${texto.ifEmpty { "(vacío)" }}")
            }
        }
        if (cambios.isEmpty()) {
            Toast.makeText(this, "No hay cambios para guardar.", Toast.LENGTH_LONG).show()
            return
        }

        AlertDialog.Builder(this)
            .setTitle("Guardar cambios")
            .setMessage("Se van a cambiar estos valores para todo el sistema:\n$resumen")
            .setPositiveButton("Sí, guardar") { _, _ -> enviar(cambios) }
            .setNegativeButton("No", null)
            .show()
    }

    private fun enviar(cambios: Map<String, String>) {
        binding.btnGuardar.isEnabled = false
        binding.btnGuardar.text = "Guardando..."

        lifecycleScope.launch {
            val r = ApiClient.guardarConfiguracion(cambios)

            binding.btnGuardar.isEnabled = true
            binding.btnGuardar.text = "Guardar cambios"

            if (revisarSesion()) return@launch

            if (r.ok) {
                Toast.makeText(this@ConfiguracionActivity, r.mensaje ?: "Cambios guardados.", Toast.LENGTH_LONG).show()
                cargar()   // se vuelve a leer del servidor: los valores guardados pasan a ser los de referencia
            } else {
                mostrarErrores(r)   // no se recarga: los valores escritos quedan para corregirlos
            }
        }
    }

    private fun mostrarErrores(r: ConfiguracionGuardarResponse) {
        binding.txtError.text =
            if (r.campos.isNullOrEmpty()) r.error ?: "No se pudieron guardar los cambios."
            else "Hay valores para corregir. No se guardó ningún cambio."
        binding.txtError.visibility = View.VISIBLE

        // Cada mensaje debajo de su casilla (la clave es la del parámetro)
        r.campos?.forEach { (clave, mensaje) ->
            filas.firstOrNull { it.p.clave == clave }?.til?.error = mensaje
        }
        binding.scroll.smoothScrollTo(0, 0)
    }
}