#pragma once
#include "afxdialogex.h"

// Ventana que pide la autorizacion de la Jefa de Area/Gerente cuando una extraccion supera los limites.
// Controles esperados en el recurso IDD_AUTORIZACION:
//   IDC_STATIC_VIOLACIONES (texto, varias lineas), IDC_EDIT_AUT_MOTIVO (varias lineas),
//   IDC_STATIC_AUT_CRED (etiqueta "Autoriza:"), IDC_EDIT_AUT_USUARIO, IDC_EDIT_AUT_CLAVE (Password = True),
//   IDC_STATIC_ERROR_AUT, IDOK ("Autorizar"), IDCANCEL ("Cancelar").
class CAutorizacionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CAutorizacionDlg)

public:
	CAutorizacionDlg(CWnd* pParent = nullptr);
	virtual ~CAutorizacionDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_AUTORIZACION };
#endif

	// Antes de DoModal: que limites se superan, si hay que pedir usuario y clave, y un error previo (si lo hubo)
	CString m_violaciones;
	bool m_pedirCredenciales = true;
	CString m_errorPrevio;

	// Despues de DoModal (si devolvio IDOK)
	CString m_motivo;
	CString m_usuario;
	CString m_clave;

protected:
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	DECLARE_MESSAGE_MAP()
};