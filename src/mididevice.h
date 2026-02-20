//
// mididevice.h
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
#include <string>
#include <unordered_map>

#include <circle/spinlock.h>

#include "config.h"
#include "userinterface.h"

#define MAX_DX7_SYSEX_LENGTH 4104
#define MAX_MIDI_MESSAGE MAX_DX7_SYSEX_LENGTH

class CMiniDexed;

class CMIDIDevice
{
public:
	enum TChannel
	{
		Channels = 16,
		OmniMode = Channels,
		Disabled,
		ChannelUnknown
	};

public:
	CMIDIDevice(CMiniDexed *pSynthesizer, CConfig *pConfig, CUserInterface *pUI);
	virtual ~CMIDIDevice();

	void SetChannel(int nChannel, int nTG);
	int GetChannel(int nTG) const;

	virtual void Send(const uint8_t *pMessage, int nLength, int nCable = 0) {}
	// Change signature to specify device name
	void SendSystemExclusiveVoice(int nVoice, const std::string &deviceName, int nCable, int nTG);
	const std::string &GetDeviceName() const { return m_DeviceName; }

protected:
	void MIDIMessageHandler(const uint8_t *pMessage, int nLength, int nCable = 0);
	void AddDevice(const char *pDeviceName);
	void HandleSystemExclusive(const uint8_t *pMessage, const int nLength, const int nCable, const int nTG);

private:
	bool HandleMIDISystemCC(const uint8_t ucCC, const uint8_t ucCCval);

private:
	CMiniDexed *m_pSynthesizer;
	CConfig *m_pConfig;
	CUserInterface *m_pUI;

	uint8_t m_ChannelMap[CConfig::AllToneGenerators];
	uint8_t m_PreviousChannelMap[CConfig::AllToneGenerators]; // Store previous channels for OMNI OFF restore

	int m_nMIDISystemCCVol;
	int m_nMIDISystemCCPan;
	int m_nMIDISystemCCDetune;
	uint32_t m_MIDISystemCCBitmap[4]; // to allow for 128 bit entries
	int m_nMIDIGlobalExpression;

	std::string m_DeviceName;

	typedef std::unordered_map<std::string, CMIDIDevice *> TDeviceMap;
	static TDeviceMap s_DeviceMap;

	CSpinLock m_MIDISpinLock;
};
