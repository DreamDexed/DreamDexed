//
// config.h
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

#include <Properties/propertiesfatfsfile.h>
#include <circle/net/ipaddress.h>
#include <circle/sysconfig.h>
#include <fatfs/ff.h>

#define SPI_INACTIVE 255
#define SPI_DEF_CLOCK 15000 // kHz
#define SPI_DEF_MODE 0 // Default mode (0,1,2,3)

class CConfig // Configuration for MiniDexed
{
public:
// Set maximum, minimum and default numbers of tone generators, depending on Pi version.
// Actual number in can be changed via config settings for some Pis.
#ifndef ARM_ALLOW_MULTI_CORE
	// Pi V1 or Zero (single core)
	static constexpr int MinToneGenerators = 1;
	static constexpr int AllToneGenerators = 1;
	static constexpr int DefToneGenerators = AllToneGenerators;
	static constexpr int Buses = 0;
	static constexpr int BusFXChains = 0;
#else
	static constexpr int TGsCore1 = 2; // process 2 TGs on core 1
	static constexpr int TGsCore23 = 3; // process 3 TGs on core 2 and 3 each
#if (RASPPI == 4)
	static constexpr int TGsCore1Opt = 4; // process optional additional 4 TGs on core 1
	static constexpr int TGsCore23Opt = 6; // process optional additional 6 TGs on core 2 and 3 each
	static constexpr int Buses = 3;
#elif (RASPPI == 5)
	static constexpr int TGsCore1Opt = 6; // process optional additional 6 TGs on core 1
	static constexpr int TGsCore23Opt = 9; // process optional additional 9 TGs on core 2 and 3 each
	static constexpr int Buses = 4;
#else // Pi 2 or 3 quad core
	static constexpr int TGsCore1Opt = 0;
	static constexpr int TGsCore23Opt = 0;
	static constexpr int Buses = 1;
#endif
	static constexpr int MinToneGenerators = TGsCore1 + 2 * TGsCore23;
	static constexpr int AllToneGenerators = TGsCore1 + TGsCore1Opt + 2 * TGsCore23 + 2 * TGsCore23Opt;
	static constexpr int DefToneGenerators = MinToneGenerators;

	static constexpr int BusFXChains = 2;
#endif
	static constexpr int Outputs = 1;
	static constexpr int FXMixers = BusFXChains * Buses;
	static constexpr int FXChains = FXMixers + Outputs;
	static constexpr int MasterFX = FXMixers;

// Set maximum polyphony, depending on PI version.  This can be changed via config settings
#if RASPPI == 1
	static constexpr int MaxNotes = 8;
	static constexpr int DefaultNotes = 8;
#elif RASPPI == 4
	static constexpr int MaxNotes = 32;
	static constexpr int DefaultNotes = 24;
#elif RASPPI == 5
	static constexpr int MaxNotes = 32;
	static constexpr int DefaultNotes = 32;
#else
	static constexpr int MaxNotes = 16;
	static constexpr int DefaultNotes = 16;
#endif

	static constexpr int MaxChunkSize = 4096;

#if RASPPI <= 3
	static constexpr int MaxUSBMIDIDevices = 2;
#else
	static constexpr int MaxUSBMIDIDevices = 4;
#endif

	static constexpr int MinLCDColumns = 15; // ST7789 240px wide LCD
	static constexpr int MinLCDRows = 2;

public:
	CConfig(FATFS *pFileSystem);
	~CConfig();

	void Load();

	// TGs and Polyphony
	int GetToneGenerators() const;
	int GetPolyphony() const;
	int GetTGsCore1() const;
	int GetTGsCore23() const;

	// USB Mode
	bool GetUSBGadget() const;
	unsigned GetUSBGadgetPin() const;
	bool GetUSBGadgetMode() const; // true if in USB gadget mode depending on USBGadget and USBGadgetPin
	void SetUSBGadgetMode(bool USBGadgetMode);

	// Sound device
	const char *GetSoundDevice() const;
	unsigned GetSampleRate() const;
	unsigned GetChunkSize() const;
	uint8_t GetDACI2CAddress() const; // 0 for auto probing
	bool GetChannelsSwapped() const;
	uint8_t GetEngineType() const;
	bool GetQuadDAC8Chan() const; // false if not specified

	// MIDI
	unsigned GetMIDIBaudRate() const;
	const char *GetMIDIThruIn() const; // "" if not specified
	const char *GetMIDIThruOut() const; // "" if not specified
	const char *GetMIDIThru2In() const; // "" if not specified
	const char *GetMIDIThru2Out() const; // "" if not specified
	bool GetMIDIThruIgnoreClock() const; // false if not specified
	bool GetMIDIThruIgnoreActiveSensing() const; // false if not specified
	bool GetMIDIRXProgramChange() const; // true if not specified
	bool GetIgnoreAllNotesOff() const;
	bool GetMIDIAutoVoiceDumpOnPC() const; // false if not specified
	bool GetHeaderlessSysExVoices() const; // false if not specified
	bool GetExpandPCAcrossBanks() const; // true if not specified
	int GetMIDISystemCCVol() const;
	int GetMIDISystemCCPan() const;
	int GetMIDISystemCCDetune() const;
	int GetMIDIGlobalExpression() const;

	// HD44780 LCD
	// GPIO pin numbers are chip numbers, not header positions
	bool GetLCDEnabled() const;
	unsigned GetLCDPinEnable() const;
	unsigned GetLCDPinRegisterSelect() const;
	unsigned GetLCDPinReadWrite() const; // set to 0 if not connected
	unsigned GetLCDPinData4() const;
	unsigned GetLCDPinData5() const;
	unsigned GetLCDPinData6() const;
	unsigned GetLCDPinData7() const;
	uint8_t GetLCDI2CAddress() const;

	// SSD1306 LCD
	uint8_t GetSSD1306LCDI2CAddress() const;
	unsigned GetSSD1306LCDWidth() const;
	unsigned GetSSD1306LCDHeight() const;
	bool GetSSD1306LCDRotate() const;
	bool GetSSD1306LCDMirror() const;

	// SPI support
	unsigned GetSPIBus() const;
	unsigned GetSPIMode() const;
	unsigned GetSPIClockKHz() const;

	// ST7789 LCD
	bool GetST7789Enabled() const;
	unsigned GetST7789Data() const;
	unsigned GetST7789Select() const;
	unsigned GetST7789Reset() const;
	unsigned GetST7789Backlight() const;
	unsigned GetST7789Width() const;
	unsigned GetST7789Height() const;
	unsigned GetST7789Rotation() const;
	unsigned GetST7789FontSize() const;

	int GetLCDColumns() const;
	int GetLCDRows() const;

	// GPIO Button Navigation
	// GPIO pin numbers are chip numbers, not header positions
	unsigned GetButtonPinPrev() const;
	unsigned GetButtonPinNext() const;
	unsigned GetButtonPinBack() const;
	unsigned GetButtonPinSelect() const;
	unsigned GetButtonPinHome() const;
	unsigned GetButtonPinShortcut() const;

	// Action type for buttons: "click", "doubleclick", "longpress", ""
	const char *GetButtonActionPrev() const;
	const char *GetButtonActionNext() const;
	const char *GetButtonActionBack() const;
	const char *GetButtonActionSelect() const;
	const char *GetButtonActionHome() const;

	// Timeouts for button events in milliseconds
	int GetDoubleClickTimeout() const;
	int GetLongPressTimeout() const;
	int GetMIDIRelativeDebounceTime() const;

	// GPIO Button Program and TG Selection
	// GPIO pin numbers are chip numbers, not header positions
	unsigned GetButtonPinPgmUp() const;
	unsigned GetButtonPinPgmDown() const;
	unsigned GetButtonPinBankUp() const;
	unsigned GetButtonPinBankDown() const;
	unsigned GetButtonPinTGUp() const;
	unsigned GetButtonPinTGDown() const;

	// Action type for buttons: "click", "doubleclick", "longpress", ""
	const char *GetButtonActionPgmUp() const;
	const char *GetButtonActionPgmDown() const;
	const char *GetButtonActionBankUp() const;
	const char *GetButtonActionBankDown() const;
	const char *GetButtonActionTGUp() const;
	const char *GetButtonActionTGDown() const;

	// MIDI Button Navigation
	uint8_t GetMIDIButtonCh() const;
	unsigned GetMIDIButtonNotes() const;

	unsigned GetMIDIButtonPrev() const;
	unsigned GetMIDIButtonNext() const;
	unsigned GetMIDIButtonBack() const;
	unsigned GetMIDIButtonSelect() const;
	unsigned GetMIDIButtonHome() const;

	// Action type for Midi buttons: "click", "doubleclick", "longpress", "dec", "inc", ""
	const char *GetMIDIButtonActionPrev() const;
	const char *GetMIDIButtonActionNext() const;
	const char *GetMIDIButtonActionBack() const;
	const char *GetMIDIButtonActionSelect() const;
	const char *GetMIDIButtonActionHome() const;

	// MIDI Button Program and TG Selection
	unsigned GetMIDIButtonPgmUp() const;
	unsigned GetMIDIButtonPgmDown() const;
	unsigned GetMIDIButtonBankUp() const;
	unsigned GetMIDIButtonBankDown() const;
	unsigned GetMIDIButtonTGUp() const;
	unsigned GetMIDIButtonTGDown() const;

	// Action type for buttons: "click", "doubleclick", "longpress", "dec", "inc", ""
	const char *GetMIDIButtonActionPgmUp() const;
	const char *GetMIDIButtonActionPgmDown() const;
	const char *GetMIDIButtonActionBankUp() const;
	const char *GetMIDIButtonActionBankDown() const;
	const char *GetMIDIButtonActionTGUp() const;
	const char *GetMIDIButtonActionTGDown() const;

	// KY-040 Rotary Encoder
	// GPIO pin numbers are chip numbers, not header positions
	bool GetEncoderEnabled() const;
	unsigned GetEncoderPinClock() const;
	unsigned GetEncoderPinData() const;
	unsigned GetEncoderDetents() const;

	// Debug
	bool GetMIDIDumpEnabled() const;
	bool GetProfileEnabled() const;
	bool GetLogThrottling() const;

	// Load performance mode. 0 for load just rotating encoder, 1 load just when Select is pushed
	bool GetPerformanceSelectToLoad() const;
	uint8_t GetPerformanceSelectChannel() const;

	int GetMasterVolume() const { return m_nMasterVolume; }

	int GetDefaultScreen() const { return m_nDefaultScreen; }

	// Network
	bool GetNetworkEnabled() const;
	bool GetNetworkDHCP() const;
	const char *GetNetworkType() const;
	const char *GetNetworkHostname() const;
	const CIPAddress &GetNetworkIPAddress() const;
	const CIPAddress &GetNetworkSubnetMask() const;
	const CIPAddress &GetNetworkDefaultGateway() const;
	const CIPAddress &GetNetworkDNSServer() const;
	bool GetSyslogEnabled() const;
	const CIPAddress &GetNetworkSyslogServerIPAddress() const;
	bool GetNetworkFTPEnabled() const;
	bool GetUDPMIDIEnabled() const;
	const CIPAddress &GetUDPMIDIIPAddress() const;

private:
	CPropertiesFatFsFile m_Properties;

	int m_nToneGenerators;
	int m_nPolyphony;

	bool m_bUSBGadget;
	unsigned m_nUSBGadgetPin;
	bool m_bUSBGadgetMode;

	std::string m_SoundDevice;
	unsigned m_nSampleRate;
	unsigned m_nChunkSize;
	uint8_t m_nDACI2CAddress;
	bool m_bChannelsSwapped;
	uint8_t m_EngineType;
	bool m_bQuadDAC8Chan;

	unsigned m_nMIDIBaudRate;
	std::string m_MIDIThruIn;
	std::string m_MIDIThruOut;
	std::string m_MIDIThru2In;
	std::string m_MIDIThru2Out;
	bool m_bMIDIThruIgnoreClock;
	bool m_bMIDIThruIgnoreActiveSensing;
	bool m_bMIDIRXProgramChange;
	bool m_bIgnoreAllNotesOff;
	bool m_bMIDIAutoVoiceDumpOnPC;
	bool m_bHeaderlessSysExVoices;
	bool m_bExpandPCAcrossBanks;
	int m_nMIDISystemCCVol;
	int m_nMIDISystemCCPan;
	int m_nMIDISystemCCDetune;
	int m_nMIDIGlobalExpression;

	bool m_bLCDEnabled;
	unsigned m_nLCDPinEnable;
	unsigned m_nLCDPinRegisterSelect;
	unsigned m_nLCDPinReadWrite;
	unsigned m_nLCDPinData4;
	unsigned m_nLCDPinData5;
	unsigned m_nLCDPinData6;
	unsigned m_nLCDPinData7;
	uint8_t m_nLCDI2CAddress;

	uint8_t m_nSSD1306LCDI2CAddress;
	unsigned m_nSSD1306LCDWidth;
	unsigned m_nSSD1306LCDHeight;
	bool m_bSSD1306LCDRotate;
	bool m_bSSD1306LCDMirror;

	unsigned m_nSPIBus;
	unsigned m_nSPIMode;
	unsigned m_nSPIClockKHz;

	bool m_bST7789Enabled;
	unsigned m_nST7789Data;
	unsigned m_nST7789Select;
	unsigned m_nST7789Reset;
	unsigned m_nST7789Backlight;
	unsigned m_nST7789Width;
	unsigned m_nST7789Height;
	unsigned m_nST7789Rotation;
	unsigned m_nST7789FontSize;

	int m_nLCDColumns;
	int m_nLCDRows;

	unsigned m_nButtonPinPrev;
	unsigned m_nButtonPinNext;
	unsigned m_nButtonPinBack;
	unsigned m_nButtonPinSelect;
	unsigned m_nButtonPinHome;
	unsigned m_nButtonPinShortcut;
	unsigned m_nButtonPinPgmUp;
	unsigned m_nButtonPinPgmDown;
	unsigned m_nButtonPinBankUp;
	unsigned m_nButtonPinBankDown;
	unsigned m_nButtonPinTGUp;
	unsigned m_nButtonPinTGDown;

	std::string m_ButtonActionPrev;
	std::string m_ButtonActionNext;
	std::string m_ButtonActionBack;
	std::string m_ButtonActionSelect;
	std::string m_ButtonActionHome;
	std::string m_ButtonActionPgmUp;
	std::string m_ButtonActionPgmDown;
	std::string m_ButtonActionBankUp;
	std::string m_ButtonActionBankDown;
	std::string m_ButtonActionTGUp;
	std::string m_ButtonActionTGDown;

	std::string m_MIDIButtonActionPrev;
	std::string m_MIDIButtonActionNext;
	std::string m_MIDIButtonActionBack;
	std::string m_MIDIButtonActionSelect;
	std::string m_MIDIButtonActionHome;
	std::string m_MIDIButtonActionPgmUp;
	std::string m_MIDIButtonActionPgmDown;
	std::string m_MIDIButtonActionBankUp;
	std::string m_MIDIButtonActionBankDown;
	std::string m_MIDIButtonActionTGUp;
	std::string m_MIDIButtonActionTGDown;

	int m_nDoubleClickTimeout;
	int m_nLongPressTimeout;
	int m_nMIDIRelativeDebounceTime;

	uint8_t m_nMIDIButtonCh;
	unsigned m_nMIDIButtonNotes;
	unsigned m_nMIDIButtonPrev;
	unsigned m_nMIDIButtonNext;
	unsigned m_nMIDIButtonBack;
	unsigned m_nMIDIButtonSelect;
	unsigned m_nMIDIButtonHome;
	unsigned m_nMIDIButtonPgmUp;
	unsigned m_nMIDIButtonPgmDown;
	unsigned m_nMIDIButtonBankUp;
	unsigned m_nMIDIButtonBankDown;
	unsigned m_nMIDIButtonTGUp;
	unsigned m_nMIDIButtonTGDown;

	bool m_bEncoderEnabled;
	unsigned m_nEncoderPinClock;
	unsigned m_nEncoderPinData;
	unsigned m_nEncoderDetents;

	bool m_bMIDIDumpEnabled;
	bool m_bProfileEnabled;
	bool m_bPerformanceSelectToLoad;
	uint8_t m_bPerformanceSelectChannel;

	int m_nMasterVolume; // Master volume 0-127

	int m_nDefaultScreen; // 0 Default, 1 Performance

	// Network
	bool m_bNetworkEnabled;
	bool m_bNetworkDHCP;
	std::string m_NetworkType;
	std::string m_NetworkHostname;
	CIPAddress m_INetworkIPAddress;
	CIPAddress m_INetworkSubnetMask;
	CIPAddress m_INetworkDefaultGateway;
	CIPAddress m_INetworkDNSServer;
	bool m_bSyslogEnabled;
	CIPAddress m_INetworkSyslogServerIPAddress;
	bool m_bNetworkFTPEnabled;
	bool m_bUDPMIDIEnabled;
	CIPAddress m_IUDPMIDIIPAddress;

	bool m_bLogThrottling;
};
