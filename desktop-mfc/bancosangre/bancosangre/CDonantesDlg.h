#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Pantalla de busqueda de donantes: alta, modificacion, baja y reactivacion.
// Tambien sirve para ELEGIR un donante (modo seleccion), por ejemplo al asignar un turno.
// Controles esperados en el recurso IDD_DONANTES:
//   IDC_EDIT_BUSCAR, IDC_CHK_INACTIVOS, IDC_LIST_DONANTES, IDC_STATIC_RESULTADO,
//   IDC_BTN_NUEVO, IDC_BTN_MODIFICAR, IDC_BTN_ESTADO, IDC_BTN_ELEGIR,
//   IDOK (boton "Buscar") e IDCANCEL (boton "Cerrar").
class CDonantesDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDonantesDlg)

public:
	CDonantesDlg(CWnd* pParent = nullptr);
	virtual ~CDonantesDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DONANTES };
#endif

	// Modo "elegir donante": llamar ANTES de DoModal(). Al elegir, la ventana se cierra con IDOK
	// y el donante elegido se obtiene con Elegido().
	void SetModoSeleccion(bool activo) { m_modoSeleccion = activo; }
	const nlohmann::json& Elegido() const { return m_elegido; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // el boton "Buscar" (IDOK) busca y NO cierra la ventana

	afx_msg void OnBnClickedBtnNuevo();
	afx_msg void OnBnClickedBtnModificar();
	afx_msg void OnBnClickedBtnEstado();
	afx_msg void OnBnClickedBtnElegir();
	afx_msg void OnNMDblclkLista(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult);

	void Buscar();
	void MostrarResultados(const nlohmann::json& donantes);
	void MostrarDonante(const nlohmann::json& donante);   // busca por su DNI para dejarlo a la vista
	void ModificarSeleccionado();
	void ElegirSeleccionado();
	void ActualizarBotones();                              // habilita/deshabilita y rotula los botones segun la seleccion
	bool SeleccionActual(int& fila, size_t& indice) const; // fila de la lista y posicion en m_donantes

	CListCtrl m_lista;
	nlohmann::json m_donantes;   // resultados de la ultima busqueda
	nlohmann::json m_elegido;    // donante elegido (modo seleccion)
	bool m_modoSeleccion = false;

	DECLARE_MESSAGE_MAP()
};