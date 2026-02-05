//
// config.cpp
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

#include "config.h"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string>

#include <circle/net/ipaddress.h>
#include <dexed.h>
#include <fatfs/ff.h>

CConfig::CConfig(FATFS *pFileSystem) :
m_Properties{"minidexed.ini", pFileSystem}
{
}

CConfig::~CConfig()
{
}

void CConfig::Load()
{
	m_Properties.Load();

	// Number of Tone Generators and Polyphony
	m_nToneGenerators = m_Properties.GetSignedNumber("ToneGenerators", DefToneGenerators);
	m_nPolyphony = m_Properties.GetSignedNumber("Polyphony", DefaultNotes);
	// At present there are only two options for tone generators: min or max
	// and for the Pi 1,2,3 these are the same anyway.
	if ((m_nToneGenerators != MinToneGenerators) && (m_nToneGenerators != AllToneGenerators))
	{
		m_nToneGenerators = DefToneGenerators;
	}
	if (m_nPolyphony > MaxNotes)
	{
		m_nPolyphony = DefaultNotes;
	}

	m_bUSBGadget = m_Properties.GetNumber("USBGadget", 0) != 0;
	m_nUSBGadgetPin = m_Properties.GetNumber("USBGadgetPin", 0); // Default OFF
	SetUSBGadgetMode(m_bUSBGadget); // Might get overriden later by USBGadgetPin state

	m_SoundDevice = m_Properties.GetString("SoundDevice", "pwm");

	m_nSampleRate = m_Properties.GetNumber("SampleRate", 48000);
	m_bQuadDAC8Chan = m_Properties.GetNumber("QuadDAC8Chan", 0) != 0;
	if (m_SoundDevice == "hdmi")
	{
		m_nChunkSize = m_Properties.GetNumber("ChunkSize", 384 * 6);
	}
	else
	{
#ifdef ARM_ALLOW_MULTI_CORE
		m_nChunkSize = m_Properties.GetNumber("ChunkSize", m_bQuadDAC8Chan ? 1024 : 256); // 128 per channel
#else
		m_nChunkSize = m_Properties.GetNumber("ChunkSize", 1024);
#endif
	}
	m_nDACI2CAddress = static_cast<uint8_t>(m_Properties.GetNumber("DACI2CAddress", 0));
	m_bChannelsSwapped = m_Properties.GetNumber("ChannelsSwapped", 0) != 0;

	int newEngineType = m_Properties.GetSignedNumber("EngineType", 1);
	if (newEngineType == 2)
	{
		m_EngineType = MKI;
	}
	else if (newEngineType == 3)
	{
		m_EngineType = OPL;
	}
	else
	{
		m_EngineType = MSFA;
	}

	m_nMIDIBaudRate = m_Properties.GetNumber("MIDIBaudRate", 31250);

	const char *pMIDIThru = m_Properties.GetString("MIDIThru");
	if (pMIDIThru)
	{
		std::string Arg(pMIDIThru);

		size_t nPos = Arg.find(',');
		if (nPos != std::string::npos)
		{
			m_MIDIThruIn = Arg.substr(0, nPos);
			m_MIDIThruOut = Arg.substr(nPos + 1);

			if (m_MIDIThruIn.empty() || m_MIDIThruOut.empty())
			{
				m_MIDIThruIn.clear();
				m_MIDIThruOut.clear();
			}
		}
	}

	const char *pMIDIThru2 = m_Properties.GetString("MIDIThru2");
	if (pMIDIThru2)
	{
		std::string Arg(pMIDIThru2);

		size_t nPos = Arg.find(',');
		if (nPos != std::string::npos)
		{
			m_MIDIThru2In = Arg.substr(0, nPos);
			m_MIDIThru2Out = Arg.substr(nPos + 1);

			if (m_MIDIThru2In.empty() || m_MIDIThru2Out.empty())
			{
				m_MIDIThru2In.clear();
				m_MIDIThru2Out.clear();
			}
		}
	}

	m_bMIDIThruIgnoreClock = m_Properties.GetNumber("MIDIThruIgnoreClock", 0) != 0;
	m_bMIDIThruIgnoreActiveSensing = m_Properties.GetNumber("MIDIThruIgnoreActiveSensing", 0) != 0;

	m_bMIDIRXProgramChange = m_Properties.GetNumber("MIDIRXProgramChange", 1) != 0;
	m_bIgnoreAllNotesOff = m_Properties.GetNumber("IgnoreAllNotesOff", 0) != 0;
	m_bMIDIAutoVoiceDumpOnPC = m_Properties.GetNumber("MIDIAutoVoiceDumpOnPC", 0) != 0;
	m_bHeaderlessSysExVoices = m_Properties.GetNumber("HeaderlessSysExVoices", 0) != 0;
	m_bExpandPCAcrossBanks = m_Properties.GetNumber("ExpandPCAcrossBanks", 1) != 0;

	m_nMIDISystemCCVol = m_Properties.GetSignedNumber("MIDISystemCCVol", 0);
	m_nMIDISystemCCPan = m_Properties.GetSignedNumber("MIDISystemCCPan", 0);
	m_nMIDISystemCCDetune = m_Properties.GetSignedNumber("MIDISystemCCDetune", 0);
	m_nMIDIGlobalExpression = m_Properties.GetSignedNumber("MIDIGlobalExpression", 0);

	m_bLCDEnabled = m_Properties.GetNumber("LCDEnabled", 0) != 0;
	m_nLCDPinEnable = m_Properties.GetNumber("LCDPinEnable", 4);
	m_nLCDPinRegisterSelect = m_Properties.GetNumber("LCDPinRegisterSelect", 27);
	m_nLCDPinReadWrite = m_Properties.GetNumber("LCDPinReadWrite", 0);
	m_nLCDPinData4 = m_Properties.GetNumber("LCDPinData4", 22);
	m_nLCDPinData5 = m_Properties.GetNumber("LCDPinData5", 23);
	m_nLCDPinData6 = m_Properties.GetNumber("LCDPinData6", 24);
	m_nLCDPinData7 = m_Properties.GetNumber("LCDPinData7", 25);
	m_nLCDI2CAddress = static_cast<uint8_t>(m_Properties.GetNumber("LCDI2CAddress", 0));

	m_nSSD1306LCDI2CAddress = static_cast<uint8_t>(m_Properties.GetNumber("SSD1306LCDI2CAddress", 0));
	m_nSSD1306LCDWidth = m_Properties.GetNumber("SSD1306LCDWidth", 128);
	m_nSSD1306LCDHeight = m_Properties.GetNumber("SSD1306LCDHeight", 32);
	m_bSSD1306LCDRotate = m_Properties.GetNumber("SSD1306LCDRotate", 0) != 0;
	m_bSSD1306LCDMirror = m_Properties.GetNumber("SSD1306LCDMirror", 0) != 0;

	m_nSPIBus = m_Properties.GetNumber("SPIBus", SPI_INACTIVE); // Disabled by default
	m_nSPIMode = m_Properties.GetNumber("SPIMode", SPI_DEF_MODE);
	m_nSPIClockKHz = m_Properties.GetNumber("SPIClockKHz", SPI_DEF_CLOCK);

	m_bST7789Enabled = m_Properties.GetNumber("ST7789Enabled", 0) != 0;
	m_nST7789Data = m_Properties.GetNumber("ST7789Data", 0);
	m_nST7789Select = m_Properties.GetNumber("ST7789Select", 0);
	m_nST7789Reset = m_Properties.GetNumber("ST7789Reset", 0); // optional
	m_nST7789Backlight = m_Properties.GetNumber("ST7789Backlight", 0); // optional
	m_nST7789Width = m_Properties.GetNumber("ST7789Width", 240);
	m_nST7789Height = m_Properties.GetNumber("ST7789Height", 240);
	m_nST7789Rotation = m_Properties.GetNumber("ST7789Rotation", 0);
	m_nST7789FontSize = m_Properties.GetNumber("ST7789FontSize", 12);

	m_nLCDColumns = std::max(MinLCDColumns, static_cast<int>(m_Properties.GetNumber("LCDColumns", 16)));
	m_nLCDRows = std::max(MinLCDRows, static_cast<int>(m_Properties.GetNumber("LCDRows", 2)));

	m_nButtonPinPrev = m_Properties.GetNumber("ButtonPinPrev", 0);
	m_nButtonPinNext = m_Properties.GetNumber("ButtonPinNext", 0);
	m_nButtonPinBack = m_Properties.GetNumber("ButtonPinBack", 11);
	m_nButtonPinSelect = m_Properties.GetNumber("ButtonPinSelect", 11);
	m_nButtonPinHome = m_Properties.GetNumber("ButtonPinHome", 11);
	m_nButtonPinShortcut = m_Properties.GetNumber("ButtonPinShortcut", 11);

	m_ButtonActionPrev = m_Properties.GetString("ButtonActionPrev", "");
	m_ButtonActionNext = m_Properties.GetString("ButtonActionNext", "");
	m_ButtonActionBack = m_Properties.GetString("ButtonActionBack", "doubleclick");
	m_ButtonActionSelect = m_Properties.GetString("ButtonActionSelect", "click");
	m_ButtonActionHome = m_Properties.GetString("ButtonActionHome", "longpress");

	m_nDoubleClickTimeout = m_Properties.GetSignedNumber("DoubleClickTimeout", 400);
	m_nLongPressTimeout = m_Properties.GetSignedNumber("LongPressTimeout", 600);
	m_nMIDIRelativeDebounceTime = m_Properties.GetSignedNumber("MIDIRelativeDebounceTime", 0);

	m_nButtonPinPgmUp = m_Properties.GetNumber("ButtonPinPgmUp", 0);
	m_nButtonPinPgmDown = m_Properties.GetNumber("ButtonPinPgmDown", 0);
	m_nButtonPinBankUp = m_Properties.GetNumber("ButtonPinBankUp", 0);
	m_nButtonPinBankDown = m_Properties.GetNumber("ButtonPinBankDown", 0);
	m_nButtonPinTGUp = m_Properties.GetNumber("ButtonPinTGUp", 0);
	m_nButtonPinTGDown = m_Properties.GetNumber("ButtonPinTGDown", 0);

	m_ButtonActionPgmUp = m_Properties.GetString("ButtonActionPgmUp", "");
	m_ButtonActionPgmDown = m_Properties.GetString("ButtonActionPgmDown", "");
	m_ButtonActionBankUp = m_Properties.GetString("ButtonActionBankUp", "");
	m_ButtonActionBankDown = m_Properties.GetString("ButtonActionBankDown", "");
	m_ButtonActionTGUp = m_Properties.GetString("ButtonActionTGUp", "");
	m_ButtonActionTGDown = m_Properties.GetString("ButtonActionTGDown", "");

	m_nMIDIButtonCh = static_cast<uint8_t>(m_Properties.GetNumber("MIDIButtonCh", 0));
	m_nMIDIButtonNotes = m_Properties.GetNumber("MIDIButtonNotes", 0);

	m_nMIDIButtonPrev = m_Properties.GetNumber("MIDIButtonPrev", 0);
	m_nMIDIButtonNext = m_Properties.GetNumber("MIDIButtonNext", 0);
	m_nMIDIButtonBack = m_Properties.GetNumber("MIDIButtonBack", 0);
	m_nMIDIButtonSelect = m_Properties.GetNumber("MIDIButtonSelect", 0);
	m_nMIDIButtonHome = m_Properties.GetNumber("MIDIButtonHome", 0);

	m_MIDIButtonActionPrev = m_Properties.GetString("MIDIButtonActionPrev", "");
	m_MIDIButtonActionNext = m_Properties.GetString("MIDIButtonActionNext", "");
	m_MIDIButtonActionBack = m_Properties.GetString("MIDIButtonActionBack", "");
	m_MIDIButtonActionSelect = m_Properties.GetString("MIDIButtonActionSelect", "");
	m_MIDIButtonActionHome = m_Properties.GetString("MIDIButtonActionHome", "");

	m_nMIDIButtonPgmUp = m_Properties.GetNumber("MIDIButtonPgmUp", 0);
	m_nMIDIButtonPgmDown = m_Properties.GetNumber("MIDIButtonPgmDown", 0);
	m_nMIDIButtonBankUp = m_Properties.GetNumber("MIDIButtonBankUp", 0);
	m_nMIDIButtonBankDown = m_Properties.GetNumber("MIDIButtonBankDown", 0);
	m_nMIDIButtonTGUp = m_Properties.GetNumber("MIDIButtonTGUp", 0);
	m_nMIDIButtonTGDown = m_Properties.GetNumber("MIDIButtonTGDown", 0);

	m_MIDIButtonActionPgmUp = m_Properties.GetString("MIDIButtonActionPgmUp", "");
	m_MIDIButtonActionPgmDown = m_Properties.GetString("MIDIButtonActionPgmDown", "");
	m_MIDIButtonActionBankUp = m_Properties.GetString("MIDIButtonActionBankUp", "");
	m_MIDIButtonActionBankDown = m_Properties.GetString("MIDIButtonActionBankDown", "");
	m_MIDIButtonActionTGUp = m_Properties.GetString("MIDIButtonActionTGUp", "");
	m_MIDIButtonActionTGDown = m_Properties.GetString("MIDIButtonActionTGDown", "");

	m_bEncoderEnabled = m_Properties.GetNumber("EncoderEnabled", 0) != 0;
	m_nEncoderPinClock = m_Properties.GetNumber("EncoderPinClock", 10);
	m_nEncoderPinData = m_Properties.GetNumber("EncoderPinData", 9);

	// Parse encoder resolution
	std::string EncoderResolution = m_Properties.GetString("EncoderResolution", "full");
	if (EncoderResolution == "full")
	{
		m_nEncoderDetents = 4;
	}
	else if (EncoderResolution == "half")
	{
		m_nEncoderDetents = 2;
	}
	else if (EncoderResolution == "quarter")
	{
		m_nEncoderDetents = 1;
	}
	else
	{
		// Invalid value, use default
		m_nEncoderDetents = 4;
	}

	m_bMIDIDumpEnabled = m_Properties.GetNumber("MIDIDumpEnabled", 0) != 0;
	m_bProfileEnabled = m_Properties.GetNumber("ProfileEnabled", 0) != 0;
	m_bLogThrottling = m_Properties.GetNumber("LogThrottling", 0);

	m_bPerformanceSelectToLoad = m_Properties.GetNumber("PerformanceSelectToLoad", 0) != 0;
	m_bPerformanceSelectChannel = static_cast<uint8_t>(m_Properties.GetNumber("PerformanceSelectChannel", 0));

	// Network
	m_bNetworkEnabled = m_Properties.GetNumber("NetworkEnabled", 0) != 0;
	m_bNetworkDHCP = m_Properties.GetNumber("NetworkDHCP", 0) != 0;
	m_NetworkType = m_Properties.GetString("NetworkType", "wlan");
	m_NetworkHostname = m_Properties.GetString("NetworkHostname", "MiniDexed");
	if (const uint8_t *pIP = m_Properties.GetIPAddress("NetworkIPAddress")) m_INetworkIPAddress.Set(pIP);
	if (const uint8_t *pIP = m_Properties.GetIPAddress("NetworkSubnetMask")) m_INetworkSubnetMask.Set(pIP);
	if (const uint8_t *pIP = m_Properties.GetIPAddress("NetworkDefaultGateway")) m_INetworkDefaultGateway.Set(pIP);
	m_bSyslogEnabled = m_Properties.GetNumber("NetworkSyslogEnabled", 0) != 0;
	if (const uint8_t *pIP = m_Properties.GetIPAddress("NetworkDNSServer")) m_INetworkDNSServer.Set(pIP);
	m_bNetworkFTPEnabled = m_Properties.GetNumber("NetworkFTPEnabled", 0) != 0;
	if (const uint8_t *pIP = m_Properties.GetIPAddress("NetworkSyslogServerIPAddress")) m_INetworkSyslogServerIPAddress.Set(pIP);
	m_bUDPMIDIEnabled = m_Properties.GetNumber("UDPMIDIEnabled", 0) != 0;
	if (const uint8_t *pIP = m_Properties.GetIPAddress("UDPMIDIIPAddress")) m_IUDPMIDIIPAddress.Set(pIP);

	m_nMasterVolume = m_Properties.GetSignedNumber("MasterVolume", 64);

	m_nDefaultScreen = m_Properties.GetSignedNumber("DefaultScreen", 0);
}

int CConfig::GetToneGenerators() const
{
	return m_nToneGenerators;
}

int CConfig::GetPolyphony() const
{
	return m_nPolyphony;
}

int CConfig::GetTGsCore1() const
{
#ifndef ARM_ALLOW_MULTI_CORE
	return 0;
#else
	if (m_nToneGenerators > MinToneGenerators)
	{
		return TGsCore1 + TGsCore1Opt;
	}
	else
	{
		return TGsCore1;
	}
#endif
}

int CConfig::GetTGsCore23() const
{
#ifndef ARM_ALLOW_MULTI_CORE
	return 0;
#else
	if (m_nToneGenerators > MinToneGenerators)
	{
		return TGsCore23 + TGsCore23Opt;
	}
	else
	{
		return TGsCore23;
	}
#endif
}

bool CConfig::GetUSBGadget() const
{
	return m_bUSBGadget;
}

unsigned CConfig::GetUSBGadgetPin() const
{
	return m_nUSBGadgetPin;
}

bool CConfig::GetUSBGadgetMode() const
{
	return m_bUSBGadgetMode;
}

void CConfig::SetUSBGadgetMode(bool USBGadgetMode)
{
	m_bUSBGadgetMode = USBGadgetMode;
}

const char *CConfig::GetSoundDevice() const
{
	return m_SoundDevice.c_str();
}

unsigned CConfig::GetSampleRate() const
{
	return m_nSampleRate;
}

unsigned CConfig::GetChunkSize() const
{
	return m_nChunkSize;
}

uint8_t CConfig::GetDACI2CAddress() const
{
	return m_nDACI2CAddress;
}

bool CConfig::GetChannelsSwapped() const
{
	return m_bChannelsSwapped;
}

uint8_t CConfig::GetEngineType() const
{
	return m_EngineType;
}

bool CConfig::GetQuadDAC8Chan() const
{
	return m_bQuadDAC8Chan;
}

unsigned CConfig::GetMIDIBaudRate() const
{
	return m_nMIDIBaudRate;
}

const char *CConfig::GetMIDIThruIn() const
{
	return m_MIDIThruIn.c_str();
}

const char *CConfig::GetMIDIThruOut() const
{
	return m_MIDIThruOut.c_str();
}

const char *CConfig::GetMIDIThru2In() const
{
	return m_MIDIThru2In.c_str();
}

const char *CConfig::GetMIDIThru2Out() const
{
	return m_MIDIThru2Out.c_str();
}

bool CConfig::GetMIDIThruIgnoreClock() const
{
	return m_bMIDIThruIgnoreClock;
}

bool CConfig::GetMIDIThruIgnoreActiveSensing() const
{
	return m_bMIDIThruIgnoreActiveSensing;
}

bool CConfig::GetMIDIRXProgramChange() const
{
	return m_bMIDIRXProgramChange;
}

bool CConfig::GetIgnoreAllNotesOff() const
{
	return m_bIgnoreAllNotesOff;
}

bool CConfig::GetMIDIAutoVoiceDumpOnPC() const
{
	return m_bMIDIAutoVoiceDumpOnPC;
}

bool CConfig::GetHeaderlessSysExVoices() const
{
	return m_bHeaderlessSysExVoices;
}

bool CConfig::GetExpandPCAcrossBanks() const
{
	return m_bExpandPCAcrossBanks;
}

int CConfig::GetMIDISystemCCVol() const
{
	return m_nMIDISystemCCVol;
}

int CConfig::GetMIDISystemCCPan() const
{
	return m_nMIDISystemCCPan;
}

int CConfig::GetMIDISystemCCDetune() const
{
	return m_nMIDISystemCCDetune;
}

int CConfig::GetMIDIGlobalExpression() const
{
	return m_nMIDIGlobalExpression;
}

bool CConfig::GetLCDEnabled() const
{
	return m_bLCDEnabled;
}

unsigned CConfig::GetLCDPinEnable() const
{
	return m_nLCDPinEnable;
}

unsigned CConfig::GetLCDPinRegisterSelect() const
{
	return m_nLCDPinRegisterSelect;
}

unsigned CConfig::GetLCDPinReadWrite() const
{
	return m_nLCDPinReadWrite;
}

unsigned CConfig::GetLCDPinData4() const
{
	return m_nLCDPinData4;
}

unsigned CConfig::GetLCDPinData5() const
{
	return m_nLCDPinData5;
}

unsigned CConfig::GetLCDPinData6() const
{
	return m_nLCDPinData6;
}

unsigned CConfig::GetLCDPinData7() const
{
	return m_nLCDPinData7;
}

uint8_t CConfig::GetLCDI2CAddress() const
{
	return m_nLCDI2CAddress;
}

uint8_t CConfig::GetSSD1306LCDI2CAddress() const
{
	return m_nSSD1306LCDI2CAddress;
}

unsigned CConfig::GetSSD1306LCDWidth() const
{
	return m_nSSD1306LCDWidth;
}

unsigned CConfig::GetSSD1306LCDHeight() const
{
	return m_nSSD1306LCDHeight;
}

bool CConfig::GetSSD1306LCDRotate() const
{
	return m_bSSD1306LCDRotate;
}

bool CConfig::GetSSD1306LCDMirror() const
{
	return m_bSSD1306LCDMirror;
}

unsigned CConfig::GetSPIBus() const
{
	return m_nSPIBus;
}

unsigned CConfig::GetSPIMode() const
{
	return m_nSPIMode;
}

unsigned CConfig::GetSPIClockKHz() const
{
	return m_nSPIClockKHz;
}

bool CConfig::GetST7789Enabled() const
{
	return m_bST7789Enabled;
}

unsigned CConfig::GetST7789Data() const
{
	return m_nST7789Data;
}

unsigned CConfig::GetST7789Select() const
{
	return m_nST7789Select;
}

unsigned CConfig::GetST7789Reset() const
{
	return m_nST7789Reset;
}

unsigned CConfig::GetST7789Backlight() const
{
	return m_nST7789Backlight;
}

unsigned CConfig::GetST7789Width() const
{
	return m_nST7789Width;
}

unsigned CConfig::GetST7789Height() const
{
	return m_nST7789Height;
}

unsigned CConfig::GetST7789Rotation() const
{
	return m_nST7789Rotation;
}

unsigned CConfig::GetST7789FontSize() const
{
	return m_nST7789FontSize;
}

int CConfig::GetLCDColumns() const
{
	return m_nLCDColumns;
}

int CConfig::GetLCDRows() const
{
	return m_nLCDRows;
}

unsigned CConfig::GetButtonPinPrev() const
{
	return m_nButtonPinPrev;
}

unsigned CConfig::GetButtonPinNext() const
{
	return m_nButtonPinNext;
}

unsigned CConfig::GetButtonPinBack() const
{
	return m_nButtonPinBack;
}

unsigned CConfig::GetButtonPinSelect() const
{
	return m_nButtonPinSelect;
}

unsigned CConfig::GetButtonPinHome() const
{
	return m_nButtonPinHome;
}

unsigned CConfig::GetButtonPinShortcut() const
{
	return m_nButtonPinShortcut;
}

const char *CConfig::GetButtonActionPrev() const
{
	return m_ButtonActionPrev.c_str();
}

const char *CConfig::GetButtonActionNext() const
{
	return m_ButtonActionNext.c_str();
}

const char *CConfig::GetButtonActionBack() const
{
	return m_ButtonActionBack.c_str();
}

const char *CConfig::GetButtonActionSelect() const
{
	return m_ButtonActionSelect.c_str();
}

const char *CConfig::GetButtonActionHome() const
{
	return m_ButtonActionHome.c_str();
}

int CConfig::GetDoubleClickTimeout() const
{
	return m_nDoubleClickTimeout;
}

int CConfig::GetLongPressTimeout() const
{
	return m_nLongPressTimeout;
}

int CConfig::GetMIDIRelativeDebounceTime() const
{
	return m_nMIDIRelativeDebounceTime;
}

unsigned CConfig::GetButtonPinPgmUp() const
{
	return m_nButtonPinPgmUp;
}

unsigned CConfig::GetButtonPinPgmDown() const
{
	return m_nButtonPinPgmDown;
}

unsigned CConfig::GetButtonPinBankUp() const
{
	return m_nButtonPinBankUp;
}

unsigned CConfig::GetButtonPinBankDown() const
{
	return m_nButtonPinBankDown;
}

unsigned CConfig::GetButtonPinTGUp() const
{
	return m_nButtonPinTGUp;
}

unsigned CConfig::GetButtonPinTGDown() const
{
	return m_nButtonPinTGDown;
}

const char *CConfig::GetButtonActionPgmUp() const
{
	return m_ButtonActionPgmUp.c_str();
}

const char *CConfig::GetButtonActionPgmDown() const
{
	return m_ButtonActionPgmDown.c_str();
}

const char *CConfig::GetButtonActionBankUp() const
{
	return m_ButtonActionBankUp.c_str();
}

const char *CConfig::GetButtonActionBankDown() const
{
	return m_ButtonActionBankDown.c_str();
}

const char *CConfig::GetButtonActionTGUp() const
{
	return m_ButtonActionTGUp.c_str();
}

const char *CConfig::GetButtonActionTGDown() const
{
	return m_ButtonActionTGDown.c_str();
}

uint8_t CConfig::GetMIDIButtonCh() const
{
	return m_nMIDIButtonCh;
}

unsigned CConfig::GetMIDIButtonNotes() const
{
	return m_nMIDIButtonNotes;
}

unsigned CConfig::GetMIDIButtonPrev() const
{
	return m_nMIDIButtonPrev;
}

unsigned CConfig::GetMIDIButtonNext() const
{
	return m_nMIDIButtonNext;
}

unsigned CConfig::GetMIDIButtonBack() const
{
	return m_nMIDIButtonBack;
}

unsigned CConfig::GetMIDIButtonSelect() const
{
	return m_nMIDIButtonSelect;
}

unsigned CConfig::GetMIDIButtonHome() const
{
	return m_nMIDIButtonHome;
}

const char *CConfig::GetMIDIButtonActionPrev() const
{
	return m_MIDIButtonActionPrev.c_str();
}

const char *CConfig::GetMIDIButtonActionNext() const
{
	return m_MIDIButtonActionNext.c_str();
}

const char *CConfig::GetMIDIButtonActionBack() const
{
	return m_MIDIButtonActionBack.c_str();
}

const char *CConfig::GetMIDIButtonActionSelect() const
{
	return m_MIDIButtonActionSelect.c_str();
}

const char *CConfig::GetMIDIButtonActionHome() const
{
	return m_MIDIButtonActionHome.c_str();
}

unsigned CConfig::GetMIDIButtonPgmUp() const
{
	return m_nMIDIButtonPgmUp;
}

unsigned CConfig::GetMIDIButtonPgmDown() const
{
	return m_nMIDIButtonPgmDown;
}

unsigned CConfig::GetMIDIButtonBankUp() const
{
	return m_nMIDIButtonBankUp;
}

unsigned CConfig::GetMIDIButtonBankDown() const
{
	return m_nMIDIButtonBankDown;
}

unsigned CConfig::GetMIDIButtonTGUp() const
{
	return m_nMIDIButtonTGUp;
}

unsigned CConfig::GetMIDIButtonTGDown() const
{
	return m_nMIDIButtonTGDown;
}

const char *CConfig::GetMIDIButtonActionPgmUp() const
{
	return m_MIDIButtonActionPgmUp.c_str();
}

const char *CConfig::GetMIDIButtonActionPgmDown() const
{
	return m_MIDIButtonActionPgmDown.c_str();
}

const char *CConfig::GetMIDIButtonActionBankUp() const
{
	return m_MIDIButtonActionBankUp.c_str();
}

const char *CConfig::GetMIDIButtonActionBankDown() const
{
	return m_MIDIButtonActionBankDown.c_str();
}

const char *CConfig::GetMIDIButtonActionTGUp() const
{
	return m_MIDIButtonActionTGUp.c_str();
}

const char *CConfig::GetMIDIButtonActionTGDown() const
{
	return m_MIDIButtonActionTGDown.c_str();
}

bool CConfig::GetEncoderEnabled() const
{
	return m_bEncoderEnabled;
}

unsigned CConfig::GetEncoderPinClock() const
{
	return m_nEncoderPinClock;
}

unsigned CConfig::GetEncoderPinData() const
{
	return m_nEncoderPinData;
}

unsigned CConfig::GetEncoderDetents() const
{
	return m_nEncoderDetents;
}

bool CConfig::GetMIDIDumpEnabled() const
{
	return m_bMIDIDumpEnabled;
}

bool CConfig::GetProfileEnabled() const
{
	return m_bProfileEnabled;
}

bool CConfig::GetLogThrottling() const
{
	return m_bLogThrottling;
}

bool CConfig::GetPerformanceSelectToLoad() const
{
	return m_bPerformanceSelectToLoad;
}

uint8_t CConfig::GetPerformanceSelectChannel() const
{
	return m_bPerformanceSelectChannel;
}

// Network
bool CConfig::GetNetworkEnabled() const
{
	return m_bNetworkEnabled;
}

bool CConfig::GetNetworkDHCP() const
{
	return m_bNetworkDHCP;
}

const char *CConfig::GetNetworkType() const
{
	return m_NetworkType.c_str();
}

const char *CConfig::GetNetworkHostname() const
{
	return m_NetworkHostname.c_str();
}

const CIPAddress &CConfig::GetNetworkIPAddress() const
{
	return m_INetworkIPAddress;
}

const CIPAddress &CConfig::GetNetworkSubnetMask() const
{
	return m_INetworkSubnetMask;
}

const CIPAddress &CConfig::GetNetworkDefaultGateway() const
{
	return m_INetworkDefaultGateway;
}

const CIPAddress &CConfig::GetNetworkDNSServer() const
{
	return m_INetworkDNSServer;
}

bool CConfig::GetSyslogEnabled() const
{
	return m_bSyslogEnabled;
}

const CIPAddress &CConfig::GetNetworkSyslogServerIPAddress() const
{
	return m_INetworkSyslogServerIPAddress;
}

bool CConfig::GetNetworkFTPEnabled() const
{
	return m_bNetworkFTPEnabled;
}

bool CConfig::GetUDPMIDIEnabled() const
{
	return m_bUDPMIDIEnabled;
}

const CIPAddress &CConfig::GetUDPMIDIIPAddress() const
{
	return m_IUDPMIDIIPAddress;
}
