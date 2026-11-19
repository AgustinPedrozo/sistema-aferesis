#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CDonantesDlg.h"
#include "CAutorizacionDlg.h"
#include "CExtraccionDlg.h"

namespace {

	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	// "AAAA-MM-DD HH:MM:SS" o "AAAA-MM-DD" -> "DD/MM/AAAA" (vacio si no tiene ese formato)
	CString fechaCorta(const std::string& iso)
	{
		int y = 0, m = 0, d = 0;
		if (sscanf_s(iso.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return CString();
		CString s;
		s.Format(_T("%02d/%02d/%04d"), d, m, y);
		return s;
	}

	// Lista de textos del JSON ("bloqueos", "violaciones", ...) -> "- uno\r\n- dos"
	CString listaConGuiones(const nlohmann::json& lista)
	{
		CString s;
		if (!lista.is_array()) return s;
		for (const auto& x : lista) {
			if (!x.is_string()) continue;
			if (!s.IsEmpty()) s += _T("\r\n");
			s += _T("- ") + aTexto(x.get<std::string>());
		}
		return s;
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

	bool soloDigitos(const CString& s)
	{
		if (s.IsEmpty()) return false;
		for (int i = 0; i < s.GetLength(); ++i) {
			if (s[i] < '0' || s[i] > '9') return false;
		}
		return true;
	}

	void ponerAyuda(CWnd* ventana, UINT id, const CString& texto)
	{
		ventana->SendDlgItemMessage(id, EM_SETCUEBANNER, TRUE, (LPARAM)(LPCTSTR)texto);
	}

	// Fecha/hora de un selector con casilla "ninguno". Devuelve false si la casilla esta destildada.
	bool leerSelector(const CDateTimeCtrl& c, SYSTEMTIME& st)
	{
		return DateTime_GetSystemtime(c.GetSafeHwnd(), &st) == GDT_VALID;
	}

}  // namespace

IMPLEMENT_DYNAMIC(CExtraccionDlg, CDialogEx)

CExtraccionDlg::CExtraccionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_EXTRACCION, pParent)
{
}

CExtraccionDlg::~CExtraccionDlg()
{
}

void CExtraccionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TIPO_EXT, m_tipo);
	DDX_Control(pDX, IDC_COMBO_BOLSA, m_bolsa);
	DDX_Control(pDX, IDC_COMBO_BRAZO, m_brazo);
	DDX_Control(pDX, IDC_DTP_VENC, m_venc);
	DDX_Control(pDX, IDC_DTP_HORA_INI, m_horaIni);
	DDX_Control(pDX, IDC_DTP_HORA_FIN, m_horaFin);
}

BEGIN_MESSAGE_MAP(CExtraccionDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ELEGIR_EXT, &CExtraccionDlg::OnBnClickedBtnElegir)
	ON_CBN_SELCHANGE(IDC_COMBO_TIPO_EXT, &CExtraccionDlg::OnCbnSelchangeTipo)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CExtraccionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Extraccion por afaresis"));

	m_bolsa.AddString(_T("Doble"));
	m_bolsa.AddString(_T("Triple"));
	m_bolsa.AddString(_T("Cuadruple"));
	m_brazo.AddString(_T("Derecho"));
	m_brazo.AddString(_T("Izquierdo"));

	// Vencimiento y horas: selectores con casilla; destildados quedan "sin dato"
	// (en el editor de recursos: Show None = True; las horas con Format = Time)
	DateTime_SetSystemtime(m_venc.GetSafeHwnd(), GDT_NONE, nullptr);
	DateTime_SetSystemtime(m_horaIni.GetSafeHwnd(), GDT_NONE, nullptr);
	DateTime_SetSystemtime(m_horaFin.GetSafeHwnd(), GDT_NONE, nullptr);

	ponerAyuda(this, IDC_EDIT_NRO_DON, _T("10 digitos, ej: 2600100001"));
	ponerAyuda(this, IDC_EDIT_TA, _T("120/80"));
	ponerAyuda(this, IDC_EDIT_HTO, _T("%"));
	ponerAyuda(this, IDC_EDIT_HB, _T("g/dl"));
	ponerAyuda(this, IDC_EDIT_PULSO, _T("lat/min"));
	ponerAyuda(this, IDC_EDIT_TEMP, _T("grados"));
	ponerAyuda(this, IDC_EDIT_PESO_BOLSA, _T("gramos"));

	SetDlgItemText(IDC_EDIT_CANT_PLAQ, _T("0"));
	SetDlgItemText(IDC_EDIT_CANT_PLASMA, _T("0"));
	SetDlgItemText(IDC_STATIC_DONANTE_EXT, _T("(ningun donante elegido)"));
	SetDlgItemText(IDC_STATIC_ESTADO_EXT, _T("Elija al donante y el tipo de donacion para verificar si puede donar hoy."));

	CargarTipos();
	return TRUE;
}

HBRUSH CExtraccionDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_ESTADO_EXT) {
		pDC->SetTextColor(m_colorEstado);   // verde / naranja / rojo segun el resultado de la verificacion
	}
	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_ERROR_EXT) {
		pDC->SetTextColor(RGB(192, 0, 0));
	}
	return hbr;
}

CString CExtraccionDlg::Texto(UINT id) const
{
	CString s;
	GetDlgItemText(id, s);
	s.Trim();
	return s;
}

void CExtraccionDlg::Error(const CString& mensaje, UINT idControl)
{
	SetDlgItemText(IDC_STATIC_ERROR_EXT, mensaje);
	if (idControl != 0) {
		CWnd* control = GetDlgItem(idControl);
		if (control != nullptr) {
			control->SetFocus();
			SendDlgItemMessage(idControl, EM_SETSEL, 0, -1);
		}
	}
}

void CExtraccionDlg::CargarTipos()
{
	m_tipo.ResetContent();
	RespuestaApi r = theApp.m_api.listarTiposDonacion();
	if (!r.ok() || !r.cuerpo.is_object()) {
		Error(mensajeDeError(r));
		return;
	}
	const nlohmann::json tipos = r.cuerpo.value("tipos", nlohmann::json::array());
	for (const auto& t : tipos) {
		const int i = m_tipo.AddString(aTexto(textoDe(t, "nombre")));
		m_tipo.SetItemData(i, static_cast<DWORD_PTR>(t.value("id_tipo_donacion", 0)));
	}
	if (m_tipo.GetCount() > 0) m_tipo.SetCurSel(0);   // se propone el primero (Plaquetas)
}

int CExtraccionDlg::TipoElegido() const
{
	const int i = m_tipo.GetCurSel();
	return i < 0 ? 0 : static_cast<int>(m_tipo.GetItemData(i));
}

CString CExtraccionDlg::NombreTipoElegido() const
{
	CString s;
	const int i = m_tipo.GetCurSel();
	if (i >= 0) m_tipo.GetLBText(i, s);
	return s;
}

void CExtraccionDlg::OnBnClickedBtnElegir()
{
	CDonantesDlg dlg;
	dlg.SetModoSeleccion(true);
	if (dlg.DoModal() == IDOK) {
		CargarDonante(dlg.Elegido());
	}
}

void CExtraccionDlg::CargarDonante(const nlohmann::json& d)
{
	m_idDonante = d.value("id_donante", 0);
	CString texto;
	texto.Format(_T("%s, %s  (DNI %s)"),
		(LPCTSTR)aTexto(textoDe(d, "apellido")),
		(LPCTSTR)aTexto(textoDe(d, "nombre")),
		(LPCTSTR)aTexto(textoDe(d, "dni")));
	SetDlgItemText(IDC_STATIC_DONANTE_EXT, texto);
	Error(CString());
	Verificar();
}

void CExtraccionDlg::OnCbnSelchangeTipo()
{
	Verificar();   // los limites dependen del tipo de donacion
}

void CExtraccionDlg::Verificar()
{
	if (m_idDonante <= 0 || TipoElegido() <= 0) return;

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.verificarDonacion(m_idDonante, TipoElegido());
	if (!r.ok() || !r.cuerpo.is_object()) {
		m_colorEstado = RGB(192, 0, 0);
		SetDlgItemText(IDC_STATIC_ESTADO_EXT, mensajeDeError(r));
		return;
	}
	const nlohmann::json& c = r.cuerpo;
	const nlohmann::json bloqueos = c.value("bloqueos", nlohmann::json::array());
	const nlohmann::json violaciones = c.value("violaciones", nlohmann::json::array());
	const nlohmann::json avisos = c.value("avisos", nlohmann::json::array());
	const nlohmann::json datos = c.value("datos", nlohmann::json::object());

	CString texto;
	if (!bloqueos.empty()) {
		m_colorEstado = RGB(192, 0, 0);
		texto = _T("NO PUEDE DONAR HOY:\r\n") + listaConGuiones(bloqueos);
	}
	else if (!violaciones.empty()) {
		m_colorEstado = RGB(200, 110, 0);
		texto = _T("SUPERA LOS LIMITES (se necesita la autorizacion de la Jefa de Area/Gerente):\r\n")
			+ listaConGuiones(violaciones);
	}
	else {
		m_colorEstado = RGB(0, 130, 0);
		texto = _T("APTO PARA DONAR. Puede cargar la planilla.");
	}
	if (!avisos.empty()) {
		texto += _T("\r\nAviso:\r\n") + listaConGuiones(avisos);
	}

	CString resumen;
	resumen.Format(_T("\r\nEsta semana: %d de %d.  Este anio: %d de %d."),
		datos.value("donaciones_semana", 0), datos.value("limite_semanal", 0),
		datos.value("donaciones_anio", 0), datos.value("limite_anual", 0));
	texto += resumen;
	const CString ultima = fechaCorta(textoDe(datos, "ultima_donacion"));
	if (!ultima.IsEmpty()) texto += _T("  Ultima donacion: ") + ultima + _T(".");

	SetDlgItemText(IDC_STATIC_ESTADO_EXT, texto);
	Invalidate();   // para que se repinte con el color nuevo
}

nlohmann::json CExtraccionDlg::ArmarDatos() const
{
	nlohmann::json detalle = {
		{"hto", aUtf8(Texto(IDC_EDIT_HTO))},
		{"hb", aUtf8(Texto(IDC_EDIT_HB))},
		{"ta", aUtf8(Texto(IDC_EDIT_TA))},
		{"pulso", aUtf8(Texto(IDC_EDIT_PULSO))},
		{"temperatura", aUtf8(Texto(IDC_EDIT_TEMP))},
		{"marca_bolsa", aUtf8(Texto(IDC_EDIT_MARCA))},
		{"lote_bolsa", aUtf8(Texto(IDC_EDIT_LOTE))},
		{"nro_tubuladura", aUtf8(Texto(IDC_EDIT_TUBULADURA))},
		{"anticoagulante", aUtf8(Texto(IDC_EDIT_ANTICOAG))},
		{"peso_bolsa", aUtf8(Texto(IDC_EDIT_PESO_BOLSA))},
		{"dificultades", IsDlgButtonChecked(IDC_CHK_DIFIC) == BST_CHECKED}
	};

	CString s;
	if (m_bolsa.GetCurSel() >= 0) { m_bolsa.GetLBText(m_bolsa.GetCurSel(), s); detalle["tipo_bolsa"] = aUtf8(s); }
	if (m_brazo.GetCurSel() >= 0) { m_brazo.GetLBText(m_brazo.GetCurSel(), s); detalle["brazo"] = aUtf8(s); }

	SYSTEMTIME st;
	if (leerSelector(m_venc, st)) {
		s.Format(_T("%04d-%02d-%02d"), st.wYear, st.wMonth, st.wDay);
		detalle["vencimiento_bolsa"] = aUtf8(s);
	}
	if (leerSelector(m_horaIni, st)) {
		s.Format(_T("%02d:%02d"), st.wHour, st.wMinute);
		detalle["hora_inicio"] = aUtf8(s);
	}
	if (leerSelector(m_horaFin, st)) {
		s.Format(_T("%02d:%02d"), st.wHour, st.wMinute);
		detalle["hora_fin"] = aUtf8(s);
	}

	return {
		{"id_donante", m_idDonante},
		{"id_tipo_donacion", TipoElegido()},
		{"numero_donacion", aUtf8(Texto(IDC_EDIT_NRO_DON))},
		{"cantidad_plaquetas", aUtf8(Texto(IDC_EDIT_CANT_PLAQ))},
		{"cantidad_plasma", aUtf8(Texto(IDC_EDIT_CANT_PLASMA))},
		{"incidencias", aUtf8(Texto(IDC_EDIT_INCIDENCIAS))},
		{"detalle", detalle}
	};
}

void CExtraccionDlg::OnOK()
{
	Error(CString());

	// ---- 1) Validacion en pantalla (el servidor vuelve a validar todo) ----
	if (m_idDonante <= 0) {
		Error(_T("Elija el donante con el boton Elegir."), IDC_BTN_ELEGIR_EXT);
		return;
	}
	if (TipoElegido() <= 0) {
		Error(_T("Elija el tipo de donacion."), IDC_COMBO_TIPO_EXT);
		return;
	}
	const CString numero = Texto(IDC_EDIT_NRO_DON);
	if (numero.GetLength() != 10 || !soloDigitos(numero)) {
		Error(_T("El numero de donacion debe tener 10 digitos (ej: 2600100001)."), IDC_EDIT_NRO_DON);
		return;
	}

	const CString plaq = Texto(IDC_EDIT_CANT_PLAQ);
	const CString plas = Texto(IDC_EDIT_CANT_PLASMA);
	if (!soloDigitos(plaq)) { Error(_T("Cantidad de plaquetas: escriba un numero entero (0 si no hubo)."), IDC_EDIT_CANT_PLAQ); return; }
	if (!soloDigitos(plas)) { Error(_T("Cantidad de plasma: escriba un numero entero (0 si no hubo)."), IDC_EDIT_CANT_PLASMA); return; }
	const int cp = _ttoi(plaq);
	const int cs = _ttoi(plas);
	const CString tipo = NombreTipoElegido();
	if (tipo == _T("Plaquetas") && (cp < 1 || cs != 0)) {
		Error(_T("En una donacion de Plaquetas: al menos 1 unidad de plaquetas y 0 de plasma."), IDC_EDIT_CANT_PLAQ);
		return;
	}
	if (tipo == _T("Plasma") && (cs < 1 || cp != 0)) {
		Error(_T("En una donacion de Plasma: al menos 1 unidad de plasma y 0 de plaquetas."), IDC_EDIT_CANT_PLASMA);
		return;
	}
	if (tipo == _T("Ambos") && (cp < 1 || cs < 1)) {
		Error(_T("En una donacion de Ambos: al menos 1 unidad de plaquetas y 1 de plasma."), IDC_EDIT_CANT_PLAQ);
		return;
	}

	// Controles suaves de la planilla (solo evitan errores de tipeo; no se juzgan los valores clinicos)
	double v = 0;
	const struct { UINT id; const TCHAR* texto; } numericos[] = {
		{ IDC_EDIT_HTO, _T("El hematocrito (Hto)") }, { IDC_EDIT_HB, _T("La hemoglobina (Hb)") },
		{ IDC_EDIT_TEMP, _T("La temperatura") }, { IDC_EDIT_PESO_BOLSA, _T("El peso de la bolsa") }
	};
	for (const auto& n : numericos) {
		const CString t = Texto(n.id);
		if (!t.IsEmpty() && !leerNumero(t, v)) {
			Error(CString(n.texto) + _T(" debe ser un numero."), n.id);
			return;
		}
	}
	const CString pulso = Texto(IDC_EDIT_PULSO);
	if (!pulso.IsEmpty() && !soloDigitos(pulso)) {
		Error(_T("El pulso debe ser un numero entero."), IDC_EDIT_PULSO);
		return;
	}
	const CString ta = Texto(IDC_EDIT_TA);
	if (!ta.IsEmpty()) {
		int a = 0, b = 0;
		TCHAR resto = 0;
		if (_stscanf_s(ta, _T("%d/%d%c"), &a, &b, &resto, 1) != 2) {
			Error(_T("La tension arterial se escribe asi: 120/80."), IDC_EDIT_TA);
			return;
		}
	}

	// ---- 2) Envio. Si el servidor dice que supera los limites, se pide la autorizacion y se reintenta ----
	const bool esJefa = (theApp.m_api.rol() == "Jefa de \xC3\x81rea/Gerente" || theApp.m_api.rol() == "Administrador");
	nlohmann::json datos = ArmarDatos();
	CString errorAutorizacion;

	for (;;) {
		CWaitCursor espera;
		RespuestaApi r = theApp.m_api.guardarDonacion(datos);

		// Guardada
		if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
			const nlohmann::json don = r.cuerpo.value("donacion", nlohmann::json::object());
			CString resumen = aTexto(r.cuerpo.value("mensaje", std::string()));
			resumen += _T("\r\nNumero de donacion: ") + aTexto(textoDe(don, "numero_donacion"));
			const nlohmann::json avisos = r.cuerpo.value("avisos", nlohmann::json::array());
			if (!avisos.empty()) resumen += _T("\r\n\r\nAviso:\r\n") + listaConGuiones(avisos);
			MessageBeep(MB_OK);   // confirmacion sonora
			AfxMessageBox(resumen, MB_ICONINFORMATION);
			CDialogEx::OnOK();
			return;
		}

		// Supera los limites: hace falta la autorizacion de la Jefa de Area/Gerente
		if (r.codigoHttp == 409 && r.cuerpo.is_object() && r.cuerpo.value("requiere_excepcion", false)) {
			CAutorizacionDlg aut;
			aut.m_violaciones = _T("Esta extraccion supera los limites:\r\n")
				+ listaConGuiones(r.cuerpo.value("violaciones", nlohmann::json::array()));
			aut.m_pedirCredenciales = !esJefa;
			aut.m_errorPrevio = errorAutorizacion;
			if (aut.DoModal() != IDOK) {
				Error(_T("Extraccion no guardada: falta la autorizacion."));
				return;
			}
			datos["excepcion"] = {
				{"motivo", aUtf8(aut.m_motivo)},
				{"usuario", aUtf8(aut.m_usuario)},
				{"clave", aUtf8(aut.m_clave)}
			};
			errorAutorizacion.Empty();
			continue;
		}

		// Autorizacion rechazada (clave mal, usuario sin permiso): se vuelve a pedir
		if (r.codigoHttp == 403 && datos.contains("excepcion")) {
			errorAutorizacion = mensajeDeError(r);
			datos.erase("excepcion");
			CAutorizacionDlg aut;
			aut.m_violaciones = _T("Vuelva a intentar la autorizacion.");
			aut.m_pedirCredenciales = !esJefa;
			aut.m_errorPrevio = errorAutorizacion;
			if (aut.DoModal() != IDOK) {
				Error(_T("Extraccion no guardada: falta la autorizacion."));
				return;
			}
			datos["excepcion"] = {
				{"motivo", aUtf8(aut.m_motivo)},
				{"usuario", aUtf8(aut.m_usuario)},
				{"clave", aUtf8(aut.m_clave)}
			};
			continue;
		}

		// Cualquier otro rechazo: se explica y la ventana queda abierta para corregir
		CString mensaje = mensajeDeError(r);
		if (r.codigoHttp == 422 && r.cuerpo.is_object()) {
			auto it = r.cuerpo.find("campos");
			if (it != r.cuerpo.end() && it->is_object() && !it->empty() && it->begin().value().is_string()) {
				mensaje = aTexto(it->begin().value().get<std::string>());
			}
		}
		Error(mensaje);
		AfxMessageBox(mensaje, MB_ICONWARNING);
		Verificar();   // el estado pudo cambiar
		return;
	}
}