#include "gui.h"
#include "guidebug.h"
#include "guiapp.h"
#include "wizard/romwizard.h"
#include "calc.h"
#include "guiopenfile.h"
#include "keys.h"
#include "sendfile.h"
#include "wabbiticon.xpm"

#include "skins/ti73.h"
#include "skins/ti81.h"
#include "skins/ti82.h"
#include "skins/ti83.h"
#include "skins/ti83p.h"
#include "skins/ti83pse.h"
#include "skins/ti84p.h"
#include "skins/ti84pse.h"
#include "skins/ti85.h"
#include "skins/ti86.h"

#include "gif.h"
#include "gifhandle.h"
#include "fileutilities.h"

#define BIG_WINDOWS_ICON 0
#ifndef max
#define max(a, b)  (((a) > (b)) ? (a) : (b))
#endif
#ifndef min
#define min(a, b)  (((a) < (b)) ? (a) : (b))
#endif

bool gif_anim_advance;
bool silent_mode = false;
int prevCalcScale = 2; // restore the default scale the first time skins are disabled
enum
{
	ID_File_New,
	ID_File_Open,
	ID_File_Save,
	ID_File_Gif,
	ID_File_Close,
	ID_File_Quit,

	ID_View_Skin,
	ID_View_Vars,

	ID_Calc_Copy,
	ID_Calc_Sound,
	ID_Calc_Pause,
	ID_Calc_Connect,
	ID_Calc_Options,
	
	ID_Speed_400,
	ID_Speed_500,
	ID_Speed_200,
	ID_Speed_100,
	ID_Speed_50,
	ID_Speed_25,
	ID_Speed_Custom,
	
	ID_Size_100,
	ID_Size_200,
	ID_Size_300,
	ID_Size_400,
	
	ID_Debug_Reset,
	ID_Debug_Open,
	ID_Debug_On,
	
	ID_Help_Setup,
	ID_Help_About,
	ID_Help_Website
};

BEGIN_EVENT_TABLE(NabbitemuFrame, wxFrame)
	EVT_PAINT(NabbitemuFrame::OnPaint)
	EVT_SIZE(NabbitemuFrame::OnResize)
	EVT_MENU(ID_File_New, NabbitemuFrame::OnFileNew)
	EVT_MENU(ID_File_Open, NabbitemuFrame::OnFileOpen)
	EVT_MENU(ID_File_Save, NabbitemuFrame::OnFileSave)
	EVT_MENU(ID_File_Close, NabbitemuFrame::OnFileClose)
	EVT_MENU(ID_File_Gif, NabbitemuFrame::OnFileGIF)
	EVT_MENU(ID_File_Quit, NabbitemuFrame::OnFileQuit)
	
	EVT_MENU(ID_Calc_Pause, NabbitemuFrame::OnPauseEmulation)
	EVT_MENU(ID_View_Skin, NabbitemuFrame::OnViewSkin)
	EVT_MENU(ID_View_Vars, NabbitemuFrame::OnViewVariables)
	EVT_MENU(ID_Speed_Custom, NabbitemuFrame::OnSetSpeedCustom)
	EVT_MENU(ID_Speed_500, NabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_400, NabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_200, NabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_100, NabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_50, NabbitemuFrame::OnSetSpeed)
	EVT_MENU(ID_Speed_25, NabbitemuFrame::OnSetSpeed)
	
	EVT_MENU(ID_Size_100, NabbitemuFrame::OnSetSize)
	EVT_MENU(ID_Size_200, NabbitemuFrame::OnSetSize)
	EVT_MENU(ID_Size_300, NabbitemuFrame::OnSetSize)
	EVT_MENU(ID_Size_400, NabbitemuFrame::OnSetSize)
	
	EVT_MENU(ID_Debug_Reset, NabbitemuFrame::OnDebugReset)
	EVT_MENU(ID_Debug_Open, NabbitemuFrame::OnDebugOpen)
	EVT_MENU(ID_Debug_On, NabbitemuFrame::OnDebugOn)
	
	EVT_MENU(ID_Help_Setup, NabbitemuFrame::OnHelpSetup)
	EVT_MENU(ID_Help_Website, NabbitemuFrame::OnHelpWebsite)
	EVT_MENU(ID_Help_About, NabbitemuFrame::OnHelpAbout)
	
	EVT_KEY_DOWN(NabbitemuFrame::OnKeyDown)
	EVT_KEY_UP(NabbitemuFrame::OnKeyUp)
	EVT_CLOSE(NabbitemuFrame::OnQuit)
	
	EVT_LEFT_DOWN(NabbitemuFrame::OnLeftButtonDown)
	EVT_LEFT_UP(NabbitemuFrame::OnLeftButtonUp)
END_EVENT_TABLE()

IMPLEMENT_APP(NabbitemuApp)

inline wxBitmap wxGetBitmapFromMemory(const unsigned char *data, int length) {
   wxMemoryInputStream is(data, length);
   return wxBitmap(wxImage(is, wxBITMAP_TYPE_PNG, -1), -1);
}

void gui_debug(LPCALC lpCalc) {
	wxWindow *oldDebugger = wxWindow::FindWindowById(wxDEBUGGERID);
	if (oldDebugger) {
		oldDebugger->Show();
		return;
	}
	lpCalc->running = FALSE;
	calc_pause_linked();
	NabbitemuDebugger *debugger = new NabbitemuDebugger(NULL, lpCalc);
	debugger->Show();
}

int NabbitemuFrame::gui_draw() {
	// Refresh instead of Update: paint events coalesce with the display, so a
	// slow frame no longer blocks the timer and redraws twice (OnPaint used to
	// force another synchronous update in skin mode).
	wxLCD->Refresh(false);

	if (lpCalc->gif_disp_state != GDS_IDLE) {
		static int skip = 0;
		if (skip == 0) {
			gif_anim_advance = true;
			this->Update();
		}
		skip = (skip + 1) % 4;
	}
	return 0;
}

extern NabbitemuFrame *frames[MAX_CALCS];
NabbitemuFrame * gui_frame(LPCALC lpCalc) {
	if (!lpCalc->scale) {
    	lpCalc->scale = 2; //Set original scale
	}
    
	NabbitemuFrame *mainFrame = new NabbitemuFrame(lpCalc);
	mainFrame->Show(true);
	frames[lpCalc->slot] = mainFrame;

	mainFrame->wxLCD = new NabbitemuLCD(mainFrame, lpCalc);
	mainFrame->wxLCD->Show(true);
	if (lpCalc->SkinEnabled) {
		mainFrame->wxLCD->Move((mainFrame->GetClientSize().GetWidth() - lpCalc->cpu.pio.lcd->width * lpCalc->scale) / 2, 0);
	} else {
		mainFrame->wxLCD->Move(lpCalc->LCDRect.GetX(), lpCalc->LCDRect.GetY());
	}
	
	lpCalc->running = TRUE;
	lpCalc->breakpoint_callback = gui_debug;
	mainFrame->SetSpeed(100);
	
	mainFrame->Center();   //Centres the frame
	
	mainFrame->gui_frame_update();
	return mainFrame;
}

void NabbitemuFrame::gui_frame_update() {
	wxMenuBar *wxMenu = this->GetMenuBar();
	switch(lpCalc->model) {
		case TI_81:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_81_png, sizeof(TI_81_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_81Keymap_png, sizeof(TI_81Keymap_png)).ConvertToImage();
			break;
		case TI_82:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_82_png, sizeof(TI_82_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_82Keymap_png, sizeof(TI_82Keymap_png)).ConvertToImage();
			break;
		case TI_83:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_83_png, sizeof(TI_83_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_83Keymap_png, sizeof(TI_83Keymap_png)).ConvertToImage();
			break;
		case TI_84P:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_84P_png, sizeof(TI_84P_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_84PKeymap_png, sizeof(TI_84PKeymap_png)).ConvertToImage();
			break;
		case TI_84PSE:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_84PSE_png, sizeof(TI_84PSE_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_84PKeymap_png, sizeof(TI_84PKeymap_png)).ConvertToImage();
			break;
		case TI_85:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_85_png, sizeof(TI_85_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_85Keymap_png, sizeof(TI_85Keymap_png)).ConvertToImage();
			break;
		case TI_86:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_86_png, sizeof(TI_86_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_86Keymap_png, sizeof(TI_86Keymap_png)).ConvertToImage();
			break;
		case TI_73:
		case TI_83P:
		default:
			lpCalc->calcSkin = wxGetBitmapFromMemory(TI_83P_png, sizeof(TI_83P_png)).ConvertToImage();
			lpCalc->keymap = wxGetBitmapFromMemory(TI_83PKeymap_png, sizeof(TI_83PKeymap_png)).ConvertToImage();
			break;
	}
	if (lpCalc->calcSkin.IsOk()) {
		// Keep a ready-to-draw bitmap: drawing the wxImage directly re-converts
		// the whole 350x725 skin on every paint event.
		m_skinBitmap = wxBitmap(lpCalc->calcSkin);
	}
	int skinWidth, skinHeight, keymapWidth, keymapHeight;
	
	if (lpCalc->calcSkin.IsOk()) {
		skinWidth = 350;//lpCalc->calcSkin.GetWidth();
		skinHeight = 725;//lpCalc->calcSkin.GetHeight();
	}
	if (lpCalc->keymap.IsOk()) {
		keymapWidth = 350;//lpCalc->keymap.GetWidth();
		keymapHeight = 725;//lpCalc->keymap.GetHeight();
	}
	int foundX = 0, foundY = 0;
	bool foundScreen = false;
	if (((skinWidth != keymapWidth) || (skinHeight != keymapHeight)) && skinHeight > 0 && skinWidth > 0) {
		lpCalc->SkinEnabled = false;
		wxMessageBox(wxT("Skin and Keymap are not the same size"), wxT("Error"),  wxOK, NULL);
	} else {
		lpCalc->SkinSize.SetWidth(skinWidth);
		lpCalc->SkinSize.SetHeight(skinHeight);		//find the screen size
		for(int y = 0; y < skinHeight && foundScreen == false; y++) {
			for (int x = 0; x < skinWidth && foundScreen == false; x++) {
				if (lpCalc->keymap.GetBlue(x, y) == 0 &&
						lpCalc->keymap.GetRed(x, y) == 255 &&
						lpCalc->keymap.GetGreen(x, y) == 0) {
					//81 92
					foundX = x;
					foundY = y;
					foundScreen = true;
				}
			}
		}
		lpCalc->LCDRect.SetLeft(foundX);
		lpCalc->LCDRect.SetTop(foundY);
		do {
			foundX++;
		} while (lpCalc->keymap.GetBlue(foundX, foundY) == 0 &&
				lpCalc->keymap.GetRed(foundX, foundY) == 255 &&
				lpCalc->keymap.GetGreen(foundX, foundY) == 0);
		lpCalc->LCDRect.SetRight(--foundX);
		do {
			foundY++;
		}while (lpCalc->keymap.GetBlue(foundX, foundY) == 0 &&
				lpCalc->keymap.GetRed(foundX, foundY) == 255 &&
				lpCalc->keymap.GetGreen(foundX, foundY) == 0);
		lpCalc->LCDRect.SetBottom(--foundY);
	}
	if (!foundScreen) {
		wxMessageBox(wxT("Unable to find the screen box"), wxT("Error"), wxOK, NULL);
		lpCalc->SkinEnabled = false;
	}

	if (!lpCalc->SkinEnabled) {
		if (wxMenu != NULL) {
			wxMenu->Check(ID_View_Skin, false);
		}
		// Create status bar
		wxStatusBar *wxStatus = this->GetStatusBar();
		if (wxStatus == NULL) {
			wxStatus = this->CreateStatusBar(2, wxST_SIZEGRIP, wxID_ANY );
		}
		const int iStatusWidths[] = {100, -1};
		wxStatus->SetFieldsCount(2, iStatusWidths);
		wxStatus->SetStatusText(wxEmptyString);
		wxStatus->SetStatusText(CalcModelTxt[lpCalc->model], 1);
		
		wxSize skinSize(128*lpCalc->scale, 64*lpCalc->scale);
		this->SetClientSize(skinSize);
	} else {
		if (wxMenu != NULL) {
			wxMenu->Check(ID_View_Skin, true);
		}
		wxStatusBar *wxStatus = this->GetStatusBar();
		if (wxStatus != NULL) {
			wxStatus->Destroy();
		}
		wxSize skinSize(350, 725);
		this->SetClientSize(skinSize);
	}
	wxLCD->SetClientSize(lpCalc->LCDRect.GetSize());
	wxLCD->Raise();
	this->SendSizeEvent();
}

NabbitemuFrame::NabbitemuFrame(LPCALC lpCalc) : wxFrame(NULL, wxID_ANY, wxT("Nabbitemu"))
{
	this->lpCalc = lpCalc;
	this->skinWindow = new SkinWindow(this, lpCalc);
	
	this->SetWindowStyleFlag(wxBORDER_RAISED);
	wxSize skinSize(350, 725);
	lpCalc->SkinSize = skinSize;
	LCD_t *lcd = lpCalc->cpu.pio.lcd;
	int scale = lpCalc->scale;
	int draw_width = lcd->width * scale;
	int draw_height = 64 * scale;
	wxRect lcdRect((128 * scale - draw_width) / 2, 0, draw_width, draw_height);
	lpCalc->LCDRect = lcdRect;

	wxSize windowSize;
	this->SetSizeHints( wxDefaultSize, wxDefaultSize );

	wxMenuBar *m_menubar = new wxMenuBar( 0 );
	wxMenu *m_fileMenu = new wxMenu();	
	wxMenuItem* m_newMenuItem;
	m_newMenuItem = new wxMenuItem( m_fileMenu, ID_File_New, wxString( wxT("New") ) + wxT('\t') + wxT("CTRL+N"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_newMenuItem );
	
	wxMenuItem* m_openMenuItem;
	m_openMenuItem = new wxMenuItem( m_fileMenu, ID_File_Open, wxString( wxT("Open...") ) + wxT('\t') + wxT("CTRL+O"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_openMenuItem );
	
	wxMenuItem* m_saveMenuItem;
	m_saveMenuItem = new wxMenuItem( m_fileMenu, ID_File_Save, wxString( wxT("Save State...") ) , wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_saveMenuItem );
	
	wxMenuItem* m_separator1;
	m_separator1 = m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_gifMenuItem;
	m_gifMenuItem = new wxMenuItem( m_fileMenu, ID_File_Gif, wxString( wxT("Record GIF") ) + wxT('\t') + wxT("Backspace"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_gifMenuItem );
	
	wxMenuItem* m_separator2;
	m_separator2 = m_fileMenu->AppendSeparator();
	
	wxMenuItem* m_closeMenuItem;
	m_closeMenuItem = new wxMenuItem( m_fileMenu, ID_File_Close, wxString( wxT("Close") ) + wxT('\t') + wxT("CTRL+W"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_closeMenuItem );
	
	wxMenuItem* m_exitMenuItem;
	m_exitMenuItem = new wxMenuItem( m_fileMenu, ID_File_Quit, wxString( wxT("Exit") ) + wxT('\t') + wxT("ALT+F4"), wxEmptyString, wxITEM_NORMAL );
	m_fileMenu->Append( m_exitMenuItem );
	
	m_menubar->Append( m_fileMenu, wxT("File") );
	
	wxMenu *m_viewMenu = new wxMenu();
	
	wxMenuItem* m_skinMenuItem;
	m_skinMenuItem = new wxMenuItem( m_viewMenu, ID_View_Skin, wxString( wxT("Enable Skin") ) , wxEmptyString, wxITEM_CHECK );
	m_viewMenu->Append( m_skinMenuItem );
	
	wxMenuItem* m_variablesMenuItem;
	m_variablesMenuItem = new wxMenuItem( m_viewMenu, ID_View_Vars, wxString( wxT("Variables")), wxEmptyString, wxITEM_NORMAL );
	m_viewMenu->Append( m_variablesMenuItem );this->SetSizeHints( wxDefaultSize, wxDefaultSize );
	
	m_menubar->Append( m_viewMenu, wxT("View") );
	
	wxMenu *m_calcMenu = new wxMenu();	
	wxMenuItem* m_soundMenuItem;
	m_soundMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Sound, wxString( wxT("Enable Sound") ) , wxEmptyString, wxITEM_CHECK );
	m_calcMenu->Append( m_soundMenuItem );
	
	wxMenuItem* m_separator3;
	m_separator3 = m_calcMenu->AppendSeparator();
	
	wxMenuItem* m_copyMenuItem;
	m_copyMenuItem = new wxMenuItem( m_viewMenu, ID_Calc_Copy, wxString( wxT("Copy last answer") ) + wxT('\t') + wxT("CTRL+C"), wxEmptyString, wxITEM_NORMAL );
	m_calcMenu->Append( m_copyMenuItem );
	
	wxMenuItem* m_separator4;
	m_separator4 = m_calcMenu->AppendSeparator();
	
	wxMenu *m_speedMenu = new wxMenu();
	m_calcMenu->Append( -1, wxT("Speed"), m_speedMenu );
	
	wxMenuItem* m_pauseMenuItem;
	m_pauseMenuItem = new wxMenuItem( m_speedMenu, ID_Calc_Pause, wxString( wxT("Pause Emulation") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_pauseMenuItem );
	
	wxMenuItem* m_setSpeed500;
	m_setSpeed500 = new wxMenuItem( m_speedMenu, ID_Speed_500, wxString( wxT("500%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed500 );
	
	wxMenuItem* m_setSpeed400;
	m_setSpeed400 = new wxMenuItem( m_speedMenu, ID_Speed_400, wxString( wxT("400%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed400 );
	
	wxMenuItem* m_setSpeed200;
	m_setSpeed200 = new wxMenuItem( m_speedMenu, ID_Speed_200, wxString( wxT("200%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed200 );
	
	wxMenuItem* m_setSpeed100;
	m_setSpeed100 = new wxMenuItem( m_speedMenu, ID_Speed_100, wxString( wxT("100%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed100 );
	
	wxMenuItem* m_setSpeed50;
	m_setSpeed50 = new wxMenuItem( m_speedMenu, ID_Speed_50, wxString( wxT("50%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed50 );
	
	wxMenuItem* m_setSpeed25;
	m_setSpeed25 = new wxMenuItem( m_speedMenu, ID_Speed_25, wxString( wxT("25%") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeed25 );
	
	wxMenuItem* m_separatorSpeedCustom;
	m_separatorSpeedCustom = m_speedMenu->AppendSeparator();
	
	wxMenuItem* m_setSpeedCustom;
	m_setSpeedCustom = new wxMenuItem( m_speedMenu, ID_Speed_Custom, wxString( wxT("Custom...") ) , wxEmptyString, wxITEM_CHECK );
	m_speedMenu->Append( m_setSpeedCustom );
	
	wxMenu *m_sizeMenu = new wxMenu();
	m_calcMenu->Append(-1,wxT("Size"), m_sizeMenu);
	
	wxMenuItem* m_setSize100;
	m_setSize100 = new wxMenuItem( m_sizeMenu, ID_Size_100, wxString( wxT("100%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize100 );
	
	wxMenuItem* m_setSize200;
	m_setSize200 = new wxMenuItem( m_sizeMenu, ID_Size_200, wxString( wxT("200%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize200 );
	
	wxMenuItem* m_setSize300;
	m_setSize300 = new wxMenuItem( m_sizeMenu, ID_Size_300, wxString( wxT("300%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize300 );
	
	wxMenuItem* m_setSize400;
	m_setSize400 = new wxMenuItem( m_sizeMenu, ID_Size_400, wxString( wxT("400%") ) , wxEmptyString, wxITEM_CHECK );
	m_sizeMenu->Append( m_setSize400 );
	
	wxMenuItem* m_separator5;
	m_separator5 = m_calcMenu->AppendSeparator();
	
	wxMenuItem* m_connectMenuItem;
	m_connectMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Connect, wxString( wxT("Connect To...") ) , wxEmptyString, wxITEM_NORMAL );
	m_calcMenu->Append( m_connectMenuItem );
	
	wxMenuItem* m_separator6;
	m_separator6 = m_calcMenu->AppendSeparator();
	
	wxMenuItem* m_optionsMenuItem;
	m_optionsMenuItem = new wxMenuItem( m_calcMenu, ID_Calc_Options, wxString( wxT("Preferences\tCtrl+Q") ) , wxEmptyString, wxITEM_NORMAL );
	m_calcMenu->Append( m_optionsMenuItem );
	
	m_menubar->Append( m_calcMenu, wxT("Calculator") );
	
	wxMenu *m_debugMenu = new wxMenu();
	wxMenuItem* m_resetMenuItem;
	m_resetMenuItem = new wxMenuItem( m_debugMenu, ID_Debug_Reset, wxString( wxT("Reset") ) , wxEmptyString, wxITEM_NORMAL );
	m_debugMenu->Append( m_resetMenuItem );
	
	wxMenuItem* m_debugMenuItem;
	m_debugMenuItem = new wxMenuItem( m_debugMenu, ID_Debug_Open, wxString( wxT("Open Debugger...") ) , wxEmptyString, wxITEM_NORMAL );
	m_debugMenu->Append( m_debugMenuItem );
	
	wxMenuItem* m_onMenuItem;
	m_onMenuItem = new wxMenuItem( m_debugMenu, ID_Debug_On, wxString( wxT("Turn Calc On") ) , wxEmptyString, wxITEM_NORMAL );
	m_debugMenu->Append( m_onMenuItem );
	
	m_menubar->Append( m_debugMenu, wxT("Debug") );
	
	wxMenu *m_helpMenu = new wxMenu();
	wxMenuItem* m_setupMenuItem;
	m_setupMenuItem = new wxMenuItem( m_helpMenu, ID_Help_Setup, wxString( wxT("Re-run setup wizard") ) , wxEmptyString, wxITEM_NORMAL );
	m_helpMenu->Append( m_setupMenuItem );
	
	wxMenuItem* m_websiteMenuItem;
	m_websiteMenuItem = new wxMenuItem( m_helpMenu, ID_Help_Website, wxString( wxT("Website") ) , wxEmptyString, wxITEM_NORMAL );
	m_helpMenu->Append( m_websiteMenuItem );
	
	wxMenuItem* m_aboutMenuItem;
	m_aboutMenuItem = new wxMenuItem( m_helpMenu, ID_Help_About, wxString( wxT("About") ) , wxEmptyString, wxITEM_NORMAL );
	m_helpMenu->Append( m_aboutMenuItem );

	
	m_menubar->Append( m_helpMenu, wxT("Help") );
	
	this->SetMenuBar( m_menubar );
	
	wxStatusBar *m_statusBar1 = new wxStatusBar(this);
	this->SetStatusBar(m_statusBar1);
	
	//Speed starts at 100%
	m_menubar->Check(ID_Speed_100, true);
	//Size starts at 200%
	m_menubar->Check(ID_Size_200, true);
	
	if (lpCalc->SkinEnabled) {
		windowSize = lpCalc->SkinSize;
	} else {
		windowSize.Set(128 * scale, 64 * scale);
	}
	
	#if (defined(__WXMSW__) && BIG_WINDOWS_ICON == 1)
	wxBitmap bitmap(wxT("wabbiticon.png"), wxBITMAP_TYPE_PNG);
	icon.CopyFromBitmap(bitmap);
	#else
	wxIcon icon = wxIcon(wxICON(wabbiticon));
	#endif
	
	this->SetIcon(icon);
	
	this->Connect(wxEVT_SHOW, (wxObjectEventFunction) &NabbitemuFrame::OnShow);
	
	this->SetSize(windowSize);
}

void NabbitemuFrame::OnShow(wxShowEvent& event) {
	this->isShownVar = 1;
}

void NabbitemuFrame::OnResize(wxSizeEvent& event)
{
	event.Skip(true);
	if (wxLCD == NULL) {
		return;
	}
	if (lpCalc->SkinEnabled) {
		wxLCD->Move(lpCalc->LCDRect.GetX(), lpCalc->LCDRect.GetY());
		return;
	}
	if (is_resizing) {
		return;
	}

	// Keep the biggest integer scale that fits, then snap the window to it.
	// The old code clamped the scale to 2 (so resizing did nothing) and moved
	// the *frame* instead of the LCD child, which made the window walk away.
	const wxSize client = GetClientSize();
	int scale = min(client.GetWidth() / 128, client.GetHeight() / 64);
	scale = max(1, min(8, scale));

	const int lcdWidth = lpCalc->cpu.pio.lcd->width;
	if (scale != lpCalc->scale) {
		lpCalc->scale = scale;
		is_resizing = true;
		this->SetClientSize(scale * 128, scale * 64);
		is_resizing = false;
	}
	lpCalc->LCDRect.SetX((scale * 128 - lcdWidth * scale) / 2);
	lpCalc->LCDRect.SetY(0);
	lpCalc->LCDRect.SetSize(wxSize(lcdWidth * scale, 64 * scale));
	wxLCD->SetClientSize(lpCalc->LCDRect.GetSize());
	wxLCD->Move(lpCalc->LCDRect.GetPosition());
}

void NabbitemuFrame::OnPaint(wxPaintEvent& event)
{
	wxPaintDC dc(this);
	if (lpCalc->SkinEnabled && m_skinBitmap.IsOk()) {
		wxLCD->Refresh(false);
		dc.DrawBitmap(m_skinBitmap, 0, 0, true);
	}
}

void NabbitemuFrame::OnFileNew(wxCommandEvent &event) {
	TCHAR *newFilePath = (TCHAR *) malloc(MAX_PATH * sizeof(TCHAR));
	if (newFilePath == NULL) {
		return;
	}
	_tcsncpy(newFilePath, lpCalc->rom_path, MAX_PATH - 1);
	newFilePath[MAX_PATH - 1] = _T('\0');
	lpCalc = calc_slot_new();
	if (rom_load(lpCalc, newFilePath) != TRUE) {
		wxMessageBox(wxT("Failed to create new calc"));
	}
	free(newFilePath);
	gui_frame(lpCalc);
}

void NabbitemuFrame::OnFileOpen(wxCommandEvent &event) {
	wxString lpstrFilter 	= wxT("\
Known File Types|*.73p;*.73P;*.82*;*.83p*;*.83P*;*.8xp*;*.8Xp*;*.8XP*;*.8xP*;*.8xk;*.8Xk;*.8XK;*.8xK;*.73k;*.73K;*.sav;*.rom;*.lab;*.8xu;*.8Xu;*.8xU;*.8XU|\
Calculator Program Files  (*.73p;*.82*;*.83p*;*.8xp*)|*.73p;*.73P;*.82*;*.83p*;*.83P*;*.8xp*;*.8Xp*;*.8XP*;*.8xP*|\
Calculator Applications  (*.8xk, *.73k)|*.8xk;*.8Xk;*.8XK;*.8xK;*.73k;*.73K|\
Calculator OSes (*.8xu)|*.8xu;*.8Xu;*.8xU;*.8XU|\
Save States  (*.sav)|*.sav|\
ROMS  (*.rom)|*.rom|\
Label Files (*.lab)|*.lab|\
All Files (*.*)|*.*\0");
	char filepath[PATH_MAX+256];
	char filestr[PATH_MAX+256];
		
	filepath[0] = '\0';
	filestr[0] = '\0';
	
	wxFileDialog dialog(NULL, wxT("Nabbitemu Open File"),
	wxT(""), wxT(""), lpstrFilter, wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE);
	if (dialog.ShowModal() != wxID_OK) {
		return;	
	}
	wxArrayString filePaths;
	dialog.GetPaths(filePaths);
	for (int i = 0; i < filePaths.GetCount(); i++) {
		SendFile(lpCalc, filePaths[i].c_str(), SEND_CUR);
		if (!_tcscmp(filePaths[i].c_str(), lpCalc->rom_path)) {
			//we've had a rom change
			gui_frame_update();
		}
	}
}

void NabbitemuFrame::OnFileSave(wxCommandEvent &event) {
	TCHAR FileName[MAX_PATH];
	const TCHAR lpstrFilter[] = _T("\
Known File types ( *.sav; *.rom; *.bin) |*.sav;*.rom;*.bin|\
Save States  (*.sav)|*.sav|\
ROMS  (*.rom; .bin)|*.rom;*.bin|\
All Files (*.*)|*.*\0");
	if (!SaveFile(FileName, lpstrFilter, _T("Nabbitemu Save State"), _T(".sav"))) {
		return;
	}	
	SAVESTATE_t* save = SaveSlot(lpCalc);

	_tcscpy(save->author, _T("Default"));
	save->comment[0] = '\0';
	WriteSave(FileName, save, false);
}

void NabbitemuFrame::OnFileClose(wxCommandEvent &event) {
	Close(TRUE);
}

void NabbitemuFrame::OnSetSize(wxCommandEvent &event) {
	/* This function is called when user changes size of LCD in menu */
    wxMenuBar *wxMenu = this->GetMenuBar();
    if (lpCalc->SkinEnabled) {
		return;
	}
	int eventID;
	wxMenu->Check(ID_Size_100,false);
	wxMenu->Check(ID_Size_200,false);
	wxMenu->Check(ID_Size_300,false);
	wxMenu->Check(ID_Size_400,false);
	
	eventID = event.GetId();
	
	switch (eventID) {
		case ID_Size_100:
			lpCalc->scale = 1;  //This is half of the Wabbit default, but equals real LCD
			wxMenu->Check(ID_Size_100, true);
			break;
		case ID_Size_200:
			lpCalc->scale = 2; //Wabbit default, twice the LCD
			wxMenu->Check(ID_Size_200, true);
			break;
		case ID_Size_300:
			lpCalc->scale = 3;
			wxMenu->Check(ID_Size_300, true);
			break;
		case ID_Size_400:
			lpCalc->scale = 4;
			wxMenu->Check(ID_Size_400, true);
			break;
	}
	if (!lpCalc->SkinEnabled) {
		/* Update the client area (not the decorated frame) to the LCD size. */
		this->SetClientSize(128 * lpCalc->scale, 64 * lpCalc->scale);
		lpCalc->LCDRect.SetX((128 - lpCalc->cpu.pio.lcd->width) * lpCalc->scale / 2);
		lpCalc->LCDRect.SetY(0);
		lpCalc->LCDRect.SetSize(wxSize(lpCalc->cpu.pio.lcd->width * lpCalc->scale, 64 * lpCalc->scale));
		if (this->wxLCD) {
			this->wxLCD->SetClientSize(lpCalc->LCDRect.GetSize());
			this->wxLCD->Move(lpCalc->LCDRect.GetPosition());
		}
	}
}

void NabbitemuFrame::OnSetSpeed(wxCommandEvent &event) {
	wxMenuBar *wxMenu = this->GetMenuBar();
	int eventID;
	wxMenu->Check(ID_Speed_500, false);
	wxMenu->Check(ID_Speed_400, false);
	wxMenu->Check(ID_Speed_200, false);
	wxMenu->Check(ID_Speed_100, false);
	wxMenu->Check(ID_Speed_50, false);
	wxMenu->Check(ID_Speed_25, false);
	wxMenu->Check(ID_Speed_Custom, false);
	wxMenu->SetLabel(ID_Speed_Custom, wxString(wxT("Custom...")));
	wxMenu->Check(ID_Speed_Custom, false);
	
	eventID = event.GetId();
	switch (eventID) {
		case ID_Speed_25:
			this->SetSpeed(25);
			wxMenu->Check(ID_Speed_25, true);
			break;
		case ID_Speed_50:
			this->SetSpeed(50);
			wxMenu->Check(ID_Speed_50, true);
			break;
		case ID_Speed_100:
			this->SetSpeed(100);
			wxMenu->Check(ID_Speed_100, true);
			break;
		case ID_Speed_200:
			this->SetSpeed(200);
			wxMenu->Check(ID_Speed_200, true);
			break;
		case ID_Speed_400:
			this->SetSpeed(400);
			wxMenu->Check(ID_Speed_400, true);
			break;
		case ID_Speed_500:
			this->SetSpeed(500);
			wxMenu->Check(ID_Speed_500, true);
			break;
	}
}

void NabbitemuFrame::OnSetSpeedCustom(wxCommandEvent &event) {
	wxMenuBar *wxMenu = this->GetMenuBar();
	long resp;
	resp = wxGetNumberFromUser(wxString(wxT("Enter the speed (in percentage) you wish to set:")), wxString(wxT("")), wxString(wxT("Nabbitemu - Custom Speed")), 100, 0, 10000);
	if (resp != -1) {
		wxMenu->SetLabel(ID_Speed_Custom, wxString::Format(wxT("%i%%"),resp));
		wxMenu->Check(ID_Speed_500, false);
		wxMenu->Check(ID_Speed_400, false);
		wxMenu->Check(ID_Speed_200, false);
		wxMenu->Check(ID_Speed_100, false);
		wxMenu->Check(ID_Speed_50, false);
		wxMenu->Check(ID_Speed_25, false);
		wxMenu->Check(ID_Speed_Custom, true);
		this->SetSpeed(resp);
	} else {
		// note to self: does the "entered something invalid" apply?? supposedly, OnSetSpeedCustom is checked...
		/* Dirty, evil hack... but I'm too lazy to create another var to indicate
		 * custom speed status, soo... :P */
		if (wxMenu->GetLabel(ID_Speed_Custom) == wxString(wxT("Custom..."))) {
			// Do nothing
			wxMenu->Check(ID_Speed_Custom, false);
		} else {
			wxMenu->Check(ID_Speed_Custom, true);
		}
	}
}

void NabbitemuFrame::OnPauseEmulation(wxCommandEvent &event) {
	wxMenuBar *wxMenu = this->GetMenuBar();
	if (lpCalc->running) {
		//Tick is checked and emulation stops
		lpCalc->running = FALSE;
		wxMenu->Check(ID_Calc_Pause, true);
	} else {
		//Tick is unchecked and emulation resumes
		lpCalc->running = TRUE;
		wxMenu->Check(ID_Calc_Pause, false);
	}
}

void NabbitemuFrame::SetSpeed(int speed) {
	lpCalc->speed = speed;
}

void NabbitemuFrame::OnKeyDown(wxKeyEvent& event)
{
	int keycode = event.GetKeyCode();
	if (keycode == WXK_F8) {
		if (lpCalc->speed == 100) {
			SetSpeed(400);
		} else {
			SetSpeed(100);
		}
	}
	if (keycode == WXK_SHIFT) {
		wxUint32 raw = event.GetRawKeyCode();
		if (raw == 65505) {
			keycode = WXK_LSHIFT;
		} else {
			keycode = WXK_RSHIFT;
		}
	}

	keyprog_t *kp = keypad_key_press(&lpCalc->cpu, keycode);
	if (kp) {
		if ((lpCalc->cpu.pio.keypad->keys[kp->group][kp->bit] & KEY_STATEDOWN) == 0) {
			lpCalc->cpu.pio.keypad->keys[kp->group][kp->bit] |= KEY_STATEDOWN;
			//for when we finally do button feedback
			this->Update();
			FinalizeButtons();
		}
	}
}

void NabbitemuFrame::OnKeyUp(wxKeyEvent& event)
{
	int key = event.GetKeyCode();
	if (key == WXK_SHIFT) {
		keypad_key_release(&lpCalc->cpu, WXK_LSHIFT);
		keypad_key_release(&lpCalc->cpu, WXK_RSHIFT);
	} else {
		keypad_key_release(&lpCalc->cpu, key);
	}
	FinalizeButtons();
}

void NabbitemuFrame::OnLeftButtonDown(wxMouseEvent& event)
{
	event.Skip(true);
	static wxPoint pt;
	keypad_t *kp = lpCalc->cpu.pio.keypad;

	//CopySkinToButtons();
	//CaptureMouse();
	pt.x	= event.GetX();
	pt.y	= event.GetY();
	/*if (lpCalc->bCutout) {
		pt.y += GetSystemMetrics(SM_CYCAPTION);
		pt.x += GetSystemMetrics(SM_CXSIZEFRAME);
	}*/
	for(int group = 0; group < 7; group++) {
		for(int bit = 0; bit < 8; bit++) {
			kp->keys[group][bit] &= (~KEY_MOUSEPRESS);
		}
	}

	lpCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;

	/*if (!event.LeftDown()) {
		//FinalizeButtons(lpCalc);
		return;
	}*/

	if (lpCalc->keymap.GetRed(pt.x, pt.y) == 0xFF) {
		//FinalizeButtons(lpCalc);
		return;
	}

	int green = lpCalc->keymap.GetGreen(pt.x, pt.y);
	int blue = lpCalc->keymap.GetBlue(pt.x, pt.y);
	if ((green >> 4) == 0x05 && (blue >> 4) == 0x00)
	{
		lpCalc->cpu.pio.keypad->on_pressed |= KEY_MOUSEPRESS;
	} else {
		kp->keys[green >> 4][blue >> 4] |= KEY_MOUSEPRESS;
		if ((kp->keys[green >> 4][blue >> 4] & KEY_STATEDOWN) == 0) {
			//DrawButtonState(lpCalc, lpCalc->hdcButtons, lpCalc->hdcKeymap, &pt, DBS_DOWN | DBS_PRESS);
			kp->keys[green >> 4][blue >> 4] |= KEY_STATEDOWN;
		}
	}
}

void NabbitemuFrame::OnLeftButtonUp(wxMouseEvent& event)
{
	int group, bit;
	event.Skip(true);
	static wxPoint pt;
	bool repostMessage = FALSE;
	keypad_t *kp = lpCalc->cpu.pio.keypad;

#define KEY_TIMER 1
	//KillTimer(hwnd, KEY_TIMER);

	for (group = 0; group < 7; group++) {
		for (bit = 0; bit < 8; bit++) {
#define MIN_KEY_DELAY 400
			if (kp->last_pressed[group][bit] - lpCalc->cpu.timer_c->tstates >= MIN_KEY_DELAY || !lpCalc->running) {
				kp->keys[group][bit] &= (~KEY_MOUSEPRESS);
			} else {
				repostMessage = TRUE;
			}
		}
	}

	if (kp->on_last_pressed - lpCalc->cpu.timer_c->tstates >= MIN_KEY_DELAY || !lpCalc->running) {
		lpCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;
	} else {
		repostMessage = TRUE;
	}

	if (repostMessage) {
		//SetTimer(hwnd, KEY_TIMER, 50, NULL);
	}

	FinalizeButtons();
}

void NabbitemuFrame::OnFileQuit(wxCommandEvent& WXUNUSED(event))
{
	Close(TRUE);
}

void NabbitemuFrame::OnViewSkin(wxCommandEvent& event)
{
	wxMenuBar *wxMenu = this->GetMenuBar();
	lpCalc->SkinEnabled = !lpCalc->SkinEnabled;
	
	// Enable/disable sizing menu items.
	//m_sizeMenu->Enable(!lpCalc->SkinEnabled);
	wxMenu->Enable(ID_Size_100, !lpCalc->SkinEnabled);
	wxMenu->Enable(ID_Size_200, !lpCalc->SkinEnabled);
	wxMenu->Enable(ID_Size_300, !lpCalc->SkinEnabled);
	wxMenu->Enable(ID_Size_400, !lpCalc->SkinEnabled);
	
	// Connect or disconnect the OnResize event.
	if (lpCalc->SkinEnabled) {
		prevCalcScale = lpCalc->scale;
		lpCalc->scale = 2;
		this->Disconnect(wxEVT_SIZE, (wxObjectEventFunction) &NabbitemuFrame::OnResize);
	} else {
		lpCalc->scale = prevCalcScale;
		this->Connect(wxEVT_SIZE, (wxObjectEventFunction) &NabbitemuFrame::OnResize);
	}
	gui_frame_update();
	this->Refresh();
	this->Update();
}

void NabbitemuFrame::OnViewVariables(wxCommandEvent & WXUNUSED(event))
{
	wxWindow *foundWindow = wxWindow::FindWindowByName(_T("Calculator Variables"), this);
	if (foundWindow && foundWindow == varTree) {
		varTree->Show();
		varTree->SetFocus();
	} else {
		varTree = new VarTree(this);
		varTree->Show();
	}
}

void NabbitemuFrame::OnDebugOpen(wxCommandEvent& WXUNUSED(event))
{
	gui_debug(lpCalc);
}

void NabbitemuFrame::OnDebugReset(wxCommandEvent& WXUNUSED(event))
{
	calc_reset(lpCalc);
}

void NabbitemuFrame::OnDebugOn(wxCommandEvent& WXUNUSED(event))
{
	calc_turn_on(lpCalc);
}

void NabbitemuFrame::OnHelpSetup(wxCommandEvent& WXUNUSED(event))
{
	int count = calc_count();
	bool success = NabbitemuApp::DoRomWizard();
	if (!success) {
		return;
	}
}

void NabbitemuFrame::OnHelpWebsite(wxCommandEvent& WXUNUSED(event))
{
	wxLaunchDefaultBrowser(wxT("https://repo.nebuff.xyz/nabbitemu/"));
}

void NabbitemuFrame::OnHelpAbout(wxCommandEvent& WXUNUSED(event))
{
	wxMessageBox(wxT("Nabbitemu - a TI-8x graphing calculator emulator for Linux.\n\n")
		wxT("Based on wxWabbitemu by Albert Huang and Chris Shappell, which in turn\n")
		wxT("is based on Wabbitemu by Spencer Putt, Don Straney, Chris Shappell and\n")
		wxT("James Montelongo. Licensed under the GPL.\n\n")
		wxT("Project page: https://repo.nebuff.xyz/nabbitemu/"),
		wxT("About Nabbitemu"), wxOK | wxICON_INFORMATION, this);
}

void NabbitemuFrame::OnFileGIF(wxCommandEvent& WXUNUSED(event))
{
	wxMenuBar *wxMenu = this->GetMenuBar();
	wxFileDialog saveGIFDialog(NULL, wxT("Save GIF file"), wxT(""), wxT(""), wxT("\
GIF File (*.gif)|*.GIF;*.gif;*.Gif;*.GIf;*.gIf;*.gIF;*.giF|\
All Files (*.*)|*.*\0"),wxFD_SAVE|wxFD_OVERWRITE_PROMPT, wxDefaultPosition);
	if (saveGIFDialog.ShowModal() != wxID_OK)
		return;
	wxString path;
	path.append( saveGIFDialog.GetDirectory() );
	path.append( wxFileName::GetPathSeparator() );
	path.append( saveGIFDialog.GetFilename() );
	_tcsncpy(gif_file_name, path.c_str(), ARRAYSIZE(gif_file_name) - 1);
	gif_file_name[ARRAYSIZE(gif_file_name) - 1] = _T('\0');
	// Now to actually do something!
	if (gif_write_state == GIF_IDLE) {
		gif_write_state = GIF_START;
		for (int i = 0; i < MAX_CALCS; i++)
			if (lpCalc->active)
				lpCalc->gif_disp_state = GDS_STARTING;
		wxMenu->Check(ID_File_Gif, true);
	} else {
		gif_write_state = GIF_END;
		for (int i = 0; i < MAX_CALCS; i++)
			if (lpCalc->active)
				lpCalc->gif_disp_state = GDS_ENDING;
		wxMenu->Check(ID_File_Gif, false);
	}
}

void NabbitemuFrame::OnQuit(wxCloseEvent& event)
{
	lpCalc->active = FALSE;
	event.Skip();
}

void NabbitemuFrame::FinalizeButtons() {
	int group, bit;
	keypad_t *kp = lpCalc->cpu.pio.keypad;
	for(group = 0; group < 7; group++) {
		for(bit = 0; bit < 8; bit++) {
			if ((kp->keys[group][bit] & KEY_STATEDOWN) &&
				((kp->keys[group][bit] & KEY_MOUSEPRESS) == 0) &&
				((kp->keys[group][bit] & KEY_KEYBOARDPRESS) == 0)) {
					kp->keys[group][bit] &= (~KEY_STATEDOWN);
			}
		}
	}
}

int SetGIFName() {
	wxFileDialog saveGIFDialog(NULL, wxT("Save GIF file"), wxT(""), wxT(""), wxT("\
GIF File (*.gif)|*.GIF;*.gif;*.Gif;*.GIf;*.gIf;*.gIF;*.giF|\
All Files (*.*)|*.*\0"),wxFD_SAVE|wxFD_OVERWRITE_PROMPT, wxDefaultPosition);
	if (saveGIFDialog.ShowModal() != wxID_OK)
		return false;
	wxString path;
	path.append( saveGIFDialog.GetDirectory() );
	path.append( wxFileName::GetPathSeparator() );
	path.append( saveGIFDialog.GetFilename() );
	_tcsncpy(gif_file_name, path.c_str(), ARRAYSIZE(gif_file_name) - 1);
	gif_file_name[ARRAYSIZE(gif_file_name) - 1] = _T('\0');
	return true;
}
