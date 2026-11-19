#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CMensajesDlg.h"

namespace {

	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	// "AAAA-MM-DD HH:MM:SS" -> "DD/MM HH:MM" (vacio si no tiene ese formato)
	CString fechaHoraCorta(const std::string& iso)
	{
		int y = 0, mo = 0, d = 0, h = 0, mi = 0;
		if (sscanf_s(iso.c_str(), "%d-%d-%d %d:%d", &y, &mo, &d, &h, &mi) != 5) return CString();
		CString s;
		s.Format(_T("%02d/%02d %02d:%02d"), d, mo, h, mi);
		return s;
	}

	// Respuestas de ejemplo para la demostracion (modo simulado)
	const TCHAR* const kRespuestas[] = {
		_T("Confirmo que voy a asistir"),
		_T("No puedo asistir al turno"),
		_T("Tengo gripe y me siento mal"),
		_T("Quiero cambiar el dia del turno"),
		_T("Tome un antibiotico hace 2 dias, puedo donar?")
	};

}  // namespace

IMPLEMENT_DYNAMIC(CMensajesDlg, CDialogEx)

CMensajesDlg::CMensajesDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_MENSAJES, pParent)
{
}

CMensajesDlg::~CMensajesDlg()
{
}

void CMensajesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_MENSAJES, m_lista);
}

BEGIN_MESSAGE_MAP(CMensajesDlg, CDialogEx)
	ON_BN_CLICKED(IDC_CHK_TODOS_MSJ, &CMensajesDlg::OnBnClickedChkTodos)
	ON_BN_CLICKED(IDC_BTN_ATENDER, &CMensajesDlg::OnBnClickedBtnAtender)
	ON_BN_CLICKED(IDC_BTN_ENVIAR_AHORA, &CMensajesDlg::OnBnClickedBtnEnviarAhora)
	ON_BN_CLICKED(IDC_BTN_SIMULAR, &CMensajesDlg::OnBnClickedBtnSimular)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_MENSAJES, &CMensajesDlg::OnLvnItemchangedLista)
END_MESSAGE_MAP()

BOOL CMensajesDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Mensajes de WhatsApp"));

	m_lista.SetExtendedStyle(m_lista.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lista.InsertColumn(0, _T("Donante"), LVCFMT_LEFT, 140);
	m_lista.InsertColumn(1, _T("Turno"), LVCFMT_LEFT, 90);
	m_lista.InsertColumn(2, _T("Estado"), LVCFMT_LEFT, 95);
	m_lista.InsertColumn(3, _T("Respuesta"), LVCFMT_LEFT, 230);
	m_lista.InsertColumn(4, _T("Recibida"), LVCFMT_LEFT, 90);

	Cargar();
	return TRUE;
}

bool CMensajesDlg::EsPendiente(const nlohmann::json& m) const
{
	return !textoDe(m, "respuesta_recibida").empty() && !m.value("atendido", false);
}

void CMensajesDlg::Cargar()
{
	const bool todos = (IsDlgButtonChecked(IDC_CHK_TODOS_MSJ) == BST_CHECKED);

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.listarMensajes(todos);
	if (!r.ok() || !r.cuerpo.is_object()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
		return;
	}

	m_mensajes = r.cuerpo.value("mensajes", nlohmann::json::array());
	const int pendientes = r.cuerpo.value("total_pendientes", 0);

	m_lista.DeleteAllItems();
	int fila = 0;
	for (const auto& m : m_mensajes) {
		const std::string estadoEnvio = textoDe(m, "estado_envio");
		const std::string respuesta = textoDe(m, "respuesta_recibida");

		CString estado;
		if (estadoEnvio == "Error")           estado = _T("Error de envio");
		else if (respuesta.empty())           estado = _T("Sin respuesta");
		else if (m.value("atendido", false))  estado = _T("Atendida");
		else                                  estado = _T("PENDIENTE");

		CString donante = aTexto(textoDe(m, "apellido")) + _T(", ") + aTexto(textoDe(m, "nombre"));
		CString textoResp = aTexto(respuesta);
		textoResp.Replace(_T("\n"), _T(" / "));

		const int i = m_lista.InsertItem(fila, donante);
		m_lista.SetItemText(i, 1, fechaHoraCorta(textoDe(m, "fecha_hora_turno")));
		m_lista.SetItemText(i, 2, estado);
		m_lista.SetItemText(i, 3, textoResp);
		m_lista.SetItemText(i, 4, fechaHoraCorta(textoDe(m, "fecha_respuesta")));
		m_lista.SetItemData(i, static_cast<DWORD_PTR>(fila));   // posicion dentro de m_mensajes
		++fila;
	}

	CString resumen;
	if (pendientes == 0)
		resumen = _T("No hay respuestas pendientes de atender.");
	else if (pendientes == 1)
		resumen = _T("1 respuesta pendiente de atender.");
	else
		resumen.Format(_T("%d respuestas pendientes de atender."), pendientes);
	if (todos && fila == 0) resumen += _T("  (Todavia no se envio ningun mensaje.)");
	SetDlgItemText(IDC_STATIC_MSJ_RESUMEN, resumen);

	SetDlgItemText(IDC_EDIT_MSJ_DETALLE, _T(""));
	ActualizarBotones();
}

int CMensajesDlg::IndiceElegido() const
{
	POSITION pos = m_lista.GetFirstSelectedItemPosition();
	if (pos == nullptr) return -1;
	const int fila = m_lista.GetNextSelectedItem(pos);
	const size_t indice = static_cast<size_t>(m_lista.GetItemData(fila));
	return indice < m_mensajes.size() ? static_cast<int>(indice) : -1;
}

void CMensajesDlg::ActualizarBotones()
{
	const int i = IndiceElegido();
	const bool hay = (i >= 0);
	const nlohmann::json vacio = nlohmann::json::object();
	const nlohmann::json& m = hay ? m_mensajes[i] : vacio;

	GetDlgItem(IDC_BTN_ATENDER)->EnableWindow(hay && EsPendiente(m));
	// responder "como si fuera el donante" solo tiene sentido en modo simulado y con un mensaje ya enviado
	GetDlgItem(IDC_BTN_SIMULAR)->EnableWindow(hay && textoDe(m, "estado_envio") == "Enviado" && textoDe(m, "modo_envio") == "simulado");
}

void CMensajesDlg::MostrarDetalle()
{
	const int i = IndiceElegido();
	if (i < 0) {
		SetDlgItemText(IDC_EDIT_MSJ_DETALLE, _T(""));
		return;
	}
	const nlohmann::json& m = m_mensajes[i];

	CString texto;
	texto.Format(_T("Mensaje enviado el %s (%s):\r\n%s"),
		(LPCTSTR)fechaHoraCorta(textoDe(m, "fecha_envio")),
		(LPCTSTR)aTexto(textoDe(m, "modo_envio")),
		(LPCTSTR)aTexto(textoDe(m, "mensaje_enviado")));

	if (textoDe(m, "estado_envio") == "Error") {
		texto += _T("\r\n\r\nNo se pudo enviar: ") + aTexto(textoDe(m, "detalle_envio"));
	}
	const std::string respuesta = textoDe(m, "respuesta_recibida");
	if (!respuesta.empty()) {
		CString r = aTexto(respuesta);
		r.Replace(_T("\n"), _T("\r\n"));
		texto += _T("\r\n\r\nRespuesta recibida el ") + fechaHoraCorta(textoDe(m, "fecha_respuesta")) + _T(":\r\n") + r;
		if (m.value("atendido", false)) {
			texto += _T("\r\n\r\nAtendida por ") + aTexto(textoDe(m, "atendido_por"));
		}
	}
	SetDlgItemText(IDC_EDIT_MSJ_DETALLE, texto);
}

void CMensajesDlg::OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW info = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((info->uChanged & LVIF_STATE) != 0) {
		MostrarDetalle();
		ActualizarBotones();
	}
	*pResult = 0;
}

void CMensajesDlg::OnBnClickedChkTodos()
{
	Cargar();
}

void CMensajesDlg::OnBnClickedBtnAtender()
{
	const int i = IndiceElegido();
	if (i < 0 || !EsPendiente(m_mensajes[i])) return;

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.atenderMensaje(m_mensajes[i].value("id_notificacion", 0));
	if (!r.ok()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
	}
	else {
		MessageBeep(MB_OK);   // confirmacion sonora
	}
	Cargar();
}

void CMensajesDlg::OnBnClickedBtnEnviarAhora()
{
	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.enviarRecordatoriosAhora();
	if (!r.ok() || !r.cuerpo.is_object()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
		return;
	}
	CString resumen = aTexto(r.cuerpo.value("mensaje", std::string()));
	const nlohmann::json detalle = r.cuerpo.value("detalle", nlohmann::json::array());
	for (const auto& d : detalle) {
		resumen += _T("\r\n- ") + aTexto(textoDe(d, "donante")) + _T(": ") + aTexto(textoDe(d, "estado"));
		if (textoDe(d, "estado") == "Error") resumen += _T(" (") + aTexto(textoDe(d, "detalle")) + _T(")");
	}
	if (r.cuerpo.value("con_error", 0) == 0) MessageBeep(MB_OK);
	AfxMessageBox(resumen, r.cuerpo.value("con_error", 0) == 0 ? MB_ICONINFORMATION : MB_ICONWARNING);

	CheckDlgButton(IDC_CHK_TODOS_MSJ, BST_CHECKED);   // se muestran todos para ver los recien enviados
	Cargar();
}

void CMensajesDlg::OnBnClickedBtnSimular()
{
	const int i = IndiceElegido();
	if (i < 0) return;

	// Menu con respuestas de ejemplo: asi la demostracion no necesita escribir nada
	CMenu menu;
	menu.CreatePopupMenu();
	const int cantidad = static_cast<int>(sizeof(kRespuestas) / sizeof(kRespuestas[0]));
	for (int k = 0; k < cantidad; ++k) {
		menu.AppendMenu(MF_STRING, 1000 + k, kRespuestas[k]);
	}
	CRect boton;
	GetDlgItem(IDC_BTN_SIMULAR)->GetWindowRect(&boton);
	const int elegido = static_cast<int>(menu.TrackPopupMenu(TPM_RETURNCMD | TPM_LEFTALIGN | TPM_TOPALIGN, boton.left, boton.bottom, this));
	if (elegido < 1000 || elegido >= 1000 + cantidad) return;

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.simularRespuesta(m_mensajes[i].value("id_notificacion", 0), aUtf8(CString(kRespuestas[elegido - 1000])));
	if (!r.ok()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
		return;
	}
	Cargar();
}