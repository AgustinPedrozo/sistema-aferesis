#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CConfiguracionDlg.h"

namespace {

	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	bool soloDigitos(const CString& s)
	{
		if (s.IsEmpty() || s.GetLength() > 7) return false;
		for (int i = 0; i < s.GetLength(); ++i) {
			if (s[i] < '0' || s[i] > '9') return false;
		}
		return true;
	}

}  // namespace

IMPLEMENT_DYNAMIC(CConfiguracionDlg, CDialogEx)

CConfiguracionDlg::CConfiguracionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_CONFIGURACION, pParent)
{
}

CConfiguracionDlg::~CConfiguracionDlg()
{
}

void CConfiguracionDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_CFG, m_lista);
}

BEGIN_MESSAGE_MAP(CConfiguracionDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_CFG_APLICAR, &CConfiguracionDlg::OnBnClickedAplicar)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_CFG, &CConfiguracionDlg::OnLvnItemchangedLista)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CConfiguracionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Configuracion del sistema"));

	m_lista.SetExtendedStyle(m_lista.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lista.InsertColumn(0, _T("Parametro"), LVCFMT_LEFT, 290);
	m_lista.InsertColumn(1, _T("Valor"), LVCFMT_RIGHT, 70);
	m_lista.InsertColumn(2, _T("Unidad"), LVCFMT_LEFT, 90);
	m_lista.InsertColumn(3, _T("Rango"), LVCFMT_LEFT, 90);

	SetDlgItemText(IDC_STATIC_CFG_AYUDA, _T("Elija un parametro de la lista para ver para que sirve y cambiar su valor."));
	GetDlgItem(IDC_EDIT_CFG_VALOR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_CFG_APLICAR)->EnableWindow(FALSE);

	Cargar();
	return TRUE;
}

HBRUSH CConfiguracionDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_CFG_ERROR) {
		pDC->SetTextColor(RGB(192, 0, 0));
	}
	else if (pWnd->GetDlgCtrlID() == IDC_STATIC_CFG_ESTADO && !m_pendientes.empty()) {
		pDC->SetTextColor(RGB(200, 110, 0));   // hay cambios sin guardar
	}
	return hbr;
}

void CConfiguracionDlg::Error(const CString& mensaje)
{
	SetDlgItemText(IDC_STATIC_CFG_ERROR, mensaje);
}

void CConfiguracionDlg::Cargar()
{
	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.listarConfiguracion();
	if (!r.ok() || !r.cuerpo.is_object()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
		return;
	}
	m_params = r.cuerpo.value("parametros", nlohmann::json::array());
	m_pendientes.clear();

	m_lista.DeleteAllItems();
	int fila = 0;
	for (const auto& p : m_params) {
		const int i = m_lista.InsertItem(fila, aTexto(textoDe(p, "etiqueta")));
		m_lista.SetItemData(i, static_cast<DWORD_PTR>(fila));   // posicion dentro de m_params
		ActualizarFila(i, fila);
		++fila;
	}
	SetDlgItemText(IDC_EDIT_CFG_VALOR, _T(""));
	GetDlgItem(IDC_EDIT_CFG_VALOR)->EnableWindow(FALSE);
	GetDlgItem(IDC_BTN_CFG_APLICAR)->EnableWindow(FALSE);
	Error(CString());
	ActualizarEstado();
}

int CConfiguracionDlg::IndiceElegido() const
{
	POSITION pos = m_lista.GetFirstSelectedItemPosition();
	if (pos == nullptr) return -1;
	const int fila = m_lista.GetNextSelectedItem(pos);
	const size_t indice = static_cast<size_t>(m_lista.GetItemData(fila));
	return indice < m_params.size() ? static_cast<int>(indice) : -1;
}

int CConfiguracionDlg::ValorActual(int indice) const
{
	const nlohmann::json& p = m_params[indice];
	auto it = m_pendientes.find(textoDe(p, "clave"));
	return it != m_pendientes.end() ? it->second : p.value("valor", 0);
}

void CConfiguracionDlg::ActualizarFila(int fila, int indice)
{
	const nlohmann::json& p = m_params[indice];
	const bool cambiado = m_pendientes.count(textoDe(p, "clave")) > 0;

	CString valor;
	valor.Format(_T("%d%s"), ValorActual(indice), cambiado ? _T(" *") : _T(""));
	CString rango;
	rango.Format(_T("%d a %d"), p.value("min", 0), p.value("max", 0));

	m_lista.SetItemText(fila, 1, valor);
	m_lista.SetItemText(fila, 2, aTexto(textoDe(p, "unidad")));
	m_lista.SetItemText(fila, 3, rango);
}

void CConfiguracionDlg::ActualizarEstado()
{
	CString texto;
	const int n = static_cast<int>(m_pendientes.size());
	if (n == 0)
		texto = _T("Sin cambios pendientes.");
	else if (n == 1)
		texto = _T("1 cambio sin guardar (marcado con *).");
	else
		texto.Format(_T("%d cambios sin guardar (marcados con *)."), n);
	SetDlgItemText(IDC_STATIC_CFG_ESTADO, texto);
	GetDlgItem(IDOK)->EnableWindow(n > 0);
	GetDlgItem(IDC_STATIC_CFG_ESTADO)->Invalidate();   // para repintar con el color que corresponda
}

void CConfiguracionDlg::OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW info = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((info->uChanged & LVIF_STATE) != 0 && (info->uNewState & LVIS_SELECTED) != 0) {
		const int i = IndiceElegido();
		if (i >= 0) {
			SetDlgItemText(IDC_STATIC_CFG_AYUDA, aTexto(textoDe(m_params[i], "ayuda")));
			CString valor;
			valor.Format(_T("%d"), ValorActual(i));
			SetDlgItemText(IDC_EDIT_CFG_VALOR, valor);
			GetDlgItem(IDC_EDIT_CFG_VALOR)->EnableWindow(TRUE);
			GetDlgItem(IDC_BTN_CFG_APLICAR)->EnableWindow(TRUE);
			Error(CString());
		}
	}
	*pResult = 0;
}

// Con Enter dentro de la caja del valor se aplica el cambio (en vez de guardar todo y cerrar)
BOOL CConfiguracionDlg::PreTranslateMessage(MSG* pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN && GetFocus() != nullptr
		&& GetFocus()->GetDlgCtrlID() == IDC_EDIT_CFG_VALOR) {
		OnBnClickedAplicar();
		return TRUE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void CConfiguracionDlg::OnBnClickedAplicar()
{
	Error(CString());
	const int i = IndiceElegido();
	if (i < 0) return;
	const nlohmann::json& p = m_params[i];

	CString texto;
	GetDlgItemText(IDC_EDIT_CFG_VALOR, texto);
	texto.Trim();

	const int minimo = p.value("min", 0);
	const int maximo = p.value("max", 0);
	if (!soloDigitos(texto) || _ttoi(texto) < minimo || _ttoi(texto) > maximo) {
		CString mensaje;
		mensaje.Format(_T("El valor debe ser un numero entero entre %d y %d."), minimo, maximo);
		Error(mensaje);
		GetDlgItem(IDC_EDIT_CFG_VALOR)->SetFocus();
		SendDlgItemMessage(IDC_EDIT_CFG_VALOR, EM_SETSEL, 0, -1);
		return;
	}

	const std::string clave = textoDe(p, "clave");
	const int nuevo = _ttoi(texto);
	if (nuevo == p.value("valor", 0))
		m_pendientes.erase(clave);        // volvio al valor original: ya no es un cambio
	else
		m_pendientes[clave] = nuevo;

	POSITION pos = m_lista.GetFirstSelectedItemPosition();
	if (pos != nullptr) ActualizarFila(m_lista.GetNextSelectedItem(pos), i);
	ActualizarEstado();
}

void CConfiguracionDlg::OnOK()
{
	Error(CString());
	if (m_pendientes.empty()) {
		AfxMessageBox(_T("No hay cambios para guardar."), MB_ICONINFORMATION);
		return;
	}

	nlohmann::json cambios = nlohmann::json::object();
	for (const auto& par : m_pendientes) {
		cambios[par.first] = par.second;
	}

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.guardarConfiguracion(cambios);
	if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
		MessageBeep(MB_OK);   // confirmacion sonora
		AfxMessageBox(aTexto(r.cuerpo.value("mensaje", std::string())), MB_ICONINFORMATION);
		Cargar();             // se vuelve a leer del servidor: lo que se ve es lo que quedo guardado
		return;
	}

	// El servidor rechazo algun valor: se muestran todos los motivos y no se guarda nada
	CString mensaje = mensajeDeError(r);
	if (r.codigoHttp == 422 && r.cuerpo.is_object()) {
		auto it = r.cuerpo.find("campos");
		if (it != r.cuerpo.end() && it->is_object() && !it->empty()) {
			mensaje.Empty();
			for (auto c = it->begin(); c != it->end(); ++c) {
				if (c.value().is_string()) {
					if (!mensaje.IsEmpty()) mensaje += _T("\r\n");
					mensaje += aTexto(c.value().get<std::string>());
				}
			}
		}
	}
	Error(_T("No se guardo nada: hay valores para corregir."));
	AfxMessageBox(mensaje, MB_ICONWARNING);
}

void CConfiguracionDlg::OnCancel()
{
	if (!m_pendientes.empty()) {
		if (AfxMessageBox(_T("Hay cambios sin guardar. Cerrar igualmente y perderlos?"), MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
	}
	CDialogEx::OnCancel();
}