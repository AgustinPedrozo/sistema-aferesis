#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Extraccion por afaresis: verifica al donante y carga la planilla HEMO 2.
//   IDC_STATIC_DONANTE_EXT, IDC_BTN_ELEGIR_EXT, IDC_COMBO_TIPO_EXT, IDC_STATIC_ESTADO_EXT,
//   IDC_EDIT_NRO_DON, IDC_EDIT_CANT_PLAQ, IDC_EDIT_CANT_PLASMA,
//   IDC_EDIT_HTO, IDC_EDIT_HB, IDC_EDIT_TA, IDC_EDIT_PULSO, IDC_EDIT_TEMP,
//   IDC_COMBO_BOLSA, IDC_EDIT_MARCA, IDC_EDIT_LOTE, IDC_EDIT_TUBULADURA, IDC_DTP_VENC, IDC_EDIT_ANTICOAG,
//   IDC_COMBO_BRAZO, IDC_CHK_DIFIC, IDC_EDIT_PESO_BOLSA, IDC_DTP_HORA_INI, IDC_DTP_HORA_FIN,
//   IDC_EDIT_INCIDENCIAS, IDC_STATIC_ERROR_EXT, IDOK ("Guardar extraccion"), IDCANCEL.
class CExtraccionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CExtraccionDlg)

public:
	CExtraccionDlg(CWnd* pParent = nullptr);
	virtual ~CExtraccionDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_EXTRACCION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // boton "Guardar extraccion"

	afx_msg void OnBnClickedBtnElegir();
	afx_msg void OnCbnSelchangeTipo();
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	void CargarTipos();
	void CargarDonante(const nlohmann::json& d);
	void Verificar();                               // consulta al servidor si puede donar y muestra el resultado
	void Error(const CString& mensaje, UINT idControl = 0);
	CString Texto(UINT id) const;
	int TipoElegido() const;                        // id del tipo de donacion elegido (0 si ninguno)
	CString NombreTipoElegido() const;
	nlohmann::json ArmarDatos() const;              // el cuerpo JSON de la donacion

	int m_idDonante = 0;
	COLORREF m_colorEstado = RGB(0, 0, 0);
	CComboBox m_tipo;
	CComboBox m_bolsa;
	CComboBox m_brazo;
	CDateTimeCtrl m_venc;
	CDateTimeCtrl m_horaIni;
	CDateTimeCtrl m_horaFin;

	DECLARE_MESSAGE_MAP()
};