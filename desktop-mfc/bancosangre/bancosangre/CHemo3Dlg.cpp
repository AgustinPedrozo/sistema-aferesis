#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CDonantesDlg.h"
#include "CHemo3Dlg.h"

namespace {

	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	// "AAAA-MM-DD HH:MM:SS" -> "DD/MM/AAAA HH:MM" (vacio si no tiene ese formato)
	CString fechaHora(const std::string& iso)
	{
		int y = 0, mo = 0, d = 0, h = 0, mi = 0;
		if (sscanf_s(iso.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) != 5) return CString();
		CString s;
		s.Format(_T("%02d/%02d/%04d %02d:%02d"), d, mo, y, h, mi);
		return s;
	}

	// Los identificadores de cada pregunta, en orden
	const UINT kTextos[3] = { IDC_STATIC_P1, IDC_STATIC_P2, IDC_STATIC_P3 };
	const UINT kSi[3] = { IDC_RADIO_P1_SI, IDC_RADIO_P2_SI, IDC_RADIO_P3_SI };
	const UINT kNo[3] = { IDC_RADIO_P1_NO, IDC_RADIO_P2_NO, IDC_RADIO_P3_NO };

}  // namespace

IMPLEMENT_DYNAMIC(CHemo3Dlg, CDialogEx)

CHemo3Dlg::CHemo3Dlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_HEMO3, pParent)
{
}

CHemo3Dlg::~CHemo3Dlg()
{
}

void CHemo3Dlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CHemo3Dlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ELEGIR_H3, &CHemo3Dlg::OnBnClickedBtnElegir)
	ON_BN_CLICKED(IDC_BTN_VER_H3, &CHemo3Dlg::OnBnClickedBtnVer)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CHemo3Dlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Formulario confidencial HEMO 3"));

	SetDlgItemText(IDC_STATIC_DONANTE_H3, _T("(ningun donante elegido)"));
	SetDlgItemText(IDC_STATIC_AVISO_H3, _T(""));

	// Solo quienes pueden leer los formularios ya cargados ven el boton "Ver anteriores"
	const std::string rol = theApp.m_api.rol();
	const bool puedeVer = (rol == "Doctora" || rol == "Jefa de \xC3\x81rea/Gerente" || rol == "Administrador");
	GetDlgItem(IDC_BTN_VER_H3)->EnableWindow(puedeVer);

	CargarPreguntas();
	return TRUE;
}

HBRUSH CHemo3Dlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_ERROR_H3) {
		pDC->SetTextColor(RGB(192, 0, 0));
	}
	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_AVISO_H3) {
		pDC->SetTextColor(RGB(200, 110, 0));
	}
	return hbr;
}

void CHemo3Dlg::Error(const CString& mensaje, UINT idControl)
{
	SetDlgItemText(IDC_STATIC_ERROR_H3, mensaje);
	if (idControl != 0) {
		CWnd* control = GetDlgItem(idControl);
		if (control != nullptr) control->SetFocus();
	}
}

void CHemo3Dlg::CargarPreguntas()
{
	RespuestaApi r = theApp.m_api.obtenerCamposHemo3(0);
	if (!r.ok() || !r.cuerpo.is_object()) {
		Error(mensajeDeError(r));
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		return;
	}
	m_preguntas = r.cuerpo.value("preguntas", nlohmann::json::array());

	// Esta pantalla tiene lugar para 3 preguntas; si el formulario cambia de largo hay que agregar controles
	if (m_preguntas.size() != 3) {
		Error(_T("El formulario del servidor no tiene 3 preguntas: esta pantalla debe actualizarse."));
		GetDlgItem(IDOK)->EnableWindow(FALSE);
		return;
	}
	for (int i = 0; i < 3; ++i) {
		CString texto;
		texto.Format(_T("%d. %s"), i + 1, (LPCTSTR)aTexto(textoDe(m_preguntas[i], "texto")));
		SetDlgItemText(kTextos[i], texto);
	}
}

void CHemo3Dlg::OnBnClickedBtnElegir()
{
	CDonantesDlg dlg;
	dlg.SetModoSeleccion(true);
	if (dlg.DoModal() == IDOK) {
		CargarDonante(dlg.Elegido());
	}
}

void CHemo3Dlg::CargarDonante(const nlohmann::json& d)
{
	m_idDonante = d.value("id_donante", 0);

	CString texto;
	texto.Format(_T("%s, %s  (DNI %s)"),
		(LPCTSTR)aTexto(textoDe(d, "apellido")),
		(LPCTSTR)aTexto(textoDe(d, "nombre")),
		(LPCTSTR)aTexto(textoDe(d, "dni")));
	SetDlgItemText(IDC_STATIC_DONANTE_H3, texto);

	// Formulario nuevo: todas las preguntas sin responder
	for (int i = 0; i < 3; ++i) {
		CheckDlgButton(kSi[i], BST_UNCHECKED);
		CheckDlgButton(kNo[i], BST_UNCHECKED);
	}
	SetDlgItemText(IDC_EDIT_OBS_H3, _T(""));
	Error(CString());

	// Aviso si ya se cargo uno hoy (no se muestra su contenido: es confidencial)
	m_existeHoy = false;
	RespuestaApi r = theApp.m_api.obtenerCamposHemo3(m_idDonante);
	if (r.ok() && r.cuerpo.is_object()) {
		m_existeHoy = r.cuerpo.value("existe_hoy", false);
	}
	SetDlgItemText(IDC_STATIC_AVISO_H3, m_existeHoy ? _T("Este donante ya tiene un formulario HEMO 3 cargado hoy.") : _T(""));
}

void CHemo3Dlg::OnBnClickedBtnVer()
{
	if (m_idDonante <= 0) {
		Error(_T("Primero elija al donante."), IDC_BTN_ELEGIR_H3);
		return;
	}
	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.listarHemo3(m_idDonante);
	if (!r.ok() || !r.cuerpo.is_object()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
		return;
	}
	const nlohmann::json formularios = r.cuerpo.value("formularios", nlohmann::json::array());
	if (formularios.empty()) {
		AfxMessageBox(_T("Este donante no tiene formularios HEMO 3 cargados."), MB_ICONINFORMATION);
		return;
	}

	CString texto;
	int cuantos = 0;
	for (const auto& f : formularios) {
		if (++cuantos > 5) break;   // en pantalla alcanza con los ultimos 5
		texto += fechaHora(textoDe(f, "fecha_registro")) + _T("  -  cargado por ") + aTexto(textoDe(f, "registrado_por")) + _T("\r\n");
		const nlohmann::json respuestas = f.value("respuestas", nlohmann::json::array());
		int n = 0;
		for (const auto& p : respuestas) {
			CString linea;
			linea.Format(_T("   %d. %s\r\n"), ++n, (LPCTSTR)aTexto(textoDe(p, "respuesta")));
			texto += linea;
		}
		const std::string obs = textoDe(f, "observaciones");
		if (!obs.empty()) texto += _T("   Observaciones: ") + aTexto(obs) + _T("\r\n");
		texto += _T("\r\n");
	}
	texto += _T("(Las respuestas siguen el orden de las preguntas de la pantalla.)");
	AfxMessageBox(texto, MB_ICONINFORMATION);
}

void CHemo3Dlg::OnOK()
{
	Error(CString());

	if (m_idDonante <= 0) {
		Error(_T("Elija el donante con el boton Elegir."), IDC_BTN_ELEGIR_H3);
		return;
	}

	// Las tres preguntas tienen que tener una respuesta
	nlohmann::json respuestas = nlohmann::json::object();
	for (int i = 0; i < 3; ++i) {
		const bool si = (IsDlgButtonChecked(kSi[i]) == BST_CHECKED);
		const bool no = (IsDlgButtonChecked(kNo[i]) == BST_CHECKED);
		if (!si && !no) {
			CString mensaje;
			mensaje.Format(_T("Falta responder Si o No en la pregunta %d."), i + 1);
			Error(mensaje, kSi[i]);
			return;
		}
		respuestas[textoDe(m_preguntas[i], "clave")] = si;
	}

	if (m_existeHoy) {
		if (AfxMessageBox(_T("Este donante ya tiene un formulario cargado hoy. Guardar otro igualmente?"), MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
	}

	CString obs;
	GetDlgItemText(IDC_EDIT_OBS_H3, obs);
	obs.Trim();

	nlohmann::json datos = {
		{"id_donante", m_idDonante},
		{"respuestas", respuestas},
		{"observaciones", aUtf8(obs)}
	};

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.guardarHemo3(datos);
	if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
		MessageBeep(MB_OK);   // confirmacion sonora
		AfxMessageBox(aTexto(r.cuerpo.value("mensaje", std::string())), MB_ICONINFORMATION);
		CDialogEx::OnOK();
		return;
	}

	CString mensaje = mensajeDeError(r);
	if (r.codigoHttp == 422 && r.cuerpo.is_object()) {
		auto it = r.cuerpo.find("campos");
		if (it != r.cuerpo.end() && it->is_object() && !it->empty() && it->begin().value().is_string()) {
			mensaje = aTexto(it->begin().value().get<std::string>());
		}
	}
	Error(mensaje);
	AfxMessageBox(mensaje, MB_ICONWARNING);
}