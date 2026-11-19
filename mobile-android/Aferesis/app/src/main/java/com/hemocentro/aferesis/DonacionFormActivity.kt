package com.hemocentro.aferesis

import android.app.DatePickerDialog
import android.app.TimePickerDialog
import android.content.DialogInterface
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.ArrayAdapter
import android.widget.EditText
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityDonacionFormBinding
import com.hemocentro.aferesis.databinding.DialogExcepcionBinding
import kotlinx.coroutines.launch
import java.time.LocalDate
import java.time.LocalTime

class DonacionFormActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"          // entrada: el donante (como JSON)
        const val EXTRA_TIPO_ID = "tipo_id"          // entrada: id del tipo de donación
        const val EXTRA_TIPO_NOMBRE = "tipo_nombre"  // entrada: nombre del tipo (Plaquetas, Plasma, Ambos)
    }

    private lateinit var binding: ActivityDonacionFormBinding
    private lateinit var donante: Donante
    private var idTipo = 0
    private var nombreTipo = ""

    private var vencimiento: LocalDate? = null
    private var horaInicio: LocalTime? = null
    private var horaFin: LocalTime? = null

    // Límites que superó el donante (los manda el servidor); se muestran en el cuadro de autorización
    private var violaciones: List<String> = emptyList()

    private val opcionesBolsa = listOf("—", "Doble", "Triple", "Cuádruple")
    private val opcionesBrazo = listOf("—", "Derecho", "Izquierdo")

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        val json = intent.getStringExtra(EXTRA_DONANTE)
        idTipo = intent.getIntExtra(EXTRA_TIPO_ID, 0)
        nombreTipo = intent.getStringExtra(EXTRA_TIPO_NOMBRE) ?: ""
        if (json == null || idTipo == 0) {
            finish()
            return
        }
        donante = Gson().fromJson(json, Donante::class.java)

        binding = ActivityDonacionFormBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtDonante.text =
            "${donante.apellido}, ${donante.nombre} · DNI ${donante.dni}\nTipo de donación: $nombreTipo"

        // Cantidades sugeridas según el tipo (el servidor las exige así); se pueden cambiar
        val (cp, cs) = when (nombreTipo) {
            "Plaquetas" -> "1" to "0"
            "Plasma" -> "0" to "1"
            "Ambos" -> "1" to "1"
            else -> "" to ""
        }
        binding.edtPlaquetas.setText(cp)
        binding.edtPlasma.setText(cs)

        binding.spnBolsa.adapter = adaptadorLista(opcionesBolsa)
        binding.spnBrazo.adapter = adaptadorLista(opcionesBrazo)

        binding.btnVencimiento.setOnClickListener { elegirVencimiento() }
        binding.btnHoraInicio.setOnClickListener {
            elegirHora(horaInicio, { horaInicio = it; mostrarFechasYHoras() }, { horaInicio = null; mostrarFechasYHoras() })
        }
        binding.btnHoraFin.setOnClickListener {
            elegirHora(horaFin, { horaFin = it; mostrarFechasYHoras() }, { horaFin = null; mostrarFechasYHoras() })
        }
        binding.btnGuardar.setOnClickListener { confirmarGuardado() }
        binding.btnCancelar.setOnClickListener { finish() }

        mostrarFechasYHoras()
    }

    private fun adaptadorLista(items: List<String>): ArrayAdapter<String> {
        val a = ArrayAdapter(this, android.R.layout.simple_spinner_item, items)
        a.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        return a
    }

    private fun formatoHora(h: LocalTime) = String.format("%02d:%02d", h.hour, h.minute)

    private fun mostrarFechasYHoras() {
        binding.btnVencimiento.text =
            vencimiento?.let { "Vencimiento de la bolsa: ${fechaCorta(it.toString())}" }
                ?: "Vencimiento de la bolsa (tocá para elegir)"
        binding.btnHoraInicio.text =
            horaInicio?.let { "Hora de inicio: ${formatoHora(it)}" } ?: "Hora de inicio (tocá para elegir)"
        binding.btnHoraFin.text =
            horaFin?.let { "Hora de fin: ${formatoHora(it)}" } ?: "Hora de fin (tocá para elegir)"
    }

    private fun elegirVencimiento() {
        val inicial = vencimiento ?: LocalDate.now()
        // en el calendario de Android los meses empiezan en 0
        val dialogo = DatePickerDialog(
            this,
            { _, anio, mes, dia ->
                vencimiento = LocalDate.of(anio, mes + 1, dia)
                mostrarFechasYHoras()
            },
            inicial.year, inicial.monthValue - 1, inicial.dayOfMonth
        )
        dialogo.setButton(DialogInterface.BUTTON_NEUTRAL, "Sin fecha") { _, _ ->
            vencimiento = null
            mostrarFechasYHoras()
        }
        dialogo.show()
    }

    private fun elegirHora(actual: LocalTime?, alElegir: (LocalTime) -> Unit, alQuitar: () -> Unit) {
        val inicial = actual ?: LocalTime.of(9, 0)
        val dialogo = TimePickerDialog(
            this,
            { _, h, m -> alElegir(LocalTime.of(h, m)) },
            inicial.hour, inicial.minute, true   // true = formato de 24 horas
        )
        dialogo.setButton(DialogInterface.BUTTON_NEUTRAL, "Sin hora") { _, _ -> alQuitar() }
        dialogo.show()
    }

    private fun txt(e: EditText) = e.text.toString().trim()

    // Arma el pedido con lo que hay en pantalla (todo como texto; el servidor valida)
    private fun construirPedido(): GuardarDonacionRequest {
        val bolsa = binding.spnBolsa.selectedItemPosition.let { if (it <= 0) "" else opcionesBolsa[it] }
        val brazo = binding.spnBrazo.selectedItemPosition.let { if (it <= 0) "" else opcionesBrazo[it] }

        return GuardarDonacionRequest(
            id_donante = donante.id_donante,
            id_tipo_donacion = idTipo,
            numero_donacion = txt(binding.edtNumero),
            cantidad_plaquetas = txt(binding.edtPlaquetas),
            cantidad_plasma = txt(binding.edtPlasma),
            peso = txt(binding.edtPeso),
            altura = txt(binding.edtAltura),
            incidencias = txt(binding.edtIncidencias),
            detalle = DetalleHemo2(
                hto = txt(binding.edtHto),
                hb = txt(binding.edtHb),
                ta = txt(binding.edtTa),
                pulso = txt(binding.edtPulso),
                temperatura = txt(binding.edtTemperatura),
                tipo_bolsa = bolsa,
                marca_bolsa = txt(binding.edtMarca),
                lote_bolsa = txt(binding.edtLote),
                nro_tubuladura = txt(binding.edtTubuladura),
                vencimiento_bolsa = vencimiento?.toString() ?: "",
                anticoagulante = txt(binding.edtAnticoagulante),
                brazo = brazo,
                dificultades = binding.chkDificultades.isChecked,
                peso_bolsa = txt(binding.edtPesoBolsa),
                hora_inicio = horaInicio?.let { formatoHora(it) } ?: "",
                hora_fin = horaFin?.let { formatoHora(it) } ?: ""
            )
        )
    }

    // Una extracción no se puede modificar después: se confirma antes de enviarla
    private fun confirmarGuardado() {
        AlertDialog.Builder(this)
            .setTitle("Guardar extracción")
            .setMessage("¿Registrar la extracción de ${donante.apellido}, ${donante.nombre}? No se puede modificar después.")
            .setPositiveButton("Sí, guardar") { _, _ ->
                limpiarErrores()
                enviar(construirPedido(), null)
            }
            .setNegativeButton("No", null)
            .show()
    }

    // Envía la planilla. Si "excepcion" es null y el servidor pide autorización, abre el cuadro.
    private fun enviar(base: GuardarDonacionRequest, excepcion: ExcepcionRequest?) {
        binding.btnGuardar.isEnabled = false
        binding.btnGuardar.text = "Guardando..."

        lifecycleScope.launch {
            val r = ApiClient.guardarDonacion(base.copy(excepcion = excepcion))

            binding.btnGuardar.isEnabled = true
            binding.btnGuardar.text = "Guardar extracción"

            if (revisarSesion()) return@launch

            if (r.ok) {
                setResult(RESULT_OK)   // avisa a la pantalla anterior que verifique de nuevo
                Toast.makeText(this@DonacionFormActivity, r.mensaje ?: "Extracción registrada.", Toast.LENGTH_LONG).show()
                finish()
                return@launch
            }

            // Supera los límites y todavía no hay autorización: pedirla
            if (r.requiere_excepcion && excepcion == null) {
                violaciones = r.violaciones ?: emptyList()
                pedirExcepcion(base, "", "", null)
                return@launch
            }

            // Se mandó la autorización y falló (clave incorrecta, sin permiso, motivo corto...):
            // se vuelve a abrir el cuadro con el mensaje, sin perder lo escrito
            val soloMotivo = r.campos.isNullOrEmpty() || r.campos.keys == setOf("motivo_excepcion")
            if (excepcion != null && r.bloqueos.isNullOrEmpty() && soloMotivo) {
                val mensaje = r.campos?.get("motivo_excepcion") ?: r.error ?: "No se pudo validar la autorización."
                pedirExcepcion(base, excepcion.motivo, excepcion.usuario ?: "", mensaje)
                return@launch
            }

            mostrarErrores(r)
        }
    }

    private fun pedirExcepcion(base: GuardarDonacionRequest, motivoPrevio: String, usuarioPrevio: String, error: String?) {
        val d = DialogExcepcionBinding.inflate(layoutInflater)

        d.txtViolaciones.text = "El donante supera los límites:\n" +
                violaciones.joinToString("\n") { "• " + it.replaceFirstChar { c -> c.uppercase() } }
        d.edtMotivo.setText(motivoPrevio)

        // Jefa y Administrador autorizan con su propia sesión; la Extractora necesita a una Jefa
        val pideAutorizador = !Sesion.puedeAutorizarExcepcion
        d.contAutoriza.visibility = if (pideAutorizador) View.VISIBLE else View.GONE
        d.edtUsuarioAut.setText(usuarioPrevio)   // la clave NUNCA se vuelve a mostrar

        if (error != null) {
            d.txtErrorDialog.text = error
            d.txtErrorDialog.visibility = View.VISIBLE
        }

        AlertDialog.Builder(this)
            .setTitle("Autorizar excepción")
            .setView(d.root)
            .setCancelable(false)
            .setPositiveButton("Autorizar y guardar") { _, _ ->
                val motivo = d.edtMotivo.text.toString().trim()
                val usuario = d.edtUsuarioAut.text.toString().trim()
                val clave = d.edtClaveAut.text.toString()

                when {
                    motivo.length < 10 ->
                        pedirExcepcion(base, motivo, usuario, "Explicá el motivo (al menos 10 caracteres).")
                    pideAutorizador && (usuario.isEmpty() || clave.isEmpty()) ->
                        pedirExcepcion(base, motivo, usuario, "Escribí el usuario y la clave de quien autoriza.")
                    else -> enviar(
                        base,
                        ExcepcionRequest(
                            motivo = motivo,
                            usuario = if (pideAutorizador) usuario else null,
                            clave = if (pideAutorizador) clave else null
                        )
                    )
                }
            }
            .setNegativeButton("Cancelar", null)
            .show()
    }

    private fun limpiarErrores() {
        binding.txtError.visibility = View.GONE
        for (t in listOf(binding.txtErrorBolsa, binding.txtErrorBrazo, binding.txtErrorVencimiento, binding.txtErrorHoras)) {
            t.visibility = View.GONE
        }
        for (til in listOf(
            binding.tilNumero, binding.tilPlaquetas, binding.tilPlasma, binding.tilPeso, binding.tilAltura,
            binding.tilHto, binding.tilHb, binding.tilTa, binding.tilPulso, binding.tilTemperatura,
            binding.tilMarca, binding.tilLote, binding.tilTubuladura, binding.tilAnticoagulante,
            binding.tilPesoBolsa, binding.tilIncidencias
        )) {
            til.error = null
        }
    }

    private fun mostrarErrores(r: DonacionResponse) {
        val bloqueos = r.bloqueos

        val general = when {
            !bloqueos.isNullOrEmpty() ->
                "No se puede registrar la extracción:\n" +
                        bloqueos.joinToString("\n") { "• " + it.replaceFirstChar { c -> c.uppercase() } }
            r.campos.isNullOrEmpty() -> r.error ?: "No se pudo guardar la extracción."
            else -> "Hay datos para corregir. Revisá los campos marcados en rojo."
        }
        binding.txtError.text = general
        binding.txtError.visibility = View.VISIBLE

        // Un mensaje debajo del control que corresponde (nombres de campo del servidor)
        r.campos?.forEach { (campo, mensaje) ->
            when (campo) {
                "numero_donacion" -> binding.tilNumero.error = mensaje
                "cantidad_plaquetas" -> binding.tilPlaquetas.error = mensaje
                "cantidad_plasma" -> binding.tilPlasma.error = mensaje
                "peso" -> binding.tilPeso.error = mensaje
                "altura" -> binding.tilAltura.error = mensaje
                "hto" -> binding.tilHto.error = mensaje
                "hb" -> binding.tilHb.error = mensaje
                "ta" -> binding.tilTa.error = mensaje
                "pulso" -> binding.tilPulso.error = mensaje
                "temperatura" -> binding.tilTemperatura.error = mensaje
                "peso_bolsa" -> binding.tilPesoBolsa.error = mensaje
                "tipo_bolsa" -> marcar(binding.txtErrorBolsa, mensaje)
                "brazo" -> marcar(binding.txtErrorBrazo, mensaje)
                "vencimiento_bolsa" -> marcar(binding.txtErrorVencimiento, mensaje)
                "hora_inicio", "hora_fin" -> marcar(binding.txtErrorHoras, mensaje)
            }
        }

        binding.scroll.smoothScrollTo(0, 0)
    }

    private fun marcar(vista: TextView, mensaje: String) {
        vista.text = mensaje
        vista.visibility = View.VISIBLE
    }
}