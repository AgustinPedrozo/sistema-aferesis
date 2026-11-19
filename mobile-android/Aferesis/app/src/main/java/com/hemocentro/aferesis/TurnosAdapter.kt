package com.hemocentro.aferesis

import android.graphics.Color
import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.hemocentro.aferesis.databinding.ItemTurnoBinding

class TurnosAdapter(
    private val alTocar: (Turno) -> Unit
) : RecyclerView.Adapter<TurnosAdapter.Fila>() {

    private var lista: List<Turno> = emptyList()

    fun mostrar(nueva: List<Turno>) {
        lista = nueva
        notifyDataSetChanged()
    }

    class Fila(val b: ItemTurnoBinding) : RecyclerView.ViewHolder(b.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Fila {
        val b = ItemTurnoBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return Fila(b)
    }

    override fun getItemCount(): Int = lista.size

    override fun onBindViewHolder(fila: Fila, posicion: Int) {
        val t = lista[posicion]
        fila.b.txtHora.text = t.hora
        fila.b.txtNombre.text = "${t.apellido}, ${t.nombre}"
        fila.b.txtDetalle.text = t.tipo_donacion + (t.lugar?.let { " · $it" } ?: "")
        fila.b.txtEstado.text = t.estado
        // Cada estado con su color para distinguirlo de un vistazo
        val color = when (t.estado) {
            "Pendiente" -> "#1565C0"
            "Completado" -> "#2E7D32"
            "Ausente" -> "#B00020"
            else -> "#757575"   // Cancelado
        }
        fila.b.txtEstado.setTextColor(Color.parseColor(color))
        fila.b.root.setOnClickListener { alTocar(t) }
    }
}