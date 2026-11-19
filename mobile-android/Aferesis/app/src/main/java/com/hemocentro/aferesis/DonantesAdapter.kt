package com.hemocentro.aferesis

import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import androidx.recyclerview.widget.RecyclerView
import com.hemocentro.aferesis.databinding.ItemDonanteBinding

class DonantesAdapter(
    private val alTocar: (Donante) -> Unit
) : RecyclerView.Adapter<DonantesAdapter.Fila>() {

    private var lista: List<Donante> = emptyList()

    // Reemplaza el contenido de la lista y la redibuja
    fun mostrar(nueva: List<Donante>) {
        lista = nueva
        notifyDataSetChanged()
    }

    class Fila(val b: ItemDonanteBinding) : RecyclerView.ViewHolder(b.root)

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): Fila {
        val b = ItemDonanteBinding.inflate(LayoutInflater.from(parent.context), parent, false)
        return Fila(b)
    }

    override fun getItemCount(): Int = lista.size

    override fun onBindViewHolder(fila: Fila, posicion: Int) {
        val d = lista[posicion]
        fila.b.txtNombre.text = "${d.apellido}, ${d.nombre}"
        fila.b.txtDetalle.text = "DNI ${d.dni}  ·  Tel. ${d.telefono}"
        fila.b.txtBaja.visibility = if (d.activo) View.GONE else View.VISIBLE
        fila.b.root.setOnClickListener { alTocar(d) }
    }
}