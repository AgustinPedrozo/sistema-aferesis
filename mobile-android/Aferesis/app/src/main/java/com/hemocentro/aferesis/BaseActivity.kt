package com.hemocentro.aferesis

import android.content.Intent
import android.os.Bundle
import android.widget.Toast
import androidx.appcompat.app.AppCompatActivity

open class BaseActivity : AppCompatActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Si Android cerró la app en segundo plano, la sesión se pierde: volver al login
        if (ApiClient.token == null) {
            irAlLogin()
        }
    }

    // Llamar después de cada pedido al servidor. Devuelve true si la sesión venció.
    protected fun revisarSesion(): Boolean {
        if (!ApiClient.sesionVencida) return false
        ApiClient.sesionVencida = false
        Toast.makeText(this, "La sesión venció. Ingresá de nuevo.", Toast.LENGTH_LONG).show()
        irAlLogin()
        return true
    }

    private fun irAlLogin() {
        val intent = Intent(this, LoginActivity::class.java)
        intent.flags = Intent.FLAG_ACTIVITY_NEW_TASK or Intent.FLAG_ACTIVITY_CLEAR_TASK
        startActivity(intent)
        finish()
    }
}