package com.hemocentro.aferesis

import android.app.DatePickerDialog
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.CheckBox
import android.widget.TextView
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityEntrevistaFormBinding
import com.hemocentro.aferesis.databinding.DialogRestriccionBinding
import com.hemocentro.aferesis.databinding.ItemCausaNuevaBinding
import kotlinx.coroutines.launch
import java.time.LocalDate

class EntrevistaFormActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"   // entrada: el donante (como JSON)
    }

    // Una causa de diferimiento agregada en esta pantalla, todavía sin guardar
    private data class CausaNueva(
        val causa: RestriccionCatalogo,
        val inicio: LocalDate,
        val obs: String
    )

    private lateinit var binding: ActivityEntrevistaFormBinding
    private lateinit var donante: Donante

    private var catalogo: List<RestriccionCatalogo> = emptyList()
    private val nuevas = mutableListOf<CausaNueva>()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        val json = intent.getStringExtra(EXTRA_DONANTE)
        if (json == null) {
            finish()
            return
        }
        donante = Gson().fromJson(json, Donante::class.java)

        binding = ActivityEntrevistaFormBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtDonante.text = "${donante.apellido}, ${donante.nombre} · DNI ${donante.dni}"

        // Peso y altura vienen precargados con los últimos valores del donante
        binding.edtPeso.setText(donante.peso?.let { numeroCorto(it) } ?: "")
        binding.edtAltura.setText(donante.altura?.let { numeroCorto(it) } ?: "")

        binding.btnAgregar.setOnClickListener { agregarCausa() }
        binding.btnGuardar.setOnClickListener { confirmarGuardado() }
        binding.btnCancelar.setOnClickListener { finish() }

        // No se puede guardar hasta que lleguen el catálogo y las restricciones vigentes
        binding.btnGuardar.isEnabled = false
        binding.btnAgregar.isEnabled = false
        cargarDatos()
        mostrarNuevas()
    }

    private fun cargarDatos() {
        lifecycleScope.launch {
            val cat = ApiClient.listarCatalogoRestricciones()
            val vig = ApiClient.listarRestriccionesDonante(donante.id_donante, true)

            if (revisarSesion()) return@launch

            if (cat.ok && vig.ok) {
                catalogo = cat.restricciones ?: emptyList()
                mostrarVigentes(vig.restricciones ?: emptyList())
                binding.btnGuardar.isEnabled = true
                binding.btnAgregar.isEnabled = catalogo.isNotEmpty()
            } else {
                mostrarErrorGeneral(cat.error ?: vig.error ?: "No se pudieron cargar los datos.")
            }
        }
    }

    // Una casilla por cada restricción vigente: tildada = esta evaluación la levanta
    private fun mostrarVigentes(lista: List<RestriccionDonante>) {
        binding.contLevantar.removeAllViews()
        if (lista.isEmpty()) {
            binding.txtLevantarAyuda.text = "Sin restricciones vigentes."
            return
        }
        binding.txtLevantarAyuda.text = "Tildá las que esta evaluación levanta:"
        for (x in lista) {
            val hasta = if (x.fecha_habilitacion_calculada != null)
                "hasta el ${fechaCorta(x.fecha_habilitacion_calculada)}"
            else
                "hasta nueva evaluación"
            val casilla = CheckBox(this)
            casilla.text = "${x.nombre} ($hasta)"
            casilla.textSize = 18f
            casilla.minHeight = 56 * resources.displayMetrics.density.toInt()
            casilla.tag = x.id_donante_restriccion
            binding.contLevantar.addView(casilla)
        }
    }

    // Redibuja la lista de causas nuevas
    private fun mostrarNuevas() {
        binding.contNuevas.removeAllViews()
        binding.txtSinNuevas.visibility = if (nuevas.isEmpty()) View.VISIBLE else View.GONE
        for (n in nuevas) {
            val fila = ItemCausaNuevaBinding.inflate(layoutInflater, binding.contNuevas, false)
            fila.txtCausa.text = "• ${n.causa.nombre}\n   desde el ${fechaCorta(n.inicio.toString())}"
            fila.btnQuitar.setOnClickListener {
                nuevas.remove(n)
                mostrarNuevas()
            }
            binding.contNuevas.addView(fila.root)
        }
    }

    private fun agregarCausa() {
        val d = DialogRestriccionBinding.inflate(layoutInflater)
        var inicio = LocalDate.now()

        val adaptador = ArrayAdapter(
            this, android.R.layout.simple_spinner_item, catalogo.map { it.nombre }
        )
        adaptador.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
        d.spnRestriccion.adapter = adaptador

        // Explica qué va a pasar con la causa elegida (el cálculo lo hace el servidor)
        fun explicar() {
            val c = catalogo[d.spnRestriccion.selectedItemPosition]
            d.txtDetalle.text = when {
                c.requiere_reevaluacion -> "Queda diferido hasta una nueva evaluación."
                c.dias_espera_minimos != null -> "Espera de ${c.dias_espera_minimos} días desde la fecha de inicio."
                else -> "Sin plazo definido: queda diferido hasta una nueva evaluación."
            }
        }
        d.spnRestriccion.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
            override fun onItemSelected(p: AdapterView<*>?, v: View?, pos: Int, id: Long) = explicar()
            override fun onNothingSelected(p: AdapterView<*>?) {}
        }
        explicar()

        d.btnInicio.text = fechaCorta(inicio.toString())
        d.btnInicio.setOnClickListener {
            // en el calendario de Android los meses empiezan en 0
            val calendario = DatePickerDialog(
                this,
                { _, anio, mes, dia ->
                    inicio = LocalDate.of(anio, mes + 1, dia)
                    d.btnInicio.text = fechaCorta(inicio.toString())
                },
                inicio.year, inicio.monthValue - 1, inicio.dayOfMonth
            )
            calendario.datePicker.maxDate = System.currentTimeMillis()   // el inicio nunca es futuro
            calendario.show()
        }

        AlertDialog.Builder(this)
            .setTitle("Agregar causa de diferimiento")
            .setView(d.root)
            .setPositiveButton("Agregar") { _, _ ->
                val causa = catalogo[d.spnRestriccion.selectedItemPosition]
                if (nuevas.any { it.causa.id_restriccion == causa.id_restriccion }) {
                    Toast.makeText(this, "Esa causa ya está agregada.", Toast.LENGTH_LONG).show()
                } else {
                    nuevas.add(CausaNueva(causa, inicio, d.edtObsCausa.text.toString().trim()))
                    mostrarNuevas()
                }
            }
            .setNegativeButton("Cancelar", null)
            .show()
    }

    // Una entrevista no se puede modificar después: se confirma antes de enviarla
    private fun confirmarGuardado() {
        AlertDialog.Builder(this)
            .setTitle("Guardar entrevista")
            .setMessage("¿Registrar la entrevista de ${donante.apellido}, ${donante.nombre}? No se puede modificar después.")
            .setPositiveButton("Sí, guardar") { _, _ -> guardar() }
            .setNegativeButton("No", null)
            .show()
    }

    private fun guardar() {
        limpiarErrores()

        val apto: Boolean? = when (binding.grupoApto.checkedRadioButtonId) {
            R.id.rbApto -> true
            R.id.rbNoApto -> false
            else -> null
        }

        // Ids de las restricciones vigentes que quedaron tildadas
        val levantar = (0 until binding.contLevantar.childCount)
            .map { binding.contLevantar.getChildAt(it) }
            .filterIsInstance<CheckBox>()
            .filter { it.isChecked }
            .map { it.tag as Int }

        val pedido = GuardarEntrevistaRequest(
            id_donante = donante.id_donante,
            apto = apto,
            peso = binding.edtPeso.text.toString().trim(),
            altura = binding.edtAltura.text.toString().trim(),
            recuento_plaquetas = binding.edtPlaquetas.text.toString().trim(),
            observaciones = binding.edtObs.text.toString().trim(),
            restricciones = nuevas.map {
                NuevaRestriccion(it.causa.id_restriccion, it.inicio.toString(), it.obs)
            },
            levantar = levantar
        )

        binding.btnGuardar.isEnabled = false
        binding.btnGuardar.text = "Guardando..."

        lifecycleScope.launch {
            val r = ApiClient.guardarEntrevista(pedido)

            binding.btnGuardar.isEnabled = true
            binding.btnGuardar.text = "Guardar entrevista"

            if (revisarSesion()) return@launch

            if (r.ok) {
                setResult(RESULT_OK)   // avisa a la pantalla anterior que recargue el historial
                Toast.makeText(this@EntrevistaFormActivity, r.mensaje ?: "Entrevista registrada.", Toast.LENGTH_LONG).show()
                finish()
            } else {
                mostrarErrores(r)
            }
        }
    }

    private fun limpiarErrores() {
        binding.txtError.visibility = View.GONE
        binding.txtErrorApto.visibility = View.GONE
        binding.txtErrorLevantar.visibility = View.GONE
        binding.txtErrorRestricciones.visibility = View.GONE
        binding.tilPeso.error = null
        binding.tilAltura.error = null
        binding.tilPlaquetas.error = null
        binding.tilObs.error = null
    }

    private fun mostrarErrorGeneral(texto: String) {
        binding.txtError.text = texto
        binding.txtError.visibility = View.VISIBLE
        binding.scroll.smoothScrollTo(0, 0)
    }

    private fun mostrarErrores(r: EntrevistaResponse) {
        val bloqueos = r.bloqueos

        // Arriba: el motivo general. Si no puede ser apto, la lista completa de motivos.
        mostrarErrorGeneral(
            when {
                !bloqueos.isNullOrEmpty() ->
                    "No se puede marcar como apto:\n" +
                            bloqueos.joinToString("\n") { "• " + it.replaceFirstChar { c -> c.uppercase() } }
                r.campos.isNullOrEmpty() -> r.error ?: "No se pudo guardar la entrevista."
                else -> "Hay datos para corregir. Revisá los campos marcados en rojo."
            }
        )

        // Y un mensaje debajo del control que corresponde (nombres de campo del servidor)
        r.campos?.forEach { (campo, mensaje) ->
            when (campo) {
                "apto" -> marcar(binding.txtErrorApto, mensaje)
                "peso" -> binding.tilPeso.error = mensaje
                "altura" -> binding.tilAltura.error = mensaje
                "recuento_plaquetas" -> binding.tilPlaquetas.error = mensaje
                "observaciones" -> binding.tilObs.error = mensaje
                "levantar" -> marcar(binding.txtErrorLevantar, mensaje)
                "restricciones" -> marcar(binding.txtErrorRestricciones, mensaje)
            }
        }
    }

    private fun marcar(vista: TextView, mensaje: String) {
        vista.text = mensaje
        vista.visibility = View.VISIBLE
    }
}