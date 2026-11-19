package com.hemocentro.aferesis

import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityFichaDonanteBinding
import kotlinx.coroutines.launch
import java.time.LocalDate
import java.time.Period
import java.time.format.DateTimeFormatter
import android.content.Intent
import androidx.activity.result.contract.ActivityResultContracts

class FichaDonanteActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"
    }

    private lateinit var binding: ActivityFichaDonanteBinding
    private lateinit var donante: Donante
    private var huboCambios = false

    // Cuando vuelve del formulario, toma el donante actualizado y redibuja la ficha
    private val abrirFormulario = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) {
            val json = resultado.data?.getStringExtra(DonanteFormActivity.EXTRA_GUARDADO)
            if (json != null) {
                donante = Gson().fromJson(json, Donante::class.java)
                setResult(RESULT_OK)   // avisa a la lista que se repita la búsqueda
                mostrar()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        // El donante llega como texto JSON desde la pantalla de búsqueda
        val json = intent.getStringExtra(EXTRA_DONANTE)
        if (json == null) {
            finish()
            return
        }
        donante = Gson().fromJson(json, Donante::class.java)

        binding = ActivityFichaDonanteBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnVolver.setOnClickListener { finish() }
        binding.btnEditar.setOnClickListener {
            val intent = Intent(this, DonanteFormActivity::class.java)
            intent.putExtra(DonanteFormActivity.EXTRA_DONANTE, Gson().toJson(donante))
            abrirFormulario.launch(intent)
        }
        binding.btnEstado.setOnClickListener { confirmarCambioDeEstado() }

        mostrar()
    }

    // Llena la pantalla con los datos del donante actual
    private fun mostrar() {
        binding.txtNombre.text = "${donante.apellido}, ${donante.nombre}"
        binding.txtBaja.visibility = if (donante.activo) View.GONE else View.VISIBLE

        val sexo = when (donante.sexo) {
            "F" -> "Femenino"
            "M" -> "Masculino"
            else -> "X"
        }
        val peso = donante.peso?.let { "$it kg" } ?: "—"
        val altura = donante.altura?.let { "$it cm" } ?: "—"

        binding.txtDatos.text = """
            DNI: ${donante.dni}
            Nacimiento: ${formatearFecha(donante.fecha_nacimiento)}
            Sexo: $sexo
            Teléfono: ${donante.telefono}
            Email: ${donante.email ?: "—"}
            Domicilio: ${donante.domicilio ?: "—"}
            Peso: $peso
            Altura: $altura
        """.trimIndent()

        // El botón de baja/reactivación solo existe para Jefa y Administrador
        if (Sesion.puedeDarDeBaja) {
            binding.btnEstado.visibility = View.VISIBLE
            binding.btnEstado.text =
                if (donante.activo) "Dar de baja" else "Reactivar donante"
        } else {
            binding.btnEstado.visibility = View.GONE
        }
    }

    // "1990-05-23" -> "23/05/1990 (35 años)"
    private fun formatearFecha(iso: String): String {
        return try {
            val f = LocalDate.parse(iso)
            val edad = Period.between(f, LocalDate.now()).years
            "${f.format(DateTimeFormatter.ofPattern("dd/MM/yyyy"))} ($edad años)"
        } catch (e: Exception) {
            iso
        }
    }

    private fun confirmarCambioDeEstado() {
        val darDeBaja = donante.activo
        val titulo = if (darDeBaja) "Dar de baja" else "Reactivar donante"
        val mensaje = if (darDeBaja)
            "¿Querés dar de baja a ${donante.apellido}, ${donante.nombre}? No se borra: su historial se conserva y se puede reactivar."
        else
            "¿Querés reactivar a ${donante.apellido}, ${donante.nombre}?"

        AlertDialog.Builder(this)
            .setTitle(titulo)
            .setMessage(mensaje)
            .setPositiveButton("Sí") { _, _ -> cambiarEstado(!darDeBaja) }
            .setNegativeButton("No", null)
            .show()
    }

    private fun cambiarEstado(activo: Boolean) {
        binding.btnEstado.isEnabled = false

        lifecycleScope.launch {
            val r = ApiClient.cambiarEstadoDonante(donante.id_donante, activo)

            binding.btnEstado.isEnabled = true
            if (revisarSesion()) return@launch

            if (r.ok && r.donante != null) {
                donante = r.donante
                huboCambios = true
                setResult(RESULT_OK)   // avisa a la lista que se repita la búsqueda
                mostrar()
                Toast.makeText(this@FichaDonanteActivity, r.mensaje ?: "Listo.", Toast.LENGTH_LONG).show()
            } else {
                // Por ejemplo 403 si el rol no tiene permiso
                AlertDialog.Builder(this@FichaDonanteActivity)
                    .setMessage(r.error ?: "No se pudo completar la operación.")
                    .setPositiveButton("Aceptar", null)
                    .show()
            }
        }
    }
}