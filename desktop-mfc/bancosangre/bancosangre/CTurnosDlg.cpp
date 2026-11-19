#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CTurnoFormDlg.h"
#include "CTurnosDlg.h"

namespace {

	// "AAAA-MM-DD" a partir de una fecha del sistema
	CString fechaIso(const SYSTEMTIME& st)
	{
		CString s;
		s.Format(_T("%04d-%02d-%02d"), st.wYear, st.wMonth, st.wDay);
		return s;
	}

	// "Apellido, Nombre" de un turno
	CString nombreDe(const nlohmann::json& turno)
	{
		return aTexto(textoDe(turno, "apellido")) + _T(", ") + aTexto(textoDe(turno, "nombre"));
	}

	// Mensaje de error listo para mostrar, segun el tipo de falla
	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

}  // namespace

IMPLEMENT_DYNAMIC(CTurnosDlg, CDialogEx)

CTurnosDlg::CTurnosDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TURNOS, pParent)
{
}

CTurnosDlg::~CTurnosDlg()
{
}

void CTurnosDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_DTP_FECHA, m_fecha);
	DDX_Control(pDX, IDC_LIST_TURNOS, m_lista);
}

BEGIN_MESSAGE_MAP(CTurnosDlg, CDialogEx)
	ON_NOTIFY(DTN_DATETIMECHANGE, IDC_DTP_FECHA, &CTurnosDlg::OnDtnDatetimechangeFecha)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_TURNOS, &CTurnosDlg::OnLvnItemchangedLista)
	ON_BN_CLICKED(IDC_BTN_NUEVO_TURNO, &CTurnosDlg::OnBnClickedBtnNuevoTurno)
	ON_BN_CLICKED(IDC_BTN_REPROGRAMAR, &CTurnosDlg::OnBnClickedBtnReprogramar)
	ON_BN_CLICKED(IDC_BTN_CANCELAR_TURNO, &CTurnosDlg::OnBnClickedBtnCancelarTurno)
	ON_BN_CLICKED(IDC_BTN_AUSENTE, &CTurnosDlg::OnBnClickedBtnAusente)
END_MESSAGE_MAP()

BOOL CTurnosDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Columnas de la agenda
	m_lista.SetExtendedStyle(m_lista.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lista.InsertColumn(0, _T("Hora"), LVCFMT_LEFT, 55);
	m_lista.InsertColumn(1, _T("DNI"), LVCFMT_LEFT, 85);
	m_lista.InsertColumn(2, _T("Apellido"), LVCFMT_LEFT, 120);
	m_lista.InsertColumn(3, _T("Nombre"), LVCFMT_LEFT, 120);
	m_lista.InsertColumn(4, _T("Telefono"), LVCFMT_LEFT, 100);
	m_lista.InsertColumn(5, _T("Donacion"), LVCFMT_LEFT, 90);
	m_lista.InsertColumn(6, _T("Estado"), LVCFMT_LEFT, 85);
	m_lista.InsertColumn(7, _T("Lugar"), LVCFMT_LEFT, 150);

	Cargar();   // al abrir se muestra la agenda de hoy (el selector arranca en la fecha de hoy)
	return TRUE;
}

void CTurnosDlg::OnOK()
{
	// "Actualizar" o tecla Enter: se vuelve a pedir la agenda. No se cierra la ventana.
	Cargar();
}

void CTurnosDlg::OnDtnDatetimechangeFecha(NMHDR* pNMHDR, LRESULT* pResult)
{
	// Se eligio otro dia en el selector: se muestra su agenda
	LPNMDATETIMECHANGE cambio = reinterpret_cast<LPNMDATETIMECHANGE>(pNMHDR);
	if (cambio->dwFlags == GDT_VALID) {
		Cargar(cambio->st);
	}
	*pResult = 0;
}

void CTurnosDlg::OnLvnItemchangedLista(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	ActualizarBotones();
	*pResult = 0;
}

bool CTurnosDlg::SeleccionActual(int& fila, size_t& indice) const
{
	fila = m_lista.GetNextItem(-1, LVNI_SELECTED);
	if (fila < 0) return false;
	indice = static_cast<size_t>(m_lista.GetItemData(fila));
	return indice < m_turnos.size();
}

void CTurnosDlg::ActualizarBotones()
{
	if (m_lista.GetSafeHwnd() == nullptr) return;

	int fila = -1;
	size_t indice = 0;
	bool pendiente = false;
	if (SeleccionActual(fila, indice)) {
		pendiente = (textoDe(m_turnos[indice], "estado") == "Pendiente");
	}
	GetDlgItem(IDC_BTN_REPROGRAMAR)->EnableWindow(pendiente);
	GetDlgItem(IDC_BTN_CANCELAR_TURNO)->EnableWindow(pendiente);
	GetDlgItem(IDC_BTN_AUSENTE)->EnableWindow(pendiente);
}

void CTurnosDlg::Cargar()
{
	SYSTEMTIME dia;
	m_fecha.GetTime(&dia);
	Cargar(dia);
}

void CTurnosDlg::Cargar(const SYSTEMTIME& dia)
{
	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.listarTurnos(aUtf8(fechaIso(dia)));

	if (!r.ok() || !r.cuerpo.is_object()) {
		m_lista.DeleteAllItems();
		m_turnos = nlohmann::json::array();
		ActualizarBotones();
		SetDlgItemText(IDC_STATIC_TURNOS, mensajeDeError(r));
		return;
	}

	m_turnos = r.cuerpo.value("turnos", nlohmann::json::array());
	m_lista.DeleteAllItems();

	int fila = 0;
	int pendientes = 0;
	for (const auto& t : m_turnos) {
		const int i = m_lista.InsertItem(fila, aTexto(textoDe(t, "hora")));
		m_lista.SetItemText(i, 1, aTexto(textoDe(t, "dni")));
		m_lista.SetItemText(i, 2, aTexto(textoDe(t, "apellido")));
		m_lista.SetItemText(i, 3, aTexto(textoDe(t, "nombre")));
		m_lista.SetItemText(i, 4, aTexto(textoDe(t, "telefono")));
		m_lista.SetItemText(i, 5, aTexto(textoDe(t, "tipo_donacion")));
		m_lista.SetItemText(i, 6, aTexto(textoDe(t, "estado")));
		m_lista.SetItemText(i, 7, aTexto(textoDe(t, "lugar")));
		m_lista.SetItemData(i, static_cast<DWORD_PTR>(fila));   // posicion dentro de m_turnos
		if (textoDe(t, "estado") == "Pendiente") ++pendientes;
		++fila;
	}

	CString mensaje;
	if (fila == 0)
		mensaje = _T("No hay turnos para este dia.");
	else
		mensaje.Format(_T("%d turno(s) en el dia, %d pendiente(s)."), fila, pendientes);
	SetDlgItemText(IDC_STATIC_TURNOS, mensaje);

	ActualizarBotones();
}

void CTurnosDlg::MostrarTurnoGuardado(const nlohmann::json& turno, const CString& mensaje)
{
	// La agenda salta al dia del turno guardado (puede ser otro dia que el que se estaba viendo)
	SYSTEMTIME st;
	m_fecha.GetTime(&st);
	int y = 0, m = 0, d = 0;
	if (sscanf_s(textoDe(turno, "fecha").c_str(), "%d-%d-%d", &y, &m, &d) == 3) {
		st.wYear = static_cast<WORD>(y);
		st.wMonth = static_cast<WORD>(m);
		st.wDay = static_cast<WORD>(d);
		m_fecha.SetTime(&st);   // cambiarlo por codigo no avisa a la ventana: se recarga a mano
	}
	Cargar();

	// Se deja seleccionado el turno recien guardado
	const int id = turno.value("id_turno", 0);
	for (size_t i = 0; i < m_turnos.size(); ++i) {
		if (m_turnos[i].value("id_turno", 0) == id) {
			m_lista.SetItemState(static_cast<int>(i), LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
			m_lista.EnsureVisible(static_cast<int>(i), FALSE);
			break;
		}
	}

	SetDlgItemText(IDC_STATIC_TURNOS, mensaje);
}

void CTurnosDlg::OnBnClickedBtnNuevoTurno()
{
	SYSTEMTIME dia;
	m_fecha.GetTime(&dia);

	CTurnoFormDlg form(nullptr, aUtf8(fechaIso(dia)));   // turno nuevo, proponiendo el dia que se esta viendo
	if (form.DoModal() == IDOK) {
		MostrarTurnoGuardado(form.Guardado(), _T("Turno asignado."));
	}
}

void CTurnosDlg::OnBnClickedBtnReprogramar()
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) return;

	CTurnoFormDlg form(&m_turnos[indice]);   // reprogramacion del turno elegido
	if (form.DoModal() == IDOK) {
		MostrarTurnoGuardado(form.Guardado(), _T("Turno reprogramado."));
	}
}

void CTurnosDlg::CambiarEstado(const char* estadoNuevo, const CString& pregunta, const CString& exito)
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) return;

	const int id = m_turnos[indice].value("id_turno", 0);

	if (AfxMessageBox(pregunta, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;   // por defecto queda "No"

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.cambiarEstadoTurno(id, estadoNuevo);

	if (!r.ok() || !r.cuerpo.is_object() || !r.cuerpo.value("ok", false)) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);   // ventana emergente: no se puede pasar por alto
		return;
	}

	Cargar();
	SetDlgItemText(IDC_STATIC_TURNOS, exito);
}

void CTurnosDlg::OnBnClickedBtnCancelarTurno()
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) return;

	const nlohmann::json& turno = m_turnos[indice];
	CString pregunta;
	pregunta.Format(_T("Desea cancelar el turno de %s a las %s?"),
		(LPCTSTR)nombreDe(turno), (LPCTSTR)aTexto(textoDe(turno, "hora")));
	CambiarEstado("Cancelado", pregunta, _T("Turno cancelado."));
}

void CTurnosDlg::OnBnClickedBtnAusente()
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) return;

	const nlohmann::json& turno = m_turnos[indice];
	CString pregunta;
	pregunta.Format(_T("Desea marcar como ausente a %s (turno de las %s)?"),
		(LPCTSTR)nombreDe(turno), (LPCTSTR)aTexto(textoDe(turno, "hora")));
	CambiarEstado("Ausente", pregunta, _T("Turno marcado como ausente."));
}