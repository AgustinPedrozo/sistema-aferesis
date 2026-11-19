
// bancosangreDlg.h: archivo de encabezado
//

#pragma once


// Cuadro de diálogo de CbancosangreDlg
class CbancosangreDlg : public CDialogEx
{
// Construcción
public:
	CbancosangreDlg(CWnd* pParent = nullptr);	// Constructor estándar

// Datos del cuadro de diálogo
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BANCOSANGRE_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// Compatibilidad con DDX/DDV


// Implementación
protected:
	HICON m_hIcon;
	void ActualizarMensajes();
	afx_msg void OnTimer(UINT_PTR nIDEvent);

	// Funciones de asignación de mensajes generadas
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnDonantes();
	afx_msg void OnBnClickedBtnTurnos();
	afx_msg void OnBnClickedBtnEntrevista();
	afx_msg void OnBnClickedBtnExtraccion();
	afx_msg void OnBnClickedBtnEstadisticas();
	afx_msg void OnBnClickedBtnMensajes();
	afx_msg void OnBnClickedBtnHemo3();
	afx_msg void OnBnClickedBtnConfig();
	afx_msg void OnBnClickedBtnUsuarios();
};
