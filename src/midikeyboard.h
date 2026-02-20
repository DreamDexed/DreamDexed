//
// midikeyboard.h
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
#include <queue>

#include <circle/device.h>
#include <circle/string.h>
#include <circle/usb/usbmidi.h>

#include "config.h"
#include "mididevice.h"
#include "userinterface.h"

#define USB_SYSEX_BUFFER_SIZE (MAX_DX7_SYSEX_LENGTH + 128) // Allow a bit spare to handle unexpected SysEx messages

class CMiniDexed;

class CMIDIKeyboard : public CMIDIDevice
{
public:
	CMIDIKeyboard(CMiniDexed *pSynthesizer, CConfig *pConfig, CUserInterface *pUI, int nInstance = 0);
	~CMIDIKeyboard();

	void Process(bool bPlugAndPlayUpdated);

	void Send(const uint8_t *pMessage, int nLength, int nCable = 0) override;

private:
	static void MIDIPacketHandler(unsigned nCable, uint8_t *pPacket, unsigned nLength, unsigned nDevice, void *pParam);
	static void DeviceRemovedHandler(CDevice *pDevice, void *pContext);

	void USBMIDIMessageHandler(uint8_t *pPacket, int nLength, int nCable, int nDevice);

private:
	struct TSendQueueEntry
	{
		uint8_t *pMessage;
		int nLength;
		int nCable;
	};
	uint8_t m_SysEx[USB_SYSEX_BUFFER_SIZE];
	int m_nSysExIdx;

private:
	int m_nInstance;
	CString m_DeviceName;

	CUSBMIDIDevice *volatile m_pMIDIDevice;

	std::queue<TSendQueueEntry> m_SendQueue;
};
