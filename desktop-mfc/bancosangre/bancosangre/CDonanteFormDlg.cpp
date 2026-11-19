#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CDonanteFormDlg.h"

namespace {

	// Valor que se guarda para cada opcion del combo "Sexo" (mismo orden que los textos).
	const char* const kSexoCodigos[] = { "F", "M", "X" };

	// "AAAA-MM-DD" -> "DD/MM/AAAA" (vacio si no tiene ese formato)
	CString fechaParaMostrar(const std::string& iso)
	{
		int y = 0, m = 0, d = 0;
		if (sscanf_s(iso.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return CString();
		CString s;
		s.Format(_T("%02d/%02d/%04d"), d, m, y);
		return s;
	}

	// "DD/MM/AAAA" -> "AAAA-MM-DD". Devuelve false si no es una fecha real o es futura.
	bool fechaParaEnviar(const CString& texto, CString& iso)
	{
		int d = 0, m = 0, y = 0;
		if (_stscanf_s(texto, _T("%d/%d/%d"), &d, &m, &y) != 3) return false;
		if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) return false;

		SYSTEMTIME st = {};
		st.wYear = static_cast<WORD>(y);
		st.wMonth = static_cast<WORD>(m);
		st.wDay = static_cast<WORD>(d);
		FILETIME ft;
		if (!SystemTimeToFileTime(&st, &ft)) return false;   // falla si el dia no existe (ej: 31/02)

		SYSTEMTIME hoy;
		GetLocalTime(&hoy);
		if (y > hoy.wYear || (y == hoy.wYear && (m > hoy.wMonth || (m == hoy.wMonth && d > hoy.wDay)))) return false;

		iso.Format(_T("%04d-%02d-%02d"), y, m, d);
		return true;
	}

	bool telefonoValido(const CString& t)
	{
		if (t.GetLength() < 8 || t.GetLength() > 30) return false;
		for (int i = 0; i < t.GetLength(); ++i) {
			const TCHAR c = t[i];
			const bool permitido = (c >= '0' && c <= '9') || c == ' ' || c == '+' || c == '-' || c == '(' || c == ')';
			if (!permitido) return false;
		}
		return true;
	}

	// Lee un numero con coma o punto decimal. Devuelve false si no es un numero.
	bool leerNumero(CString texto, double& valor)
	{
		texto.Replace(_T(','), _T('.'));
		if (texto.IsEmpty()) return false;
		LPTSTR fin = nullptr;
		valor = _tcstod(texto, &fin);
		return fin != nullptr && *fin == 0;
	}

	// Numero del JSON como texto ("72.5"); vacio si falta o es null.
	CString numeroTexto(const nlohmann::json& d, const char* clave)
	{
		auto it = d.find(clave);
		if (it == d.end() || !it->is_number()) return CString();
		CString s;
		s.Format(_T("%g"), it->get<double>());
		return s;
	}

	// Campo del servidor -> control de la pantalla (para poner el foco donde esta el error)
	UINT campoDe(const std::string& clave)
	{
		if (clave == "dni") return IDC_EDIT_DNI;
		if (clave == "apellido") return IDC_EDIT_APELLIDO;
		if (clave == "nombre") return IDC_EDIT_NOMBRE;
		if (clave == "fecha_nacimiento") return IDC_EDIT_NACIMIENTO;
		if (clave == "sexo") return IDC_COMBO_SEXO;
		if (clave == "domicilio") return IDC_EDIT_DOMICILIO;
		if (clave == "telefono") return IDC_EDIT_TELEFONO;
		if (clave == "email") return IDC_EDIT_EMAIL;
		if (clave == "peso") return IDC_EDIT_PESO;
		if (clave == "altura") return IDC_EDIT_ALTURA;
		return 0;
	}

}  // namespace

IMPLEMENT_DYNAMIC(CDonanteFormDlg, CDialogEx)

CDonanteFormDlg::CDonanteFormDlg(const nlohmann::json* donanteExistente, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DONANTE_FORM, pParent)
{
	if (donanteExistente != nullptr && donanteExistente->is_object()) {
		m_original = *donanteExistente;
		m_idDonante = m_original.value("id_donante", 0);
	}
}

CDonanteFormDlg::~CDonanteFormDlg()
{
}

void CDonanteFormDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_SEXO, m_sexo);
}

BEGIN_MESSAGE_MAP(CDonanteFormDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CDonanteFormDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_sexo.AddString(_T("Femenino"));
	m_sexo.AddString(_T("Masculino"));
	m_sexo.AddString(_T("Otro / X"));

	// Textos de ayuda grises dentro de las cajas vacias
	SendDlgItemMessage(IDC_EDIT_NACIMIENTO, EM_SETCUEBANNER, TRUE, (LPARAM)L"dd/mm/aaaa");
	SendDlgItemMessage(IDC_EDIT_DOMICILIO, EM_SETCUEBANNER, TRUE, (LPARAM)L"opcional");
	SendDlgItemMessage(IDC_EDIT_EMAIL, EM_SETCUEBANNER, TRUE, (LPARAM)L"opcional");
	SendDlgItemMessage(IDC_EDIT_PESO, EM_SETCUEBANNER, TRUE, (LPARAM)L"kg (opcional)");
	SendDlgItemMessage(IDC_EDIT_ALTURA, EM_SETCUEBANNER, TRUE, (LPARAM)L"cm (opcional)");

	if (m_idDonante > 0) {
		SetWindowText(_T("Modificar donante"));
		CargarDatos(m_original);
	}
	else {
		SetWindowText(_T("Nuevo donante"));
	}

	GetDlgItem(IDC_EDIT_DNI)->SetFocus();
	return FALSE;   // FALSE porque el foco se puso a mano
}

void CDonanteFormDlg::CargarDatos(const nlohmann::json& d)
{
	SetDlgItemText(IDC_EDIT_DNI, aTexto(textoDe(d, "dni")));
	SetDlgItemText(IDC_EDIT_APELLIDO, aTexto(textoDe(d, "apellido")));
	SetDlgItemText(IDC_EDIT_NOMBRE, aTexto(textoDe(d, "nombre")));
	SetDlgItemText(IDC_EDIT_NACIMIENTO, fechaParaMostrar(textoDe(d, "fecha_nacimiento")));

	const std::string sexo = textoDe(d, "sexo");
	for (int i = 0; i < 3; ++i) {
		if (sexo == kSexoCodigos[i]) m_sexo.SetCurSel(i);
	}

	SetDlgItemText(IDC_EDIT_DOMICILIO, aTexto(textoDe(d, "domicilio")));
	SetDlgItemText(IDC_EDIT_TELEFONO, aTexto(textoDe(d, "telefono")));
	SetDlgItemText(IDC_EDIT_EMAIL, aTexto(textoDe(d, "email")));
	SetDlgItemText(IDC_EDIT_PESO, numeroTexto(d, "peso"));
	SetDlgItemText(IDC_EDIT_ALTURA, numeroTexto(d, "altura"));
}

CString CDonanteFormDlg::Texto(UINT id) const
{
	CString s;
	GetDlgItemText(id, s);
	s.Trim();
	return s;
}

void CDonanteFormDlg::Error(UINT idControl, const CString& mensaje)
{
	SetDlgItemText(IDC_STATIC_ERROR, mensaje);
	if (idControl != 0) {
		CWnd* control = GetDlgItem(idControl);
		if (control != nullptr) {
			control->SetFocus();
			SendDlgItemMessage(idControl, EM_SETSEL, 0, -1);   // selecciona el texto para corregirlo rapido
		}
	}
}

void CDonanteFormDlg::OnOK()
{
	SetDlgItemText(IDC_STATIC_ERROR, _T(""));

	// ---- 1) Validacion en pantalla (el servidor vuelve a validar todo) ----
	CString dni = Texto(IDC_EDIT_DNI);
	dni.Remove(_T('.'));
	dni.Remove(_T(' '));
	bool dniOk = (dni.GetLength() == 7 || dni.GetLength() == 8);
	for (int i = 0; dniOk && i < dni.GetLength(); ++i) {
		dniOk = (dni[i] >= '0' && dni[i] <= '9');
	}
	if (!dniOk) {
		Error(IDC_EDIT_DNI, _T("El DNI debe tener 7 u 8 numeros."));
		return;
	}

	const CString apellido = Texto(IDC_EDIT_APELLIDO);
	if (apellido.IsEmpty()) {
		Error(IDC_EDIT_APELLIDO, _T("Escriba el apellido."));
		return;
	}

	const CString nombre = Texto(IDC_EDIT_NOMBRE);
	if (nombre.IsEmpty()) {
		Error(IDC_EDIT_NOMBRE, _T("Escriba el nombre."));
		return;
	}

	CString fechaIso;
	if (!fechaParaEnviar(Texto(IDC_EDIT_NACIMIENTO), fechaIso)) {
		Error(IDC_EDIT_NACIMIENTO, _T("Escriba la fecha de nacimiento como dia/mes/anio, por ejemplo 25/03/1985."));
		return;
	}

	const int sexoElegido = m_sexo.GetCurSel();
	if (sexoElegido < 0) {
		Error(IDC_COMBO_SEXO, _T("Elija el sexo."));
		return;
	}

	const CString telefono = Texto(IDC_EDIT_TELEFONO);
	if (!telefonoValido(telefono)) {
		Error(IDC_EDIT_TELEFONO, _T("Escriba un telefono valido, con codigo de area."));
		return;
	}

	const CString email = Texto(IDC_EDIT_EMAIL);
	if (!email.IsEmpty()) {
		const int arroba = email.Find(_T('@'));
		if (arroba < 1 || email.Find(_T('.'), arroba) < 0) {
			Error(IDC_EDIT_EMAIL, _T("El email no parece valido (ejemplo: nombre@correo.com)."));
			return;
		}
	}

	double valor = 0;
	const CString peso = Texto(IDC_EDIT_PESO);
	if (!peso.IsEmpty() && (!leerNumero(peso, valor) || valor < 20 || valor > 300)) {
		Error(IDC_EDIT_PESO, _T("El peso debe ser un numero entre 20 y 300 (en kg)."));
		return;
	}
	const CString altura = Texto(IDC_EDIT_ALTURA);
	if (!altura.IsEmpty() && (!leerNumero(altura, valor) || valor < 100 || valor > 250)) {
		Error(IDC_EDIT_ALTURA, _T("La altura debe ser un numero entre 100 y 250 (en cm)."));
		return;
	}

	// ---- 2) Envio al servidor ----
	nlohmann::json datos = {
		{"dni", aUtf8(dni)},
		{"apellido", aUtf8(apellido)},
		{"nombre", aUtf8(nombre)},
		{"fecha_nacimiento", aUtf8(fechaIso)},
		{"sexo", kSexoCodigos[sexoElegido]},
		{"domicilio", aUtf8(Texto(IDC_EDIT_DOMICILIO))},
		{"telefono", aUtf8(telefono)},
		{"email", aUtf8(email)},
		{"peso", aUtf8(peso)},
		{"altura", aUtf8(altura)}
	};
	if (m_idDonante > 0) {
		datos["id_donante"] = m_idDonante;   // con id: modificacion. Sin id: alta.
	}

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.guardarDonante(datos);

	if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
		m_guardado = r.cuerpo.value("donante", nlohmann::json::object());
		CDialogEx::OnOK();   // guardado: se cierra la ventana
		return;
	}

	// ---- 3) Algo salio mal: se explica y la ventana queda abierta ----
	if (!r.errorRed.empty()) {
		Error(0, _T("No se pudo conectar con el servidor. Avise a un administrador."));
		return;
	}

	CString mensaje = aTexto(r.error());
	UINT campo = 0;
	if (r.cuerpo.is_object()) {
		auto it = r.cuerpo.find("campos");
		if (it != r.cuerpo.end() && it->is_object() && !it->empty()) {
			auto primero = it->begin();
			campo = campoDe(primero.key());
			if (r.codigoHttp == 422 && primero.value().is_string()) {
				mensaje = aTexto(primero.value().get<std::string>());
			}
		}
	}
	Error(campo, mensaje);
}