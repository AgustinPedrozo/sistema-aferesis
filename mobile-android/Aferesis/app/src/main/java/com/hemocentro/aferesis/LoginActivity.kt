package com.hemocentro.aferesis

import android.content.Intent
import android.os.Bundle
import android.view.View
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.hemocentro.aferesis.databinding.ActivityLoginBinding
import kotlinx.coroutines.launch

class LoginActivity : AppCompatActivity() {

    private lateinit var binding: ActivityLoginBinding

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityLoginBinding.inflate(layoutInflater)
        setContentView(binding.root)

        binding.btnIngresar.setOnClickListener { ingresar() }
    }

    private fun ingresar() {
        val usuario = binding.edtUsuario.text.toString().trim()
        val clave = binding.edtClave.text.toString().trim()

        binding.txtError.visibility = View.GONE

        // Validación de ayuda visual (la de verdad la hace el servidor)
        if (usuario.isEmpty() || clave.isEmpty()) {
            mostrarError("Escribí tu usuario y tu clave.")
            return
        }

        // Mientras espera, el botón queda apagado para evitar dobles toques
        binding.btnIngresar.isEnabled = false
        binding.btnIngresar.text = "Ingresando..."

        lifecycleScope.launch {
            val r = ApiClient.login(usuario, clave)

            binding.btnIngresar.isEnabled = true
            binding.btnIngresar.text = "Ingresar"

            if (r.ok) {
                startActivity(Intent(this@LoginActivity, MenuActivity::class.java))
                finish()   // cierra el login para que "Atrás" no vuelva a él
            } else {
                mostrarError(r.error ?: "No se pudo iniciar sesión.")
                binding.edtClave.text?.clear()
            }
        }
    }

    private fun mostrarError(mensaje: String) {
        binding.txtError.text = mensaje
        binding.txtError.visibility = View.VISIBLE
    }
}