#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Entrevista medica previa a la donacion.
// Controles esperados en el recurso IDD_ENTREVISTA:
//   IDC_STATIC_DONANTE_ENT, IDC_BTN_ELEGIR_ENT,
//   IDC_STATIC_VIGENTES, IDC_LIST_VIGENTES,
//   IDC_EDIT_ENT_PESO, IDC_EDIT_ENT_ALTURA, IDC_EDIT_ENT_PLAQUETAS,
//   IDC_RADIO_APTO, IDC_RADIO_NOAPTO,
//   IDC_LIST_CAUSAS, IDC_DTP_INICIO, IDC_EDIT_ENT_OBS, IDC_STATIC_ERROR_ENT,
//   IDOK ("Guardar entrevista") e IDCANCEL ("Cancelar").
class CEntrevistaDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CEntrevistaDlg)

public:
	CEntrevistaDlg(CWnd* pParent = nullptr);
	virtual ~CEntrevistaDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ENTREVISTA };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // boton "Guardar entrevista"

	afx_msg void OnBnClickedBtnElegir();
	afx_msg void OnBnClickedAptitud();

	void CargarCatalogo();                          // causas de diferimiento (lista de abajo)
	void CargarDonante(const nlohmann::json& d);    // muestra al donante y busca sus restricciones vigentes
	void CargarVigentes();                          // restricciones vigentes (lista de arriba)
	void ActualizarAptitud();                       // las causas solo se usan si el donante NO es apto
	void Error(const CString& mensaje, UINT idControl = 0);
	CString Texto(UINT id) const;

	int m_idDonante = 0;
	nlohmann::json m_vigentes;   // restricciones vigentes del donante elegido
	nlohmann::json m_catalogo;   // causas de diferimiento disponibles
	CListCtrl m_listaVigentes;
	CListCtrl m_listaCausas;
	CDateTimeCtrl m_inicio;

	DECLARE_MESSAGE_MAP()
};