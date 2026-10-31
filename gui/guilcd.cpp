#include "guilcd.h"
#include "gui.h"
#include "core.h"
#include "droptarget.h"

BEGIN_EVENT_TABLE(NabbitemuLCD, wxWindow)
	EVT_PAINT(NabbitemuLCD::OnPaint)
	EVT_KEY_DOWN(NabbitemuLCD::OnKeyDown)
	EVT_KEY_UP(NabbitemuLCD::OnKeyUp)
	
	EVT_LEFT_DOWN(NabbitemuLCD::OnLeftButtonDown)
	EVT_LEFT_UP(NabbitemuLCD::OnLeftButtonUp)
END_EVENT_TABLE()

unsigned char redColors[MAX_SHADES+1];
unsigned char greenColors[MAX_SHADES+1];
unsigned char blueColors[MAX_SHADES+1];
NabbitemuLCD::NabbitemuLCD(wxFrame *mainFrame, LPCALC lpCalc)
	: wxWindow(mainFrame, ID_LCD, wxPoint(0,0), lpCalc->LCDRect.GetSize()) {
	this->lpCalc = lpCalc;
	this->mainFrame = mainFrame;
	this->SetDropTarget(new DnDFile(this, lpCalc));
#define LCD_HIGH	255
	for (int i = 0; i <= MAX_SHADES; i++) {
		redColors[i] = (0x9E*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
		greenColors[i] = (0xAB*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
		blueColors[i] = (0x88*(256-(LCD_HIGH/MAX_SHADES)*i))/255;
	}
	hasDrawnLCD = false;
	m_lcdImage.Create(128, 64, false);
	m_skinOverlaySize = wxSize(0, 0);
	m_skinStamp = NULL;
}

void NabbitemuLCD::OnLeftButtonDown(wxMouseEvent& event)
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

void NabbitemuLCD::OnLeftButtonUp(wxMouseEvent& event)
{
	event.Skip(true);
	static wxPoint pt;
	keypad_t *kp = lpCalc->cpu.pio.keypad;

	//ReleaseMouse();

	for(int group = 0; group < 7; group++) {
		for(int bit = 0; bit < 8; bit++) {
			kp->keys[group][bit] &= ~(KEY_MOUSEPRESS | KEY_STATEDOWN);
		}
	}

	lpCalc->cpu.pio.keypad->on_pressed &= ~KEY_MOUSEPRESS;
}

void NabbitemuLCD::OnResize(wxSizeEvent& event)
{
	event.Skip(false);
}

//TODO: forward these events somehow
void NabbitemuLCD::OnKeyDown(wxKeyEvent& event)
{
	int keycode = event.GetKeyCode();
	if (keycode == WXK_F8) {
		if (lpCalc->speed == 100)
			lpCalc->speed = 400;
		else
			lpCalc->speed = 100;
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
			this->Update();
			FinalizeButtons();
		}
	}
}

void NabbitemuLCD::OnKeyUp(wxKeyEvent& event)
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

void NabbitemuLCD::FinalizeButtons() {
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

void NabbitemuLCD::OnPaint(wxPaintEvent& event)
{
	if (this->mainFrame->IsIconized()) {
		return;
	}
	wxPaintDC dc(this);
	PaintLCD(this, &dc);
	LCD_t *lcd = lpCalc->cpu.pio.lcd;
	wxStatusBar *wxStatus = mainFrame->GetStatusBar();
	if (wxStatus) {
		if (clock() > lpCalc->sb_refresh + CLOCKS_PER_SEC / 2) {
			wxString sz_status;
			if (lcd->active) {
				sz_status.sprintf(wxT("FPS: %0.2lf"), lcd->ufps);
			} else {
				sz_status.sprintf(wxT("FPS: -"));
			}
			wxStatus->SetStatusText(sz_status, 0);
			lpCalc->sb_refresh = clock();
		}
	}
} 

void NabbitemuLCD::PaintLCD(wxWindow *window, wxPaintDC *wxDCDest)
{
	LCD_t *lcd = lpCalc->cpu.pio.lcd;
	if (lcd == NULL) {
		return;
	}
	if (!m_lcdImage.IsOk()) {
		m_lcdImage.Create(128, 64, false);
	}

	// Convert the shaded LCD buffer into the reusable RGB image.
	unsigned char *rgb = m_lcdImage.GetData();
	const unsigned char *shades = lcd->active ? LCD_image(lcd) : NULL;
	for (int i = 0; i < 128 * 64; i++) {
		const unsigned char shade = shades ? shades[i] : 0;
		rgb[3 * i] = redColors[shade];
		rgb[3 * i + 1] = greenColors[shade];
		rgb[3 * i + 2] = blueColors[shade];
	}

	// One small upload and a nearest-neighbour stretch: the previous code
	// allocated two full-size images (plus an alpha buffer) per frame.
	wxBitmap lcdBitmap(m_lcdImage);
	wxMemoryDC memDC;
	memDC.SelectObject(lcdBitmap);
	const int destWidth = lpCalc->LCDRect.GetWidth();
	const int destHeight = lpCalc->LCDRect.GetHeight();
	wxDCDest->StretchBlit(0, 0, destWidth, destHeight, &memDC, 0, 0, lcd->width, 64);
	memDC.SelectObject(wxNullBitmap);

	if (lpCalc->SkinEnabled && lpCalc->calcSkin.IsOk()) {
		if (!m_skinOverlay.IsOk() || m_skinOverlaySize != lpCalc->LCDRect.GetSize()
				|| m_skinStamp != lpCalc->calcSkin.GetData()) {
			wxImage texture = lpCalc->calcSkin.GetSubImage(lpCalc->LCDRect);
			const int pixels = texture.GetWidth() * texture.GetHeight();
			unsigned char *alpha = (unsigned char *) malloc(pixels);
			if (alpha != NULL) {
				memset(alpha, 108, pixels);
				texture.SetAlpha(alpha);
			}
			m_skinOverlay = wxBitmap(texture);
			m_skinOverlaySize = lpCalc->LCDRect.GetSize();
			m_skinStamp = lpCalc->calcSkin.GetData();
		}
		wxDCDest->DrawBitmap(m_skinOverlay, 0, 0, true);
	}
}
