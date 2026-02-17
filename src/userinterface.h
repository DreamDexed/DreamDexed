//
// userinterface.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#pragma once

#include <cstdint>

#include <circle/gpiomanager.h>
#include <circle/i2cmaster.h>
#include <circle/spimaster.h>
#include <circle/writebuffer.h>
#include <display/chardevice.h>
#include <display/hd44780device.h>
#include <display/ssd1306device.h>
#include <display/st7789device.h>
#include <display/st7789display.h>
#include <sensor/ky040.h>

#include "config.h"
#include "uibuttons.h"
#include "uimenu.h"

class CMiniDexed;

class CUserInterface
{
public:
	CUserInterface(CMiniDexed *pMiniDexed, CGPIOManager *pGPIOManager, CI2CMaster *pI2CMaster, CSPIMaster *pSPIMaster, CConfig *pConfig);
	~CUserInterface();

	bool Initialize();

	void LoadDefaultScreen();

	void Process();

	void ParameterChanged();
	void DisplayChanged();

	// Write to display in this format:
	// +----------------+
	// |PARAM       MENU|
	// |[<]VALUE     [>]|
	// +----------------+
	void DisplayWrite(const char *pMenu, const char *pParam, const char *pValue,
			  bool bArrowDown, bool bArrowUp);

	// To be called from the MIDI device on reception of a MIDI CC message
	void UIMIDICmdHandler(int nMidiCh, uint8_t nMidiType, uint8_t nMidiData1, uint8_t nMidiData2);

private:
	void LCDWrite(const char *pString); // Print to optional HD44780 display

	void EncoderEventHandler(CKY040::TEvent Event);
	static void EncoderEventStub(CKY040::TEvent Event, void *pParam);
	void UIButtonsEventHandler(CUIButton::BtnEvent Event);
	static void UIButtonsEventStub(CUIButton::BtnEvent Event, void *pParam);
	void UISetMIDIButtonChannel(int nCh);

private:
	CMiniDexed *m_pMiniDexed;
	CGPIOManager *m_pGPIOManager;
	CI2CMaster *m_pI2CMaster;
	CSPIMaster *m_pSPIMaster;
	CConfig *m_pConfig;

	CCharDevice *m_pLCD;
	CHD44780Device *m_pHD44780;
	CSSD1306Device *m_pSSD1306;
	CST7789Display *m_pST7789Display;
	CST7789Device *m_pST7789;
	CWriteBufferDevice *m_pLCDBuffered;

	CUIButtons *m_pUIButtons;

	int m_nMIDIButtonCh;

	CKY040 *m_pRotaryEncoder;
	bool m_bSwitchPressed;

	CUIMenu m_Menu;
};
