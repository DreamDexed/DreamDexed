//
// serialmididevice.h
//
// MiniDexed - Dexed FM synthesizer for bare metal Raspberry Pi
// Copyright (C) 2022  The MiniDexed Team
//
// Original author of this class:
//	R. Stange <rsta2@o2online.de>
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

#include <circle/interrupt.h>
#include <circle/serial.h>
#include <circle/writebuffer.h>

#include "config.h"
#include "mididevice.h"
#include "userinterface.h"

class CMiniDexed;

class CSerialMIDIDevice : public CMIDIDevice
{
public:
	CSerialMIDIDevice(CMiniDexed *pSynthesizer, CInterruptSystem *pInterrupt, CConfig *pConfig, CUserInterface *pUI);
	~CSerialMIDIDevice();

	bool Initialize();

	void Process();

	void Send(const uint8_t *pMessage, int nLength, int nCable = 0) override;

private:
	CConfig *m_pConfig;

	CSerialDevice m_Serial;
	int m_nSerialState;
	int m_nSysEx;
	uint8_t m_SerialMessage[MAX_MIDI_MESSAGE];

	CWriteBufferDevice m_SendBuffer;
};
