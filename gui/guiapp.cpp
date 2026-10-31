#include "guiapp.h"
#include "gui.h"
#include "wizard/romwizard.h"
#include "sendfile.h"

NabbitemuFrame *frames[MAX_CALCS];

BOOL NabbitemuApp::DoRomWizard() {
	RomWizard wizard;	
	bool success = wizard.Begin();
	return success;
}

void NabbitemuApp::LoadSettings(LPCALC lpCalc)
{
	settingsConfig = new wxConfig(wxT("Nabbitemu"));
	wxString romPath;
	settingsConfig->Read(wxT("/rom_path"), &romPath, wxEmptyString);
	settingsConfig->Read(wxT("/SkinEnabled"), &lpCalc->SkinEnabled, FALSE);

	// One-time import of the wxWabbitemu-era settings so upgrading keeps the
	// ROM path and skin choice instead of dropping back to the wizard.
	if (romPath.empty()) {
		wxConfig legacy(wxT("Wabbitemu"));
		if (legacy.GetNumberOfEntries(true) > 0) {
			legacy.Read(wxT("/rom_path"), &romPath, wxEmptyString);
			bool legacySkin = false;
			if (legacy.Read(wxT("/SkinEnabled"), &legacySkin)) {
				lpCalc->SkinEnabled = legacySkin;
				settingsConfig->Write(wxT("/SkinEnabled"), legacySkin);
			}
			if (!romPath.empty()) {
				settingsConfig->Write(wxT("/rom_path"), romPath);
			}
		}
	}

	// rom_path is a fixed MAX_PATH buffer; the old code used an unbounded copy.
	_tcsncpy(lpCalc->rom_path, romPath.c_str(), MAX_PATH - 1);
	lpCalc->rom_path[MAX_PATH - 1] = _T('\0');
}

bool NabbitemuApp::OnInit()
{
	wxImage::AddHandler(new wxPNGHandler);
	//stolen from the windows version
	ParseCommandLineArgs();

	memset(frames, 0, sizeof(frames));
	m_pendingMs = 0;
	m_frameWatch.Start();
	LPCALC lpCalc = calc_slot_new();
	LoadSettings(lpCalc);
	
	int result = rom_load(lpCalc, lpCalc->rom_path);
	if (result == TRUE) {
		gui_frame(lpCalc);
	} else {
		calc_slot_free(lpCalc);
		BOOL loadedRom = FALSE;
		if (parsedArgs.num_rom_files > 0) {
			for (int i = 0; i < parsedArgs.num_rom_files; i++) {
				if (rom_load(lpCalc, parsedArgs.rom_files[i])) {
					gui_frame(lpCalc);
					loadedRom = TRUE;
					break;
				}
			}
		}
		if (!loadedRom) {
			bool success = DoRomWizard();
			if (!success) {
				return FALSE;
			}
		}
	}
	LoadCommandlineFiles((INT_PTR) lpCalc, LoadToLPCALC);
	timer = new wxTimer();
	timer->Connect(wxEVT_TIMER, (wxObjectEventFunction) &NabbitemuApp::OnTimer);
	timer->Start(TPF, false);
	return TRUE;
}

void NabbitemuApp::SaveSettings(LPCALC lpCalc) {
#ifdef _UNICODE
	wxString rom_path(lpCalc->rom_path, wxConvUTF8);
#else
	wxString rom_path(lpCalc->rom_path);
#endif
	settingsConfig->Write(wxT("rom_path"), rom_path);
	settingsConfig->Write(wxT("SkinEnabled"), lpCalc->SkinEnabled);
	settingsConfig->Flush();
}

int NabbitemuApp::OnExit() {
	if (calcs[0].active) {
		SaveSettings(&calcs[0]);
	}
	//load ROMs first
	for (int i = 0; i < parsedArgs.num_rom_files; i++) {
		free(parsedArgs.rom_files[i]);
		parsedArgs.rom_files[i] = NULL;
	}
	//then archived files
	for (int i = 0; i < parsedArgs.num_archive_files; i++) {
		free(parsedArgs.archive_files[i]);
		parsedArgs.archive_files[i] = NULL;
	}
	//then ram
	for (int i = 0; i < parsedArgs.num_ram_files; i++) {
		free(parsedArgs.ram_files[i]);
		parsedArgs.ram_files[i] = NULL;
	}
	//finally utility files (label, break, etc)
	for (int i = 0; i < parsedArgs.num_utility_files; i++) {
		free(parsedArgs.utility_files[i]);
		parsedArgs.utility_files[i] = NULL;
	}
	return 0;
}


// Emulation is paced from a monotonic stopwatch. The previous code masked a
// 32-bit millisecond counter with 0x3F, so every ~64 ms the frame accounting
// reset and the emulator would stutter or run fast.
void NabbitemuApp::OnTimer(wxTimerEvent& event) {
	long elapsed = m_frameWatch.Time();
	m_frameWatch.Start();
	if (elapsed < 0) {
		elapsed = 0;
	}
	// Anything beyond a quarter second is unrecoverable backlog (suspend,
	// debugger stop, heavy load): drop it instead of freezing the UI.
	if (elapsed > 250) {
		elapsed = 250;
	}
	m_pendingMs += elapsed;

	int framesRun = 0;
	while (m_pendingMs >= TPF && framesRun < 4) {
		calc_run_all();
		m_pendingMs -= TPF;
		framesRun++;
	}
	if (m_pendingMs > TPF) {
		m_pendingMs = TPF;
	}

	if (framesRun > 0) {
		for (int i = 0; i < MAX_CALCS; i++) {
			if (calcs[i].active && frames[i] != NULL) {
				frames[i]->gui_draw();
			}
		}
	}
}

void NabbitemuApp::ParseCommandLineArgs()
{
	ZeroMemory(&parsedArgs, sizeof(ParsedCmdArgs));
	TCHAR tmpstring[512];
	SEND_FLAG ram = SEND_CUR;

	if (argv && argc > 1) {
		for(int i = 1; i < argc; i++) {
			_tcsncpy(tmpstring, argv[i], ARRAYSIZE(tmpstring) - 1);
			tmpstring[ARRAYSIZE(tmpstring) - 1] = _T('\0');
			TCHAR secondChar = toupper(tmpstring[1]);
			if (*tmpstring != '-' && *tmpstring != '/') {
				const size_t pathLen = _tcslen(tmpstring);
				TCHAR *temp = (TCHAR *) malloc((pathLen + 1) * sizeof(TCHAR));
				if (temp == NULL) {
					continue;
				}
				_tcscpy(temp, tmpstring);
				TCHAR extension[5] = _T("");
				const TCHAR *pext = _tcsrchr(tmpstring, _T('.'));
				if (pext != NULL) {
					_tcsncpy(extension, pext, ARRAYSIZE(extension) - 1);
					extension[ARRAYSIZE(extension) - 1] = _T('\0');
				}
				if (!_tcsicmp(extension, _T(".rom")) || !_tcsicmp(extension, _T(".sav")) || !_tcsicmp(extension, _T(".clc"))) {
					parsedArgs.rom_files[parsedArgs.num_rom_files++] = temp;
				}
				else if (!_tcsicmp(extension, _T(".brk")) || !_tcsicmp(extension, _T(".lab")) 
					|| !_tcsicmp(extension, _T(".zip")) || !_tcsicmp(extension, _T(".tig"))) {
						parsedArgs.utility_files[parsedArgs.num_utility_files++] = temp;
				}
				else if (ram) {
					parsedArgs.ram_files[parsedArgs.num_ram_files++] = temp;
				} else {
					parsedArgs.archive_files[parsedArgs.num_archive_files++] = temp;
				}
			} else if (secondChar == 'R') {
				ram = SEND_RAM;
			} else if (secondChar == 'A') {
				ram = SEND_ARC;
			} else if (secondChar == 'S') {
				parsedArgs.silent_mode = TRUE;
			} else if (secondChar == 'F') {
				parsedArgs.force_focus = TRUE;
			} else if (secondChar == 'N') {
				parsedArgs.force_new_instance = TRUE;
			}
		}
	}
}

/*void LoadAlreadyExistingWabbit(unsigned int lParam, LPTSTR filePath, SEND_FLAG sendLoc)
{
	HWND hwnd = (HWND) lParam;
	COPYDATASTRUCT *cds = (COPYDATASTRUCT *) malloc(sizeof(COPYDATASTRUCT));
	cds->dwData = sendLoc;
	size_t strLen;
	cds->lpData = filePath;
	if (PathIsRelative(filePath)) {
		TCHAR tempPath[MAX_PATH];
		TCHAR *tempPath2 = (TCHAR *) malloc(MAX_PATH);
		_tgetcwd(tempPath, MAX_PATH);
		PathCombine(tempPath2, tempPath, filePath);
		cds->lpData = tempPath2;
	}
	StringCbLength(filePath, 512, &strLen);
	cds->cbData = strLen;
	SendMessage(hwnd, WM_COPYDATA, (WPARAM) NULL, (LPARAM) cds);
}*/

void LoadToLPCALC(INT_PTR lParam, LPTSTR filePath, SEND_FLAG sendLoc)
{
	LPCALC lpCalc = (LPCALC) lParam;
	SendFile(lpCalc, filePath, sendLoc);
}

void NabbitemuApp::LoadCommandlineFiles(INT_PTR lParam,  void (*load_callback)(INT_PTR, LPTSTR, SEND_FLAG))
{
	//load ROMs first
	for (int i = 0; i < parsedArgs.num_rom_files; i++) {
		load_callback(lParam, parsedArgs.rom_files[i], SEND_ARC);
	}
	//then archived files
	for (int i = 0; i < parsedArgs.num_archive_files; i++) {
		load_callback(lParam, parsedArgs.archive_files[i], SEND_ARC);
	}
	//then ram
	for (int i = 0; i < parsedArgs.num_ram_files; i++) {
		load_callback(lParam, parsedArgs.ram_files[i], SEND_RAM);
	}
	//finally utility files (label, break, etc)
	for (int i = 0; i < parsedArgs.num_utility_files; i++) {
		load_callback(lParam, parsedArgs.utility_files[i], SEND_ARC);
	}
}