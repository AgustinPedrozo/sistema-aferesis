#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Administracion de usuarios (solo Administrador).

class CUsuariosDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CUsuariosDlg)

public:
	CUsuariosDlg(CWnd* pParent = nullptr);
	virtual ~CUsuariosDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_USUARIOS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();       // boton "Guardar"

	afx_msg void OnBnClickedNuevo();
	afx_msg void OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	void Cargar(const CString& seleccionar);
	void ModoAlta();
	int IndiceElegido() const;
	void Error(const CString& mensaje);

	nlohmann::json m_usuarios;
	nlohmann::json m_roles;
	int m_idEditando = 0;      // 0 = usuario nuevo
	CListCtrl m_lista;
	CComboBox m_comboRol;

	DECLARE_MESSAGE_MAP()
};