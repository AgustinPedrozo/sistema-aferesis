#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Formulario confidencial HEMO 3: tres preguntas que se responden Si o No.
// Controles esperados en IDD_HEMO3:
//   IDC_STATIC_DONANTE_H3, IDC_BTN_ELEGIR_H3, IDC_STATIC_AVISO_H3,
//   IDC_STATIC_P1, IDC_RADIO_P1_SI, IDC_RADIO_P1_NO,
//   IDC_STATIC_P2, IDC_RADIO_P2_SI, IDC_RADIO_P2_NO,
//   IDC_STATIC_P3, IDC_RADIO_P3_SI, IDC_RADIO_P3_NO,
//   IDC_EDIT_OBS_H3, IDC_BTN_VER_H3, IDC_STATIC_ERROR_H3, IDOK ("Guardar formulario"), IDCANCEL.
class CHemo3Dlg : public CDialogEx
{
	DECLARE_DYNAMIC(CHemo3Dlg)

public:
	CHemo3Dlg(CWnd* pParent = nullptr);
	virtual ~CHemo3Dlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_HEMO3 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // boton "Guardar formulario"

	afx_msg void OnBnClickedBtnElegir();
	afx_msg void OnBnClickedBtnVer();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	void CargarPreguntas();
	void CargarDonante(const nlohmann::json& d);
	void Error(const CString& mensaje, UINT idControl = 0);

	int m_idDonante = 0;
	bool m_existeHoy = false;
	nlohmann::json m_preguntas;   // [{clave, texto}, ...] que manda el servidor

	DECLARE_MESSAGE_MAP()
};