#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CUsuariosDlg.h"

namespace {

	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	bool soloDigitos(const CString& s)
	{
		if (s.IsEmpty()) return false;
		for (int i = 0; i < s.GetLength(); ++i) {
			if (s[i] < '0' || s[i] > '9') return false;
		}
		return true;
	}

}  // namespace

IMPLEMENT_DYNAMIC(CUsuariosDlg, CDialogEx)

CUsuariosDlg::CUsuariosDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_USUARIOS, pParent)
{
}

CUsuariosDlg::~CUsuariosDlg()
{
}

void CUsuariosDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_USU, m_lista);
	DDX_Control(pDX, IDC_COMBO_USU_ROL, m_comboRol);
}

BEGIN_MESSAGE_MAP(CUsuariosDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_USU_NUEVO, &CUsuariosDlg::OnBnClickedNuevo)
	ON_NOTIFY(LVN_ITEMCHANGED, IDC_LIST_USU, &CUsuariosDlg::OnLvnItemchangedLista)
	ON_WM_CTLCOLOR()
END_MESSAGE_MAP()

BOOL CUsuariosDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Usuarios del sistema"));

	m_lista.SetExtendedStyle(m_lista.GetExtendedStyle() | LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);
	m_lista.InsertColumn(0, _T("Usuario"), LVCFMT_LEFT, 80);
	m_lista.InsertColumn(1, _T("Nombre"), LVCFMT_LEFT, 190);
	m_lista.InsertColumn(2, _T("Rol"), LVCFMT_LEFT, 190);
	m_lista.InsertColumn(3, _T("Estado"), LVCFMT_LEFT, 80);

	Cargar(CString());
	return TRUE;
}

HBRUSH CUsuariosDlg::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CDialogEx::OnCtlColor(pDC, pWnd, nCtlColor);
	if (pWnd->GetDlgCtrlID() == IDC_STATIC_USU_ERROR) {
		pDC->SetTextColor(RGB(192, 0, 0));
	}
	return hbr;
}

void CUsuariosDlg::Error(const CString& mensaje)
{
	SetDlgItemText(IDC_STATIC_USU_ERROR, mensaje);
}

void CUsuariosDlg::Cargar(const CString& seleccionar)
{
	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.listarUsuarios();
	if (!r.ok() || !r.cuerpo.is_object()) {
		AfxMessageBox(mensajeDeError(r), MB_ICONWARNING);
		return;
	}
	m_usuarios = r.cuerpo.value("usuarios", nlohmann::json::array());
	m_roles = r.cuerpo.value("roles", nlohmann::json::array());

	m_comboRol.ResetContent();
	for (const auto& rol : m_roles) {
		const int i = m_comboRol.AddString(aTexto(textoDe(rol, "nombre")));
		m_comboRol.SetItemData(i, static_cast<DWORD_PTR>(rol.value("id_rol", 0)));
	}

	m_lista.DeleteAllItems();
	int filaAElegir = -1;
	int fila = 0;
	for (const auto& u : m_usuarios) {
		const CString usuario = aTexto(textoDe(u, "nombre_usuario"));
		const int i = m_lista.InsertItem(fila, usuario);
		m_lista.SetItemData(i, static_cast<DWORD_PTR>(fila));
		m_lista.SetItemText(i, 1, aTexto(textoDe(u, "nombre_completo")));
		m_lista.SetItemText(i, 2, aTexto(textoDe(u, "rol")));
		CString estado = u.value("activo", false) ? _T("Activo") : _T("Inactivo");
		if (u.value("activo", false) && u.value("bloqueado", false)) estado = _T("Bloqueado");
		m_lista.SetItemText(i, 3, estado);
		if (!seleccionar.IsEmpty() && usuario == seleccionar) filaAElegir = i;
		++fila;
	}

	if (filaAElegir >= 0) {
		m_lista.SetItemState(filaAElegir, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_lista.EnsureVisible(filaAElegir, FALSE);
	}
	else {
		ModoAlta();
	}
}

void CUsuariosDlg::ModoAlta()
{
	m_idEditando = 0;
	POSITION pos = m_lista.GetFirstSelectedItemPosition();
	while (pos != nullptr) {
		const int fila = m_lista.GetNextSelectedItem(pos);
		m_lista.SetItemState(fila, 0, LVIS_SELECTED | LVIS_FOCUSED);
		pos = m_lista.GetFirstSelectedItemPosition();
	}
	SetDlgItemText(IDC_EDIT_USU_USUARIO, _T(""));
	SetDlgItemText(IDC_EDIT_USU_NOMBRE, _T(""));
	SetDlgItemText(IDC_EDIT_USU_CLAVE, _T(""));
	SendDlgItemMessage(IDC_EDIT_USU_USUARIO, EM_SETREADONLY, FALSE, 0);
	SendDlgItemMessage(IDC_EDIT_USU_USUARIO, EM_SETCUEBANNER, TRUE, (LPARAM)L"Iniciales, ej: cc");
	SendDlgItemMessage(IDC_EDIT_USU_CLAVE, EM_SETCUEBANNER, FALSE, (LPARAM)L"Numero de 4 a 10 digitos");
	m_comboRol.SetCurSel(-1);
	CheckDlgButton(IDC_CHECK_USU_ACTIVO, BST_CHECKED);
	GetDlgItem(IDC_CHECK_USU_ACTIVO)->EnableWindow(FALSE);   // un usuario nuevo siempre arranca activo
	Error(CString());
	GetDlgItem(IDC_EDIT_USU_USUARIO)->SetFocus();
}

int CUsuariosDlg::IndiceElegido() const
{
	POSITION pos = m_lista.GetFirstSelectedItemPosition();
	if (pos == nullptr) return -1;
	const int fila = m_lista.GetNextSelectedItem(pos);
	const size_t indice = static_cast<size_t>(m_lista.GetItemData(fila));
	return indice < m_usuarios.size() ? static_cast<int>(indice) : -1;
}

void CUsuariosDlg::OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMLISTVIEW info = reinterpret_cast<LPNMLISTVIEW>(pNMHDR);
	if ((info->uChanged & LVIF_STATE) != 0 && (info->uNewState & LVIS_SELECTED) != 0) {
		const int i = IndiceElegido();
		if (i >= 0) {
			const nlohmann::json& u = m_usuarios[i];
			m_idEditando = u.value("id_usuario", 0);
			SetDlgItemText(IDC_EDIT_USU_USUARIO, aTexto(textoDe(u, "nombre_usuario")));
			SendDlgItemMessage(IDC_EDIT_USU_USUARIO, EM_SETREADONLY, TRUE, 0);   // el usuario no se renombra
			SetDlgItemText(IDC_EDIT_USU_NOMBRE, aTexto(textoDe(u, "nombre_completo")));
			SetDlgItemText(IDC_EDIT_USU_CLAVE, _T(""));
			SendDlgItemMessage(IDC_EDIT_USU_CLAVE, EM_SETCUEBANNER, FALSE, (LPARAM)L"Vacia = no cambiarla");
			const int idRol = u.value("id_rol", 0);
			m_comboRol.SetCurSel(-1);
			for (int k = 0; k < m_comboRol.GetCount(); ++k) {
				if (static_cast<int>(m_comboRol.GetItemData(k)) == idRol) { m_comboRol.SetCurSel(k); break; }
			}
			CheckDlgButton(IDC_CHECK_USU_ACTIVO, u.value("activo", false) ? BST_CHECKED : BST_UNCHECKED);
			GetDlgItem(IDC_CHECK_USU_ACTIVO)->EnableWindow(TRUE);
			Error(CString());
		}
	}
	*pResult = 0;
}

void CUsuariosDlg::OnBnClickedNuevo()
{
	ModoAlta();
}

void CUsuariosDlg::OnOK()
{
	Error(CString());

	CString usuario, nombre, clave;
	GetDlgItemText(IDC_EDIT_USU_USUARIO, usuario);
	GetDlgItemText(IDC_EDIT_USU_NOMBRE, nombre);
	GetDlgItemText(IDC_EDIT_USU_CLAVE, clave);
	usuario.Trim();
	nombre.Trim();
	clave.Trim();

	// Controles suaves para ayudar; las reglas reales las aplica el servidor
	if (m_idEditando == 0 && usuario.GetLength() < 2) {
		Error(_T("Escriba las iniciales del usuario (minimo 2 letras)."));
		GetDlgItem(IDC_EDIT_USU_USUARIO)->SetFocus();
		return;
	}
	if (nombre.GetLength() < 3) {
		Error(_T("Escriba el nombre completo."));
		GetDlgItem(IDC_EDIT_USU_NOMBRE)->SetFocus();
		return;
	}
	if (m_idEditando == 0 && clave.IsEmpty()) {
		Error(_T("Escriba una clave para el usuario nuevo."));
		GetDlgItem(IDC_EDIT_USU_CLAVE)->SetFocus();
		return;
	}
	if (!clave.IsEmpty() && (!soloDigitos(clave) || clave.GetLength() < 4 || clave.GetLength() > 10)) {
		Error(_T("La clave debe ser un numero de 4 a 10 digitos."));
		GetDlgItem(IDC_EDIT_USU_CLAVE)->SetFocus();
		return;
	}
	const int sel = m_comboRol.GetCurSel();
	if (sel < 0) {
		Error(_T("Elija un rol de la lista."));
		m_comboRol.SetFocus();
		return;
	}
	if (m_idEditando != 0 && !IsDlgButtonChecked(IDC_CHECK_USU_ACTIVO)) {
		if (AfxMessageBox(_T("El usuario va a quedar inactivo y no podra entrar al sistema. Continuar?"),
			MB_YESNO | MB_ICONQUESTION) != IDYES)
			return;
	}

	nlohmann::json datos = {
		{"id_usuario", m_idEditando},
		{"nombre_usuario", aUtf8(usuario)},
		{"nombre_completo", aUtf8(nombre)},
		{"clave", aUtf8(clave)},
		{"id_rol", static_cast<int>(m_comboRol.GetItemData(sel))},
		{"activo", IsDlgButtonChecked(IDC_CHECK_USU_ACTIVO) != 0}
	};

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.guardarUsuario(datos);
	if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
		MessageBeep(MB_OK);
		AfxMessageBox(aTexto(r.cuerpo.value("mensaje", std::string())), MB_ICONINFORMATION);
		usuario.MakeLower();
		Cargar(usuario);
		return;
	}
	Error(mensajeDeError(r));
}