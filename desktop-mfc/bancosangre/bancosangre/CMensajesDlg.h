#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Mensajes de WhatsApp: respuestas de los donantes a los recordatorios.
// Controles esperados en IDD_MENSAJES:
//   IDC_CHK_TODOS_MSJ, IDC_STATIC_MSJ_RESUMEN, IDC_LIST_MENSAJES, IDC_EDIT_MSJ_DETALLE,
//   IDC_BTN_ATENDER, IDC_BTN_ENVIAR_AHORA, IDC_BTN_SIMULAR, IDCANCEL ("Cerrar").
class CMensajesDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CMensajesDlg)

public:
	CMensajesDlg(CWnd* pParent = nullptr);
	virtual ~CMensajesDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_MENSAJES };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();

	afx_msg void OnBnClickedChkTodos();
	afx_msg void OnBnClickedBtnAtender();
	afx_msg void OnBnClickedBtnEnviarAhora();
	afx_msg void OnBnClickedBtnSimular();
	afx_msg void OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult);

	void Cargar();                       // pide la lista al servidor y la muestra
	void MostrarDetalle();               // texto completo del mensaje elegido
	void ActualizarBotones();
	int IndiceElegido() const;           // posicion dentro de m_mensajes (-1 si no hay nada elegido)
	bool EsPendiente(const nlohmann::json& m) const;

	nlohmann::json m_mensajes;
	CListCtrl m_lista;

	DECLARE_MESSAGE_MAP()
};