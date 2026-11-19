#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CDonantesDlg.h"
#include "CTurnoFormDlg.h"

namespace {

	// Mensaje de error listo para mostrar, segun el tipo de falla
	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	// "AAAA-MM-DD" -> fecha del sistema (solo cambia anio, mes y dia). Devuelve false si el texto no es valido.
	bool leerFechaIso(const std::string& iso, SYSTEMTIME& st)
	{
		int y = 0, m = 0, d = 0;
		if (sscanf_s(iso.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return false;
		if (y < 1900 || y > 2100 || m < 1 || m > 12 || d < 1 || d > 31) return false;
		st.wYear = static_cast<WORD>(y);
		st.wMonth = static_cast<WORD>(m);
		st.wDay = static_cast<WORD>(d);
		return true;
	}

	// Numero comparable de una fecha (AAAAMMDD)
	int valorFecha(const SYSTEMTIME& s)
	{
		return s.wYear * 10000 + s.wMonth * 100 + s.wDay;
	}

}  // namespace

IMPLEMENT_DYNAMIC(CTurnoFormDlg, CDialogEx)

CTurnoFormDlg::CTurnoFormDlg(const nlohmann::json* turnoExistente, const std::string& fechaInicial, CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TURNO_FORM, pParent)
	, m_fechaInicial(fechaInicial)
{
	if (turnoExistente != nullptr && turnoExistente->is_object()) {
		m_original = *turnoExistente;
		m_idTurno = m_original.value("id_turno", 0);
	}
}

CTurnoFormDlg::~CTurnoFormDlg()
{
}

void CTurnoFormDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COMBO_TIPO, m_tipo);
	DDX_Control(pDX, IDC_DTP_FECHA_TURNO, m_fecha);
	DDX_Control(pDX, IDC_DTP_HORA_TURNO, m_hora);
}

BEGIN_MESSAGE_MAP(CTurnoFormDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ELEGIR_DONANTE, &CTurnoFormDlg::OnBnClickedBtnElegirDonante)
END_MESSAGE_MAP()

BOOL CTurnoFormDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	m_hora.SetFormat(_T("HH:mm"));   // hora en formato 24 h, sin segundos

	SYSTEMTIME ahora;
	GetLocalTime(&ahora);
	SYSTEMTIME fecha = ahora;
	SYSTEMTIME hora = ahora;
	hora.wHour = 9;       // hora que se propone
	hora.wMinute = 0;
	hora.wSecond = 0;
	hora.wMilliseconds = 0;
	int idTipo = 0;

	if (m_idTurno > 0) {
		// ---- Reprogramar: se cargan los datos del turno y el donante no se puede cambiar ----
		SetWindowText(_T("Reprogramar turno"));
		m_idDonante = m_original.value("id_donante", 0);
		MostrarDonante(m_original);
		GetDlgItem(IDC_BTN_ELEGIR_DONANTE)->EnableWindow(FALSE);

		leerFechaIso(textoDe(m_original, "fecha"), fecha);
		int h = 0, mi = 0;
		if (sscanf_s(textoDe(m_original, "hora").c_str(), "%d:%d", &h, &mi) == 2) {
			hora.wHour = static_cast<WORD>(h);
			hora.wMinute = static_cast<WORD>(mi);
		}
		idTipo = m_original.value("id_tipo_donacion", 0);
		SetDlgItemText(IDC_EDIT_LUGAR, aTexto(textoDe(m_original, "lugar")));
	}
	else {
		// ---- Turno nuevo ----
		SetWindowText(_T("Nuevo turno"));
		SetDlgItemText(IDC_STATIC_DONANTE, _T("(ningun donante elegido)"));
		SetDlgItemText(IDC_EDIT_LUGAR, _T("Hemocentro Regional"));   // propuesta, se puede cambiar
		leerFechaIso(m_fechaInicial, fecha);
		if (valorFecha(fecha) < valorFecha(ahora)) {
			fecha = ahora;   // no se propone un dia que ya paso
		}
	}

	m_fecha.SetTime(&fecha);
	m_hora.SetTime(&hora);
	CargarTipos(idTipo);

	return TRUE;
}

void CTurnoFormDlg::CargarTipos(int idSeleccionado)
{
	m_tipo.ResetContent();

	RespuestaApi r = theApp.m_api.listarTiposDonacion();
	if (!r.ok() || !r.cuerpo.is_object()) {
		Error(mensajeDeError(r));
		return;
	}

	const nlohmann::json tipos = r.cuerpo.value("tipos", nlohmann::json::array());
	int elegido = -1;
	for (const auto& t : tipos) {
		const int id = t.value("id_tipo_donacion", 0);
		const int i = m_tipo.AddString(aTexto(textoDe(t, "nombre")));
		m_tipo.SetItemData(i, static_cast<DWORD_PTR>(id));   // el numero de tipo viaja con cada opcion
		if (id == idSeleccionado) elegido = i;
	}
	if (elegido < 0 && idSeleccionado == 0 && m_tipo.GetCount() > 0) {
		elegido = 0;   // turno nuevo: se propone el primer tipo de la lista
	}
	m_tipo.SetCurSel(elegido);
}

void CTurnoFormDlg::MostrarDonante(const nlohmann::json& d)
{
	CString texto;
	texto.Format(_T("%s, %s  (DNI %s)"),
		(LPCTSTR)aTexto(textoDe(d, "apellido")),
		(LPCTSTR)aTexto(textoDe(d, "nombre")),
		(LPCTSTR)aTexto(textoDe(d, "dni")));
	SetDlgItemText(IDC_STATIC_DONANTE, texto);
}

void CTurnoFormDlg::Error(const CString& mensaje)
{
	SetDlgItemText(IDC_STATIC_ERROR_TURNO, mensaje);
}

void CTurnoFormDlg::OnBnClickedBtnElegirDonante()
{
	CDonantesDlg dlg;
	dlg.SetModoSeleccion(true);   // la pantalla de busqueda, pero para elegir un donante
	if (dlg.DoModal() == IDOK) {
		const nlohmann::json& donante = dlg.Elegido();
		m_idDonante = donante.value("id_donante", 0);
		MostrarDonante(donante);
		Error(CString());   // limpia cualquier aviso anterior
	}
}

void CTurnoFormDlg::OnOK()
{
	Error(CString());

	// ---- 1) Validacion en pantalla (el servidor vuelve a validar todo) ----
	if (m_idDonante <= 0) {
		Error(_T("Elija el donante con el boton Elegir."));
		return;
	}

	const int tipoElegido = m_tipo.GetCurSel();
	if (tipoElegido < 0) {
		Error(_T("Elija el tipo de donacion."));
		return;
	}
	const int idTipo = static_cast<int>(m_tipo.GetItemData(tipoElegido));

	SYSTEMTIME f, h;
	m_fecha.GetTime(&f);
	m_hora.GetTime(&h);
	CString fechaHora;
	fechaHora.Format(_T("%04d-%02d-%02d %02d:%02d"), f.wYear, f.wMonth, f.wDay, h.wHour, h.wMinute);

	CString lugar;
	GetDlgItemText(IDC_EDIT_LUGAR, lugar);
	lugar.Trim();

	// ---- 2) Envio al servidor ----
	nlohmann::json datos = {
		{"id_donante", m_idDonante},
		{"id_tipo_donacion", idTipo},
		{"fecha_hora", aUtf8(fechaHora)},
		{"lugar", aUtf8(lugar)}
	};
	if (m_idTurno > 0) {
		datos["id_turno"] = m_idTurno;   // con id: reprogramacion. Sin id: turno nuevo.
	}

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.guardarTurno(datos);

	if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
		m_guardado = r.cuerpo.value("turno", nlohmann::json::object());
		CDialogEx::OnOK();   // guardado: se cierra la ventana
		return;
	}

	// ---- 3) El servidor lo rechazo (fecha pasada, cupo completo, turno repetido...) ----
	CString mensaje = mensajeDeError(r);
	if (r.codigoHttp == 422 && r.cuerpo.is_object()) {
		auto it = r.cuerpo.find("campos");
		if (it != r.cuerpo.end() && it->is_object() && !it->empty()) {
			auto primero = it->begin();
			if (primero.value().is_string()) {
				mensaje = aTexto(primero.value().get<std::string>());
			}
		}
	}
	Error(mensaje);
	AfxMessageBox(mensaje, MB_ICONWARNING);   // ventana emergente: no se puede pasar por alto
}