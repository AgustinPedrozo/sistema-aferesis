package com.hemocentro.aferesis

import android.os.Bundle
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.Toast
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import com.hemocentro.aferesis.databinding.ActivityDonantesBinding
import kotlinx.coroutines.launch
import android.view.View
import android.content.Intent
import androidx.activity.result.contract.ActivityResultContracts
import com.google.gson.Gson

class DonantesActivity : BaseActivity() {

    companion object {
        const val EXTRA_ELEGIR = "elegir"            // entrada: true = la pantalla sirve para elegir un donante
        const val EXTRA_ELEGIDO = "donante_elegido"  // salida: el donante elegido
    }

    private var modoElegir = false

    private lateinit var binding: ActivityDonantesBinding
    private lateinit var adaptador: DonantesAdapter

    // Se ejecuta cuando vuelve de la ficha; si hubo cambios, repite la búsqueda
    private val abrirFicha = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) buscar()
    }

    // Después de registrar un donante nuevo, se lo busca por DNI para mostrarlo en la lista
    private val abrirFormulario = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) {
            val json = resultado.data?.getStringExtra(DonanteFormActivity.EXTRA_GUARDADO)
            if (json != null) {
                val nuevo = Gson().fromJson(json, Donante::class.java)
                binding.edtBuscar.setText(nuevo.dni)
                binding.chkInactivos.isChecked = false
                buscar()
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        if (isFinishing) return   // BaseActivity ya mandó al login

        binding = ActivityDonantesBinding.inflate(layoutInflater)
        setContentView(binding.root)

        modoElegir = intent.getBooleanExtra(EXTRA_ELEGIR, false)
        if (modoElegir) {
            binding.txtTitulo.text = "Elegir donante"
            // A un donante dado de baja no se le pueden dar turnos: no se ofrecen
            binding.chkInactivos.visibility = View.GONE
        }

        adaptador = DonantesAdapter { donante ->
            if (modoElegir) {
                // Devuelve el donante a la pantalla que nos abrió y se cierra
                setResult(RESULT_OK, Intent().putExtra(EXTRA_ELEGIDO, Gson().toJson(donante)))
                finish()
            } else {
                val intent = Intent(this, FichaDonanteActivity::class.java)
                intent.putExtra(FichaDonanteActivity.EXTRA_DONANTE, Gson().toJson(donante))
                abrirFicha.launch(intent)
            }
        }

        binding.rvDonantes.layoutManager = LinearLayoutManager(this)
        binding.rvDonantes.adapter = adaptador

        binding.btnBuscar.setOnClickListener { buscar() }
        binding.btnVolver.setOnClickListener { finish() }
        binding.btnNuevo.setOnClickListener {
            abrirFormulario.launch(Intent(this, DonanteFormActivity::class.java))
        }

        // Tecla "buscar" del teclado = tocar el botón Buscar
        binding.edtBuscar.setOnEditorActionListener { _, accion, _ ->
            if (accion == EditorInfo.IME_ACTION_SEARCH) {
                buscar()
                true
            } else {
                false
            }
        }
    }

    private fun buscar() {
        val texto = binding.edtBuscar.text.toString().trim()

        // Validación de ayuda visual (la de verdad la hace el servidor)
        if (texto.length < 2) {
            mostrarMensaje("Escribí al menos 2 caracteres para buscar.")
            return
        }

        // Esconder el teclado para ver la lista completa
        val teclado = getSystemService(INPUT_METHOD_SERVICE) as InputMethodManager
        teclado.hideSoftInputFromWindow(binding.edtBuscar.windowToken, 0)

        binding.btnBuscar.isEnabled = false
        binding.btnBuscar.text = "Buscando..."

        lifecycleScope.launch {
            val r = ApiClient.buscarDonantes(texto, binding.chkInactivos.isChecked)

            binding.btnBuscar.isEnabled = true
            binding.btnBuscar.text = "Buscar"

            if (revisarSesion()) return@launch

            if (r.ok) {
                val lista = r.donantes ?: emptyList()
                adaptador.mostrar(lista)
                when {
                    lista.isEmpty() -> mostrarMensaje("No se encontró ningún donante.")
                    lista.size >= 50 -> mostrarMensaje("Se muestran los primeros 50. Escribí más datos para acotar la búsqueda.")
                    lista.size == 1 -> mostrarMensaje("1 donante encontrado.")
                    else -> mostrarMensaje("${lista.size} donantes encontrados.")
                }
            } else {
                adaptador.mostrar(emptyList())
                mostrarMensaje(r.error ?: "No se pudo buscar.")
            }
        }
    }

    private fun mostrarMensaje(texto: String) {
        binding.txtMensaje.text = texto
        binding.txtMensaje.visibility = View.VISIBLE
    }
}