package com.hemocentro.aferesis

import android.app.DatePickerDialog
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityDonanteFormBinding
import kotlinx.coroutines.launch
import java.time.LocalDate
import java.time.format.DateTimeFormatter

class DonanteFormActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"             // entrada: donante a modificar (si falta = alta)
        const val EXTRA_GUARDADO = "donante_guardado"   // salida: el donante tal como quedó guardado
    }

    private lateinit var binding: ActivityDonanteFormBinding
    private var idDonante: Int? = null

    // La fecha se ve como 23/05/1990 y viaja al servidor como 1990-05-23
    private val formatoPantalla = DateTimeFormatter.ofPattern("dd/MM/yyyy")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        binding = ActivityDonanteFormBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val json = intent.getStringExtra(EXTRA_DONANTE)
        if (json == null) {
            binding.txtTitulo.text = "Nuevo donante"
        } else {
            val donante = Gson().fromJson(json, Donante::class.java)
            idDonante = donante.id_donante
            binding.txtTitulo.text = "Modificar donante"
            // Si el celu se rota, Android devuelve solo lo escrito; no pisarlo
            if (savedInstanceState == null) cargar(donante)
        }

        binding.edtFecha.setOnClickListener { elegirFecha() }
        binding.btnGuardar.setOnClickListener { guardar() }
        binding.btnCancelar.setOnClickListener { finish() }
    }

    // Llena las casillas con los datos de un donante existente
    private fun cargar(d: Donante) {
        binding.edtApellido.setText(d.apellido)
        binding.edtNombre.setText(d.nombre)
        binding.edtDni.setText(d.dni)
        binding.edtFecha.setText(
            try {
                LocalDate.parse(d.fecha_nacimiento).format(formatoPantalla)
            } catch (e: Exception) {
                ""
            }
        )
        when (d.sexo) {
            "F" -> binding.rbF.isChecked = true
            "M" -> binding.rbM.isChecked = true
            "X" -> binding.rbX.isChecked = true
        }
        binding.edtTelefono.setText(d.telefono)
        binding.edtEmail.setText(d.email ?: "")
        binding.edtDomicilio.setText(d.domicilio ?: "")
        binding.edtPeso.setText(d.peso?.let { sinCeroDecimal(it) } ?: "")
        binding.edtAltura.setText(d.altura?.let { sinCeroDecimal(it) } ?: "")
    }

    // 70.0 -> "70"   ·   70.5 -> "70.5"
    private fun sinCeroDecimal(n: Double): String =
        if (n % 1.0 == 0.0) n.toInt().toString() else n.toString()

    private fun elegirFecha() {
        val inicial = try {
            LocalDate.parse(binding.edtFecha.text.toString(), formatoPantalla)
        } catch (e: Exception) {
            LocalDate.of(1990, 1, 1)
        }
        // en el calendario de Android los meses empiezan en 0
        val dialogo = DatePickerDialog(
            this,
            { _, anio, mes, dia ->
                binding.edtFecha.setText(LocalDate.of(anio, mes + 1, dia).format(formatoPantalla))
            },
            inicial.year, inicial.monthValue - 1, inicial.dayOfMonth
        )
        dialogo.datePicker.maxDate = System.currentTimeMillis()   // no se puede elegir el futuro
        dialogo.show()
    }

    private fun guardar() {
        limpiarErrores()

        val sexo = when (binding.grupoSexo.checkedRadioButtonId) {
            R.id.rbF -> "F"
            R.id.rbM -> "M"
            R.id.rbX -> "X"
            else -> ""
        }
        // De "23/05/1990" a "1990-05-23"; si está vacía, se manda vacía y el servidor avisa
        val fechaIso = try {
            LocalDate.parse(binding.edtFecha.text.toString(), formatoPantalla).toString()
        } catch (e: Exception) {
            ""
        }

        val pedido = GuardarDonanteRequest(
            id_donante = idDonante,
            dni = binding.edtDni.text.toString().trim(),
            nombre = binding.edtNombre.text.toString().trim(),
            apellido = binding.edtApellido.text.toString().trim(),
            fecha_nacimiento = fechaIso,
            sexo = sexo,
            domicilio = binding.edtDomicilio.text.toString().trim(),
            telefono = binding.edtTelefono.text.toString().trim(),
            email = binding.edtEmail.text.toString().trim(),
            peso = binding.edtPeso.text.toString().trim(),
            altura = binding.edtAltura.text.toString().trim()
        )

        // Mientras espera, el botón queda apagado para evitar doble toque
        binding.btnGuardar.isEnabled = false
        binding.btnGuardar.text = "Guardando..."

        lifecycleScope.launch {
            val r = ApiClient.guardarDonante(pedido)

            binding.btnGuardar.isEnabled = true
            binding.btnGuardar.text = "Guardar"

            if (revisarSesion()) return@launch

            if (r.ok && r.donante != null) {
                // Devuelve el donante guardado a la pantalla que nos abrió
                val resultado = Intent().putExtra(EXTRA_GUARDADO, Gson().toJson(r.donante))
                setResult(RESULT_OK, resultado)
                Toast.makeText(this@DonanteFormActivity, r.mensaje ?: "Guardado.", Toast.LENGTH_LONG).show()
                finish()
            } else {
                mostrarErrores(r)
            }
        }
    }

    private fun limpiarErrores() {
        binding.txtError.visibility = View.GONE
        binding.txtErrorSexo.visibility = View.GONE
        binding.tilApellido.error = null
        binding.tilNombre.error = null
        binding.tilDni.error = null
        binding.tilFecha.error = null
        binding.tilTelefono.error = null
        binding.tilEmail.error = null
        binding.tilDomicilio.error = null
        binding.tilPeso.error = null
        binding.tilAltura.error = null
    }

    private fun mostrarErrores(r: DonanteResponse) {
        // Mensaje general arriba (también cubre errores sin campos: 403, 500, sin conexión...)
        binding.txtError.text = if (r.campos.isNullOrEmpty())
            r.error ?: "No se pudo guardar."
        else
            "Hay datos para corregir. Revisá los campos marcados en rojo."
        binding.txtError.visibility = View.VISIBLE

        // Un mensaje debajo de cada casilla (los nombres de campo son los del servidor)
        r.campos?.forEach { (campo, mensaje) ->
            when (campo) {
                "apellido" -> binding.tilApellido.error = mensaje
                "nombre" -> binding.tilNombre.error = mensaje
                "dni" -> binding.tilDni.error = mensaje
                "fecha_nacimiento" -> binding.tilFecha.error = mensaje
                "sexo" -> {
                    binding.txtErrorSexo.text = mensaje
                    binding.txtErrorSexo.visibility = View.VISIBLE
                }
                "telefono" -> binding.tilTelefono.error = mensaje
                "email" -> binding.tilEmail.error = mensaje
                "domicilio" -> binding.tilDomicilio.error = mensaje
                "peso" -> binding.tilPeso.error = mensaje
                "altura" -> binding.tilAltura.error = mensaje
            }
        }

        // Subir arriba para que se vea el mensaje general
        binding.scroll.smoothScrollTo(0, 0)
    }
}