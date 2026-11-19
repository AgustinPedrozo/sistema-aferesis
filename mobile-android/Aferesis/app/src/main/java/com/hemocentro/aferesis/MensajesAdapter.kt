package com.hemocentro.aferesis

import android.graphics.Color
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.hemocentro.aferesis.databinding.ItemMensajeBinding

class MensajesAdapter(
    private val alTocar: (MensajeWhatsapp) -> Unit
) : RecyclerView.Adapter<MensajesAdapter.Fila>() {

    private var lista: List<MensajeWhatsapp> = emptyList()

    fun mostrar(nueva: List<MensajeWhatsapp>) {
        lista = nueva
        notifyDataSetChanged()
    }

    class Fila(val b: ItemMensajeBinding) : RecyclerView.ViewHolder(b.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Fila {
        val b = ItemMensajeBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return Fila(b)
    }

    override fun getItemCount(): Int = lista.size

    override fun onBindViewHolder(fila: Fila, posicion: Int) {
        val m = lista[posicion]
        fila.b.txtNombre.text = "${m.apellido}, ${m.nombre}"
        fila.b.txtTurno.text = "Turno: ${fechaHoraCorta(m.fecha_hora_turno)}"

        // Las filas se reciclan: el color se fija siempre, en las dos ramas
        if (m.respuesta_recibida != null) {
            fila.b.txtRespuesta.text = "Respondió: ${m.respuesta_recibida}"
            fila.b.txtRespuesta.setTextColor(Color.BLACK)
        } else {
            fila.b.txtRespuesta.text = "Sin respuesta del donante"
            fila.b.txtRespuesta.setTextColor(Color.parseColor("#757575"))
        }

        val (texto, color) = when {
            m.estado_envio == "Error" -> "No se pudo enviar" to "#B00020"
            m.estado_envio == "Pendiente" -> "Envío pendiente" to "#757575"
            m.pendienteDeAtender -> "RESPUESTA PENDIENTE DE ATENDER" to "#E65100"
            m.respuesta_recibida != null -> "Atendida por ${m.atendido_por ?: "—"}" to "#2E7D32"
            else -> "Enviado, sin respuesta" to "#757575"
        }
        fila.b.txtEstado.text = texto
        fila.b.txtEstado.setTextColor(Color.parseColor(color))

        fila.b.root.setOnClickListener { alTocar(m) }
    }
}