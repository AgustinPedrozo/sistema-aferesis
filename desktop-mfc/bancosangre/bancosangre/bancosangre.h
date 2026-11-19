
// bancosangre.h: archivo de encabezado principal para la aplicación PROJECT_NAME
//

#pragma once

#ifndef __AFXWIN_H__
	#error "incluir 'pch.h' antes de incluir este archivo para PCH"
#endif

#include "resource.h"		// Símbolos principales
#include "ApiClient.h"


// CbancosangreApp:
// Consulte bancosangre.cpp para obtener información sobre la implementación de esta clase
//

class CbancosangreApp : public CWinApp
{
public:
	CbancosangreApp();

	ApiClient m_api{ "http://localhost/bancosangre/api" };

// Reemplazos
public:
	virtual BOOL InitInstance();

// Implementación

	DECLARE_MESSAGE_MAP()
};

extern CbancosangreApp theApp;
