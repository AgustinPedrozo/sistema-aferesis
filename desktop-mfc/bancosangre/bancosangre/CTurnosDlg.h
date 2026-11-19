#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Agenda de turnos de un dia, con alta, reprogramacion, cancelacion y ausentes.
// Controles esperados en el recurso IDD_TURNOS:
//   IDC_DTP_FECHA (Date Time Picker), IDC_LIST_TURNOS, IDC_STATIC_TURNOS,
//   IDC_BTN_NUEVO_TURNO, IDC_BTN_REPROGRAMAR, IDC_BTN_CANCELAR_TURNO, IDC_BTN_AUSENTE,
//   IDOK (boton "Actualizar") e IDCANCEL (boton "Cerrar").
class CTurnosDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTurnosDlg)

public:
	CTurnosDlg(CWnd* pParent = nullptr);
	virtual ~CTurnosDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TURNOS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // el boton "Actualizar" (IDOK) recarga la agenda y NO cierra la ventana

	afx_msg void OnDtnDatetimechangeFecha(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnLvnItemchangedLista(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnBnClickedBtnNuevoTurno();
	afx_msg void OnBnClickedBtnReprogramar();
	afx_msg void OnBnClickedBtnCancelarTurno();
	afx_msg void OnBnClickedBtnAusente();

	void Cargar();                           // agenda del dia que muestra el selector de fecha
	void Cargar(const SYSTEMTIME& dia);      // agenda de un dia dado
	void MostrarTurnoGuardado(const nlohmann::json& turno, const CString& mensaje);   // salta a su dia y lo deja seleccionado
	void CambiarEstado(const char* estadoNuevo, const CString& pregunta, const CString& exito);
	void ActualizarBotones();                // acciones sobre el turno elegido: solo si esta pendiente
	bool SeleccionActual(int& fila, size_t& indice) const;

	CListCtrl m_lista;
	CDateTimeCtrl m_fecha;
	nlohmann::json m_turnos;   // turnos del dia mostrado

	DECLARE_MESSAGE_MAP()
};