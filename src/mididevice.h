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
#ifndef _mididevice_h
#define _mididevice_h

#include "config.h"
#include <string>
#include <unordered_map>
#include <circle/types.h>
#include <circle/spinlock.h>
#include "userinterface.h"

#define MAX_DX7_SYSEX_LENGTH 4104
#define MAX_MIDI_MESSAGE MAX_DX7_SYSEX_LENGTH

class CMiniDexed;
class CMIDIDevice;

struct TMIDIRoute
{
	~TMIDIRoute ();

	enum TRouteOP
	{
		P2 = 0x82,
		LtCenter = 0x90,
		GtCenter = 0x91,
		Betw00n07 = 0x92,
		Betw08n15 = 0x93,
		Betw16n23 = 0x94
	};

	u8 ucSCable;
	u8 ucSCh;
	u8 ucSType;
	u8 ucSP1;
	u8 ucSP2;
	u8 ucDCh;
	u8 ucDType;
	u8 ucDP1;
	u8 ucDP2;
	u8 ucTimerTarget;
	unsigned usTimerExpire;
	TKernelTimerHandle hTimer;
	bool bSkip;
	bool bToggle;
	bool bGroup;
	bool bGroupHead;
	bool bGroupActive;
	bool bGroupHold; // hold flag for GroupHeads, or set hold the group
};

void GetRoutedMIDI (TMIDIRoute *pRouteMap, CMIDIDevice *pDevice, u8 *pCable, u8 *pChannel, u8 *pType, u8 *pP1, u8 *pP2, bool *bSkip);

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
	CMIDIDevice (CMiniDexed *pSynthesizer, CConfig *pConfig, CUserInterface *pUI);
	virtual ~CMIDIDevice (void);

	void SetChannel (u8 ucChannel, unsigned nTG);
	u8 GetChannel (unsigned nTG) const;

	void SetRouteMap (TMIDIRoute *pRouteMap);

	virtual void Send (const u8 *pMessage, size_t nLength, unsigned nCable = 0) {}
	// Change signature to specify device name
	void SendSystemExclusiveVoice(uint8_t nVoice, const std::string& deviceName, unsigned nCable, uint8_t nTG);
	const std::string& GetDeviceName() const { return m_DeviceName; }

	static void s_HandleTimerTimeout(TKernelTimerHandle hTimer, void *pParam, void *pContext);

protected:
	void MIDIMessageHandler (const u8 *pMessage, size_t nLength, unsigned nCable = 0);
	void AddDevice (const char *pDeviceName);
	void HandleSystemExclusive(const uint8_t* pMessage, const size_t nLength, const unsigned nCable, const uint8_t nTG);

	virtual void MIDIListener (u8 ucCable, u8 ucChannel, u8 ucType, u8 ucP1, u8 ucP2);

private:
	bool HandleMIDISystemCC(const u8 ucCC, const u8 ucCCval);

private:
	CMiniDexed *m_pSynthesizer;
	CConfig *m_pConfig;
	CUserInterface *m_pUI;

	u8 m_ChannelMap[CConfig::AllToneGenerators];
	u8 m_PreviousChannelMap[CConfig::AllToneGenerators]; // Store previous channels for OMNI OFF restore
	
	unsigned m_nMIDISystemCCVol;
	unsigned m_nMIDISystemCCPan;
	unsigned m_nMIDISystemCCDetune;
	u32	 m_MIDISystemCCBitmap[4]; // to allow for 128 bit entries
	unsigned m_nMIDIGlobalExpression;

	std::string m_DeviceName;

	TMIDIRoute *m_pRouteMap;

	typedef std::unordered_map<std::string, CMIDIDevice *> TDeviceMap;
	static TDeviceMap s_DeviceMap;

	CSpinLock m_MIDISpinLock;
};

#endif
