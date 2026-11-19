package com.hemocentro.aferesis

import java.text.NumberFormat
import java.time.LocalDate
import java.time.format.DateTimeFormatter
import java.util.Locale
import java.time.LocalDateTime

private val FORMATO_CORTO = DateTimeFormatter.ofPattern("dd/MM/yyyy")

// "2026-10-21" o "2026-10-21 09:30:00" -> "21/10/2026"  (vacío -> "—")
fun fechaCorta(texto: String?): String {
    if (texto.isNullOrBlank()) return "—"
    return try {
        LocalDate.parse(texto.substring(0, 10)).format(FORMATO_CORTO)
    } catch (e: Exception) {
        texto
    }
}

// 70.0 -> "70"   ·   70.5 -> "70.5"
fun numeroCorto(n: Double): String =
    if (n % 1.0 == 0.0) n.toInt().toString() else n.toString()

// 210000 -> "210.000"
fun miles(n: Int): String =
    NumberFormat.getIntegerInstance(Locale("es", "AR")).format(n)

// "2026-10-21 09:30:00" -> "21/10/2026 09:30"  (vacío -> "—")
fun fechaHoraCorta(texto: String?): String {
    if (texto.isNullOrBlank()) return "—"
    return try {
        LocalDateTime.parse(texto.replace(' ', 'T'))
            .format(DateTimeFormatter.ofPattern("dd/MM/yyyy HH:mm"))
    } catch (e: Exception) {
        texto
    }
}