package com.hemocentro.aferesis

import android.app.DatePickerDialog
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.hemocentro.aferesis.databinding.ActivityTurnosBinding
import kotlinx.coroutines.launch
import java.time.LocalDate
import java.time.format.DateTimeFormatter
import java.util.Locale
import android.content.Intent
import androidx.activity.result.contract.ActivityResultContracts
import com.google.gson.Gson

class TurnosActivity : BaseActivity() {

    private lateinit var binding: ActivityTurnosBinding
    private lateinit var adaptador: TurnosAdapter

    private var fecha: LocalDate = LocalDate.now()
    private val estados = listOf("Todos", "Pendiente", "Completado", "Cancelado", "Ausente")
    private val formatoFecha = DateTimeFormatter.ofPattern("EEEE dd/MM/yyyy", Locale("es", "AR"))

    // Cuenta los pedidos: si el usuario cambia de día rápido, se ignoran las respuestas viejas
    private var pedidoActual = 0

    // Vuelve del formulario: si se guardó un turno, la agenda salta al día de ese turno
    private val abrirFormulario = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) {
            val json = resultado.data?.getStringExtra(TurnoFormActivity.EXTRA_GUARDADO)
            if (json != null) {
                val turno = Gson().fromJson(json, Turno::class.java)
                fecha = LocalDate.parse(turno.fecha)
            }
            cargar()
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        binding = ActivityTurnosBinding.inflate(layoutInflater)
        setContentView(binding.root)

        adaptador = TurnosAdapter { turno -> mostrarOpciones(turno) }
        binding.rvTurnos.layoutManager = LinearLayoutManager(this)
        binding.rvTurnos.adapter = adaptador

        // Lista desplegable de estados
        val lista = ArrayAdapter(this, android.R.layout.simple_spinner_item, estados)
        lista.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        binding.spnEstado.adapter = lista
        binding.spnEstado.setSelection(0, false)   // false = no dispara el aviso inicial
        binding.spnEstado.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(p: AdapterView<*>?, v: View?, pos: Int, id: Long) = cargar()
            override fun onNothingSelected(p: AdapterView<*>?) {}
        }

        binding.btnAnterior.setOnClickListener { fecha = fecha.minusDays(1); cargar() }
        binding.btnSiguiente.setOnClickListener { fecha = fecha.plusDays(1); cargar() }
        binding.btnHoy.setOnClickListener { fecha = LocalDate.now(); cargar() }
        binding.btnFecha.setOnClickListener { elegirFecha() }
        binding.btnNuevo.setOnClickListener {
            abrirFormulario.launch(Intent(this, TurnoFormActivity::class.java))
        }
        binding.btnVolver.setOnClickListener { finish() }

        cargar()
    }

    private fun elegirFecha() {
        // en el calendario de Android los meses empiezan en 0
        DatePickerDialog(
            this,
            { _, anio, mes, dia ->
                fecha = LocalDate.of(anio, mes + 1, dia)
                cargar()
            },
            fecha.year, fecha.monthValue - 1, fecha.dayOfMonth
        ).show()
    }

    // Pide al servidor los turnos del día elegido y llena la lista
    private fun cargar() {
        binding.btnFecha.text = formatoFecha.format(fecha).replaceFirstChar { it.uppercase() }

        val posicion = binding.spnEstado.selectedItemPosition
        val estado = if (posicion <= 0) null else estados[posicion]

        val miPedido = ++pedidoActual
        mostrarMensaje("Cargando...")

        lifecycleScope.launch {
            val r = ApiClient.listarTurnos(fecha.toString(), estado)

            if (miPedido != pedidoActual) return@launch   // llegó tarde: hay un pedido más nuevo
            if (revisarSesion()) return@launch

            if (r.ok) {
                val turnos = r.turnos ?: emptyList()
                adaptador.mostrar(turnos)
                mostrarMensaje(
                    when (turnos.size) {
                        0 -> "No hay turnos para este día."
                        1 -> "1 turno."
                        else -> "${turnos.size} turnos."
                    }
                )
            } else {
                adaptador.mostrar(emptyList())
                mostrarMensaje(r.error ?: "No se pudieron cargar los turnos.")
            }
        }
    }

    private fun mostrarMensaje(texto: String) {
        binding.txtMensaje.text = texto
        binding.txtMensaje.visibility = View.VISIBLE
    }

    // Al tocar un turno: si está pendiente se ofrecen acciones; si no, solo se muestran los datos
    private fun mostrarOpciones(t: Turno) {
        val titulo = "${t.apellido}, ${t.nombre} - ${t.hora}"

        if (t.estado != "Pendiente") {
            AlertDialog.Builder(this)
                .setTitle(titulo)
                .setMessage(
                    "DNI ${t.dni}\nTel. ${t.telefono}\n${t.tipo_donacion}" +
                            (t.lugar?.let { "\n$it" } ?: "") + "\nEstado: ${t.estado}"
                )
                .setPositiveButton("Aceptar", null)
                .show()
            return
        }

        val opciones = arrayOf("Reprogramar", "Cancelar el turno", "Marcar como ausente", "Cerrar")
        AlertDialog.Builder(this)
            .setTitle(titulo)
            .setItems(opciones) { _, cual ->
                when (cual) {
                    0 -> {
                        val intent = Intent(this, TurnoFormActivity::class.java)
                        intent.putExtra(TurnoFormActivity.EXTRA_TURNO, Gson().toJson(t))
                        abrirFormulario.launch(intent)
                    }
                    1 -> confirmarEstado(t, "Cancelado")
                    2 -> confirmarEstado(t, "Ausente")
                }
            }
            .show()
    }

    private fun confirmarEstado(t: Turno, nuevo: String) {
        val mensaje = if (nuevo == "Cancelado")
            "¿Querés cancelar el turno de ${t.apellido}, ${t.nombre} de las ${t.hora}?"
        else
            "¿Querés marcar a ${t.apellido}, ${t.nombre} como ausente?"

        AlertDialog.Builder(this)
            .setTitle(if (nuevo == "Cancelado") "Cancelar turno" else "Marcar ausente")
            .setMessage(mensaje)
            .setPositiveButton("Sí") { _, _ -> cambiarEstado(t, nuevo) }
            .setNegativeButton("No", null)
            .show()
    }

    private fun cambiarEstado(t: Turno, nuevo: String) {
        lifecycleScope.launch {
            val r = ApiClient.cambiarEstadoTurno(t.id_turno, nuevo)

            if (revisarSesion()) return@launch

            if (r.ok) {
                Toast.makeText(this@TurnosActivity, r.mensaje ?: "Listo.", Toast.LENGTH_LONG).show()
                cargar()   // repite la consulta para ver el estado nuevo
            } else {
                // Por ejemplo: "Todavía no pasó la hora del turno..."
                AlertDialog.Builder(this@TurnosActivity)
                    .setMessage(r.error ?: "No se pudo completar la operación.")
                    .setPositiveButton("Aceptar", null)
                    .show()
            }
        }
    }
}