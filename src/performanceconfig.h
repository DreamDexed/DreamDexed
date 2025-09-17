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
	unsigned GetFXSend (unsigned nTG, unsigned nFX) const;	// 0 .. 127
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
	void SetFXSend (unsigned nValue, unsigned nTG, unsigned nFX);
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
	unsigned GetFXChorusMix (unsigned nFX) const;
	bool GetFXChorusEnable1 (unsigned nFX) const;
	bool GetFXChorusEnable2 (unsigned nFX) const;
	unsigned GetFXChorusLFORate1 (unsigned nFX) const;
	unsigned GetFXChorusLFORate2 (unsigned nFX) const;

	unsigned GetFXDelayMix (unsigned nFX) const;
	unsigned GetFXDelayMode (unsigned nFX) const;
	unsigned GetFXDelayTimeL (unsigned nFX) const;
	unsigned GetFXDelayTimeR (unsigned nFX) const;
	unsigned GetFXDelayTempo (unsigned nFX) const;
	unsigned GetFXDelayFeedback (unsigned nFX) const;
	unsigned GetFXDelayHighCut (unsigned nFX) const;

	unsigned GetFXReverbMix (unsigned nFX) const;			// 0 .. 100
	unsigned GetFXReverbSize (unsigned nFX) const;			// 0 .. 99
	unsigned GetFXReverbHighDamp (unsigned nFX) const;		// 0 .. 99
	unsigned GetFXReverbLowDamp (unsigned nFX) const;			// 0 .. 99
	unsigned GetFXReverbLowPass (unsigned nFX) const;			// 0 .. 99
	unsigned GetFXReverbDiffusion (unsigned nFX) const;		// 0 .. 99
	unsigned GetFXLevel (unsigned nFX) const;			// 0 .. 99

	void SetFXChorusMix (unsigned nValue, unsigned nFX);
	void SetFXChorusEnable1 (bool bValue, unsigned nFX);
	void SetFXChorusEnable2 (bool bValue, unsigned nFX);
	void SetFXChorusLFORate1 (unsigned nValue, unsigned nFX);	// 0 .. 100
	void SetFXChorusLFORate2 (unsigned nValue, unsigned nFX);	// 0 .. 100

	void SetFXDelayMix (unsigned nValue, unsigned nFX);
	void SetFXDelayMode (unsigned nValue, unsigned nFX);
	void SetFXDelayTimeL (unsigned nValue, unsigned nFX);
	void SetFXDelayTimeR (unsigned nValue, unsigned nFX);
	void SetFXDelayTempo (unsigned nValue, unsigned nFX);
	void SetFXDelayFeedback (unsigned nValue, unsigned nFX);
	void SetFXDelayHighCut (unsigned nValue, unsigned nFX);

	void SetFXReverbMix (unsigned nValue, unsigned nFX);
	void SetFXReverbSize (unsigned nValue, unsigned nFX);
	void SetFXReverbHighDamp (unsigned nValue, unsigned nFX);
	void SetFXReverbLowDamp (unsigned nValue, unsigned nFX);
	void SetFXReverbLowPass (unsigned nValue, unsigned nFX);
	void SetFXReverbDiffusion (unsigned nValue, unsigned nFX);
	void SetFXLevel (unsigned nValue, unsigned nFX);

	int GetMasterEQLow () const;
	int GetMasterEQMid () const;
	int GetMasterEQHigh () const;
	int GetMasterEQGain () const;
	unsigned GetMasterEQLowMidFreq () const;
	unsigned GetMasterEQMidHighFreq () const;

	void SetMasterEQLow (int nValue);
	void SetMasterEQMid (int nValue);
	void SetMasterEQHigh (int nValue);
	void SetMasterEQGain (int nValue);
	void SetMasterEQLowMidFreq (unsigned nValue);
	void SetMasterEQMidHighFreq (unsigned nValue);

	bool GetMasterCompressorEnable () const;
	int GetMasterCompressorPreGain () const;
	int GetMasterCompressorThresh () const;
	unsigned GetMasterCompressorRatio () const;
	unsigned GetMasterCompressorAttack () const;
	unsigned GetMasterCompressorRelease () const;
	bool GetMasterCompressorHPFilterEnable () const;

	void SetMasterCompressorEnable (bool nValue);
	void SetMasterCompressorPreGain (int nValue);
	void SetMasterCompressorThresh (int nValue);
	void SetMasterCompressorRatio (unsigned nValue);
	void SetMasterCompressorAttack (unsigned nValue);
	void SetMasterCompressorRelease (unsigned nValue);	
	void SetMasterCompressorHPFilterEnable (bool nValue);

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
	int m_nFXSend[CConfig::AllToneGenerators][CConfig::FXChains];
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

	unsigned m_nFXChorusMix[CConfig::FXChains];
	bool m_bFXChorusEnable1[CConfig::FXChains];
	bool m_bFXChorusEnable2[CConfig::FXChains];
	unsigned m_nFXChorusLFORate1[CConfig::FXChains];
	unsigned m_nFXChorusLFORate2[CConfig::FXChains];

	unsigned m_nFXDelayMix[CConfig::FXChains];
	unsigned m_nFXDelayMode[CConfig::FXChains];
	unsigned m_nFXDelayTimeL[CConfig::FXChains];
	unsigned m_nFXDelayTimeR[CConfig::FXChains];
	unsigned m_nFXDelayTempo[CConfig::FXChains];
	unsigned m_nFXDelayFeedback[CConfig::FXChains];
	unsigned m_nFXDelayHighCut[CConfig::FXChains];

	unsigned m_nFXReverbMix[CConfig::FXChains];
	unsigned m_nFXReverbSize[CConfig::FXChains];
	unsigned m_nFXReverbHighDamp[CConfig::FXChains];
	unsigned m_nFXReverbLowDamp[CConfig::FXChains];
	unsigned m_nFXReverbLowPass[CConfig::FXChains];
	unsigned m_nFXReverbDiffusion[CConfig::FXChains];
	unsigned m_nFXLevel[CConfig::FXChains];

	int m_nMasterEQLow;
	int m_nMasterEQMid;
	int m_nMasterEQHigh;
	int m_nMasterEQGain;
	unsigned m_nMasterEQLowMidFreq;
	unsigned m_nMasterEQMidHighFreq;

	bool m_bMasterCompressorEnable;
	int m_nMasterCompressorPreGain;
	int m_nMasterCompressorThresh;
	unsigned m_nMasterCompressorRatio;
	unsigned m_nMasterCompressorAttack;
	unsigned m_nMasterCompressorRelease;
	bool m_bMasterCompressorHPFilterEnable;
};

#endif
