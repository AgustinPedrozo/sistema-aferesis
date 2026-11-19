package com.hemocentro.aferesis
import android.content.Intent
import android.os.Bundle
import android.view.View
import android.widget.Toast
import androidx.appcompat.app.AlertDialog
import androidx.appcompat.app.AppCompatActivity
import com.hemocentro.aferesis.databinding.ActivityMenuBinding
import androidx.activity.result.contract.ActivityResultContracts
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.launch

class MenuActivity : AppCompatActivity() {

    private lateinit var binding: ActivityMenuBinding

    // A qué pantalla ir con el donante elegido
    private var destino: Class<*> = EntrevistaActivity::class.java

    // Vuelve de la búsqueda de donantes: abre la pantalla elegida (entrevista o extracción) con el donante
    private val elegirDonante = registerForActivityResult(
        ActivityResultContracts.StartActivityForResult()
    ) { resultado ->
        if (resultado.resultCode == RESULT_OK) {
            val json = resultado.data?.getStringExtra(DonantesActivity.EXTRA_ELEGIDO)
            if (json != null) {
                val i = Intent(this, destino)
                // Las dos pantallas reciben el donante con la misma clave "donante"
                i.putExtra("donante", json)
                startActivity(i)
            }
        }
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        // Si no hay sesión, no se puede estar acá: volver al login
        val usuario = Sesion.usuario
        if (usuario == null || ApiClient.token == null) {
            volverAlLogin()
            return
        }

        binding = ActivityMenuBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.txtBienvenida.text = "Hola, ${usuario.nombre_completo}"
        binding.txtRol.text = "Rol: ${usuario.rol}"

        // Ocultar lo que el rol no puede usar
        binding.btnEstadisticas.visibility =
            if (Sesion.puedeVerEstadisticas) View.VISIBLE else View.GONE
        binding.btnConfiguracion.visibility =
            if (Sesion.puedeVerConfiguracion) View.VISIBLE else View.GONE
        binding.btnEntrevista.visibility =
            if (Sesion.puedeHacerEntrevista) View.VISIBLE else View.GONE
        binding.btnExtraccion.visibility =
            if (Sesion.puedeVerExtraccion) View.VISIBLE else View.GONE
        binding.btnHemo3.visibility =
            if (Sesion.puedeCargarHemo3) View.VISIBLE else View.GONE
        binding.btnMensajes.visibility =
            if (Sesion.puedeVerMensajes) View.VISIBLE else View.GONE

        // Por ahora cada módulo solo avisa que falta construirlo
        binding.btnDonantes.setOnClickListener {
            startActivity(Intent(this, DonantesActivity::class.java))
        }
        binding.btnTurnos.setOnClickListener {
            startActivity(Intent(this, TurnosActivity::class.java))
        }
        binding.btnEntrevista.setOnClickListener {
            val i = Intent(this, DonantesActivity::class.java)
            i.putExtra(DonantesActivity.EXTRA_ELEGIR, true)
            destino = EntrevistaActivity::class.java
            elegirDonante.launch(i)
        }

        binding.btnHemo3.setOnClickListener {
            val i = Intent(this, DonantesActivity::class.java)
            i.putExtra(DonantesActivity.EXTRA_ELEGIR, true)
            destino = Hemo3Activity::class.java
            elegirDonante.launch(i)
        }

        binding.btnExtraccion.setOnClickListener {
            val i = Intent(this, DonantesActivity::class.java)
            i.putExtra(DonantesActivity.EXTRA_ELEGIR, true)
            destino = ExtraccionActivity::class.java
            elegirDonante.launch(i)
        }
        binding.btnMensajes.setOnClickListener {
            startActivity(Intent(this, MensajesActivity::class.java))
        }
        binding.btnEstadisticas.setOnClickListener {
            startActivity(Intent(this, EstadisticasActivity::class.java))
        }
        binding.btnConfiguracion.setOnClickListener {
            startActivity(Intent(this, ConfiguracionActivity::class.java))
        }

        binding.btnCerrarSesion.setOnClickListener { confirmarCierre() }
    }

    // Cada vez que se vuelve al menú, se actualiza el contador de respuestas pendientes
    override fun onResume() {
        super.onResume()
        // Si el menú se cerró por falta de sesión, no hay pantalla que actualizar
        if (!::binding.isInitialized || !Sesion.puedeVerMensajes) return

        lifecycleScope.launch {
            val r = ApiClient.listarWhatsapp(false)

            if (ApiClient.sesionVencida) {
                ApiClient.sesionVencida = false
                Toast.makeText(this@MenuActivity, "La sesión venció. Ingresá de nuevo.", Toast.LENGTH_LONG).show()
                volverAlLogin()
                return@launch
            }
            if (r.ok) {
                binding.btnMensajes.text =
                    if (r.total_pendientes > 0) "Mensajes de WhatsApp (${r.total_pendientes})"
                    else "Mensajes de WhatsApp"
            }
        }
    }

    private fun confirmarCierre() {
        AlertDialog.Builder(this)
            .setTitle("Cerrar sesión")
            .setMessage("¿Querés cerrar la sesión?")
            .setPositiveButton("Sí, cerrar") { _, _ -> volverAlLogin() }
            .setNegativeButton("No", null)
            .show()
    }

    private fun volverAlLogin() {
        Sesion.cerrar()
        val intent = Intent(this, LoginActivity::class.java)
        // Borra las pantallas anteriores: "Atrás" no puede volver al menú
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
        startActivity(intent)
        finish()
    }
}