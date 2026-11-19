#pragma once
#include "afxdialogex.h"
#include "ApiClient.h"

// Formulario para registrar (alta) o modificar un donante.
// Controles esperados en el recurso IDD_DONANTE_FORM:
//   IDC_EDIT_DNI, IDC_EDIT_APELLIDO, IDC_EDIT_NOMBRE, IDC_EDIT_NACIMIENTO, IDC_COMBO_SEXO,
//   IDC_EDIT_DOMICILIO, IDC_EDIT_TELEFONO, IDC_EDIT_EMAIL, IDC_EDIT_PESO, IDC_EDIT_ALTURA,
//   IDC_STATIC_ERROR, IDOK ("Guardar") e IDCANCEL ("Cancelar").
class CDonanteFormDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CDonanteFormDlg)

public:
	// donanteExistente == nullptr -> alta. Si no, modificacion de ese donante
	// (el JSON tal como lo devolvio el servidor).
	CDonanteFormDlg(const nlohmann::json* donanteExistente = nullptr, CWnd* pParent = nullptr);
	virtual ~CDonanteFormDlg();

#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DONANTE_FORM };
#endif

	// Donante tal como quedo guardado en el servidor (valido si DoModal() devolvio IDOK).
	const nlohmann::json& Guardado() const { return m_guardado; }

protected:
	virtual void DoDataExchange(CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();   // boton "Guardar"

	void CargarDatos(const nlohmann::json& d);
	void Error(UINT idControl, const CString& mensaje);   // muestra el mensaje y pone el foco en el campo
	CString Texto(UINT id) const;

	int m_idDonante = 0;          // 0 = alta
	nlohmann::json m_original;    // datos actuales (solo en modificacion)
	nlohmann::json m_guardado;    // respuesta del servidor
	CComboBox m_sexo;

	DECLARE_MESSAGE_MAP()
};