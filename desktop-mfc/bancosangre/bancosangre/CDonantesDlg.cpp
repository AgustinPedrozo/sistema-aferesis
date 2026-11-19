#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CDonanteFormDlg.h"
#include "CDonantesDlg.h"

IMPLEMENT_DYNAMIC(CDonantesDlg, CDialogEx)

CDonantesDlg::CDonantesDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DONANTES, pParent)
{
}

CDonantesDlg::~CDonantesDlg()
{
}

void CDonantesDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_DONANTES, m_lista);
}

BEGIN_MESSAGE_MAP(CDonantesDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_NUEVO, &CDonantesDlg::OnBnClickedBtnNuevo)
	ON_BN_CLICKED(IDC_BTN_MODIFICAR, &CDonantesDlg::OnBnClickedBtnModificar)
	ON_BN_CLICKED(IDC_BTN_ESTADO, &CDonantesDlg::OnBnClickedBtnEstado)
	ON_BN_CLICKED(IDC_BTN_ELEGIR, &CDonantesDlg::OnBnClickedBtnElegir)
	ON_NOTIFY(NM_DBLCLK, IDC_LIST_DONANTES, &CDonantesDlg::OnNMDblclkLista)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_DONANTES, &CDonantesDlg::OnLvnItemchangedLista)
END_MESSAGE_MAP()

BOOL CDonantesDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Columnas de la lista de resultados
	m_lista.SetExtendedStyle(m_lista.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lista.InsertColumn(0, _T("DNI"), LVCFMT_LEFT, 90);
	m_lista.InsertColumn(1, _T("Apellido"), LVCFMT_LEFT, 140);
	m_lista.InsertColumn(2, _T("Nombre"), LVCFMT_LEFT, 140);
	m_lista.InsertColumn(3, _T("Telefono"), LVCFMT_LEFT, 110);
	m_lista.InsertColumn(4, _T("Estado"), LVCFMT_LEFT, 80);

	// Segun el modo, se muestra el boton "Elegir este donante" o el de baja/reactivacion
	if (m_modoSeleccion) {
		SetWindowText(_T("Elegir donante"));
		GetDlgItem(IDC_BTN_ESTADO)->ShowWindow(SW_HIDE);
	}
	else {
		GetDlgItem(IDC_BTN_ELEGIR)->ShowWindow(SW_HIDE);
	}

	ActualizarBotones();   // sin seleccion: los botones de accion quedan apagados

	GetDlgItem(IDC_EDIT_BUSCAR)->SetFocus();
	return FALSE;   // FALSE porque el foco se puso a mano
}

void CDonantesDlg::OnOK()
{
	// Se llama al apretar "Buscar" o la tecla Enter. No se cierra la ventana.
	Buscar();
}

bool CDonantesDlg::SeleccionActual(int& fila, size_t& indice) const
{
	fila = m_lista.GetNextItem(-1, LVNI_SELECTED);
	if (fila < 0) return false;
	indice = static_cast<size_t>(m_lista.GetItemData(fila));
	return indice < m_donantes.size();
}

void CDonantesDlg::ActualizarBotones()
{
	if (m_lista.GetSafeHwnd() == nullptr) return;

	int fila = -1;
	size_t indice = 0;
	const bool hay = SeleccionActual(fila, indice);

	GetDlgItem(IDC_BTN_MODIFICAR)->EnableWindow(hay);
	GetDlgItem(IDC_BTN_ESTADO)->EnableWindow(hay);
	GetDlgItem(IDC_BTN_ELEGIR)->EnableWindow(hay);

	// El boton de estado dice lo que hace segun el donante elegido
	const bool activo = hay ? m_donantes[indice].value("activo", true) : true;
	SetDlgItemText(IDC_BTN_ESTADO, activo ? _T("Dar de baja") : _T("Reactivar"));
}

void CDonantesDlg::OnLvnItemchangedLista(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	ActualizarBotones();
	*pResult = 0;
}

void CDonantesDlg::OnBnClickedBtnNuevo()
{
	CDonanteFormDlg form;   // sin donante existente: es un alta
	if (form.DoModal() == IDOK) {
		MostrarDonante(form.Guardado());
	}
}

void CDonantesDlg::OnBnClickedBtnModificar()
{
	ModificarSeleccionado();
}

void CDonantesDlg::OnBnClickedBtnElegir()
{
	ElegirSeleccionado();
}

void CDonantesDlg::OnNMDblclkLista(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	// Doble clic: en modo seleccion elige al donante; si no, lo abre para modificarlo
	if (m_modoSeleccion)
		ElegirSeleccionado();
	else
		ModificarSeleccionado();
	*pResult = 0;
}

void CDonantesDlg::ElegirSeleccionado()
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) {
		SetDlgItemText(IDC_STATIC_RESULTADO, _T("Elija un donante de la lista."));
		return;
	}

	const nlohmann::json& donante = m_donantes[indice];
	if (!donante.value("activo", true)) {
		AfxMessageBox(_T("Ese donante esta dado de baja. Reactivelo antes de elegirlo."), MB_ICONWARNING);
		return;
	}

	m_elegido = donante;
	EndDialog(IDOK);   // cierra la ventana indicando que se eligio un donante
}

void CDonantesDlg::ModificarSeleccionado()
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) {
		SetDlgItemText(IDC_STATIC_RESULTADO, _T("Elija un donante de la lista para modificarlo."));
		return;
	}

	CDonanteFormDlg form(&m_donantes[indice]);
	if (form.DoModal() == IDOK) {
		MostrarDonante(form.Guardado());
	}
}

void CDonantesDlg::OnBnClickedBtnEstado()
{
	int fila = -1;
	size_t indice = 0;
	if (!SeleccionActual(fila, indice)) return;

	const nlohmann::json donante = m_donantes[indice];   // copia
	const bool estabaActivo = donante.value("activo", true);
	const int id = donante.value("id_donante", 0);

	// Confirmacion: la baja no es algo para hacer por error
	const CString nombre = aTexto(textoDe(donante, "apellido")) + _T(", ") + aTexto(textoDe(donante, "nombre"));
	CString pregunta;
	if (estabaActivo)
		pregunta.Format(_T("Desea dar de baja a %s?\r\n\r\nNo se borra: su historial se conserva y se puede reactivar despues."), (LPCTSTR)nombre);
	else
		pregunta.Format(_T("Desea reactivar a %s?"), (LPCTSTR)nombre);

	if (AfxMessageBox(pregunta, MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2) != IDYES) return;   // por defecto queda "No"

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.cambiarEstadoDonante(id, !estabaActivo);

	if (!r.ok() || !r.cuerpo.is_object() || !r.cuerpo.value("ok", false)) {
		CString mensaje;
		if (!r.errorRed.empty())
			mensaje = _T("No se pudo conectar con el servidor. Avise a un administrador.");
		else
			mensaje = aTexto(r.error());   // por ejemplo: "No tiene permiso para esta accion."
		AfxMessageBox(mensaje, MB_ICONWARNING);   // ventana emergente: no se puede pasar por alto
		return;
	}

	// Se actualiza la fila en pantalla sin volver a buscar
	const nlohmann::json actualizado = r.cuerpo.value("donante", nlohmann::json::object());
	if (actualizado.is_object() && !actualizado.empty()) {
		m_donantes[indice] = actualizado;
	}
	m_lista.SetItemText(fila, 4, estabaActivo ? _T("De baja") : _T("Activo"));
	ActualizarBotones();
	SetDlgItemText(IDC_STATIC_RESULTADO, estabaActivo ? _T("Donante dado de baja.") : _T("Donante reactivado."));
}

void CDonantesDlg::MostrarDonante(const nlohmann::json& donante)
{
	// Se busca por el DNI del donante guardado para que aparezca actualizado en la lista.
	SetDlgItemText(IDC_EDIT_BUSCAR, aTexto(textoDe(donante, "dni")));
	if (!donante.value("activo", true)) {
		CheckDlgButton(IDC_CHK_INACTIVOS, BST_CHECKED);   // si esta de baja, hay que incluir inactivos para verlo
	}
	Buscar();
}

void CDonantesDlg::Buscar()
{
	CString texto;
	GetDlgItemText(IDC_EDIT_BUSCAR, texto);
	texto.Trim();
	if (texto.GetLength() < 2) {
		SetDlgItemText(IDC_STATIC_RESULTADO, _T("Escriba al menos 2 caracteres para buscar."));
		return;
	}

	const bool inactivos = (IsDlgButtonChecked(IDC_CHK_INACTIVOS) == BST_CHECKED);

	CWaitCursor espera;   // cursor de espera mientras se consulta al servidor
	RespuestaApi r = theApp.m_api.buscarDonantes(aUtf8(texto), inactivos);

	if (!r.ok() || !r.cuerpo.is_object()) {
		CString mensaje;
		if (!r.errorRed.empty())
			mensaje = _T("No se pudo conectar con el servidor. Avise a un administrador.");
		else
			mensaje = aTexto(r.error());
		m_lista.DeleteAllItems();
		m_donantes = nlohmann::json::array();
		ActualizarBotones();
		SetDlgItemText(IDC_STATIC_RESULTADO, mensaje);
		return;
	}

	m_donantes = r.cuerpo.value("donantes", nlohmann::json::array());
	MostrarResultados(m_donantes);
}

void CDonantesDlg::MostrarResultados(const nlohmann::json& donantes)
{
	m_lista.DeleteAllItems();

	int fila = 0;
	for (const auto& d : donantes) {
		const int i = m_lista.InsertItem(fila, aTexto(textoDe(d, "dni")));
		m_lista.SetItemText(i, 1, aTexto(textoDe(d, "apellido")));
		m_lista.SetItemText(i, 2, aTexto(textoDe(d, "nombre")));
		m_lista.SetItemText(i, 3, aTexto(textoDe(d, "telefono")));
		m_lista.SetItemText(i, 4, d.value("activo", true) ? _T("Activo") : _T("De baja"));
		m_lista.SetItemData(i, static_cast<DWORD_PTR>(fila));   // posicion dentro de m_donantes
		++fila;
	}

	CString mensaje;
	if (fila == 0)
		mensaje = _T("No se encontraron donantes.");
	else if (fila == 1)
		mensaje = _T("1 donante encontrado.");
	else
		mensaje.Format(_T("%d donantes encontrados."), fila);
	SetDlgItemText(IDC_STATIC_RESULTADO, mensaje);

	ActualizarBotones();
}