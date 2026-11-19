#pragma once
// Funciones de ayuda para convertir textos y leer JSON.
// Incluir DESPUES de "pch.h" (usa CString de MFC).
#include <string>
#include <nlohmann/json.hpp>

// Texto UTF-8 (el que viene de PHP) -> CString
inline CString aTexto(const std::string& utf8)
{
	return CString(CA2W(utf8.c_str(), CP_UTF8));
}

// CString -> texto UTF-8 (el que espera PHP)
inline std::string aUtf8(const CString& texto)
{
	return std::string(CT2A(texto, CP_UTF8));
}

// Lee un texto del JSON. Devuelve "" si la clave no existe o vale null.
inline std::string textoDe(const nlohmann::json& d, const char* clave)
{
	auto it = d.find(clave);
	if (it == d.end() || !it->is_string()) return std::string();
	return it->get<std::string>();
}
