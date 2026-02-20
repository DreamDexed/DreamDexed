//
// pckeyboard.h
//
// MiniSynth Pi - A virtual analogue synthesizer for Raspberry Pi
// Copyright (C) 2017-2020  R. Stange <rsta2@o2online.de>
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

#include <circle/device.h>
#include <circle/usb/usbkeyboard.h>

#include "config.h"
#include "mididevice.h"
#include "userinterface.h"

class CMiniDexed;

class CPCKeyboard : public CMIDIDevice
{
public:
	CPCKeyboard(CMiniDexed *pSynthesizer, CConfig *pConfig, CUserInterface *pUI);
	~CPCKeyboard();

	void Process(bool bPlugAndPlayUpdated);

private:
	static void KeyStatusHandlerRaw(unsigned char ucModifiers, const unsigned char RawKeys[6]);

	static uint8_t GetKeyNumber(uint8_t ucKeyCode);

	static bool FindByte(const uint8_t *pBuffer, uint8_t ucByte, int nLength);

	static void DeviceRemovedHandler(CDevice *pDevice, void *pContext);

private:
	CUSBKeyboardDevice *volatile m_pKeyboard;

	uint8_t m_LastKeys[6];

	static CPCKeyboard *s_pThis;
};
