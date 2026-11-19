#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "CAutorizacionDlg.h"

IMPLEMENT_DYNAMIC(CAutorizacionDlg, CDialogEx)

CAutorizacionDlg::CAutorizacionDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_AUTORIZACION, pParent)
{
}

CAutorizacionDlg::~CAutorizacionDlg()
{
}

BEGIN_MESSAGE_MAP(CAutorizacionDlg, CDialogEx)
END_MESSAGE_MAP()

BOOL CAutorizacionDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Autorizacion de excepcion"));

	SetDlgItemText(IDC_STATIC_VIOLACIONES, m_violaciones);
	SetDlgItemText(IDC_STATIC_ERROR_AUT, m_errorPrevio);

	// Si quien registra ya es Jefa de Area/Gerente (o Administrador), autoriza con su propia sesion:
	// solo hace falta el motivo
	if (!m_pedirCredenciales) {
		GetDlgItem(IDC_STATIC_AUT_CRED)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_AUT_USUARIO)->ShowWindow(SW_HIDE);
		GetDlgItem(IDC_EDIT_AUT_CLAVE)->ShowWindow(SW_HIDE);
	}
	return TRUE;
}

void CAutorizacionDlg::OnOK()
{
	GetDlgItemText(IDC_EDIT_AUT_MOTIVO, m_motivo);
	m_motivo.Trim();
	GetDlgItemText(IDC_EDIT_AUT_USUARIO, m_usuario);
	m_usuario.Trim();
	GetDlgItemText(IDC_EDIT_AUT_CLAVE, m_clave);

	if (m_motivo.GetLength() < 10) {
		SetDlgItemText(IDC_STATIC_ERROR_AUT, _T("Explique el motivo de la excepcion (al menos 10 letras)."));
		GetDlgItem(IDC_EDIT_AUT_MOTIVO)->SetFocus();
		return;
	}
	if (m_pedirCredenciales && (m_usuario.IsEmpty() || m_clave.IsEmpty())) {
		SetDlgItemText(IDC_STATIC_ERROR_AUT, _T("La Jefa de Area/Gerente debe escribir su usuario y su clave."));
		GetDlgItem(IDC_EDIT_AUT_USUARIO)->SetFocus();
		return;
	}
	CDialogEx::OnOK();
}