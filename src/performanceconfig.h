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
#ifndef _performanceconfig_h
#define _performanceconfig_h

#include "config.h"
#include <fatfs/ff.h>
#include <Properties/propertiesfatfsfile.h>
#include "effect.h"
#define NUM_VOICE_PARAM 156
#define NUM_PERFORMANCES 128
#define NUM_PERFORMANCE_BANKS 128

class CPerformanceConfig	// Performance configuration
{
public:
	CPerformanceConfig (FATFS *pFileSystem);
	~CPerformanceConfig (void);
	
	bool Init (unsigned nToneGenerators);

	bool Load (void);

	bool Save (void);

	// TG#
	unsigned GetBankNumber (unsigned nTG) const;		// 0 .. 127
	unsigned GetVoiceNumber (unsigned nTG) const;		// 0 .. 31
	unsigned GetMIDIChannel (unsigned nTG) const;		// 0 .. 15, omni, off
	unsigned GetVolume (unsigned nTG) const;		// 0 .. 127
	unsigned GetPan (unsigned nTG) const;			// 0 .. 127
	int GetDetune (unsigned nTG) const;			// -99 .. 99
	unsigned GetCutoff (unsigned nTG) const;		// 0 .. 99
	unsigned GetResonance (unsigned nTG) const;		// 0 .. 99
	unsigned GetNoteLimitLow (unsigned nTG) const;		// 0 .. 127
	unsigned GetNoteLimitHigh (unsigned nTG) const;		// 0 .. 127
	int GetNoteShift (unsigned nTG) const;			// -24 .. 24
	unsigned GetFX1Send (unsigned nTG) const;	// 0 .. 99
	unsigned GetFX2Send (unsigned nTG) const;	// 0 .. 99
	unsigned GetPitchBendRange (unsigned nTG) const;		// 0 .. 12
	unsigned GetPitchBendStep (unsigned nTG) const;		// 0 .. 12
	unsigned GetPortamentoMode (unsigned nTG) const;		// 0 .. 1
	unsigned GetPortamentoGlissando (unsigned nTG) const;		// 0 .. 1
	unsigned GetPortamentoTime (unsigned nTG) const;		// 0 .. 99
	bool GetMonoMode (unsigned nTG) const; 				// 0 .. 1
	unsigned GetTGLink (unsigned nTG) const;		// 0 .. 4
	
	unsigned GetModulationWheelRange (unsigned nTG) const; // 0 .. 99
	unsigned GetModulationWheelTarget (unsigned nTG) const; // 0 .. 7
	unsigned GetFootControlRange (unsigned nTG) const; // 0 .. 99
	unsigned GetFootControlTarget (unsigned nTG) const;  // 0 .. 7
	unsigned GetBreathControlRange (unsigned nTG) const; // 0 .. 99
	unsigned GetBreathControlTarget (unsigned nTG) const;  // 0 .. 7
	unsigned GetAftertouchRange (unsigned nTG) const; // 0 .. 99
	unsigned GetAftertouchTarget (unsigned nTG) const;  // 0 .. 7

	bool GetCompressorEnable (unsigned nTG) const; 		// 0 .. 1
	int GetCompressorPreGain (unsigned nTG) const;
	int GetCompressorThresh (unsigned nTG) const;
	unsigned GetCompressorRatio (unsigned nTG) const;
	unsigned GetCompressorAttack (unsigned nTG) const;
	unsigned GetCompressorRelease (unsigned nTG) const;
	int GetCompressorMakeupGain (unsigned nTG) const;

	void SetBankNumber (unsigned nValue, unsigned nTG);
	void SetVoiceNumber (unsigned nValue, unsigned nTG);
	void SetMIDIChannel (unsigned nValue, unsigned nTG);
	void SetVolume (unsigned nValue, unsigned nTG);
	void SetPan (unsigned nValue, unsigned nTG);
	void SetDetune (int nValue, unsigned nTG);
	void SetCutoff (unsigned nValue, unsigned nTG);
	void SetResonance (unsigned nValue, unsigned nTG);
	void SetNoteLimitLow (unsigned nValue, unsigned nTG);
	void SetNoteLimitHigh (unsigned nValue, unsigned nTG);
	void SetNoteShift (int nValue, unsigned nTG);
	void SetFX1Send (unsigned nValue, unsigned nTG);
	void SetFX2Send (unsigned nValue, unsigned nTG);
	void SetPitchBendRange (unsigned nValue, unsigned nTG);
	void SetPitchBendStep (unsigned nValue, unsigned nTG);
	void SetPortamentoMode (unsigned nValue, unsigned nTG);
	void SetPortamentoGlissando (unsigned nValue, unsigned nTG);
	void SetPortamentoTime (unsigned nValue, unsigned nTG);
	void SetVoiceDataToTxt (const uint8_t *pData, unsigned nTG); 
	uint8_t *GetVoiceDataFromTxt (unsigned nTG);
	void SetMonoMode (bool bOKValue, unsigned nTG); 
	void SetTGLink (unsigned nTGLink, unsigned nTG);

	void SetModulationWheelRange (unsigned nValue, unsigned nTG);
	void SetModulationWheelTarget (unsigned nValue, unsigned nTG);
	void SetFootControlRange (unsigned nValue, unsigned nTG);
	void SetFootControlTarget (unsigned nValue, unsigned nTG);
	void SetBreathControlRange (unsigned nValue, unsigned nTG);
	void SetBreathControlTarget (unsigned nValue, unsigned nTG);
	void SetAftertouchRange (unsigned nValue, unsigned nTG);
	void SetAftertouchTarget (unsigned nValue, unsigned nTG);

	void SetCompressorEnable (bool nValue, unsigned nTG);
	void SetCompressorPreGain (int nValue, unsigned nTG);
	void SetCompressorThresh (int nValue, unsigned nTG);
	void SetCompressorRatio (unsigned nValue, unsigned nTG);	
	void SetCompressorAttack (unsigned nValue, unsigned nTG);
	void SetCompressorRelease (unsigned nValue, unsigned nTG);	
	void SetCompressorMakeupGain (int nValue, unsigned nTG);

	int GetEQLow (unsigned nTG) const;
	int GetEQMid (unsigned nTG) const;
	int GetEQHigh (unsigned nTG) const;
	int GetEQGain (unsigned nTG) const;
	unsigned GetEQLowMidFreq (unsigned nTG) const;
	unsigned GetEQMidHighFreq (unsigned nTG) const;

	void SetEQLow (int nValue, unsigned nTG);
	void SetEQMid (int nValue, unsigned nTG);
	void SetEQHigh (int nValue, unsigned nTG);
	void SetEQGain (int nValue, unsigned nTG);
	void SetEQLowMidFreq (unsigned nValue, unsigned nTG);
	void SetEQMidHighFreq (unsigned nValue, unsigned nTG);

	// Effects
	int GetFXParameter (FX::TFXParameter nParameter, unsigned nFX) const;
	void SetFXParameter (FX::TFXParameter nParameter, int nValue, unsigned nFX);

	unsigned GetMixerDryLevel () const;
	void SetMixerDryLevel (unsigned nValue);

	unsigned GetFXBypass () const;
	void SetFXBypass (unsigned nValue);

	bool VoiceDataFilled(unsigned nTG);
	bool ListPerformances(); 
	//std::string m_DirName;
	void SetNewPerformance (unsigned nID);
	unsigned FindFirstPerformance (void);
	std::string GetPerformanceFileName(unsigned nID);
	std::string GetPerformanceFullFilePath(unsigned nID);
	std::string GetPerformanceName(unsigned nID);
	unsigned GetLastPerformance();
	unsigned GetLastPerformanceBank();
	void SetActualPerformanceID(unsigned nID);
	unsigned GetActualPerformanceID();
	void SetActualPerformanceBankID(unsigned nBankID);
	unsigned GetActualPerformanceBankID();
	bool CreateNewPerformanceFile(void);
	bool GetInternalFolderOk(); 
	std::string GetNewPerformanceDefaultName(void);
	void SetNewPerformanceName(const std::string &Name);
	bool DeletePerformance(unsigned nID);
	bool CheckFreePerformanceSlot(void);
	std::string AddPerformanceBankDirName(unsigned nBankID);
	bool IsValidPerformance(unsigned nID);

	bool ListPerformanceBanks(void); 
	void SetNewPerformanceBank(unsigned nBankID);
	unsigned GetPerformanceBank(void);
	std::string GetPerformanceBankName(unsigned nBankID);
	bool IsValidPerformanceBank(unsigned nBankID);

private:
	CPropertiesFatFsFile m_Properties;
	
	unsigned m_nToneGenerators;

	unsigned m_nBankNumber[CConfig::AllToneGenerators];
	unsigned m_nVoiceNumber[CConfig::AllToneGenerators];
	unsigned m_nMIDIChannel[CConfig::AllToneGenerators];
	unsigned m_nVolume[CConfig::AllToneGenerators];
	unsigned m_nPan[CConfig::AllToneGenerators];
	int m_nDetune[CConfig::AllToneGenerators];
	unsigned m_nCutoff[CConfig::AllToneGenerators];
	unsigned m_nResonance[CConfig::AllToneGenerators];
	unsigned m_nNoteLimitLow[CConfig::AllToneGenerators];
	unsigned m_nNoteLimitHigh[CConfig::AllToneGenerators];
	int m_nNoteShift[CConfig::AllToneGenerators];
	int m_nFX1Send[CConfig::AllToneGenerators];
	int m_nFX2Send[CConfig::AllToneGenerators];
	unsigned m_nPitchBendRange[CConfig::AllToneGenerators];
	unsigned m_nPitchBendStep[CConfig::AllToneGenerators];
	unsigned m_nPortamentoMode[CConfig::AllToneGenerators];
	unsigned m_nPortamentoGlissando[CConfig::AllToneGenerators];
	unsigned m_nPortamentoTime[CConfig::AllToneGenerators];
	std::string m_nVoiceDataTxt[CConfig::AllToneGenerators]; 
	bool m_bMonoMode[CConfig::AllToneGenerators]; 
	unsigned m_nTGLink[CConfig::AllToneGenerators];

	unsigned m_nModulationWheelRange[CConfig::AllToneGenerators];
	unsigned m_nModulationWheelTarget[CConfig::AllToneGenerators];
	unsigned m_nFootControlRange[CConfig::AllToneGenerators];	
	unsigned m_nFootControlTarget[CConfig::AllToneGenerators];	
	unsigned m_nBreathControlRange[CConfig::AllToneGenerators];	
	unsigned m_nBreathControlTarget[CConfig::AllToneGenerators];	
	unsigned m_nAftertouchRange[CConfig::AllToneGenerators];	
	unsigned m_nAftertouchTarget[CConfig::AllToneGenerators];
	
	bool m_bCompressorEnable[CConfig::AllToneGenerators];
	int m_nCompressorPreGain[CConfig::AllToneGenerators];
	int m_nCompressorThresh[CConfig::AllToneGenerators];
	unsigned m_nCompressorRatio[CConfig::AllToneGenerators];
	unsigned m_nCompressorAttack[CConfig::AllToneGenerators];
	unsigned m_nCompressorRelease[CConfig::AllToneGenerators];
	int m_nCompressorMakeupGain[CConfig::AllToneGenerators];

	int m_nEQLow[CConfig::AllToneGenerators];
	int m_nEQMid[CConfig::AllToneGenerators];
	int m_nEQHigh[CConfig::AllToneGenerators];
	int m_nEQGain[CConfig::AllToneGenerators];
	unsigned m_nEQLowMidFreq[CConfig::AllToneGenerators];
	unsigned m_nEQMidHighFreq[CConfig::AllToneGenerators];

	unsigned m_nLastPerformance;  
	unsigned m_nActualPerformance = 0;  
	unsigned m_nActualPerformanceBank = 0;  
	unsigned m_nPerformanceBank;
	unsigned m_nLastPerformanceBank;  
	bool     m_bPerformanceDirectoryExists;
	//unsigned nMenuSelectedPerformance = 0; 
	std::string m_PerformanceFileName[NUM_PERFORMANCES];
	std::string m_PerformanceBankName[NUM_PERFORMANCE_BANKS];
	FATFS *m_pFileSystem; 

	std::string NewPerformanceName="";

	int m_nFXParameter[CConfig::FXChains][FX::FXParameterUnknown];

	unsigned m_nMixerDryLevel;
	unsigned m_nFXBypass;
};

#endif
