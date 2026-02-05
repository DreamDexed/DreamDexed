//
// sysexfileloader.h
//
// See: https://github.com/asb2m10/dexed/blob/master/Documentation/sysex-format.txt
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
#include <string>

#include <circle/macros.h>

class CSysExFileLoader // Loader for DX7 .syx files
{
public:
	static const int MaxVoiceBankID = 16383; // i.e. 14-bit MSB/LSB value between 0 and 16383
	static const int VoicesPerBank = 32;
	static const int SizePackedVoice = 128;
	static const int SizeSingleVoice = 156;
	static const int VoiceSysExHdrSize = 8; // Additional (optional) Header/Footer bytes for bank of 32 voices
	static const int VoiceSysExSize = 4096; // Bank of 32 voices as per DX7 MIDI Spec
	static const int MaxSubDirs = 3; // Number of nested subdirectories supported.

	struct TVoiceBank
	{
		uint8_t StatusStart; // 0xF0
		uint8_t CompanyID; // 0x43
		uint8_t SubStatus; // 0x00
		uint8_t Format; // 0x09
		uint8_t ByteCountMS; // 0x20
		uint8_t ByteCountLS; // 0x00

		uint8_t Voice[VoicesPerBank][SizePackedVoice];

		uint8_t Checksum;
		uint8_t StatusEnd; // 0xF7
	} PACKED;

public:
	CSysExFileLoader(const char *pDirName = "/sysex");
	~CSysExFileLoader();

	void Load(bool bHeaderlessSysExVoices = false);

	std::string GetBankName(int nBankID); // 0 .. MaxVoiceBankID
	std::string GetVoiceName(int nBankID, int nVoice); // 0 .. MaxVoiceBankID, 0 .. VoicesPerBank-1
	int GetNumHighestBank(); // 0 .. MaxVoiceBankID
	bool IsValidBank(int nBankID);
	int GetNextBankUp(int nBankID);
	int GetNextBankDown(int nBankID);

	void GetVoice(int nBankID, // 0 .. MaxVoiceBankID
		      int nVoiceID, // 0 .. 31
		      uint8_t *pVoiceData); // returns unpacked format (156 bytes)

private:
	static void DecodePackedVoice(const uint8_t *pPackedData, uint8_t *pDecodedData);

private:
	std::string m_DirName;

	int m_nNumHighestBank;
	int m_nBanksLoaded;

	TVoiceBank *m_pVoiceBank[MaxVoiceBankID + 1];
	std::string m_BankFileName[MaxVoiceBankID + 1];

	static uint8_t s_DefaultVoice[SizeSingleVoice];

	void LoadBank(const char *sDirName, const char *sBankName, bool bHeaderlessSysExVoices, int nSubDirCount);
};
