
// bancosangreDlg.cpp: archivo de implementación
//

#include "pch.h"
#include "framework.h"
#include "bancosangre.h"
#include "bancosangreDlg.h"
#include "afxdialogex.h"
#include "CDonantesDlg.h"
#include "CTurnosDlg.h"
#include "CEntrevistaDlg.h"
#include "CExtraccionDlg.h"
#include "Utilidades.h"
#include "ApiClient.h"
#include "CMensajesDlg.h"
#include "CHemo3Dlg.h"
#include "CConfiguracionDlg.h"
#include "CUsuariosDlg.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// Cuadro de diálogo CAboutDlg utilizado para el comando Acerca de

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// Datos del cuadro de diálogo
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // Compatibilidad con DDX/DDV

// Implementación
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cuadro de diálogo de CbancosangreDlg



CbancosangreDlg::CbancosangreDlg(CWnd* pParent /*= nullptr*/)
	: CDialogEx(IDD_BANCOSANGRE_DIALOG, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CbancosangreDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CbancosangreDlg, CDialogEx)
	ON_WM_TIMER()
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_DONANTES, &CbancosangreDlg::OnBnClickedBtnDonantes)
	ON_BN_CLICKED(IDC_BTN_TURNOS, &CbancosangreDlg::OnBnClickedBtnTurnos)
	ON_BN_CLICKED(IDC_BTN_ENTREVISTA, &CbancosangreDlg::OnBnClickedBtnEntrevista)
	ON_BN_CLICKED(IDC_BTN_EXTRACCION, &CbancosangreDlg::OnBnClickedBtnExtraccion)
	ON_BN_CLICKED(IDC_BTN_ESTADISTICAS, &CbancosangreDlg::OnBnClickedBtnEstadisticas)
	ON_BN_CLICKED(IDC_BTN_MENSAJES, &CbancosangreDlg::OnBnClickedBtnMensajes)
	ON_BN_CLICKED(IDC_BTN_HEMO3, &CbancosangreDlg::OnBnClickedBtnHemo3)
	ON_BN_CLICKED(IDC_BTN_CONFIG, &CbancosangreDlg::OnBnClickedBtnConfig)
	ON_BN_CLICKED(IDC_BTN_USUARIOS, &CbancosangreDlg::OnBnClickedBtnUsuarios)
END_MESSAGE_MAP()


// Controladores de mensajes de CbancosangreDlg

BOOL CbancosangreDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Agregar el elemento de menú "Acerca de..." al menú del sistema.

	// IDM_ABOUTBOX debe estar en el intervalo de comandos del sistema.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Establecer el icono para este cuadro de diálogo.  El marco de trabajo realiza esta operación
	//  automáticamente cuando la ventana principal de la aplicación no es un cuadro de diálogo
	SetIcon(m_hIcon, TRUE);			// Establecer icono grande
	SetIcon(m_hIcon, FALSE);		// Establecer icono pequeño

	// TODO: agregar aquí inicialización adicional
	CString nombre(CA2W(theApp.m_api.nombreCompleto().c_str(), CP_UTF8));
	CString rol(CA2W(theApp.m_api.rol().c_str(), CP_UTF8));
	CString titulo;
	titulo.Format(_T("Banco de Sangre - %s (%s)"), (LPCTSTR)nombre, (LPCTSTR)rol);
	SetWindowText(titulo);

	const std::string rolSesion = theApp.m_api.rol();
	const bool puedeVerEstadisticas = (rolSesion == "Jefa de \xC3\x81rea/Gerente" || rolSesion == "Administrador");
	GetDlgItem(IDC_BTN_ESTADISTICAS)->EnableWindow(puedeVerEstadisticas);

	ActualizarMensajes();
	SetTimer(1, 60000, nullptr);   // se vuelve a consultar cada minuto

	const bool puedeCargarHemo3 = (rolSesion == "Doctora" || rolSesion == "Entrevistadora/Administrativo"
		|| rolSesion == "Jefa de \xC3\x81rea/Gerente" || rolSesion == "Administrador");
	GetDlgItem(IDC_BTN_HEMO3)->EnableWindow(puedeCargarHemo3);

	GetDlgItem(IDC_BTN_CONFIG)->EnableWindow(puedeVerEstadisticas);

	GetDlgItem(IDC_BTN_USUARIOS)->EnableWindow(rolSesion == "Administrador");

	return TRUE;  // Devuelve TRUE  a menos que establezca el foco en un control
}

void CbancosangreDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// Si agrega un botón Minimizar al cuadro de diálogo, necesitará el siguiente código
//  para dibujar el icono.  Para aplicaciones MFC que utilicen el modelo de documentos y vistas,
//  esta operación la realiza automáticamente el marco de trabajo.

void CbancosangreDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // Contexto de dispositivo para dibujo

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Centrar icono en el rectángulo de cliente
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Dibujar el icono
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// El sistema llama a esta función para obtener el cursor que se muestra mientras el usuario arrastra
//  la ventana minimizada.
HCURSOR CbancosangreDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CbancosangreDlg::OnBnClickedBtnDonantes()
{
	CDonantesDlg dlg;
	dlg.DoModal();
}

void CbancosangreDlg::OnBnClickedBtnTurnos()
{
	CTurnosDlg dlg;
	dlg.DoModal();
}

void CbancosangreDlg::OnBnClickedBtnEntrevista()
{
	CEntrevistaDlg dlg;
	dlg.DoModal();
}

void CbancosangreDlg::OnBnClickedBtnExtraccion()
{
	CExtraccionDlg dlg;
	dlg.DoModal();
}

void CbancosangreDlg::OnBnClickedBtnEstadisticas()
{
	CString url = _T("http://localhost/bancosangre/estadisticas.html#token=")
		+ aTexto(theApp.m_api.token());
	ShellExecute(nullptr, _T("open"), url, nullptr, nullptr, SW_SHOWNORMAL);
}

void CbancosangreDlg::OnBnClickedBtnMensajes()
{
	CMensajesDlg dlg;
	dlg.DoModal();
	ActualizarMensajes();
}

void CbancosangreDlg::ActualizarMensajes()
{
	RespuestaApi r = theApp.m_api.listarMensajes(false);
	if (!r.ok() || !r.cuerpo.is_object()) {
		GetDlgItem(IDC_BTN_MENSAJES)->EnableWindow(FALSE);   // sin conexion o sin permiso (la Extractora no ve mensajes)
		SetDlgItemText(IDC_BTN_MENSAJES, _T("Mensajes"));
		return;
	}
	const int pendientes = r.cuerpo.value("total_pendientes", 0);
	CString texto = _T("Mensajes");
	if (pendientes > 0) texto.Format(_T("Mensajes (%d)"), pendientes);
	GetDlgItem(IDC_BTN_MENSAJES)->EnableWindow(TRUE);
	SetDlgItemText(IDC_BTN_MENSAJES, texto);
}

void CbancosangreDlg::OnTimer(UINT_PTR nIDEvent)
{
	if (nIDEvent == 1) ActualizarMensajes();
	CDialogEx::OnTimer(nIDEvent);
}
void CbancosangreDlg::OnBnClickedBtnHemo3()
{
	CHemo3Dlg dlg;
	dlg.DoModal();
}

void CbancosangreDlg::OnBnClickedBtnConfig()
{
	CConfiguracionDlg dlg(this);
	dlg.DoModal();
}

void CbancosangreDlg::OnBnClickedBtnUsuarios()
{
	CUsuariosDlg dlg(this);
	dlg.DoModal();
}
