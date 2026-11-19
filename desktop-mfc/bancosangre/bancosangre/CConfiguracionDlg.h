#pragma once
#include "afxdialogex.h"
#include <map>
#include <string>
#include "ApiClient.h"

// Panel de configuracion: la Jefa de Area/Gerente ajusta limites y requisitos sin tocar la base de datos.
// Controles esperados en IDD_CONFIGURACION:
//   IDC_LIST_CFG, IDC_STATIC_CFG_AYUDA, IDC_EDIT_CFG_VALOR, IDC_BTN_CFG_APLICAR,
//   IDC_STATIC_CFG_ESTADO, IDC_STATIC_CFG_ERROR, IDOK ("Guardar cambios"), IDCANCEL ("Cerrar").
class CConfiguracionDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CConfiguracionDlg)

public:
	CConfiguracionDlg(CWnd* pParent = nullptr);
	virtual ~CConfiguracionDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_CONFIGURACION };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual void OnOK();       // boton "Guardar cambios"
	virtual void OnCancel();   // boton "Cerrar" (avisa si quedan cambios sin guardar)

	afx_msg void OnBnClickedAplicar();
	afx_msg void OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);

	void Cargar();
	int IndiceElegido() const;                 // posicion dentro de m_params (-1 si no hay nada elegido)
	int ValorActual(int indice) const;         // valor con el cambio pendiente, si lo hay
	void ActualizarFila(int fila, int indice);
	void ActualizarEstado();
	void Error(const CString& mensaje);

	nlohmann::json m_params;
	std::map<std::string, int> m_pendientes;   // clave -> valor nuevo, todavia sin guardar
	CListCtrl m_lista;

	DECLARE_MESSAGE_MAP()
};