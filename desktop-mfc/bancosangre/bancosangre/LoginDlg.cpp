// LoginDlg.cpp: archivo de implementación
//

#include "pch.h"
#include "bancosangre.h"
#include "afxdialogex.h"
#include "LoginDlg.h"


// Cuadro de diálogo de LoginDlg

IMPLEMENT_DYNAMIC(LoginDlg, CDialogEx)

LoginDlg::LoginDlg(CWnd* pParent /*= nullptr*/)
	: CDialogEx(IDD_LOGIN, pParent)
{

}

LoginDlg::~LoginDlg()
{
}

void LoginDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(LoginDlg, CDialogEx)
	ON_BN_CLICKED(IDOK, &LoginDlg::OnBnClickedOk)
END_MESSAGE_MAP()


// Controladores de mensajes de LoginDlg

void LoginDlg::OnBnClickedOk()
{

    CString usuario, clave;
    GetDlgItemText(IDC_EDIT_USUARIO, usuario);
    GetDlgItemText(IDC_EDIT_CLAVE, clave);
    usuario.Trim();

    if (usuario.IsEmpty() || clave.IsEmpty()) {
        SetDlgItemText(IDC_STATIC_MENSAJE, _T("Escriba su usuario y su clave."));
        return;
    }

    CWaitCursor espera;   // cursor de espera mientras se consulta al servidor
    RespuestaApi r = theApp.m_api.iniciarSesion(
        std::string(CT2A(usuario, CP_UTF8)),
        std::string(CT2A(clave, CP_UTF8)));

    if (!theApp.m_api.tieneSesion()) {
        CString mensaje;
        if (!r.errorRed.empty())
            mensaje = _T("No se pudo conectar con el servidor. Avise a un administrador.");
        else
            mensaje = CString(CA2W(r.error().c_str(), CP_UTF8));
        SetDlgItemText(IDC_STATIC_MENSAJE, mensaje);
        SetDlgItemText(IDC_EDIT_CLAVE, _T(""));
        GetDlgItem(IDC_EDIT_CLAVE)->SetFocus();
        return;   // la ventana queda abierta para reintentar
    }

	CDialogEx::OnOK();
}
