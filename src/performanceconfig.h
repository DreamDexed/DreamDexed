//
// performanceconfig.h
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
#include <fatfs/ff.h>

#include "config.h"
#include "effect.h"

#define NUM_VOICE_PARAM 156
#define NUM_PERFORMANCES 128
#define NUM_PERFORMANCE_BANKS 128

class CPerformanceConfig // Performance configuration
{
public:
	CPerformanceConfig(FATFS *pFileSystem);
	~CPerformanceConfig();

	bool Init(int nToneGenerators);

	bool Load();

	bool Save();

	// TG#
	int GetBankNumber(int nTG) const; // 0 .. 127
	int GetVoiceNumber(int nTG) const; // 0 .. 31
	int GetMIDIChannel(int nTG) const; // 0 .. 15, omni, off
	int GetSysExChannel(int nTG) const; // 0 .. 15
	bool GetSysExEnable(int nTG) const;
	bool GetMIDIRxSustain(int nTG) const;
	bool GetMIDIRxPortamento(int nTG) const;
	bool GetMIDIRxSostenuto(int nTG) const;
	bool GetMIDIRxHold2(int nTG) const;
	int GetVolume(int nTG) const; // 0 .. 127
	int GetPan(int nTG) const; // 0 .. 127
	int GetDetune(int nTG) const; // -99 .. 99
	int GetCutoff(int nTG) const; // 0 .. 99
	int GetResonance(int nTG) const; // 0 .. 99
	int GetNoteLimitLow(int nTG) const; // 0 .. 127
	int GetNoteLimitHigh(int nTG) const; // 0 .. 127
	int GetNoteShift(int nTG) const; // -24 .. 24
	int GetFX1Send(int nTG) const; // 0 .. 99
	int GetFX2Send(int nTG) const; // 0 .. 99
	int GetPitchBendRange(int nTG) const; // 0 .. 12
	int GetPitchBendStep(int nTG) const; // 0 .. 12
	int GetPortamentoMode(int nTG) const; // 0 .. 1
	int GetPortamentoGlissando(int nTG) const; // 0 .. 1
	int GetPortamentoTime(int nTG) const; // 0 .. 99
	bool GetMonoMode(int nTG) const; // 0 .. 1
	int GetTGLink(int nTG) const; // 0 .. 4

	int GetModulationWheelRange(int nTG) const; // 0 .. 99
	int GetModulationWheelTarget(int nTG) const; // 0 .. 7
	int GetFootControlRange(int nTG) const; // 0 .. 99
	int GetFootControlTarget(int nTG) const; // 0 .. 7
	int GetBreathControlRange(int nTG) const; // 0 .. 99
	int GetBreathControlTarget(int nTG) const; // 0 .. 7
	int GetAftertouchRange(int nTG) const; // 0 .. 99
	int GetAftertouchTarget(int nTG) const; // 0 .. 7

	bool GetCompressorEnable(int nTG) const; // 0 .. 1
	int GetCompressorPreGain(int nTG) const;
	int GetCompressorThresh(int nTG) const;
	int GetCompressorRatio(int nTG) const;
	int GetCompressorAttack(int nTG) const;
	int GetCompressorRelease(int nTG) const;
	int GetCompressorMakeupGain(int nTG) const;

	void SetBankNumber(int nValue, int nTG);
	void SetVoiceNumber(int nValue, int nTG);
	void SetMIDIChannel(int nValue, int nTG);
	void SetSysExChannel(int nValue, int nTG);
	void SetSysExEnable(bool bValue, int nTG);
	void SetMIDIRxSustain(bool bValue, int nTG);
	void SetMIDIRxPortamento(bool bValue, int nTG);
	void SetMIDIRxSostenuto(bool bValue, int nTG);
	void SetMIDIRxHold2(bool bValue, int nTG);
	void SetVolume(int nValue, int nTG);
	void SetPan(int nValue, int nTG);
	void SetDetune(int nValue, int nTG);
	void SetCutoff(int nValue, int nTG);
	void SetResonance(int nValue, int nTG);
	void SetNoteLimitLow(int nValue, int nTG);
	void SetNoteLimitHigh(int nValue, int nTG);
	void SetNoteShift(int nValue, int nTG);
	void SetFX1Send(int nValue, int nTG);
	void SetFX2Send(int nValue, int nTG);
	void SetPitchBendRange(int nValue, int nTG);
	void SetPitchBendStep(int nValue, int nTG);
	void SetPortamentoMode(int nValue, int nTG);
	void SetPortamentoGlissando(int nValue, int nTG);
	void SetPortamentoTime(int nValue, int nTG);
	void SetVoiceDataToTxt(const uint8_t *pData, int nTG);
	uint8_t *GetVoiceDataFromTxt(int nTG);
	void SetMonoMode(bool bOKValue, int nTG);
	void SetTGLink(int nTGLink, int nTG);

	void SetModulationWheelRange(int nValue, int nTG);
	void SetModulationWheelTarget(int nValue, int nTG);
	void SetFootControlRange(int nValue, int nTG);
	void SetFootControlTarget(int nValue, int nTG);
	void SetBreathControlRange(int nValue, int nTG);
	void SetBreathControlTarget(int nValue, int nTG);
	void SetAftertouchRange(int nValue, int nTG);
	void SetAftertouchTarget(int nValue, int nTG);

	void SetCompressorEnable(bool nValue, int nTG);
	void SetCompressorPreGain(int nValue, int nTG);
	void SetCompressorThresh(int nValue, int nTG);
	void SetCompressorRatio(int nValue, int nTG);
	void SetCompressorAttack(int nValue, int nTG);
	void SetCompressorRelease(int nValue, int nTG);
	void SetCompressorMakeupGain(int nValue, int nTG);

	int GetEQLow(int nTG) const;
	int GetEQMid(int nTG) const;
	int GetEQHigh(int nTG) const;
	int GetEQGain(int nTG) const;
	int GetEQLowMidFreq(int nTG) const;
	int GetEQMidHighFreq(int nTG) const;
	int GetEQPreLowcut(int nTG) const;
	int GetEQPreHighcut(int nTG) const;

	void SetEQLow(int nValue, int nTG);
	void SetEQMid(int nValue, int nTG);
	void SetEQHigh(int nValue, int nTG);
	void SetEQGain(int nValue, int nTG);
	void SetEQLowMidFreq(int nValue, int nTG);
	void SetEQMidHighFreq(int nValue, int nTG);
	void SetEQPreLowcut(int nValue, int nTG);
	void SetEQPreHighcut(int nValue, int nTG);

	// Effects
	int GetFXParameter(FX::TFXParameter nParameter, int nFX) const;
	void SetFXParameter(FX::TFXParameter nParameter, int nValue, int nFX);

	int GetMixerDryLevel() const;
	void SetMixerDryLevel(int nValue);

	int GetFXBypass() const;
	void SetFXBypass(int nValue);

	bool VoiceDataFilled(int nTG);
	bool ListPerformances();
	// std::string m_DirName;
	void SetNewPerformance(int nID);
	int FindFirstPerformance();
	std::string GetPerformanceFileName(int nID);
	std::string GetPerformanceFullFilePath(int nID);
	std::string GetPerformanceName(int nID);
	int GetLastPerformance();
	int GetLastPerformanceBank();
	void SetActualPerformanceID(int nID);
	int GetActualPerformanceID();
	void SetActualPerformanceBankID(int nBankID);
	int GetActualPerformanceBankID();
	bool CreateNewPerformanceFile();
	bool GetInternalFolderOk();
	std::string GetNewPerformanceDefaultName();
	void SetNewPerformanceName(const std::string &Name);
	bool DeletePerformance(int nID);
	bool CheckFreePerformanceSlot();
	std::string AddPerformanceBankDirName(int nBankID);
	bool IsValidPerformance(int nID);

	bool ListPerformanceBanks();
	void SetNewPerformanceBank(int nBankID);
	int GetPerformanceBank();
	std::string GetPerformanceBankName(int nBankID);
	bool IsValidPerformanceBank(int nBankID);

private:
	CPropertiesFatFsFile m_Properties;

	int m_nToneGenerators;

	int m_nBankNumber[CConfig::AllToneGenerators];
	int m_nVoiceNumber[CConfig::AllToneGenerators];
	int m_nMIDIChannel[CConfig::AllToneGenerators];
	int m_nSysExChannel[CConfig::AllToneGenerators];
	bool m_bSysExEnable[CConfig::AllToneGenerators];
	bool m_bMIDIRxSustain[CConfig::AllToneGenerators];
	bool m_bMIDIRxPortamento[CConfig::AllToneGenerators];
	bool m_bMIDIRxSostenuto[CConfig::AllToneGenerators];
	bool m_bMIDIRxHold2[CConfig::AllToneGenerators];
	int m_nVolume[CConfig::AllToneGenerators];
	int m_nPan[CConfig::AllToneGenerators];
	int m_nDetune[CConfig::AllToneGenerators];
	int m_nCutoff[CConfig::AllToneGenerators];
	int m_nResonance[CConfig::AllToneGenerators];
	int m_nNoteLimitLow[CConfig::AllToneGenerators];
	int m_nNoteLimitHigh[CConfig::AllToneGenerators];
	int m_nNoteShift[CConfig::AllToneGenerators];
	int m_nFX1Send[CConfig::AllToneGenerators];
	int m_nFX2Send[CConfig::AllToneGenerators];
	int m_nPitchBendRange[CConfig::AllToneGenerators];
	int m_nPitchBendStep[CConfig::AllToneGenerators];
	int m_nPortamentoMode[CConfig::AllToneGenerators];
	int m_nPortamentoGlissando[CConfig::AllToneGenerators];
	int m_nPortamentoTime[CConfig::AllToneGenerators];
	std::string m_nVoiceDataTxt[CConfig::AllToneGenerators];
	bool m_bMonoMode[CConfig::AllToneGenerators];
	int m_nTGLink[CConfig::AllToneGenerators];

	int m_nModulationWheelRange[CConfig::AllToneGenerators];
	int m_nModulationWheelTarget[CConfig::AllToneGenerators];
	int m_nFootControlRange[CConfig::AllToneGenerators];
	int m_nFootControlTarget[CConfig::AllToneGenerators];
	int m_nBreathControlRange[CConfig::AllToneGenerators];
	int m_nBreathControlTarget[CConfig::AllToneGenerators];
	int m_nAftertouchRange[CConfig::AllToneGenerators];
	int m_nAftertouchTarget[CConfig::AllToneGenerators];

	bool m_bCompressorEnable[CConfig::AllToneGenerators];
	int m_nCompressorPreGain[CConfig::AllToneGenerators];
	int m_nCompressorThresh[CConfig::AllToneGenerators];
	int m_nCompressorRatio[CConfig::AllToneGenerators];
	int m_nCompressorAttack[CConfig::AllToneGenerators];
	int m_nCompressorRelease[CConfig::AllToneGenerators];
	int m_nCompressorMakeupGain[CConfig::AllToneGenerators];

	int m_nEQLow[CConfig::AllToneGenerators];
	int m_nEQMid[CConfig::AllToneGenerators];
	int m_nEQHigh[CConfig::AllToneGenerators];
	int m_nEQGain[CConfig::AllToneGenerators];
	int m_nEQLowMidFreq[CConfig::AllToneGenerators];
	int m_nEQMidHighFreq[CConfig::AllToneGenerators];
	int m_nEQPreLowcut[CConfig::AllToneGenerators];
	int m_nEQPreHighcut[CConfig::AllToneGenerators];

	int m_nLastPerformance;
	int m_nActualPerformance = 0;
	int m_nActualPerformanceBank = 0;
	int m_nPerformanceBank;
	int m_nLastPerformanceBank;
	bool m_bPerformanceDirectoryExists;
	// int nMenuSelectedPerformance = 0;
	std::string m_PerformanceFileName[NUM_PERFORMANCES];
	std::string m_PerformanceBankName[NUM_PERFORMANCE_BANKS];
	FATFS *m_pFileSystem;

	std::string NewPerformanceName = "";

	int m_nFXParameter[CConfig::FXChains][FX::FXParameterUnknown];

	int m_nMixerDryLevel;
	int m_nFXBypass;
};
