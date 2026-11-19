package com.hemocentro.aferesis

import android.app.DatePickerDialog
import android.app.TimePickerDialog
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.activity.result.contract.ActivityResultContracts
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityTurnoFormBinding
import kotlinx.coroutines.launch
import java.time.LocalDate
import java.time.LocalTime
import java.time.format.DateTimeFormatter
import java.util.Locale

class TurnoFormActivity : BaseActivity() {

    companion object {
        const val EXTRA_TURNO = "turno"                // entrada: turno a reprogramar (si falta = turno nuevo)
        const val EXTRA_GUARDADO = "turno_guardado"    // salida: el turno tal como quedó guardado
    }

    private lateinit var binding: ActivityTurnoFormBinding

    private var turnoOriginal: Turno? = null     // solo al reprogramar
    private var donante: Donante? = null         // solo en turno nuevo
    private var tipos: List<TipoDonacion> = emptyList()
    private var fecha: LocalDate? = null
    private var hora: LocalTime? = null

    private val formatoFecha = DateTimeFormatter.ofPattern("EEEE dd/MM/yyyy", Locale("es", "AR"))

    // Vuelve de la pantalla de búsqueda con el donante elegido
    private val elegirDonante = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) {
            val json = resultado.data?.getStringExtra(DonantesActivity.EXTRA_ELEGIDO)
            if (json != null) {
                donante = Gson().fromJson(json, Donante::class.java)
                mostrarDonante()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        binding = ActivityTurnoFormBinding.inflate(layoutInflater)
        setContentView(binding.root)

        val json = intent.getStringExtra(EXTRA_TURNO)
        if (json == null) {
            binding.txtTitulo.text = "Nuevo turno"
        } else {
            val t = Gson().fromJson(json, Turno::class.java)
            turnoOriginal = t
            binding.txtTitulo.text = "Reprogramar turno"
            binding.btnDonante.visibility = View.GONE   // el donante de un turno no se cambia
            fecha = LocalDate.parse(t.fecha)
            hora = LocalTime.parse(t.hora)
            binding.edtLugar.setText(t.lugar ?: "")
        }

        mostrarDonante()
        mostrarFechaHora()

        binding.btnDonante.setOnClickListener {
            val i = Intent(this, DonantesActivity::class.java)
            i.putExtra(DonantesActivity.EXTRA_ELEGIR, true)
            elegirDonante.launch(i)
        }
        binding.btnFecha.setOnClickListener { elegirFecha() }
        binding.btnHora.setOnClickListener { elegirHora() }
        binding.btnGuardar.setOnClickListener { guardar() }
        binding.btnCancelar.setOnClickListener { finish() }

        // No se puede guardar hasta que lleguen los tipos de donación
        binding.btnGuardar.isEnabled = false
        cargarTipos()
    }

    private fun cargarTipos() {
        lifecycleScope.launch {
            val r = ApiClient.listarTipos()
            if (revisarSesion()) return@launch

            val lista = r.tipos
            if (r.ok && !lista.isNullOrEmpty()) {
                tipos = lista
                val adaptador = ArrayAdapter(
                    this@TurnoFormActivity,
                    android.R.layout.simple_spinner_item,
                    lista.map { it.nombre }
                )
                adaptador.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
                binding.spnTipo.adapter = adaptador

                // Al reprogramar, dejar seleccionado el tipo que ya tenía el turno
                turnoOriginal?.let { t ->
                    val pos = lista.indexOfFirst { it.id_tipo_donacion == t.id_tipo_donacion }
                    if (pos >= 0) binding.spnTipo.setSelection(pos)
                }
                binding.btnGuardar.isEnabled = true
            } else {
                mostrarErrorGeneral(r.error ?: "No hay tipos de donación habilitados.")
            }
        }
    }

    private fun mostrarDonante() {
        val t = turnoOriginal
        val d = donante
        binding.txtDonante.text = when {
            t != null -> "${t.apellido}, ${t.nombre}\nDNI ${t.dni}"
            d != null -> "${d.apellido}, ${d.nombre}\nDNI ${d.dni}"
            else -> "Todavía no elegiste ningún donante"
        }
        if (t == null) {
            binding.btnDonante.text = if (d == null) "Elegir donante" else "Cambiar donante"
        }
    }

    private fun mostrarFechaHora() {
        binding.btnFecha.text = fecha?.let {
            formatoFecha.format(it).replaceFirstChar { c -> c.uppercase() }
        } ?: "Elegir fecha"
        binding.btnHora.text = hora?.let { String.format("%02d:%02d", it.hour, it.minute) } ?: "Elegir hora"
    }

    private fun elegirFecha() {
        val inicial = fecha ?: LocalDate.now()
        //  en el calendario de Android los meses empiezan en 0
        val dialogo = DatePickerDialog(
            this,
            { _, anio, mes, dia ->
                fecha = LocalDate.of(anio, mes + 1, dia)
                mostrarFechaHora()
            },
            inicial.year, inicial.monthValue - 1, inicial.dayOfMonth
        )
        dialogo.datePicker.minDate = System.currentTimeMillis() - 1000   // no se puede elegir el pasado
        dialogo.show()
    }

    private fun elegirHora() {
        val inicial = hora ?: LocalTime.of(9, 0)
        TimePickerDialog(
            this,
            { _, h, m ->
                hora = LocalTime.of(h, m)
                mostrarFechaHora()
            },
            inicial.hour, inicial.minute, true   // true = formato de 24 horas
        ).show()
    }

    private fun guardar() {
        limpiarErrores()

        // De fecha y hora al formato del servidor: "2026-10-26 09:30" (vacío si falta algo)
        val f = fecha
        val h = hora
        val fechaHora = if (f != null && h != null)
            "$f ${String.format("%02d:%02d", h.hour, h.minute)}"
        else
            ""

        val idTipo = tipos.getOrNull(binding.spnTipo.selectedItemPosition)?.id_tipo_donacion
        val idDonante = turnoOriginal?.id_donante ?: donante?.id_donante

        val pedido = GuardarTurnoRequest(
            id_turno = turnoOriginal?.id_turno,
            id_donante = idDonante,
            id_tipo_donacion = idTipo,
            fecha_hora = fechaHora,
            lugar = binding.edtLugar.text.toString().trim()
        )

        binding.btnGuardar.isEnabled = false
        binding.btnGuardar.text = "Guardando..."

        lifecycleScope.launch {
            val r = ApiClient.guardarTurno(pedido)

            binding.btnGuardar.isEnabled = true
            binding.btnGuardar.text = "Guardar"

            if (revisarSesion()) return@launch

            if (r.ok && r.turno != null) {
                setResult(RESULT_OK, Intent().putExtra(EXTRA_GUARDADO, Gson().toJson(r.turno)))
                Toast.makeText(this@TurnoFormActivity, r.mensaje ?: "Guardado.", Toast.LENGTH_LONG).show()
                finish()
            } else {
                mostrarErrores(r)
            }
        }
    }

    private fun limpiarErrores() {
        binding.txtError.visibility = View.GONE
        binding.txtErrorDonante.visibility = View.GONE
        binding.txtErrorTipo.visibility = View.GONE
        binding.txtErrorFecha.visibility = View.GONE
        binding.tilLugar.error = null
    }

    private fun mostrarErrorGeneral(texto: String) {
        binding.txtError.text = texto
        binding.txtError.visibility = View.VISIBLE
        binding.scroll.smoothScrollTo(0, 0)
    }

    private fun mostrarErrores(r: TurnoResponse) {
        // Mensaje general arriba: por ejemplo "Ya se completó el cupo de ese día (20 turnos)."
        mostrarErrorGeneral(r.error ?: "No se pudo guardar el turno.")

        // Y un mensaje debajo del control que corresponde (nombres de campo del servidor)
        r.campos?.forEach { (campo, mensaje) ->
            when (campo) {
                "id_donante" -> marcar(binding.txtErrorDonante, mensaje)
                "id_tipo_donacion" -> marcar(binding.txtErrorTipo, mensaje)
                "fecha_hora" -> marcar(binding.txtErrorFecha, mensaje)
                "lugar" -> binding.tilLugar.error = mensaje
            }
        }
    }

    private fun marcar(vista: android.widget.TextView, mensaje: String) {
        vista.text = mensaje
        vista.visibility = View.VISIBLE
    }
}