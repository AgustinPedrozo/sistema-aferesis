package com.hemocentro.aferesis

import android.graphics.Color
import android.os.Bundle
import android.view.View
import androidx.lifecycle.lifecycleScope
import com.hemocentro.aferesis.databinding.ActivityEstadisticasBinding
import kotlinx.coroutines.launch
import java.time.LocalDate

class EstadisticasActivity : BaseActivity() {

    private lateinit var binding: ActivityEstadisticasBinding

    private val anioActual = LocalDate.now().year
    private var anio = anioActual

    // Si se cambia de año rápido, se ignoran las respuestas viejas
    private var pedidoActual = 0

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        // Solo Jefa de Área/Gerente y Administrador
        if (!Sesion.puedeVerEstadisticas) {
            finish()
            return
        }

        binding = ActivityEstadisticasBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnAnterior.setOnClickListener { anio--; cargar() }
        binding.btnSiguiente.setOnClickListener { anio++; cargar() }
        binding.btnVolver.setOnClickListener { finish() }

        cargar()
    }

    private fun cargar() {
        binding.txtAnio.text = anio.toString()
        // El servidor acepta de 2000 a 2100; la app llega hasta el año actual
        binding.btnAnterior.isEnabled = anio > 2000
        binding.btnSiguiente.isEnabled = anio < anioActual

        val miPedido = ++pedidoActual
        binding.contDatos.visibility = View.GONE
        binding.txtMensaje.setTextColor(Color.DKGRAY)
        binding.txtMensaje.text = "Cargando..."
        binding.txtMensaje.visibility = View.VISIBLE

        lifecycleScope.launch {
            val r = ApiClient.estadisticas(anio)

            if (miPedido != pedidoActual) return@launch   // llegó tarde
            if (revisarSesion()) return@launch

            val d = r.donantes
            val dn = r.donaciones
            val df = r.diferimientos
            val en = r.entrevistas
            val tu = r.turnos
            val co = r.control

            if (!r.ok || d == null || dn == null || df == null || en == null || tu == null || co == null) {
                binding.txtMensaje.setTextColor(Color.parseColor("#B00020"))
                binding.txtMensaje.text = r.error ?: "No se pudieron cargar las estadísticas."
                return@launch
            }

            binding.txtMensaje.visibility = View.GONE
            binding.contDatos.visibility = View.VISIBLE
            binding.txtGenerado.text = "Generado el ${fechaHoraCorta(r.generado)}"
            mostrar(d, dn, df, en, tu, co)
        }
    }

    private fun mostrar(
        d: ResumenDonantes, dn: ResumenDonaciones, df: ResumenDiferimientos,
        en: ResumenEntrevistas, tu: ResumenTurnos, co: ResumenControl
    ) {
        // ---- Donantes (hoy) ----
        binding.txtDonantes.text = buildString {
            append("Registrados: ${d.total}")
            append("\nActivos: ${d.activos}")
            append("\nDados de baja: ${d.inactivos}")
            append("\nNuevos en $anio: ${d.nuevos_en_anio}")
            append("\n\nPor sexo (activos):")
            d.por_sexo.forEach { append("\n   ${it.sexo}: ${it.cantidad}") }
            append("\n\nPor edad (activos):")
            d.por_edad.forEach { append("\n   ${it.rango}: ${it.cantidad}") }
        }

        // ---- Donaciones (año) ----
        binding.tituloDonaciones.text = "Donaciones de $anio"
        binding.txtDonaciones.text = buildString {
            append("Total: ${dn.total}")
            append("\nDonantes distintos: ${dn.donantes_distintos}")
            append("\nPromedio por donante: ${dn.promedio_por_donante ?: "—"}")
            append("\nUnidades de plaquetas: ${dn.unidades_plaquetas}")
            append("\nUnidades de plasma: ${dn.unidades_plasma}")
            append("\n\nPor tipo:")
            dn.por_tipo.forEach { append("\n   ${it.tipo}: ${it.cantidad}") }
        }

        // ---- Por mes: una barra de texto por mes, proporcional al mes con más donaciones ----
        val maximo = dn.por_mes.maxOfOrNull { it.donaciones } ?: 0
        binding.txtMeses.text = if (dn.total == 0) {
            "Sin donaciones en $anio."
        } else {
            dn.por_mes.joinToString("\n") { m ->
                val largo = if (maximo > 0) Math.round(m.donaciones * 14f / maximo) else 0
                val barra = if (m.donaciones > 0 && largo == 0) "▌" else "█".repeat(largo)
                String.format("%-3s %3d %s", m.nombre.take(3), m.donaciones, barra)
            }
        }

        // ---- Diferimientos ----
        binding.txtDiferimientos.text = buildString {
            append("Donantes diferidos hoy: ${df.donantes_diferidos_vigentes}")
            append("\n\nCausas más frecuentes de $anio:")
            if (df.causas_mas_frecuentes.isEmpty()) {
                append("\n   Sin diferimientos registrados.")
            } else {
                df.causas_mas_frecuentes.forEachIndexed { i, c ->
                    append("\n   ${i + 1}. ${c.causa}: ${c.cantidad}")
                }
            }
        }

        // ---- Entrevistas (año) ----
        binding.tituloEntrevistas.text = "Entrevistas de $anio"
        binding.txtEntrevistas.text = buildString {
            append("Total: ${en.total}")
            append("\nAptos: ${en.aptos}${porcentaje(en.aptos, en.total)}")
            append("\nNo aptos: ${en.no_aptos}${porcentaje(en.no_aptos, en.total)}")
        }

        // ---- Turnos (año) ----
        binding.tituloTurnos.text = "Turnos de $anio"
        binding.txtTurnos.text = buildString {
            tu.por_estado.forEach { append("${it.estado}: ${it.cantidad}\n") }
            append("\nAusentismo: ")
            append(tu.ausentismo_porcentaje?.let { "$it %" } ?: "—")
            append("\n(ausentes sobre completados + ausentes)")
        }

        // ---- Control ----
        binding.txtControl.text = "Extracciones con excepción autorizada en $anio: ${co.excepciones_autorizadas}"
    }

    // " (62 %)"; vacío si no hay con qué calcular
    private fun porcentaje(parte: Int, total: Int): String =
        if (total > 0) " (${Math.round(parte * 100f / total)} %)" else ""
}