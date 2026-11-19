#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Formulario para asignar un turno nuevo o reprogramar uno existente.
// Controles esperados en el recurso IDD_TURNO_FORM:
//   IDC_STATIC_DONANTE (texto con el donante elegido), IDC_BTN_ELEGIR_DONANTE,
//   IDC_COMBO_TIPO, IDC_DTP_FECHA_TURNO, IDC_DTP_HORA_TURNO, IDC_EDIT_LUGAR,
//   IDC_STATIC_ERROR_TURNO, IDOK ("Guardar") e IDCANCEL ("Cancelar").
class CTurnoFormDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTurnoFormDlg)

public:
	// turnoExistente == nullptr -> turno nuevo. Si no, reprogramacion de ese turno
	// (el JSON tal como lo devolvio la agenda).
	// fechaInicial ("AAAA-MM-DD") propone el dia en un turno nuevo.
	CTurnoFormDlg(const nlohmann::json* turnoExistente = nullptr,
		const std::string& fechaInicial = std::string(),
		CWnd* pParent = nullptr);
	virtual ~CTurnoFormDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TURNO_FORM };
#endif

	// Turno tal como quedo guardado en el servidor (valido si DoModal() devolvio IDOK).
	const nlohmann::json& Guardado() const { return m_guardado; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // boton "Guardar"

	afx_msg void OnBnClickedBtnElegirDonante();

	void MostrarDonante(const nlohmann::json& d);   // acepta un donante o un turno (ambos tienen apellido, nombre y dni)
	void CargarTipos(int idSeleccionado);
	void Error(const CString& mensaje);

	int m_idTurno = 0;      // 0 = turno nuevo
	int m_idDonante = 0;
	std::string m_fechaInicial;
	nlohmann::json m_original;   // turno actual (solo al reprogramar)
	nlohmann::json m_guardado;   // respuesta del servidor
	CComboBox m_tipo;
	CDateTimeCtrl m_fecha;
	CDateTimeCtrl m_hora;

	DECLARE_MESSAGE_MAP()
};