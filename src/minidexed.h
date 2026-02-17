//
// minidexed.h
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

#include <atomic>
#include <cstdint>
#include <string>

#include <circle/gpiomanager.h>
#include <circle/i2cmaster.h>
#include <circle/interrupt.h>
#include <circle/memorymap.h>
#include <circle/multicore.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/netdevice.h>
#include <circle/sched/scheduler.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/spimaster.h>
#include <circle/spinlock.h>
#include <fatfs/ff.h>
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>

#include "config.h"
#include "dexedadapter.h"
#include "effect.h"
#include "effect_chain.h"
#include "effect_mixer.hpp"
#include "midikeyboard.h"
#include "net/ftpdaemon.h"
#include "net/mdnspublisher.h"
#include "pckeyboard.h"
#include "performanceconfig.h"
#include "perftimer.h"
#include "sdfilter.h"
#include "serialmididevice.h"
#include "status.h"
#include "sysexfileloader.h"
#include "udpmididevice.h"
#include "userinterface.h"

class CMiniDexed
#ifdef ARM_ALLOW_MULTI_CORE
: public CMultiCoreSupport
#endif
{
public:
	CMiniDexed(CConfig *pConfig, CInterruptSystem *pInterrupt,
		   CGPIOManager *pGPIOManager, CI2CMaster *pI2CMaster, CSPIMaster *pSPIMaster, FATFS *pFileSystem);
	~CMiniDexed(); // Add destructor

	bool Initialize();

	void Process(bool bPlugAndPlayUpdated);

#ifdef ARM_ALLOW_MULTI_CORE
	void Run(unsigned nCore);
#endif

	CSysExFileLoader *GetSysExFileLoader();
	CPerformanceConfig *GetPerformanceConfig();

	void BankSelect(int nBank, int nTG);
	void BankSelectPerformance(int nBank);
	void BankSelectMSB(int nBankMSB, int nTG);
	void BankSelectMSBPerformance(int nBankMSB);
	void BankSelectLSB(int nBankLSB, int nTG);
	void BankSelectLSBPerformance(int nBankLSB);
	void ProgramChange(int nProgram, int nTG);
	void ProgramChangePerformance(int nProgram);
	void SetVolume(int nVolume, int nTG);
	void SetExpression(int nExpression, int nTG);
	void SetPan(int nPan, int nTG); // 0 .. 127
	void SetMasterTune(int nMasterTune, int nTG); // -99 .. 99
	void SetCutoff(int nCutoff, int nTG); // 0 .. 99
	void SetResonance(int nResonance, int nTG); // 0 .. 99
	void SetMIDIChannel(int nChannel, int nTG);
	void SetSysExChannel(int nChannel, int nTG);
	void SetSysExEnable(bool value, int nTG);
	void SetMIDIRxSustain(bool value, int nTG);
	void SetMIDIRxPortamento(bool value, int nTG);
	void SetMIDIRxSostenuto(bool value, int nTG);
	void SetMIDIRxHold2(bool value, int nTG);

	int GetSysExChannel(int nTG);
	bool GetSysExEnable(int nTG);

	void keyup(int pitch, int nTG);
	void keydown(int pitch, int velocity, int nTG);

	void setSustain(bool sustain, int nTG);
	void setSostenuto(bool sostenuto, int nTG);
	void setHoldMode(bool holdmode, int nTG);
	void panic(int value, int nTG);
	void notesOff(int value, int nTG);
	void setModWheel(int value, int nTG);
	void setPitchbend(int16_t value, int nTG);
	void ControllersRefresh(int nTG);

	void setFootController(uint8_t value, int nTG);
	void setBreathController(uint8_t value, int nTG);
	void setAftertouch(uint8_t value, int nTG);

	void SetFX1Send(int nFXSend, int nTG); // 0 .. 99
	void SetFX2Send(int nFXSend, int nTG); // 0 .. 99

	void SetCompressorEnable(bool compressor, int nTG); // 0 .. 1 (default 1)
	void SetCompressorPreGain(int preGain, int nTG); // -20 .. 20 dB (default 0)
	void SetCompressorThresh(int thresh, int nTG); // -60 .. 0 dBFS (default -20)
	void SetCompressorRatio(int ratio, int nTG); // 1 .. 20 (default 5)
	void SetCompressorAttack(int attack, int nTG); // 0 .. 1000 ms (default 5)
	void SetCompressorRelease(int release, int nTG); // 0 .. 1000 ms (default 200)
	void SetCompressorMakeupGain(int makeupGain, int nTG); // -20 .. 20 dB (default 0)

	void SetEQLow(int nValue, int nTG);
	void SetEQMid(int nValue, int nTG);
	void SetEQHigh(int nValue, int nTG);
	void SetEQGain(int nValue, int nTG);
	void SetEQLowMidFreq(int nValue, int nTG);
	void SetEQMidHighFreq(int nValue, int nTG);
	void SetEQPreLowcut(int nValue, int nTG);
	void SetEQPreHighcut(int nValue, int nTG);

	void setMonoMode(bool mono, int nTG);

	void setTGLink(int nTGLink, int nTG);

	void setPitchbendRange(int range, int nTG);
	void setPitchbendStep(int step, int nTG);
	void setPortamentoMode(int mode, int nTG);
	void setPortamentoGlissando(int glissando, int nTG);
	void setPortamentoTime(int time, int nTG);
	void setNoteLimitLow(int limit, int nTG);
	void setNoteLimitHigh(int limit, int nTG);
	void setNoteShift(int shift, int nTG);
	void setModWheelRange(int range, int nTG);
	void setModWheelTarget(int target, int nTG);
	void setFootControllerRange(int range, int nTG);
	void setFootControllerTarget(int target, int nTG);
	void setBreathControllerRange(int range, int nTG);
	void setBreathControllerTarget(int target, int nTG);
	void setAftertouchRange(int range, int nTG);
	void setAftertouchTarget(int target, int nTG);
	void loadVoiceParameters(const uint8_t *data, int nTG);
	void setVoiceDataElement(int address, int value, int nTG);
	void getSysExVoiceDump(uint8_t *dest, int nTG);
	void setOPMask(uint8_t uchOPMask, int nTG);

	void setModController(int controller, int parameter, int value, int nTG);
	int getModController(int controller, int parameter, int nTG);

	int16_t checkSystemExclusive(const uint8_t *pMessage, int nLength, int nTG);

	std::string GetPerformanceFileName(int nID);
	std::string GetPerformanceName(int nID);
	int GetLastPerformance();
	int GetPerformanceBank();
	int GetLastPerformanceBank();
	int GetActualPerformanceID();
	bool SetNewPerformance(int nID);
	bool SetNewPerformanceBank(int nBankID);
	void SetFirstPerformance();
	void DoSetFirstPerformance();
	bool SavePerformanceNewFile();

	bool DoSavePerformanceNewFile();
	bool DoSetNewPerformance();
	bool DoSetNewPerformanceBank();
	bool GetPerformanceSelectToLoad();
	bool SavePerformance(bool bSaveAsDeault);
	int GetPerformanceSelectChannel();
	void SetPerformanceSelectChannel(int nCh);
	bool IsValidPerformance(int nID);
	bool IsValidPerformanceBank(int nBankID);

	int GetLastKeyDown();

	// Must match the order in CUIMenu::TParameter
	enum TParameter
	{
		ParameterPerformanceSelectChannel,
		ParameterPerformanceBank,
		ParameterMasterVolume,
		ParameterSDFilter,
		ParameterMixerDryLevel,
		ParameterFXBypass,
		ParameterUnknown
	};

	void SetParameter(TParameter Parameter, int nValue);
	int GetParameter(TParameter Parameter);

	std::string GetNewPerformanceDefaultName();
	void SetNewPerformanceName(const std::string &Name);
	void SetVoiceName(const std::string &VoiceName, int nTG);
	bool DeletePerformance(int nID);
	bool DoDeletePerformance();

	void SetFXParameter(FX::Parameter Parameter, int nValue, int nFX, bool bSaveOnly = false);
	int GetFXParameter(FX::Parameter Parameter, int nFX);

	// Must match the order in CUIMenu::TGParameter
	enum TTGParameter
	{
		TGParameterVoiceBank,
		TGParameterVoiceBankMSB,
		TGParameterVoiceBankLSB,
		TGParameterProgram,
		TGParameterVolume,
		TGParameterPan,
		TGParameterMasterTune,
		TGParameterCutoff,
		TGParameterResonance,
		TGParameterMIDIChannel,
		TGParameterSysExChannel,
		TGParameterSysExEnable,
		TGParameterMIDIRxSustain,
		TGParameterMIDIRxPortamento,
		TGParameterMIDIRxSostenuto,
		TGParameterMIDIRxHold2,
		TGParameterFX1Send,
		TGParameterFX2Send,
		TGParameterPitchBendRange,
		TGParameterPitchBendStep,
		TGParameterPortamentoMode,
		TGParameterPortamentoGlissando,
		TGParameterPortamentoTime,
		TGParameterNoteLimitLow,
		TGParameterNoteLimitHigh,
		TGParameterNoteShift,
		TGParameterMonoMode,
		TGParameterTGLink,

		TGParameterMWRange,
		TGParameterMWPitch,
		TGParameterMWAmplitude,
		TGParameterMWEGBias,

		TGParameterFCRange,
		TGParameterFCPitch,
		TGParameterFCAmplitude,
		TGParameterFCEGBias,

		TGParameterBCRange,
		TGParameterBCPitch,
		TGParameterBCAmplitude,
		TGParameterBCEGBias,

		TGParameterATRange,
		TGParameterATPitch,
		TGParameterATAmplitude,
		TGParameterATEGBias,

		TGParameterCompressorEnable,
		TGParameterCompressorPreGain,
		TGParameterCompressorThresh,
		TGParameterCompressorRatio,
		TGParameterCompressorAttack,
		TGParameterCompressorRelease,
		TGParameterCompressorMakeupGain,

		TGParameterEQLow,
		TGParameterEQMid,
		TGParameterEQHigh,
		TGParameterEQGain,
		TGParameterEQLowMidFreq,
		TGParameterEQMidHighFreq,
		TGParameterEQPreLowcut,
		TGParameterEQPreHighcut,

		TGParameterUnknown
	};

	void SetTGParameter(TTGParameter Parameter, int nValue, int nTG);
	int GetTGParameter(TTGParameter Parameter, int nTG);

	// access (global or OP-related) parameter of the active voice of a TG
	static constexpr int NoOP = 6; // for global parameters
	void SetVoiceParameter(int nOffset, int nValue, int nOP, int nTG);
	int GetVoiceParameter(int nOffset, int nOP, int nTG);

	std::string GetVoiceName(int nTG);

	bool SavePerformance();
	bool DoSavePerformance();

	void setMasterVolume(float vol);

	bool SDFilterOut(int nTG);

	bool InitNetwork();
	void UpdateNetwork();
	const CIPAddress &GetNetworkIPAddress();

private:
	bool ApplyNoteLimits(int *pitch, int nTG); // returns < 0 to ignore note
	uint8_t m_uchOPMask[CConfig::AllToneGenerators];
	void LoadPerformanceParameters();
	void ProcessSound();
	const char *GetNetworkDeviceShortName() const;

#ifdef ARM_ALLOW_MULTI_CORE
	enum TCoreStatus
	{
		CoreStatusInit,
		CoreStatusIdle,
		CoreStatusBusy,
		CoreStatusExit,
		CoreStatusUnknown
	};
#endif

private:
	CConfig *m_pConfig;

	int m_nParameter[ParameterUnknown]; // global (non-TG) parameters
	int m_nFXParameter[CConfig::FXChains][FX::Parameter::Unknown]; // FX parameters

	int m_nToneGenerators;
	int m_nPolyphony;

	CDexedAdapter *m_pTG[CConfig::AllToneGenerators];

	int m_nVoiceBankID[CConfig::AllToneGenerators];
	int m_nVoiceBankIDMSB[CConfig::AllToneGenerators];
	int m_nVoiceBankIDPerformance;
	int m_nVoiceBankIDMSBPerformance;
	int m_nProgram[CConfig::AllToneGenerators];
	int m_nVolume[CConfig::AllToneGenerators];
	int m_nExpression[CConfig::AllToneGenerators];
	int m_nPan[CConfig::AllToneGenerators];
	int m_nMasterTune[CConfig::AllToneGenerators];
	int m_nCutoff[CConfig::AllToneGenerators];
	int m_nResonance[CConfig::AllToneGenerators];
	int m_nMIDIChannel[CConfig::AllToneGenerators];
	int m_nSysExChannel[CConfig::AllToneGenerators];
	bool m_bSysExEnable[CConfig::AllToneGenerators];
	bool m_bMIDIRxSustain[CConfig::AllToneGenerators];
	bool m_bMIDIRxPortamento[CConfig::AllToneGenerators];
	bool m_bMIDIRxSostenuto[CConfig::AllToneGenerators];
	bool m_bMIDIRxHold2[CConfig::AllToneGenerators];
	int m_nPitchBendRange[CConfig::AllToneGenerators];
	int m_nPitchBendStep[CConfig::AllToneGenerators];
	int m_nPortamentoMode[CConfig::AllToneGenerators];
	int m_nPortamentoGlissando[CConfig::AllToneGenerators];
	int m_nPortamentoTime[CConfig::AllToneGenerators];
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

	int m_nNoteLimitLow[CConfig::AllToneGenerators];
	int m_nNoteLimitHigh[CConfig::AllToneGenerators];
	int m_nNoteShift[CConfig::AllToneGenerators];

	int m_nFX1Send[CConfig::AllToneGenerators];
	int m_nFX2Send[CConfig::AllToneGenerators];

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

	uint8_t m_nRawVoiceData[156];

	CUserInterface m_UI;
	CSysExFileLoader m_SysExFileLoader;
	CPerformanceConfig m_PerformanceConfig;

	CMIDIKeyboard *m_pMIDIKeyboard[CConfig::MaxUSBMIDIDevices];
	CPCKeyboard m_PCKeyboard;
	CSerialMIDIDevice m_SerialMIDI;
	float m_fMasterVolume[8];
	float m_fMasterVolumeW;

	SDFilter m_SDFilter;

	bool m_bUseSerial;
	bool m_bQuadDAC8Chan;

	CSoundBaseDevice *m_pSoundDevice;
	bool m_bChannelsSwapped;
	int m_nQueueSizeFrames;

#ifdef ARM_ALLOW_MULTI_CORE
	//	int m_nActiveTGsLog2;
	std::atomic<TCoreStatus> m_CoreStatus[CORES];
	std::atomic<int> m_nFramesToProcess;
	float m_OutputLevel[CConfig::AllToneGenerators][CConfig::MaxChunkSize];
#endif

	int m_nLastKeyDown;

	CPerformanceTimer m_GetChunkTimer;
	bool m_bProfileEnabled;

	AudioFXChain *fx_chain[CConfig::FXChains];
	AudioStereoMixer<CConfig::AllToneGenerators> *tg_mixer;
	AudioStereoMixer<CConfig::AllToneGenerators> *sendfx_mixer[CConfig::FXMixers];

	CSpinLock m_FXSpinLock;

	CStatus m_Status;

	// Network
	CNetSubSystem *m_pNet;
	CNetDevice *m_pNetDevice;
	CBcm4343Device *m_WLAN; // Changed to pointer
	CWPASupplicant *m_WPASupplicant; // Changed to pointer
	bool m_bNetworkReady;
	bool m_bNetworkInit;
	CUDPMIDIDevice *m_UDPMIDI; // Changed to pointer
	CFTPDaemon *m_pFTPDaemon;
	CmDNSPublisher *m_pmDNSPublisher;

	bool m_bSavePerformance;
	bool m_bSavePerformanceNewFile;
	bool m_bSetNewPerformance;
	int m_nSetNewPerformanceID;
	bool m_bSetNewPerformanceBank;
	int m_nSetNewPerformanceBankID;
	bool m_bSetFirstPerformance;
	bool m_bDeletePerformance;
	int m_nDeletePerformanceID;
	bool m_bSaveAsDeault;

	std::atomic<bool> m_bVolRampDownWait;
	std::atomic<bool> m_bVolRampedDown;

	const float m_fRamp;
};
