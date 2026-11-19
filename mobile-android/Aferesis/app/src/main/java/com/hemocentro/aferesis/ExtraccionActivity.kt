package com.hemocentro.aferesis

import android.graphics.Color
import android.os.Bundle
import android.view.View
import android.widget.AdapterView
import android.widget.ArrayAdapter
import android.widget.Toast
import androidx.lifecycle.lifecycleScope
import com.google.gson.Gson
import com.hemocentro.aferesis.databinding.ActivityExtraccionBinding
import kotlinx.coroutines.launch
import android.content.Intent
import androidx.activity.result.contract.ActivityResultContracts

class ExtraccionActivity : BaseActivity() {

    companion object {
        const val EXTRA_DONANTE = "donante"   // entrada: el donante (como JSON)
    }

    private lateinit var binding: ActivityExtraccionBinding
    private lateinit var donante: Donante
    private var tipos: List<TipoDonacion> = emptyList()

    // Si el usuario cambia de tipo rápido, se ignoran las respuestas viejas
    private var pedidoActual = 0

    // Cuando vuelve de la planilla con la extracción guardada, verifica de nuevo y recarga el historial
    private val abrirPlanilla = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) {
            verificar()
            cargarHistorial()
        }
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

        binding = ActivityExtraccionBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtDonante.text = "${donante.apellido}, ${donante.nombre}\nDNI ${donante.dni}"

        // El botón de registrar solo existe para Extractora, Jefa y Administrador
        binding.btnRegistrar.visibility =
            if (Sesion.puedeRegistrarExtraccion) View.VISIBLE else View.GONE
        binding.btnRegistrar.isEnabled = false
        binding.btnRegistrar.setOnClickListener {
            val tipo = tipos.getOrNull(binding.spnTipo.selectedItemPosition)
                ?: return@setOnClickListener
            val i = Intent(this, DonacionFormActivity::class.java)
            i.putExtra(DonacionFormActivity.EXTRA_DONANTE, Gson().toJson(donante))
            i.putExtra(DonacionFormActivity.EXTRA_TIPO_ID, tipo.id_tipo_donacion)
            i.putExtra(DonacionFormActivity.EXTRA_TIPO_NOMBRE, tipo.nombre)
            abrirPlanilla.launch(i)
        }
        binding.btnVolver.setOnClickListener { finish() }

        cargarTipos()
        cargarHistorial()
    }

    private fun cargarTipos() {
        setEstado("Cargando...", "#757575")
        lifecycleScope.launch {
            val r = ApiClient.listarTipos()
            if (revisarSesion()) return@launch

            val lista = r.tipos
            if (r.ok && !lista.isNullOrEmpty()) {
                tipos = lista
                val adaptador = ArrayAdapter(
                    this@ExtraccionActivity,
                    android.R.layout.simple_spinner_item,
                    lista.map { it.nombre }
                )
                adaptador.setDropDownViewResource(android.R.layout.simple_spinner_dropdown_item)
                binding.spnTipo.adapter = adaptador
                // Al armarse y cada vez que cambia el tipo, se vuelve a verificar
                binding.spnTipo.onItemSelectedListener = object : AdapterView.OnItemSelectedListener {
                    override fun onItemSelected(p: AdapterView<*>?, v: View?, pos: Int, id: Long) = verificar()
                    override fun onNothingSelected(p: AdapterView<*>?) {}
                }
            } else {
                setEstado(r.error ?: "No hay tipos de donación habilitados.", "#B00020")
            }
        }
    }

    // Pregunta al servidor si el donante puede donar hoy con el tipo elegido
    private fun verificar() {
        val tipo = tipos.getOrNull(binding.spnTipo.selectedItemPosition) ?: return
        val miPedido = ++pedidoActual

        binding.btnRegistrar.isEnabled = false
        binding.txtDetalle.text = ""
        binding.txtDatos.text = ""
        setEstado("Verificando...", "#757575")

        lifecycleScope.launch {
            val r = ApiClient.verificarDonacion(donante.id_donante, tipo.id_tipo_donacion)

            if (miPedido != pedidoActual) return@launch   // llegó tarde
            if (revisarSesion()) return@launch

            mostrarVerificacion(r)
        }
    }

    private fun mostrarVerificacion(r: VerificarDonacionResponse) {
        if (!r.ok) {
            setEstado(r.error ?: "No se pudo verificar.", "#B00020")
            return
        }
        val bloqueos = r.bloqueos ?: emptyList()
        val violaciones = r.violaciones ?: emptyList()
        val avisos = r.avisos ?: emptyList()

        val detalle = StringBuilder()
        when {
            bloqueos.isNotEmpty() -> {
                setEstado("NO PUEDE DONAR HOY", "#B00020")
                detalle.append(lista(bloqueos))
                if (violaciones.isNotEmpty()) {
                    detalle.append("\n\nAdemás supera los límites:\n").append(lista(violaciones))
                }
            }
            r.requiere_excepcion -> {
                setEstado("SUPERA LOS LÍMITES", "#E65100")
                detalle.append(lista(violaciones))
                detalle.append("\n\nSolo se puede registrar con la autorización de la Jefa de Área/Gerente.")
            }
            r.puede_donar -> setEstado("PUEDE DONAR HOY", "#2E7D32")
            else -> setEstado("No se pudo determinar el estado.", "#757575")
        }
        if (avisos.isNotEmpty()) {
            if (detalle.isNotEmpty()) detalle.append("\n\n")
            detalle.append(avisos.joinToString("\n") { "ℹ $it" })
        }
        binding.txtDetalle.text = detalle.toString()

        // Datos para tener a la vista: entrevista de hoy y cuánto lleva donado
        val datos = StringBuilder()
        val e = r.entrevista
        datos.append(
            if (e == null) "Entrevista de hoy: no tiene"
            else "Entrevista de hoy: ${if (e.apto) "Apto" else "No apto"}"
        )
        val d = r.datos
        if (d != null) {
            datos.append("\nÚltima donación: ")
            datos.append(if (d.ultima_donacion == null) "todavía no donó" else fechaHoraCorta(d.ultima_donacion))
            if (d.habilitado_desde != null) {
                datos.append("\nHabilitado desde: ${fechaHoraCorta(d.habilitado_desde)}")
            }
            datos.append("\nEsta semana: ${d.donaciones_semana} de ${d.limite_semanal}")
            datos.append("\nEste año: ${d.donaciones_anio} de ${d.limite_anual}")
        }
        binding.txtDatos.text = datos.toString()

        // Con un bloqueo médico no se puede registrar (no hay excepción posible)
        binding.btnRegistrar.isEnabled = bloqueos.isEmpty()
    }

    private fun cargarHistorial() {
        binding.txtHistorial.setTextColor(Color.DKGRAY)
        binding.txtHistorial.text = "Cargando..."

        lifecycleScope.launch {
            val r = ApiClient.listarDonaciones(donante.id_donante)
            if (revisarSesion()) return@launch

            if (!r.ok) {
                binding.txtHistorial.setTextColor(Color.parseColor("#B00020"))
                binding.txtHistorial.text = r.error ?: "No se pudo cargar el historial."
                return@launch
            }
            val donaciones = r.donaciones ?: emptyList()
            if (donaciones.isEmpty()) {
                binding.txtHistorial.text = "Todavía no donó."
                return@launch
            }
            binding.txtHistorial.text = donaciones.joinToString("\n\n") { x ->
                "${fechaHoraCorta(x.fecha_donacion)}  ·  ${x.tipo_donacion}" +
                        "\nN.º ${x.numero_donacion}  ·  Plaquetas ${x.cantidad_plaquetas}, plasma ${x.cantidad_plasma}" +
                        "\nExtrajo: ${x.extractora}" +
                        (if (x.es_excepcion)
                            "\n⚠ Excepción autorizada por ${x.autorizada_por ?: "—"}" +
                                    (x.motivo_excepcion?.let { "\nMotivo: $it" } ?: "")
                        else "") +
                        (x.incidencias?.let { "\nIncidencias: $it" } ?: "")
            }
        }
    }

    private fun setEstado(texto: String, color: String) {
        binding.txtEstado.text = texto
        binding.txtEstado.setTextColor(Color.parseColor(color))
    }

    // Lista con viñetas, con la primera letra en mayúscula
    private fun lista(items: List<String>): String =
        items.joinToString("\n") { "• " + it.replaceFirstChar { c -> c.uppercase() } }
}