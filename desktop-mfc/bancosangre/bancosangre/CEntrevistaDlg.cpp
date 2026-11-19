#include "pch.h"
#include "BancoSangre.h"
#include "afxdialogex.h"
#include "Utilidades.h"
#include "CDonantesDlg.h"
#include "CEntrevistaDlg.h"

namespace {

	// Mensaje de error listo para mostrar, segun el tipo de falla
	CString mensajeDeError(const RespuestaApi& r)
	{
		if (!r.errorRed.empty())
			return _T("No se pudo conectar con el servidor. Avise a un administrador.");
		return aTexto(r.error());
	}

	// "AAAA-MM-DD" -> "DD/MM/AAAA" (vacio si no tiene ese formato)
	CString fechaCorta(const std::string& iso)
	{
		int y = 0, m = 0, d = 0;
		if (sscanf_s(iso.c_str(), "%d-%d-%d", &y, &m, &d) != 3) return CString();
		CString s;
		s.Format(_T("%02d/%02d/%04d"), d, m, y);
		return s;
	}

	// Plazo de una causa del catalogo: "180 dias" o "Hasta nueva evaluacion" (igual criterio que el servidor)
	CString plazoDe(const nlohmann::json& causa)
	{
		const bool reevaluacion = causa.value("requiere_reevaluacion", false);
		auto it = causa.find("dias_espera_minimos");
		CString s;
		if (reevaluacion || it == causa.end() || !it->is_number()) {
			s = _T("Hasta nueva evaluacion");
		}
		else {
			s.Format(_T("%d dias"), it->get<int>());
		}
		return s;
	}

	// Numero del JSON como texto ("72.5"); vacio si falta o es null.
	CString numeroTexto(const nlohmann::json& d, const char* clave)
	{
		auto it = d.find(clave);
		if (it == d.end() || !it->is_number()) return CString();
		CString s;
		s.Format(_T("%g"), it->get<double>());
		return s;
	}

	// Lee un numero con coma o punto decimal. Devuelve false si no es un numero.
	bool leerNumero(CString texto, double& valor)
	{
		texto.Replace(_T(','), _T('.'));
		if (texto.IsEmpty()) return false;
		LPTSTR fin = nullptr;
		valor = _tcstod(texto, &fin);
		return fin != nullptr && *fin == 0;
	}

	// Pone un texto de ayuda gris dentro de una caja vacia
	void ponerAyuda(CWnd* ventana, UINT id, const CString& texto)
	{
		ventana->SendDlgItemMessage(id, EM_SETCUEBANNER, TRUE, (LPARAM)(LPCTSTR)texto);
	}

}  // namespace

IMPLEMENT_DYNAMIC(CEntrevistaDlg, CDialogEx)

CEntrevistaDlg::CEntrevistaDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_ENTREVISTA, pParent)
{
}

CEntrevistaDlg::~CEntrevistaDlg()
{
}

void CEntrevistaDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_LIST_VIGENTES, m_listaVigentes);
	DDX_Control(pDX, IDC_LIST_CAUSAS, m_listaCausas);
	DDX_Control(pDX, IDC_DTP_INICIO, m_inicio);
}

BEGIN_MESSAGE_MAP(CEntrevistaDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_ELEGIR_ENT, &CEntrevistaDlg::OnBnClickedBtnElegir)
	ON_BN_CLICKED(IDC_RADIO_APTO, &CEntrevistaDlg::OnBnClickedAptitud)
	ON_BN_CLICKED(IDC_RADIO_NOAPTO, &CEntrevistaDlg::OnBnClickedAptitud)
END_MESSAGE_MAP()

BOOL CEntrevistaDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	SetWindowText(_T("Entrevista medica"));

	// Las dos listas llevan casillas para marcar con una tilde
	const DWORD estilo = LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES | LVS_EX_CHECKBOXES;

	m_listaVigentes.SetExtendedStyle(m_listaVigentes.GetExtendedStyle() | estilo);
	m_listaVigentes.InsertColumn(0, _T("Restriccion vigente"), LVCFMT_LEFT, 210);
	m_listaVigentes.InsertColumn(1, _T("Desde"), LVCFMT_LEFT, 80);
	m_listaVigentes.InsertColumn(2, _T("Hasta"), LVCFMT_LEFT, 130);

	m_listaCausas.SetExtendedStyle(m_listaCausas.GetExtendedStyle() | estilo);
	m_listaCausas.InsertColumn(0, _T("Causa de diferimiento"), LVCFMT_LEFT, 230);
	m_listaCausas.InsertColumn(1, _T("Plazo"), LVCFMT_LEFT, 140);

	// El plazo no puede empezar en el futuro: el selector no deja pasar de hoy
	SYSTEMTIME hoy;
	GetLocalTime(&hoy);
	SYSTEMTIME rango[2] = {};
	rango[1] = hoy;
	DateTime_SetRange(m_inicio.GetSafeHwnd(), GDTR_MAX, rango);
	m_inicio.SetTime(&hoy);

	ponerAyuda(this, IDC_EDIT_ENT_PESO, _T("kg de hoy"));
	ponerAyuda(this, IDC_EDIT_ENT_ALTURA, _T("cm"));
	ponerAyuda(this, IDC_EDIT_ENT_PLAQUETAS, _T("por mm3, ej: 210000"));

	SetDlgItemText(IDC_STATIC_DONANTE_ENT, _T("(ningun donante elegido)"));
	SetDlgItemText(IDC_STATIC_VIGENTES, _T("Primero elija al donante con el boton Elegir."));

	CargarCatalogo();
	ActualizarAptitud();   // sin ninguna opcion marcada, la lista de causas queda apagada
	return TRUE;
}

CString CEntrevistaDlg::Texto(UINT id) const
{
	CString s;
	GetDlgItemText(id, s);
	s.Trim();
	return s;
}

void CEntrevistaDlg::Error(const CString& mensaje, UINT idControl)
{
	SetDlgItemText(IDC_STATIC_ERROR_ENT, mensaje);
	if (idControl != 0) {
		CWnd* control = GetDlgItem(idControl);
		if (control != nullptr) {
			control->SetFocus();
			SendDlgItemMessage(idControl, EM_SETSEL, 0, -1);   // selecciona el texto para corregirlo rapido
		}
	}
}

void CEntrevistaDlg::CargarCatalogo()
{
	m_listaCausas.DeleteAllItems();
	m_catalogo = nlohmann::json::array();

	RespuestaApi r = theApp.m_api.listarRestricciones();
	if (!r.ok() || !r.cuerpo.is_object()) {
		Error(mensajeDeError(r));
		return;
	}

	m_catalogo = r.cuerpo.value("restricciones", nlohmann::json::array());
	int fila = 0;
	for (const auto& c : m_catalogo) {
		const int i = m_listaCausas.InsertItem(fila, aTexto(textoDe(c, "nombre")));
		m_listaCausas.SetItemText(i, 1, plazoDe(c));
		m_listaCausas.SetItemData(i, static_cast<DWORD_PTR>(fila));   // posicion dentro de m_catalogo
		++fila;
	}
}

void CEntrevistaDlg::OnBnClickedBtnElegir()
{
	CDonantesDlg dlg;
	dlg.SetModoSeleccion(true);   // la pantalla de busqueda, pero para elegir un donante
	if (dlg.DoModal() == IDOK) {
		CargarDonante(dlg.Elegido());
		Error(CString());   // limpia cualquier aviso anterior
	}
}

void CEntrevistaDlg::CargarDonante(const nlohmann::json& d)
{
	m_idDonante = d.value("id_donante", 0);

	CString texto;
	texto.Format(_T("%s, %s  (DNI %s)"),
		(LPCTSTR)aTexto(textoDe(d, "apellido")),
		(LPCTSTR)aTexto(textoDe(d, "nombre")),
		(LPCTSTR)aTexto(textoDe(d, "dni")));
	SetDlgItemText(IDC_STATIC_DONANTE_ENT, texto);

	// Peso y altura se miden en cada entrevista: las cajas quedan vacias para no aceptar un dato viejo.
	// El ultimo valor registrado se muestra solo como ayuda gris.
	SetDlgItemText(IDC_EDIT_ENT_PESO, _T(""));
	SetDlgItemText(IDC_EDIT_ENT_ALTURA, _T(""));
	SetDlgItemText(IDC_EDIT_ENT_PLAQUETAS, _T(""));
	const CString pesoViejo = numeroTexto(d, "peso");
	const CString alturaViejo = numeroTexto(d, "altura");
	ponerAyuda(this, IDC_EDIT_ENT_PESO, pesoViejo.IsEmpty() ? CString(_T("kg de hoy")) : CString(_T("ultimo: ")) + pesoViejo);
	ponerAyuda(this, IDC_EDIT_ENT_ALTURA, alturaViejo.IsEmpty() ? CString(_T("cm")) : CString(_T("ultima: ")) + alturaViejo);

	// Las causas marcadas para otro donante no se arrastran
	for (int i = 0; i < m_listaCausas.GetItemCount(); ++i) {
		m_listaCausas.SetCheck(i, FALSE);
	}

	CargarVigentes();
}

void CEntrevistaDlg::CargarVigentes()
{
	m_listaVigentes.DeleteAllItems();
	m_vigentes = nlohmann::json::array();

	RespuestaApi r = theApp.m_api.listarRestriccionesDonante(m_idDonante, true);
	if (!r.ok() || !r.cuerpo.is_object()) {
		SetDlgItemText(IDC_STATIC_VIGENTES, mensajeDeError(r));
		return;
	}

	m_vigentes = r.cuerpo.value("restricciones", nlohmann::json::array());

	int fila = 0;
	for (const auto& v : m_vigentes) {
		const int i = m_listaVigentes.InsertItem(fila, aTexto(textoDe(v, "nombre")));
		m_listaVigentes.SetItemText(i, 1, fechaCorta(textoDe(v, "fecha_inicio_restriccion")));

		const std::string hasta = textoDe(v, "fecha_habilitacion_calculada");
		CString textoHasta;
		if (hasta.empty())
			textoHasta = _T("Nueva evaluacion");
		else
			textoHasta = fechaCorta(hasta);
		m_listaVigentes.SetItemText(i, 2, textoHasta);

		m_listaVigentes.SetItemData(i, static_cast<DWORD_PTR>(fila));   // posicion dentro de m_vigentes
		++fila;
	}

	if (fila == 0)
		SetDlgItemText(IDC_STATIC_VIGENTES, _T("El donante no tiene restricciones vigentes."));
	else
		SetDlgItemText(IDC_STATIC_VIGENTES, _T("Restricciones vigentes: marque con una tilde las que se levantan en esta evaluacion."));
}

void CEntrevistaDlg::OnBnClickedAptitud()
{
	ActualizarAptitud();
}

void CEntrevistaDlg::ActualizarAptitud()
{
	// Las causas de diferimiento (y desde cuando corre el plazo) solo se usan si el donante NO es apto
	const bool noApto = (IsDlgButtonChecked(IDC_RADIO_NOAPTO) == BST_CHECKED);
	m_listaCausas.EnableWindow(noApto);
	m_inicio.EnableWindow(noApto);
}

void CEntrevistaDlg::OnOK()
{
	Error(CString());

	// ---- 1) Validacion en pantalla (el servidor vuelve a validar todo) ----
	if (m_idDonante <= 0) {
		Error(_T("Elija el donante con el boton Elegir."), IDC_BTN_ELEGIR_ENT);
		return;
	}

	const bool apto = (IsDlgButtonChecked(IDC_RADIO_APTO) == BST_CHECKED);
	const bool noApto = (IsDlgButtonChecked(IDC_RADIO_NOAPTO) == BST_CHECKED);
	if (!apto && !noApto) {
		Error(_T("Indique si el donante es APTO o NO APTO."), IDC_RADIO_APTO);
		return;
	}

	double valor = 0;
	const CString peso = Texto(IDC_EDIT_ENT_PESO);
	if (!peso.IsEmpty() && (!leerNumero(peso, valor) || valor < 20 || valor > 300)) {
		Error(_T("El peso debe ser un numero entre 20 y 300 (en kg)."), IDC_EDIT_ENT_PESO);
		return;
	}
	const CString altura = Texto(IDC_EDIT_ENT_ALTURA);
	if (!altura.IsEmpty() && (!leerNumero(altura, valor) || valor < 100 || valor > 250)) {
		Error(_T("La altura debe ser un numero entre 100 y 250 (en cm)."), IDC_EDIT_ENT_ALTURA);
		return;
	}

	// Plaquetas: solo digitos (se aceptan puntos y espacios, como en 210.000)
	CString plaquetas = Texto(IDC_EDIT_ENT_PLAQUETAS);
	plaquetas.Remove(_T('.'));
	plaquetas.Remove(_T(' '));
	if (!plaquetas.IsEmpty()) {
		bool soloDigitos = (plaquetas.GetLength() <= 7);
		for (int i = 0; soloDigitos && i < plaquetas.GetLength(); ++i) {
			soloDigitos = (plaquetas[i] >= '0' && plaquetas[i] <= '9');
		}
		if (!soloDigitos || _ttoi(plaquetas) < 1000 || _ttoi(plaquetas) > 2000000) {
			Error(_T("El recuento de plaquetas debe ser un numero entero, por ejemplo 210000."), IDC_EDIT_ENT_PLAQUETAS);
			return;
		}
	}

	// Restricciones vigentes: las marcadas se levantan; el resto sigue vigente
	nlohmann::json levantar = nlohmann::json::array();
	int sinLevantar = 0;
	for (int i = 0; i < m_listaVigentes.GetItemCount(); ++i) {
		const size_t indice = static_cast<size_t>(m_listaVigentes.GetItemData(i));
		if (indice >= m_vigentes.size()) continue;
		if (m_listaVigentes.GetCheck(i))
			levantar.push_back(m_vigentes[indice].value("id_donante_restriccion", 0));
		else
			++sinLevantar;
	}

	nlohmann::json causas = nlohmann::json::array();

	if (apto) {
		if (peso.IsEmpty()) {
			Error(_T("Para marcar APTO hace falta el peso de hoy."), IDC_EDIT_ENT_PESO);
			return;
		}
		if (plaquetas.IsEmpty()) {
			Error(_T("Para marcar APTO hace falta el recuento de plaquetas del analisis previo."), IDC_EDIT_ENT_PLAQUETAS);
			return;
		}
		if (sinLevantar > 0) {
			Error(_T("El donante tiene restricciones vigentes. Marque con una tilde las que se levantan, o elija NO APTO."));
			return;
		}
	}
	else {
		SYSTEMTIME inicio;
		m_inicio.GetTime(&inicio);
		CString inicioIso;
		inicioIso.Format(_T("%04d-%02d-%02d"), inicio.wYear, inicio.wMonth, inicio.wDay);

		for (int i = 0; i < m_listaCausas.GetItemCount(); ++i) {
			if (!m_listaCausas.GetCheck(i)) continue;
			const size_t indice = static_cast<size_t>(m_listaCausas.GetItemData(i));
			if (indice >= m_catalogo.size()) continue;
			causas.push_back({
				{"id_restriccion", m_catalogo[indice].value("id_restriccion", 0)},
				{"fecha_inicio", aUtf8(inicioIso)}
				});
		}
		if (causas.empty() && sinLevantar == 0) {
			Error(_T("Elija con una tilde al menos una causa de diferimiento."));
			return;
		}
	}

	// ---- 2) Envio al servidor ----
	nlohmann::json datos = {
		{"id_donante", m_idDonante},
		{"apto", apto},
		{"peso", aUtf8(peso)},
		{"altura", aUtf8(altura)},
		{"recuento_plaquetas", aUtf8(plaquetas)},
		{"observaciones", aUtf8(Texto(IDC_EDIT_ENT_OBS))},
		{"levantar", levantar},
		{"restricciones", causas}
	};

	CWaitCursor espera;
	RespuestaApi r = theApp.m_api.guardarEntrevista(datos);

	if (r.ok() && r.cuerpo.is_object() && r.cuerpo.value("ok", false)) {
		// Confirmacion: el resultado y, si queda diferido, desde cuando puede volver a donar
		CString resumen = aTexto(r.cuerpo.value("mensaje", std::string()));

		const nlohmann::json vigentes = r.cuerpo.value("restricciones_vigentes", nlohmann::json::array());
		if (!vigentes.empty()) {
			resumen += _T("\r\n\r\nRestricciones vigentes:");
			for (const auto& v : vigentes) {
				const std::string hasta = textoDe(v, "fecha_habilitacion_calculada");
				CString linea;
				if (hasta.empty())
					linea.Format(_T("\r\n- %s: hasta nueva evaluacion"), (LPCTSTR)aTexto(textoDe(v, "nombre")));
				else
					linea.Format(_T("\r\n- %s: puede volver a donar desde el %s"),
						(LPCTSTR)aTexto(textoDe(v, "nombre")), (LPCTSTR)fechaCorta(hasta));
				resumen += linea;
			}
		}
		AfxMessageBox(resumen, MB_ICONINFORMATION);
		CDialogEx::OnOK();   // guardado: se cierra la ventana
		return;
	}

	// ---- 3) El servidor la rechazo: se explica y la ventana queda abierta ----
	CString mensaje = mensajeDeError(r);
	if (r.codigoHttp == 422 && r.cuerpo.is_object()) {
		auto it = r.cuerpo.find("campos");
		if (it != r.cuerpo.end() && it->is_object() && !it->empty()) {
			auto primero = it->begin();
			if (primero.value().is_string()) {
				mensaje = aTexto(primero.value().get<std::string>());
			}
		}
	}
	Error(mensaje);
	AfxMessageBox(mensaje, MB_ICONWARNING);   // ventana emergente: no se puede pasar por alto
}