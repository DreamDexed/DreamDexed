//
// minidexed.cpp
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

#include "minidexed.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#include <circle/gpiopin.h>
#include <circle/i2cmaster.h>
#include <circle/interrupt.h>
#include <circle/logger.h>
#include <circle/memory.h>
#include <circle/memorymap.h>
#include <circle/multicore.h>
#include <circle/net/ipaddress.h>
#include <circle/net/netsubsystem.h>
#include <circle/net/syslogdaemon.h>
#include <circle/netdevice.h>
#include <circle/sched/scheduler.h>
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/sound/soundbasedevice.h>
#include <circle/spimaster.h>
#include <circle/string.h>
#include <circle/synchronize.h>
#include <dsp/basic_math_functions.h>
#include <fatfs/ff.h>
#include <wlan/bcm4343.h>
#include <wlan/hostap/wpa_supplicant/wpasupplicant.h>

#include "arm/arm_float_to_q23.h"
#include "arm/arm_scale_zip_f32.h"
#include "arm/arm_zip_f32.h"
#include "bus.h"
#include "common.h"
#include "config.h"
#include "dexedadapter.h"
#include "effect.h"
#include "effect_chain.h"
#include "effect_compressor.h"
#include "effect_dreamdelay.h"
#include "effect_mixer.hpp"
#include "midi.h"
#include "mididevice.h"
#include "midikeyboard.h"
#include "net/ftpdaemon.h"
#include "net/mdnspublisher.h"
#include "performanceconfig.h"
#include "sdfilter.h"
#include "status.h"
#include "sysexfileloader.h"
#include "udpmididevice.h"
#include "util.h"

const char WLANFirmwarePath[] = "firmware/";
const char WLANConfigFile[] = "wpa_supplicant.conf";
#define FTPUSERNAME "admin"
#define FTPPASSWORD "admin"

LOGMODULE("minidexed");

CMiniDexed::CMiniDexed(CConfig *pConfig, CInterruptSystem *pInterrupt,
		       CGPIOManager *pGPIOManager, CI2CMaster *pI2CMaster, CSPIMaster *pSPIMaster, FATFS *pFileSystem) :
#ifdef ARM_ALLOW_MULTI_CORE
CMultiCoreSupport{CMemorySystem::Get()},
#endif
m_pConfig{pConfig},
m_pTG{},
m_UI{this, pGPIOManager, pI2CMaster, pSPIMaster, pConfig},
m_PerformanceConfig{pFileSystem},
m_pMIDIKeyboard{},
m_PCKeyboard{this, pConfig, &m_UI},
m_SerialMIDI{this, pInterrupt, pConfig, &m_UI},
m_fMasterVolume{},
m_fMasterVolumeW{},
m_fBusGain{},
m_SDFilter{},
m_bUseSerial{},
m_bQuadDAC8Chan{},
m_pSoundDevice{},
m_bChannelsSwapped{pConfig->GetChannelsSwapped()},
#ifdef ARM_ALLOW_MULTI_CORE
// m_nActiveTGsLog2{0},
#endif
m_nLastKeyDown{},
m_GetChunkTimer{"GetChunk", 1000000 * pConfig->GetChunkSize() / 2 / pConfig->GetSampleRate()},
m_bProfileEnabled{m_pConfig->GetProfileEnabled()},
fx_chain{},
bus_mixer{},
sendfx_mixer{},
m_pNet{},
m_pNetDevice{},
m_WLAN{},
m_WPASupplicant{},
m_bNetworkReady{},
m_bNetworkInit{},
m_UDPMIDI{},
m_pFTPDaemon{},
m_pmDNSPublisher{},
m_bSavePerformance{},
m_bSavePerformanceNewFile{},
m_bSetNewPerformance{},
m_bSetNewPerformanceBank{},
m_bSetFirstPerformance{},
m_bDeletePerformance{},
m_bVolRampDownWait{},
m_bVolRampedDown{},
m_fRamp{10.0f / pConfig->GetSampleRate()}
{
	assert(m_pConfig);

	m_nToneGenerators = m_pConfig->GetToneGenerators();
	m_nPolyphony = m_pConfig->GetPolyphony();
	LOGNOTE("Tone Generators=%d, Polyphony=%d", m_nToneGenerators, m_nPolyphony);

	for (int i = 0; i < CConfig::AllToneGenerators; i++)
	{
		m_nVoiceBankID[i] = 0;
		m_nVoiceBankIDMSB[i] = 0;
		m_nProgram[i] = 0;
		m_nVolume[i] = 100;
		m_nExpression[i] = 127;
		m_nPan[i] = 64;
		m_nMasterTune[i] = 0;
		m_nCutoff[i] = 99;
		m_nResonance[i] = 0;
		m_nMIDIChannel[i] = CMIDIDevice::Disabled;
		m_nSysExChannel[i] = 0;
		m_bSysExEnable[i] = 1;
		m_bMIDIRxSustain[i] = 1;
		m_bMIDIRxPortamento[i] = 1;
		m_bMIDIRxSostenuto[i] = 1;
		m_bMIDIRxHold2[i] = 1;
		m_nPitchBendRange[i] = 2;
		m_nPitchBendStep[i] = 0;
		m_nPortamentoMode[i] = 0;
		m_nPortamentoGlissando[i] = 0;
		m_nPortamentoTime[i] = 0;
		m_bMonoMode[i] = 0;
		m_nTGLink[i] = 0;
		m_nNoteLimitLow[i] = 0;
		m_nNoteLimitHigh[i] = 127;
		m_nNoteShift[i] = 0;

		m_nModulationWheelRange[i] = 99;
		m_nModulationWheelTarget[i] = 7;
		m_nFootControlRange[i] = 99;
		m_nFootControlTarget[i] = 0;
		m_nBreathControlRange[i] = 99;
		m_nBreathControlTarget[i] = 0;
		m_nAftertouchRange[i] = 99;
		m_nAftertouchTarget[i] = 0;

		m_nFX1Send[i] = 25;
		m_nFX2Send[i] = 0;

		m_bCompressorEnable[i] = 0;
		m_nCompressorPreGain[i] = 0;
		m_nCompressorThresh[i] = -20;
		m_nCompressorRatio[i] = 5;
		m_nCompressorAttack[i] = 5;
		m_nCompressorRelease[i] = 200;
		m_nCompressorMakeupGain[i] = 0;

		m_nEQLow[i] = 0;
		m_nEQMid[i] = 0;
		m_nEQHigh[i] = 0;
		m_nEQGain[i] = 0;
		m_nEQLowMidFreq[i] = 24;
		m_nEQMidHighFreq[i] = 44;
		m_nEQPreLowcut[i] = 0;
		m_nEQPreHighcut[i] = 60;

		// Active the required number of active TGs
		if (i < m_nToneGenerators)
		{
			m_uchOPMask[i] = 0b111111; // All operators on

			m_pTG[i] = new CDexedAdapter(m_nPolyphony, pConfig->GetSampleRate());
			assert(m_pTG[i]);

			m_pTG[i]->setEngineType(pConfig->GetEngineType());
			m_pTG[i]->activate();
		}
	}

	unsigned nUSBGadgetPin = pConfig->GetUSBGadgetPin();
	bool bUSBGadget = pConfig->GetUSBGadget();
	bool bUSBGadgetMode = pConfig->GetUSBGadgetMode();

	if (bUSBGadgetMode)
	{
#if RASPPI == 5
		LOGNOTE("USB Gadget (Device) Mode NOT supported on RPI 5");
#else
		if (nUSBGadgetPin == 0)
		{
			LOGNOTE("USB In Gadget (Device) Mode");
		}
		else
		{
			LOGNOTE("USB In Gadget (Device) Mode [USBGadgetPin %d = LOW]", nUSBGadgetPin);
		}
#endif
	}
	else
	{
		if (bUSBGadget)
		{
			if (nUSBGadgetPin == 0)
			{
				// This shouldn't be possible...
				LOGNOTE("USB State Unknown");
			}
			else
			{
				LOGNOTE("USB In Host Mode [USBGadgetPin %d = HIGH]", nUSBGadgetPin);
			}
		}
		else
		{
			LOGNOTE("USB In Host Mode");
		}
	}

	for (int i = 0; i < CConfig::MaxUSBMIDIDevices; i++)
	{
		m_pMIDIKeyboard[i] = new CMIDIKeyboard(this, pConfig, &m_UI, i);
		assert(m_pMIDIKeyboard[i]);
	}

	// select the sound device
	const char *pDeviceName = pConfig->GetSoundDevice();
	if (strcmp(pDeviceName, "i2s") == 0)
	{
		LOGNOTE("I2S mode");
#if RASPPI == 5
		// Quad DAC 8-channel mono only an option for RPI 5
		m_bQuadDAC8Chan = pConfig->GetQuadDAC8Chan();
#endif
		if (m_bQuadDAC8Chan && (m_nToneGenerators != 8))
		{
			LOGNOTE("ERROR: Quad DAC Mode is only valid when number of TGs = 8.  Defaulting to non-Quad DAC mode,");
			m_bQuadDAC8Chan = false;
		}
		if (m_bQuadDAC8Chan)
		{
			LOGNOTE("Configured for Quad DAC 8-channel Mono audio");
			m_pSoundDevice = new CI2SSoundBaseDevice(pInterrupt, pConfig->GetSampleRate(),
								 pConfig->GetChunkSize(), false,
								 pI2CMaster, pConfig->GetDACI2CAddress(),
								 CI2SSoundBaseDevice::DeviceModeTXOnly,
								 8); // 8 channels - L+R x4 across 4 I2S lanes
		}
		else
		{
			m_pSoundDevice = new CI2SSoundBaseDevice(pInterrupt, pConfig->GetSampleRate(),
								 pConfig->GetChunkSize(), false,
								 pI2CMaster, pConfig->GetDACI2CAddress(),
								 CI2SSoundBaseDevice::DeviceModeTXOnly,
								 2); // 2 channels - L+R
		}
	}
	else if (strcmp(pDeviceName, "hdmi") == 0)
	{
#if RASPPI == 5
		LOGNOTE("HDMI mode NOT supported on RPI 5.");
#else
		LOGNOTE("HDMI mode");

		m_pSoundDevice = new CHDMISoundBaseDevice(pInterrupt, pConfig->GetSampleRate(),
							  pConfig->GetChunkSize());
#endif
	}
	else
	{
		LOGNOTE("PWM mode");

		m_pSoundDevice = new CPWMSoundBaseDevice(pInterrupt, pConfig->GetSampleRate(),
							 pConfig->GetChunkSize());
	}

#ifdef ARM_ALLOW_MULTI_CORE
	for (int nCore = 0; nCore < CORES; nCore++)
	{
		m_CoreStatus[nCore] = CoreStatusInit;
	}
#endif

	for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
	{
		bus_mixer[nBus] = new AudioStereoMixer<CConfig::AllToneGenerators>(pConfig->GetChunkSize() / 2, pConfig->GetSampleRate());

		for (int nParam = 0; nParam < Bus::Parameter::Unknown; ++nParam)
		{
			const Bus::ParameterType &p = Bus::s_Parameter[nParam];
			SetBusParameter(Bus::Parameter(nParam), p.Default, nBus);
		}
	}

	for (int nMX = 0; nMX < CConfig::FXMixers; nMX++)
	{
		sendfx_mixer[nMX] = new AudioStereoMixer<CConfig::AllToneGenerators>(pConfig->GetChunkSize() / 2, pConfig->GetSampleRate());
	}

	for (int nFX = 0; nFX < CConfig::FXChains; nFX++)
	{
		fx_chain[nFX] = new AudioFXChain(pConfig->GetSampleRate());

		for (int nParam = 0; nParam < FX::Parameter::Unknown; ++nParam)
		{
			const FX::ParameterType &p = FX::s_Parameter[nParam];
			bool bSaveOnly = p.Flags & FX::Flag::Composite;
			SetFXParameter(FX::Parameter(nParam), p.Default, nFX, bSaveOnly);
		}
	}

	SetParameter(ParameterMasterVolume, pConfig->GetMasterVolume());
	SetParameter(ParameterSDFilter, 0);

	SetPerformanceSelectChannel(m_pConfig->GetPerformanceSelectChannel());

	SetParameter(ParameterPerformanceBank, 0);
};

CMiniDexed::~CMiniDexed()
{
	delete m_WLAN;
	delete m_WPASupplicant;
	delete m_UDPMIDI;
	delete m_pFTPDaemon;
	delete m_pmDNSPublisher;
}

bool CMiniDexed::Initialize()
{
	LOGNOTE("CMiniDexed::Initialize called");
	assert(m_pConfig);
	assert(m_pSoundDevice);

	if (!m_UI.Initialize())
	{
		return false;
	}

	m_SysExFileLoader.Load(m_pConfig->GetHeaderlessSysExVoices());

	if (m_SerialMIDI.Initialize())
	{
		LOGNOTE("Serial MIDI interface enabled");

		m_bUseSerial = true;
	}

	if (m_pConfig->GetMIDIRXProgramChange())
	{
		int nPerfCh = GetParameter(ParameterPerformanceSelectChannel);
		if (nPerfCh == CMIDIDevice::Disabled)
		{
			LOGNOTE("Program Change: Enabled for Voices");
		}
		else if (nPerfCh == CMIDIDevice::OmniMode)
		{
			LOGNOTE("Program Change: Enabled for Performances (Omni)");
		}
		else
		{
			LOGNOTE("Program Change: Enabled for Performances (CH %d)", nPerfCh + 1);
		}
	}
	else
	{
		LOGNOTE("Program Change: Disabled");
	}

	for (int i = 0; i < m_nToneGenerators; i++)
	{
		assert(m_pTG[i]);

		SetVolume(100, i);
		SetExpression(127, i);
		ProgramChange(0, i);

		m_pTG[i]->setTranspose(24);

		m_pTG[i]->setPBController(2, 0);
		m_pTG[i]->setMWController(99, 1, 0);

		m_pTG[i]->setFCController(99, 1, 0);
		m_pTG[i]->setBCController(99, 1, 0);
		m_pTG[i]->setATController(99, 1, 0);

		for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
		{
			bus_mixer[nBus]->pan(i, mapfloat(m_nPan[i], 0, 127, 0.0f, 1.0f));
			bus_mixer[nBus]->gain(i, 1.0f);

			for (int idFX = 0; idFX < CConfig::BusFXChains; ++idFX)
			{
				int nFX = idFX + CConfig::BusFXChains * nBus;
				sendfx_mixer[nFX]->pan(i, mapfloat(m_nPan[i], 0, 127, 0.0f, 1.0f));
				sendfx_mixer[nFX]->gain(i, mapfloat(idFX == 0 ? m_nFX1Send[i] : m_nFX2Send[i], 0, 99, 0.0f, 1.0f));
			}
		}
	}

	m_PerformanceConfig.Init(m_nToneGenerators);
	if (m_PerformanceConfig.Load())
	{
		LoadPerformanceParameters();
	}
	else
	{
		SetMIDIChannel(CMIDIDevice::OmniMode, 0);
	}

	// setup and start the sound device
	unsigned Channels = 1; // 16-bit Mono
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_bQuadDAC8Chan)
	{
		Channels = 8; // 16-bit 8-channel mono
	}
	else
	{
		Channels = 2; // 16-bit Stereo
	}
#endif
	// Need 2 x ChunkSize / Channel queue frames as the audio driver uses
	// two DMA channels each of ChunkSize and one single single frame
	// contains a sample for each of all the channels.
	//
	// See discussion here: https://github.com/rsta2/circle/discussions/453
	if (!m_pSoundDevice->AllocateQueueFrames(2 * m_pConfig->GetChunkSize() / Channels))
	{
		LOGERR("Cannot allocate sound queue");

		return false;
	}

	m_pSoundDevice->SetWriteFormat(SoundFormatSigned24_32, Channels);

	m_nQueueSizeFrames = static_cast<int>(m_pSoundDevice->GetQueueSizeFrames());

	m_pSoundDevice->Start();

	m_UI.LoadDefaultScreen();

#ifdef ARM_ALLOW_MULTI_CORE
	// start secondary cores
	if (!CMultiCoreSupport::Initialize())
	{
		return false;
	}

	InitNetwork(); // returns bool but we continue even if something goes wrong
	LOGNOTE("CMiniDexed::Initialize: InitNetwork() called");
#endif

	return true;
}

void CMiniDexed::Process(bool bPlugAndPlayUpdated)
{
	CScheduler *const pScheduler = CScheduler::Get();
#ifndef ARM_ALLOW_MULTI_CORE
	ProcessSound();
	pScheduler->Yield();
#endif

	for (int i = 0; i < CConfig::MaxUSBMIDIDevices; i++)
	{
		assert(m_pMIDIKeyboard[i]);
		m_pMIDIKeyboard[i]->Process(bPlugAndPlayUpdated);
		pScheduler->Yield();
	}

	m_PCKeyboard.Process(bPlugAndPlayUpdated);
	pScheduler->Yield();

	if (m_bUseSerial)
	{
		m_SerialMIDI.Process();
		pScheduler->Yield();
	}

	m_UI.Process();

	if (m_bSavePerformance)
	{
		DoSavePerformance();

		m_bSavePerformance = false;
	}

	if (m_bSavePerformanceNewFile)
	{
		DoSavePerformanceNewFile();
		m_bSavePerformanceNewFile = false;
	}

	if (m_bSetNewPerformanceBank)
	{
		m_PerformanceConfig.SetNewPerformanceBank(m_nSetNewPerformanceBankID);

		if (m_nSetNewPerformanceBankID == m_PerformanceConfig.GetPerformanceBankID())
		{
			m_bSetNewPerformanceBank = false;
		}

		// If there is no pending SetNewPerformance already, then see if we need to find the first performance to load
		// NB: If called from the UI, then there will not be a SetNewPerformance, so load the first existing one.
		//     If called from MIDI, there will probably be a SetNewPerformance alongside the Bank select.
		if (!m_bSetNewPerformance && m_bSetFirstPerformance)
		{
			DoSetFirstPerformance();
		}
	}

	if (m_bSetNewPerformance && m_bVolRampedDown && !m_bSetNewPerformanceBank)
	{
		for (int i = 0; i < m_nToneGenerators; ++i)
		{
			m_pTG[i]->resetState();
		}

		DoSetNewPerformance();

		for (int nFX = 0; nFX < CConfig::FXChains; ++nFX)
		{
			fx_chain[nFX]->resetState();
		}

		if (m_nSetNewPerformanceID == GetActualPerformanceID())
		{
			m_bSetNewPerformance = false;
			m_bVolRampedDown = false;
		}
	}

	if (m_bDeletePerformance)
	{
		DoDeletePerformance();
		m_bDeletePerformance = false;
	}

	if (m_bProfileEnabled)
	{
		m_GetChunkTimer.Dump();
	}

	m_Status.Update();

	if (m_pNet)
	{
		UpdateNetwork();
	}
	// Allow other tasks to run
	pScheduler->Yield();
}

#ifdef ARM_ALLOW_MULTI_CORE

void CMiniDexed::Run(unsigned nCore)
{
	assert(1 <= nCore && nCore < CORES);

	if (nCore == 1)
	{
		m_CoreStatus[nCore] = CoreStatusIdle; // core 1 ready

		// wait for cores 2 and 3 to be ready
		for (int nCore = 2; nCore < CORES; nCore++)
		{
			while (m_CoreStatus[nCore] != CoreStatusIdle)
			{
				WaitForEvent();
			}
		}

		while (m_CoreStatus[nCore] != CoreStatusExit)
		{
			ProcessSound();
		}
	}
	else // core 2 and 3
	{
		while (1)
		{
			m_CoreStatus[nCore] = CoreStatusIdle; // ready to be kicked
			SendIPI(1, IPI_USER);

			while (m_CoreStatus[nCore] == CoreStatusIdle)
			{
				WaitForEvent();
			}

			// now kicked from core 1

			if (m_CoreStatus[nCore] == CoreStatusExit)
			{
				m_CoreStatus[nCore] = CoreStatusUnknown;

				break;
			}

			assert(m_CoreStatus[nCore] == CoreStatusBusy);

			// process the TGs, assigned to this core (2 or 3)

			assert(m_nFramesToProcess <= m_pConfig->MaxChunkSize);
			int nTG = m_pConfig->GetTGsCore1() + (static_cast<int>(nCore) - 2) * m_pConfig->GetTGsCore23();
			for (int i = 0; i < m_pConfig->GetTGsCore23(); i++, nTG++)
			{
				assert(nTG < CConfig::AllToneGenerators);
				if (nTG < m_pConfig->GetToneGenerators())
				{
					assert(m_pTG[nTG]);
					m_pTG[nTG]->getSamples(m_OutputLevel[nTG], m_nFramesToProcess);
				}
			}
		}
	}
}

#endif

CSysExFileLoader *CMiniDexed::GetSysExFileLoader()
{
	return &m_SysExFileLoader;
}

CPerformanceConfig *CMiniDexed::GetPerformanceConfig()
{
	return &m_PerformanceConfig;
}

void CMiniDexed::BankSelect(int nBank, int nTG)
{
	nBank = constrain(nBank, 0, 16383);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	if (GetSysExFileLoader()->IsValidBank(nBank))
	{
		// Only change if we have the bank loaded
		m_nVoiceBankID[nTG] = nBank;

		m_UI.ParameterChanged();
	}
}

void CMiniDexed::BankSelectPerformance(int nBank)
{
	nBank = constrain(nBank, 0, 16383);

	if (GetPerformanceConfig()->IsValidPerformanceBank(nBank))
	{
		// Only change if we have the bank loaded
		m_nVoiceBankIDPerformance = nBank;
		SetNewPerformanceBank(nBank);

		m_UI.ParameterChanged();
	}
}

void CMiniDexed::BankSelectMSB(int nBankMSB, int nTG)
{
	nBankMSB = constrain(nBankMSB, 0, 127);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	// MIDI Spec 1.0 "BANK SELECT" states:
	//   "The transmitter must transmit the MSB and LSB as a pair,
	//   and the Program Change must be sent immediately after
	//   the Bank Select pair."
	//
	// So it isn't possible to validate the selected bank ID until
	// we receive both MSB and LSB so just store the MSB for now.
	m_nVoiceBankIDMSB[nTG] = nBankMSB;
}

void CMiniDexed::BankSelectMSBPerformance(int nBankMSB)
{
	nBankMSB = constrain(nBankMSB, 0, 127);
	m_nVoiceBankIDMSBPerformance = nBankMSB;
}

void CMiniDexed::BankSelectLSB(int nBankLSB, int nTG)
{
	nBankLSB = constrain(nBankLSB, 0, 127);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	int nBank = m_nVoiceBankID[nTG];
	int nBankMSB = m_nVoiceBankIDMSB[nTG];
	nBank = (nBankMSB << 7) + nBankLSB;

	// Now should have both MSB and LSB so enable the BankSelect
	BankSelect(nBank, nTG);
}

void CMiniDexed::BankSelectLSBPerformance(int nBankLSB)
{
	nBankLSB = constrain(nBankLSB, 0, 127);

	int nBank = m_nVoiceBankIDPerformance;
	int nBankMSB = m_nVoiceBankIDMSBPerformance;
	nBank = (nBankMSB << 7) + nBankLSB;

	// Now should have both MSB and LSB so enable the BankSelect
	BankSelectPerformance(nBank);
}

void CMiniDexed::ProgramChange(int nProgram, int nTG)
{
	assert(m_pConfig);

	int nBankOffset;
	bool bPCAcrossBanks = m_pConfig->GetExpandPCAcrossBanks();
	if (bPCAcrossBanks)
	{
		// Note: This doesn't actually change the bank in use
		//       but will allow PC messages of 0..127
		//       to select across four consecutive banks of voices.
		//
		//   So if the current bank = 5 then:
		//       PC  0-31  = Bank 5, Program 0-31
		//       PC 32-63  = Bank 6, Program 0-31
		//       PC 64-95  = Bank 7, Program 0-31
		//       PC 96-127 = Bank 8, Program 0-31
		nProgram = constrain(nProgram, 0, 127);
		nBankOffset = nProgram >> 5;
		nProgram = nProgram % 32;
	}
	else
	{
		nBankOffset = 0;
		nProgram = constrain(nProgram, 0, 31);
	}

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nProgram[nTG] = nProgram;

	uint8_t Buffer[156];
	m_SysExFileLoader.GetVoice(m_nVoiceBankID[nTG] + nBankOffset, nProgram, Buffer);

	assert(m_pTG[nTG]);
	m_pTG[nTG]->loadVoiceParameters(Buffer);
	setOPMask(0b111111, nTG);

	if (m_pConfig->GetMIDIAutoVoiceDumpOnPC())
	{
		// Only do the voice dump back out over MIDI if we have enabled
		// the SysEx for this TG
		if (m_bSysExEnable[nTG])
		{
			m_SerialMIDI.SendSystemExclusiveVoice(nProgram, m_SerialMIDI.GetDeviceName(), 0, nTG);
		}
	}

	m_UI.ParameterChanged();
}

void CMiniDexed::ProgramChangePerformance(int nProgram)
{
	if (m_nParameter[ParameterPerformanceSelectChannel] != CMIDIDevice::Disabled)
	{
		// Program Change messages change Performances.
		if (m_PerformanceConfig.IsValidPerformance(nProgram))
		{
			SetNewPerformance(nProgram);
		}
		m_UI.ParameterChanged();
	}
}

void CMiniDexed::SetVolume(int nVolume, int nTG)
{
	nVolume = constrain(nVolume, 0, 127);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nVolume[nTG] = nVolume;

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setGain((m_nVolume[nTG] * m_nExpression[nTG]) / (127.0f * 127.0f));

	m_UI.ParameterChanged();
}

void CMiniDexed::SetExpression(int nExpression, int nTG)
{
	nExpression = constrain(nExpression, 0, 127);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nExpression[nTG] = nExpression;

	m_pTG[nTG]->setGain((m_nVolume[nTG] * m_nExpression[nTG]) / (127.0f * 127.0f));

	// Expression is a "live performance" parameter only set
	// via MIDI and not via the UI.
	// m_UI.ParameterChanged ();
}

void CMiniDexed::SetPan(int nPan, int nTG)
{
	nPan = constrain(nPan, 0, 127);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nPan[nTG] = nPan;

	for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
	{
		bus_mixer[nBus]->pan(nTG, mapfloat(nPan, 0, 127, 0.0f, 1.0f));

		for (int idFX = 0; idFX < CConfig::BusFXChains; ++idFX)
		{
			int nFX = idFX + CConfig::BusFXChains * nBus;
			sendfx_mixer[nFX]->pan(nTG, mapfloat(nPan, 0, 127, 0.0f, 1.0f));
		}
	}

	m_UI.ParameterChanged();
}

void CMiniDexed::SetFX1Send(int nFX1Send, int nTG)
{
	nFX1Send = constrain(nFX1Send, 0, 99);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG
	if (0 >= CConfig::FXMixers) return;

	m_nFX1Send[nTG] = nFX1Send;

	for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
	{
		int nFX = 0 + CConfig::BusFXChains * nBus;
		sendfx_mixer[nFX]->gain(nTG, mapfloat(nFX1Send, 0, 99, 0.0f, 1.0f));
	}

	m_UI.ParameterChanged();
}

void CMiniDexed::SetFX2Send(int nFX2Send, int nTG)
{
	nFX2Send = constrain(nFX2Send, 0, 99);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG
	if (1 >= CConfig::FXMixers) return;

	m_nFX2Send[nTG] = nFX2Send;

	for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
	{
		int nFX = 1 + CConfig::BusFXChains * nBus;
		sendfx_mixer[nFX]->gain(nTG, mapfloat(nFX2Send, 0, 99, 0.0f, 1.0f));
	}

	m_UI.ParameterChanged();
}

void CMiniDexed::SetMasterTune(int nMasterTune, int nTG)
{
	nMasterTune = constrain(nMasterTune, -99, 99);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nMasterTune[nTG] = nMasterTune;

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setMasterTune((int8_t)nMasterTune);

	m_UI.ParameterChanged();
}

void CMiniDexed::SetCutoff(int nCutoff, int nTG)
{
	nCutoff = constrain(nCutoff, 0, 99);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nCutoff[nTG] = nCutoff;

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setFilterCutoff(mapfloat(nCutoff, 0, 99, 0.0f, 1.0f));

	m_UI.ParameterChanged();
}

void CMiniDexed::SetResonance(int nResonance, int nTG)
{
	nResonance = constrain(nResonance, 0, 99);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nResonance[nTG] = nResonance;

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setFilterResonance(mapfloat(nResonance, 0, 99, 0.0f, 1.0f));

	m_UI.ParameterChanged();
}

void CMiniDexed::SetMIDIChannel(int nChannel, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(nChannel < CMIDIDevice::ChannelUnknown);

	m_nMIDIChannel[nTG] = nChannel;

	for (int i = 0; i < CConfig::MaxUSBMIDIDevices; i++)
	{
		assert(m_pMIDIKeyboard[i]);
		m_pMIDIKeyboard[i]->SetChannel(nChannel, nTG);
	}

	m_PCKeyboard.SetChannel(nChannel, nTG);

	if (m_bUseSerial)
	{
		m_SerialMIDI.SetChannel(nChannel, nTG);
	}
	if (m_UDPMIDI)
	{
		m_UDPMIDI->SetChannel(nChannel, nTG);
	}

#ifdef ARM_ALLOW_MULTI_CORE
/* This doesn't appear to be used anywhere...
	int nActiveTGs = 0;
	for (int nTG = 0; nTG < CConfig::ToneGenerators; nTG++)
	{
		if (m_nMIDIChannel[nTG] != CMIDIDevice::Disabled)
		{
			nActiveTGs++;
		}
	}

	assert (nActiveTGs <= 8);
	static const int Log2[] = {0, 0, 1, 2, 2, 3, 3, 3, 3};
		m_nActiveTGsLog2 = Log2[nActiveTGs];
*/
#endif

	m_UI.ParameterChanged();
}

void CMiniDexed::SetSysExChannel(int nChannel, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(nChannel < CMIDIDevice::Channels);

	m_nSysExChannel[nTG] = nChannel;

	m_UI.ParameterChanged();
}

void CMiniDexed::SetSysExEnable(bool value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_bSysExEnable[nTG] = value;

	m_UI.ParameterChanged();
}

void CMiniDexed::SetMIDIRxSustain(bool value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_bMIDIRxSustain[nTG] = value;

	m_UI.ParameterChanged();
}

void CMiniDexed::SetMIDIRxPortamento(bool value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_bMIDIRxPortamento[nTG] = value;

	m_UI.ParameterChanged();
}

void CMiniDexed::SetMIDIRxSostenuto(bool value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_bMIDIRxSostenuto[nTG] = value;

	m_UI.ParameterChanged();
}

void CMiniDexed::SetMIDIRxHold2(bool value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_bMIDIRxHold2[nTG] = value;

	m_UI.ParameterChanged();
}

int CMiniDexed::GetSysExChannel(int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nSysExChannel[nTG];
}

bool CMiniDexed::GetSysExEnable(int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bSysExEnable[nTG];
}

void CMiniDexed::keyup(int pitch, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	if (ApplyNoteLimits(&pitch, nTG))
	{
		m_pTG[nTG]->keyup(static_cast<uint8_t>(pitch));
	}
}

void CMiniDexed::keydown(int pitch, int velocity, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nLastKeyDown = pitch;

	if (ApplyNoteLimits(&pitch, nTG))
	{
		m_pTG[nTG]->keydown(static_cast<uint8_t>(pitch), static_cast<uint8_t>(velocity));
	}
}

bool CMiniDexed::ApplyNoteLimits(int *pitch, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return -1; // Not an active TG

	if (*pitch < m_nNoteLimitLow[nTG] || *pitch > m_nNoteLimitHigh[nTG])
	{
		return false;
	}

	int p = *pitch + m_nNoteShift[nTG];

	if (p < 0 || p > 127)
	{
		return false;
	}

	*pitch = p;
	return true;
}

void CMiniDexed::setSustain(bool sustain, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setSustain(sustain);

	// TODO: use the bus MIDI channel for sustain
	for (int i = 0; i < CConfig::FXChains; ++i)
		fx_chain[i]->zyn_sympathetic.sustain(sustain);
}

void CMiniDexed::setSostenuto(bool sostenuto, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setSostenuto(sostenuto);
}

void CMiniDexed::setHoldMode(bool holdmode, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setHold(holdmode);
}

void CMiniDexed::panic(int value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	if (value == 0)
	{
		m_pTG[nTG]->panic();
	}
}

void CMiniDexed::notesOff(int value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	if (value == 0)
	{
		m_pTG[nTG]->notesOff();
	}
}

void CMiniDexed::setModWheel(int value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setModWheel(static_cast<uint8_t>(value));
}

void CMiniDexed::setFootController(uint8_t value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setFootController(value);
}

void CMiniDexed::setBreathController(uint8_t value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setBreathController(value);
}

void CMiniDexed::setAftertouch(uint8_t value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setAftertouch(value);
}

void CMiniDexed::setPitchbend(int16_t value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->setPitchbend(value);
}

void CMiniDexed::ControllersRefresh(int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_pTG[nTG]->ControllersRefresh();
}

void CMiniDexed::SetParameter(TParameter Parameter, int nValue)
{
	assert(Parameter < ParameterUnknown);

	switch (Parameter)
	{
	case ParameterPerformanceSelectChannel:
		// Nothing more to do
		break;

	case ParameterPerformanceBank:
		BankSelectPerformance(nValue);
		break;

	case ParameterMasterVolume:
		nValue = constrain(nValue, 0, 127);
		setMasterVolume(nValue / 127.0f);
		m_UI.ParameterChanged();
		break;

	case ParameterSDFilter:
		m_SDFilter = SDFilter::to_filter(nValue, m_pConfig->GetToneGenerators());
		m_UI.ParameterChanged();
		break;

	default:
		assert(0);
		break;
	}

	m_nParameter[Parameter] = nValue;
}

int CMiniDexed::GetParameter(TParameter Parameter)
{
	assert(Parameter < ParameterUnknown);
	return m_nParameter[Parameter];
}

void CMiniDexed::SetFXParameter(FX::Parameter Parameter, int nValue, int nFX, bool bSaveOnly)
{
	assert(nFX < CConfig::FXChains);
	assert(Parameter < FX::Parameter::Unknown);

	const FX::ParameterType &p = FX::s_Parameter[Parameter];
	nValue = constrain(nValue, p.Minimum, p.Maximum);

	m_nFXParameter[nFX][Parameter] = nValue;

	if (bSaveOnly) return;

	switch (Parameter)
	{
	case FX::Parameter::Slot0:
	case FX::Parameter::Slot1:
	case FX::Parameter::Slot2:
		fx_chain[nFX]->setSlot(Parameter - FX::Parameter::Slot0, nValue);
		break;

	case FX::Parameter::ZynDistortionPreset:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_distortion.loadpreset(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynDistortionMix:
	case FX::Parameter::ZynDistortionPanning:
	case FX::Parameter::ZynDistortionDrive:
	case FX::Parameter::ZynDistortionLevel:
	case FX::Parameter::ZynDistortionType:
	case FX::Parameter::ZynDistortionNegate:
	case FX::Parameter::ZynDistortionFiltering:
	case FX::Parameter::ZynDistortionLowcut:
	case FX::Parameter::ZynDistortionHighcut:
	case FX::Parameter::ZynDistortionStereo:
	case FX::Parameter::ZynDistortionLRCross:
	case FX::Parameter::ZynDistortionShape:
	case FX::Parameter::ZynDistortionOffset:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_distortion.changepar(Parameter - FX::Parameter::ZynDistortionMix, nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynDistortionBypass:
		fx_chain[nFX]->zyn_distortion.bypass = nValue;
		break;

	case FX::Parameter::YKChorusMix:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->yk_chorus.setMix(nValue / 100.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::YKChorusEnable1:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->yk_chorus.setChorus1(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::YKChorusEnable2:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->yk_chorus.setChorus2(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::YKChorusLFORate1:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->yk_chorus.setChorus1LFORate(nValue / 100.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::YKChorusLFORate2:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->yk_chorus.setChorus2LFORate(nValue / 100.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::YKChorusBypass:
		fx_chain[nFX]->yk_chorus.bypass = nValue;
		break;

	case FX::Parameter::ZynChorusPreset:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_chorus.loadpreset(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynChorusMix:
	case FX::Parameter::ZynChorusPanning:
	case FX::Parameter::ZynChorusLFOFreq:
	case FX::Parameter::ZynChorusLFORandomness:
	case FX::Parameter::ZynChorusLFOType:
	case FX::Parameter::ZynChorusLFOLRDelay:
	case FX::Parameter::ZynChorusDepth:
	case FX::Parameter::ZynChorusDelay:
	case FX::Parameter::ZynChorusFeedback:
	case FX::Parameter::ZynChorusLRCross:
	case FX::Parameter::ZynChorusMode:
	case FX::Parameter::ZynChorusSubtractive:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_chorus.changepar(Parameter - FX::Parameter::ZynChorusMix, nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynChorusBypass:
		fx_chain[nFX]->zyn_chorus.bypass = nValue;
		break;

	case FX::Parameter::ZynSympatheticPreset:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_sympathetic.loadpreset(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynSympatheticMix:
	case FX::Parameter::ZynSympatheticPanning:
	case FX::Parameter::ZynSympatheticQ:
	case FX::Parameter::ZynSympatheticQSustain:
	case FX::Parameter::ZynSympatheticDrive:
	case FX::Parameter::ZynSympatheticLevel:
	case FX::Parameter::ZynSympatheticType:
	case FX::Parameter::ZynSympatheticUnisonSize:
	case FX::Parameter::ZynSympatheticUnisonSpread:
	case FX::Parameter::ZynSympatheticStrings:
	case FX::Parameter::ZynSympatheticInterval:
	case FX::Parameter::ZynSympatheticBaseNote:
	case FX::Parameter::ZynSympatheticLowcut:
	case FX::Parameter::ZynSympatheticHighcut:
	case FX::Parameter::ZynSympatheticNegate:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_sympathetic.changepar(Parameter - FX::Parameter::ZynSympatheticMix, nValue, true);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynSympatheticBypass:
		fx_chain[nFX]->zyn_sympathetic.bypass = nValue;
		break;

	case FX::Parameter::ZynAPhaserPreset:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_aphaser.loadpreset(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynAPhaserMix:
	case FX::Parameter::ZynAPhaserPanning:
	case FX::Parameter::ZynAPhaserLFOFreq:
	case FX::Parameter::ZynAPhaserLFORandomness:
	case FX::Parameter::ZynAPhaserLFOType:
	case FX::Parameter::ZynAPhaserLFOLRDelay:
	case FX::Parameter::ZynAPhaserDepth:
	case FX::Parameter::ZynAPhaserFeedback:
	case FX::Parameter::ZynAPhaserStages:
	case FX::Parameter::ZynAPhaserLRCross:
	case FX::Parameter::ZynAPhaserSubtractive:
	case FX::Parameter::ZynAPhaserWidth:
	case FX::Parameter::ZynAPhaserDistortion:
	case FX::Parameter::ZynAPhaserMismatch:
	case FX::Parameter::ZynAPhaserHyper:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_aphaser.changepar(Parameter - FX::Parameter::ZynAPhaserMix, nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynAPhaserBypass:
		fx_chain[nFX]->zyn_aphaser.bypass = nValue;
		break;

	case FX::Parameter::ZynPhaserPreset:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_phaser.loadpreset(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynPhaserMix:
	case FX::Parameter::ZynPhaserPanning:
	case FX::Parameter::ZynPhaserLFOFreq:
	case FX::Parameter::ZynPhaserLFORandomness:
	case FX::Parameter::ZynPhaserLFOType:
	case FX::Parameter::ZynPhaserLFOLRDelay:
	case FX::Parameter::ZynPhaserDepth:
	case FX::Parameter::ZynPhaserFeedback:
	case FX::Parameter::ZynPhaserStages:
	case FX::Parameter::ZynPhaserLRCross:
	case FX::Parameter::ZynPhaserSubtractive:
	case FX::Parameter::ZynPhaserPhase:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->zyn_phaser.changepar(Parameter - FX::Parameter::ZynPhaserMix, nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::ZynPhaserBypass:
		fx_chain[nFX]->zyn_phaser.bypass = nValue;
		break;

	case FX::Parameter::DreamDelayMix:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->dream_delay.setMix(nValue / 100.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayMode:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->dream_delay.setMode((AudioEffectDreamDelay::Mode)nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayTime:
		SetFXParameter(FX::Parameter::DreamDelayTimeL, nValue, nFX);
		SetFXParameter(FX::Parameter::DreamDelayTimeR, nValue, nFX);
		break;

	case FX::Parameter::DreamDelayTimeL:
		m_FXSpinLock.Acquire();

		if (nValue <= 100)
		{
			fx_chain[nFX]->dream_delay.setTimeL(nValue / 100.f);
			fx_chain[nFX]->dream_delay.setTimeLSync(AudioEffectDreamDelay::SYNC_NONE);
		}
		else
		{
			fx_chain[nFX]->dream_delay.setTimeLSync((AudioEffectDreamDelay::Sync)(nValue - 100));
		}

		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayTimeR:
		m_FXSpinLock.Acquire();

		if (nValue <= 100)
		{
			fx_chain[nFX]->dream_delay.setTimeR(nValue / 100.f);
			fx_chain[nFX]->dream_delay.setTimeRSync(AudioEffectDreamDelay::SYNC_NONE);
		}
		else
		{
			fx_chain[nFX]->dream_delay.setTimeRSync((AudioEffectDreamDelay::Sync)(nValue - 100));
		}

		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayTempo:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->dream_delay.setTempo(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayFeedback:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->dream_delay.setFeedback(nValue / 100.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayHighCut:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->dream_delay.setHighCut(MIDI_EQ_HZ[nValue]);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::DreamDelayBypass:
		fx_chain[nFX]->dream_delay.bypass = nValue;
		break;

	case FX::Parameter::PlateReverbMix:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->plate_reverb.set_mix(nValue / 100.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::PlateReverbSize:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->plate_reverb.size(nValue / 99.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::PlateReverbHighDamp:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->plate_reverb.hidamp(nValue / 99.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::PlateReverbLowDamp:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->plate_reverb.lodamp(nValue / 99.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::PlateReverbLowPass:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->plate_reverb.lowpass(nValue / 99.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::PlateReverbDiffusion:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->plate_reverb.diffusion(nValue / 99.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::PlateReverbBypass:
		fx_chain[nFX]->plate_reverb.bypass = nValue;
		break;

	case FX::Parameter::CloudSeed2Preset:
		fx_chain[nFX]->cloudseed2.loadPreset(nValue);
		break;

	case FX::Parameter::CloudSeed2Interpolation:
	case FX::Parameter::CloudSeed2LowCutEnabled:
	case FX::Parameter::CloudSeed2HighCutEnabled:
	case FX::Parameter::CloudSeed2InputMix:
	case FX::Parameter::CloudSeed2LowCut:
	case FX::Parameter::CloudSeed2HighCut:
	case FX::Parameter::CloudSeed2DryOut:
	case FX::Parameter::CloudSeed2EarlyOut:
	case FX::Parameter::CloudSeed2LateOut:
	case FX::Parameter::CloudSeed2TapEnabled:
	case FX::Parameter::CloudSeed2TapCount:
	case FX::Parameter::CloudSeed2TapDecay:
	case FX::Parameter::CloudSeed2TapPredelay:
	case FX::Parameter::CloudSeed2TapLength:
	case FX::Parameter::CloudSeed2EarlyDiffuseEnabled:
	case FX::Parameter::CloudSeed2EarlyDiffuseCount:
	case FX::Parameter::CloudSeed2EarlyDiffuseDelay:
	case FX::Parameter::CloudSeed2EarlyDiffuseModAmount:
	case FX::Parameter::CloudSeed2EarlyDiffuseFeedback:
	case FX::Parameter::CloudSeed2EarlyDiffuseModRate:
	case FX::Parameter::CloudSeed2LateMode:
	case FX::Parameter::CloudSeed2LateLineCount:
	case FX::Parameter::CloudSeed2LateDiffuseEnabled:
	case FX::Parameter::CloudSeed2LateDiffuseCount:
	case FX::Parameter::CloudSeed2LateLineSize:
	case FX::Parameter::CloudSeed2LateLineModAmount:
	case FX::Parameter::CloudSeed2LateDiffuseDelay:
	case FX::Parameter::CloudSeed2LateDiffuseModAmount:
	case FX::Parameter::CloudSeed2LateLineDecay:
	case FX::Parameter::CloudSeed2LateLineModRate:
	case FX::Parameter::CloudSeed2LateDiffuseFeedback:
	case FX::Parameter::CloudSeed2LateDiffuseModRate:
	case FX::Parameter::CloudSeed2EqLowShelfEnabled:
	case FX::Parameter::CloudSeed2EqHighShelfEnabled:
	case FX::Parameter::CloudSeed2EqLowpassEnabled:
	case FX::Parameter::CloudSeed2EqLowFreq:
	case FX::Parameter::CloudSeed2EqHighFreq:
	case FX::Parameter::CloudSeed2EqCutoff:
	case FX::Parameter::CloudSeed2EqLowGain:
	case FX::Parameter::CloudSeed2EqHighGain:
	case FX::Parameter::CloudSeed2EqCrossSeed:
	case FX::Parameter::CloudSeed2SeedTap:
	case FX::Parameter::CloudSeed2SeedDiffusion:
	case FX::Parameter::CloudSeed2SeedDelay:
	case FX::Parameter::CloudSeed2SeedPostDiffusion:
		fx_chain[nFX]->cloudseed2.setParameter(Parameter - FX::Parameter::CloudSeed2Interpolation, mapfloat(nValue, p.Minimum, p.Maximum, 0.0f, 1.0f));
		break;

	case FX::Parameter::CloudSeed2Bypass:
		fx_chain[nFX]->cloudseed2.bypass = nValue;
		break;

	case FX::Parameter::CompressorPreGain:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.setPreGain_dB(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorThresh:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.setThresh_dBFS(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorRatio:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.setCompressionRatio(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorAttack:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.setAttack_sec((nValue ?: 1) / 1000.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorRelease:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.setRelease_sec((nValue ?: 1) / 1000.0f);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorMakeupGain:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.setMakeupGain_dB(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorHPFilterEnable:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->compressor.enableHPFilter(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::CompressorBypass:
		fx_chain[nFX]->compressor.bypass = nValue;
		break;

	case FX::Parameter::EQLow:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->eq.setLow_dB(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQMid:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->eq.setMid_dB(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQHigh:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->eq.setHigh_dB(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQGain:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->eq.setGain_dB(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQLowMidFreq:
		m_FXSpinLock.Acquire();
		m_nFXParameter[nFX][Parameter] = fx_chain[nFX]->eq.setLowMidFreq_n(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQMidHighFreq:
		m_FXSpinLock.Acquire();
		m_nFXParameter[nFX][Parameter] = fx_chain[nFX]->eq.setMidHighFreq_n(nValue);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQPreLowCut:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->eq.setPreLowCut(MIDI_EQ_HZ[nValue]);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQPreHighCut:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->eq.setPreHighCut(MIDI_EQ_HZ[nValue]);
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::EQBypass:
		fx_chain[nFX]->eq.bypass = nValue;
		break;

	case FX::Parameter::ReturnLevel:
		m_FXSpinLock.Acquire();
		fx_chain[nFX]->set_level(powf(nValue / 99.0f, 2));
		m_FXSpinLock.Release();
		break;

	case FX::Parameter::Bypass:
		fx_chain[nFX]->bypass = nValue;
		break;

	default:
		assert(0);
		break;
	}
}

int CMiniDexed::GetFXParameter(FX::Parameter Parameter, int nFX)
{
	assert(nFX < CConfig::FXChains);
	assert(Parameter < FX::Parameter::Unknown);

	if (Parameter >= FX::Parameter::ZynDistortionMix && Parameter <= FX::Parameter::ZynDistortionOffset)
	{
		return fx_chain[nFX]->zyn_distortion.getpar(Parameter - FX::Parameter::ZynDistortionMix);
	}

	if (Parameter >= FX::Parameter::ZynChorusMix && Parameter <= FX::Parameter::ZynChorusSubtractive)
	{
		return fx_chain[nFX]->zyn_chorus.getpar(Parameter - FX::Parameter::ZynChorusMix);
	}

	if (Parameter >= FX::Parameter::ZynSympatheticMix && Parameter <= FX::Parameter::ZynSympatheticNegate)
	{
		return fx_chain[nFX]->zyn_sympathetic.getpar(Parameter - FX::Parameter::ZynSympatheticMix);
	}

	if (Parameter >= FX::Parameter::ZynAPhaserMix && Parameter <= FX::Parameter::ZynAPhaserHyper)
	{
		return fx_chain[nFX]->zyn_aphaser.getpar(Parameter - FX::Parameter::ZynAPhaserMix);
	}

	if (Parameter >= FX::Parameter::ZynPhaserMix && Parameter <= FX::Parameter::ZynPhaserPhase)
	{
		return fx_chain[nFX]->zyn_phaser.getpar(Parameter - FX::Parameter::ZynPhaserMix);
	}

	if (Parameter >= FX::Parameter::CloudSeed2Interpolation && Parameter <= FX::Parameter::CloudSeed2SeedPostDiffusion)
	{
		const FX::ParameterType &p = FX::s_Parameter[Parameter];
		return mapfloat(fx_chain[nFX]->cloudseed2.getParameter(Parameter - FX::Parameter::CloudSeed2Interpolation), 0.0f, 1.0f, p.Minimum, p.Maximum);
	}

	return m_nFXParameter[nFX][Parameter];
}

void CMiniDexed::SetBusParameter(Bus::Parameter Parameter, int nValue, int nBus)
{
	assert(nBus < CConfig::Buses);
	assert(Parameter < Bus::Parameter::Unknown);

	const Bus::ParameterType &p = Bus::s_Parameter[Parameter];
	nValue = constrain(nValue, p.Minimum, p.Maximum);

	m_nBusParameter[nBus][Parameter] = nValue;

	switch (Parameter)
	{
	case Bus::Parameter::MixerDryLevel:
		bus_mixer[nBus]->gain(mapfloat(nValue, 0, 99, 0.0f, 1.0f));
		break;
	case Bus::Parameter::ReturnLevel:
		m_fBusGain[nBus] = powf(mapfloat(nValue, 0, 99, 0.0f, 1.0f), 2);
		break;
	case Bus::Parameter::FXBypass:
		break;
	default:
		assert(0);
		break;
	}
}

int CMiniDexed::GetBusParameter(Bus::Parameter Parameter, int nBus)
{
	assert(nBus < CConfig::Buses);
	assert(Parameter < Bus::Parameter::Unknown);

	return m_nBusParameter[nBus][Parameter];
}

void CMiniDexed::SetTGParameter(TTGParameter Parameter, int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);

	if (nTG >= m_nToneGenerators) return; // Not an active TG

	int nTGLink = m_nTGLink[nTG];

	for (int i = 0; i < m_nToneGenerators; i++)
	{
		if (i != nTG && (!nTGLink || m_nTGLink[i] != nTGLink || i / 8 != nTG / 8))
			continue;

		if (i != nTG && Parameter == TGParameterTGLink)
			continue;

		if (i != nTG && Parameter == TGParameterPan)
			continue;

		if (i != nTG && Parameter == TGParameterMasterTune)
			continue;

		switch (Parameter)
		{
		case TGParameterVoiceBank:
			BankSelect(nValue, i);
			break;
		case TGParameterVoiceBankMSB:
			BankSelectMSB(nValue, i);
			break;
		case TGParameterVoiceBankLSB:
			BankSelectLSB(nValue, i);
			break;
		case TGParameterProgram:
			ProgramChange(nValue, i);
			break;
		case TGParameterVolume:
			SetVolume(nValue, i);
			break;
		case TGParameterPan:
			SetPan(nValue, i);
			break;
		case TGParameterMasterTune:
			SetMasterTune(nValue, i);
			break;
		case TGParameterCutoff:
			SetCutoff(nValue, i);
			break;
		case TGParameterResonance:
			SetResonance(nValue, i);
			break;
		case TGParameterPitchBendRange:
			setPitchbendRange(nValue, i);
			break;
		case TGParameterPitchBendStep:
			setPitchbendStep(nValue, i);
			break;
		case TGParameterPortamentoMode:
			setPortamentoMode(nValue, i);
			break;
		case TGParameterPortamentoGlissando:
			setPortamentoGlissando(nValue, i);
			break;
		case TGParameterPortamentoTime:
			setPortamentoTime(nValue, i);
			break;
		case TGParameterNoteLimitLow:
			setNoteLimitLow(nValue, i);
			break;
		case TGParameterNoteLimitHigh:
			setNoteLimitHigh(nValue, i);
			break;
		case TGParameterNoteShift:
			setNoteShift(nValue, i);
			break;
		case TGParameterMonoMode:
			setMonoMode(nValue, i);
			break;
		case TGParameterTGLink:
			setTGLink(nValue, i);
			break;

		case TGParameterMWRange:
			setModController(0, 0, nValue, i);
			break;
		case TGParameterMWPitch:
			setModController(0, 1, nValue, i);
			break;
		case TGParameterMWAmplitude:
			setModController(0, 2, nValue, i);
			break;
		case TGParameterMWEGBias:
			setModController(0, 3, nValue, i);
			break;

		case TGParameterFCRange:
			setModController(1, 0, nValue, i);
			break;
		case TGParameterFCPitch:
			setModController(1, 1, nValue, i);
			break;
		case TGParameterFCAmplitude:
			setModController(1, 2, nValue, i);
			break;
		case TGParameterFCEGBias:
			setModController(1, 3, nValue, i);
			break;

		case TGParameterBCRange:
			setModController(2, 0, nValue, i);
			break;
		case TGParameterBCPitch:
			setModController(2, 1, nValue, i);
			break;
		case TGParameterBCAmplitude:
			setModController(2, 2, nValue, i);
			break;
		case TGParameterBCEGBias:
			setModController(2, 3, nValue, i);
			break;

		case TGParameterATRange:
			setModController(3, 0, nValue, i);
			break;
		case TGParameterATPitch:
			setModController(3, 1, nValue, i);
			break;
		case TGParameterATAmplitude:
			setModController(3, 2, nValue, i);
			break;
		case TGParameterATEGBias:
			setModController(3, 3, nValue, i);
			break;

		case TGParameterMIDIChannel:
			SetMIDIChannel((uint8_t)nValue, i);
			break;

		case TGParameterSysExChannel:
			SetSysExChannel(nValue, i);
			break;
		case TGParameterSysExEnable:
			SetSysExEnable(nValue, i);
			break;

		case TGParameterMIDIRxSustain:
			SetMIDIRxSustain(nValue, i);
			break;
		case TGParameterMIDIRxPortamento:
			SetMIDIRxPortamento(nValue, i);
			break;
		case TGParameterMIDIRxSostenuto:
			SetMIDIRxSostenuto(nValue, i);
			break;
		case TGParameterMIDIRxHold2:
			SetMIDIRxHold2(nValue, i);
			break;

		case TGParameterFX1Send:
			SetFX1Send(nValue, i);
			break;
		case TGParameterFX2Send:
			SetFX2Send(nValue, i);
			break;

		case TGParameterCompressorEnable:
			SetCompressorEnable(nValue, i);
			break;
		case TGParameterCompressorPreGain:
			SetCompressorPreGain(nValue, i);
			break;
		case TGParameterCompressorThresh:
			SetCompressorThresh(nValue, i);
			break;
		case TGParameterCompressorRatio:
			SetCompressorRatio(nValue, i);
			break;
		case TGParameterCompressorAttack:
			SetCompressorAttack(nValue, i);
			break;
		case TGParameterCompressorRelease:
			SetCompressorRelease(nValue, i);
			break;
		case TGParameterCompressorMakeupGain:
			SetCompressorMakeupGain(nValue, i);
			break;

		case TGParameterEQLow:
			SetEQLow(nValue, i);
			break;
		case TGParameterEQMid:
			SetEQMid(nValue, i);
			break;
		case TGParameterEQHigh:
			SetEQHigh(nValue, i);
			break;
		case TGParameterEQGain:
			SetEQGain(nValue, i);
			break;
		case TGParameterEQLowMidFreq:
			SetEQLowMidFreq(nValue, i);
			break;
		case TGParameterEQMidHighFreq:
			SetEQMidHighFreq(nValue, i);
			break;
		case TGParameterEQPreLowcut:
			SetEQPreLowcut(nValue, i);
			break;
		case TGParameterEQPreHighcut:
			SetEQPreHighcut(nValue, i);
			break;

		default:
			assert(0);
			break;
		}
	}
}

int CMiniDexed::GetTGParameter(TTGParameter Parameter, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);

	switch (Parameter)
	{
	case TGParameterVoiceBank:
		return m_nVoiceBankID[nTG];
	case TGParameterVoiceBankMSB:
		return m_nVoiceBankID[nTG] >> 7;
	case TGParameterVoiceBankLSB:
		return m_nVoiceBankID[nTG] & 0x7F;
	case TGParameterProgram:
		return m_nProgram[nTG];
	case TGParameterVolume:
		return m_nVolume[nTG];
	case TGParameterPan:
		return m_nPan[nTG];
	case TGParameterMasterTune:
		return m_nMasterTune[nTG];
	case TGParameterCutoff:
		return m_nCutoff[nTG];
	case TGParameterResonance:
		return m_nResonance[nTG];
	case TGParameterMIDIChannel:
		return m_nMIDIChannel[nTG];
	case TGParameterSysExChannel:
		return m_nSysExChannel[nTG];
	case TGParameterSysExEnable:
		return m_bSysExEnable[nTG];
	case TGParameterMIDIRxSustain:
		return m_bMIDIRxSustain[nTG];
	case TGParameterMIDIRxPortamento:
		return m_bMIDIRxPortamento[nTG];
	case TGParameterMIDIRxSostenuto:
		return m_bMIDIRxSostenuto[nTG];
	case TGParameterMIDIRxHold2:
		return m_bMIDIRxHold2[nTG];
	case TGParameterFX1Send:
		return m_nFX1Send[nTG];
	case TGParameterFX2Send:
		return m_nFX2Send[nTG];
	case TGParameterPitchBendRange:
		return m_nPitchBendRange[nTG];
	case TGParameterPitchBendStep:
		return m_nPitchBendStep[nTG];
	case TGParameterPortamentoMode:
		return m_nPortamentoMode[nTG];
	case TGParameterPortamentoGlissando:
		return m_nPortamentoGlissando[nTG];
	case TGParameterPortamentoTime:
		return m_nPortamentoTime[nTG];
	case TGParameterNoteLimitLow:
		return m_nNoteLimitLow[nTG];
	case TGParameterNoteLimitHigh:
		return m_nNoteLimitHigh[nTG];
	case TGParameterNoteShift:
		return m_nNoteShift[nTG];
	case TGParameterMonoMode:
		return m_bMonoMode[nTG] ? 1 : 0;

	case TGParameterTGLink:
		return m_nTGLink[nTG];

	case TGParameterMWRange:
		return getModController(0, 0, nTG);
	case TGParameterMWPitch:
		return getModController(0, 1, nTG);
	case TGParameterMWAmplitude:
		return getModController(0, 2, nTG);
	case TGParameterMWEGBias:
		return getModController(0, 3, nTG);

	case TGParameterFCRange:
		return getModController(1, 0, nTG);
	case TGParameterFCPitch:
		return getModController(1, 1, nTG);
	case TGParameterFCAmplitude:
		return getModController(1, 2, nTG);
	case TGParameterFCEGBias:
		return getModController(1, 3, nTG);

	case TGParameterBCRange:
		return getModController(2, 0, nTG);
	case TGParameterBCPitch:
		return getModController(2, 1, nTG);
	case TGParameterBCAmplitude:
		return getModController(2, 2, nTG);
	case TGParameterBCEGBias:
		return getModController(2, 3, nTG);

	case TGParameterATRange:
		return getModController(3, 0, nTG);
	case TGParameterATPitch:
		return getModController(3, 1, nTG);
	case TGParameterATAmplitude:
		return getModController(3, 2, nTG);
	case TGParameterATEGBias:
		return getModController(3, 3, nTG);

	case TGParameterCompressorEnable:
		return m_bCompressorEnable[nTG];
	case TGParameterCompressorPreGain:
		return m_nCompressorPreGain[nTG];
	case TGParameterCompressorThresh:
		return m_nCompressorThresh[nTG];
	case TGParameterCompressorRatio:
		return m_nCompressorRatio[nTG];
	case TGParameterCompressorAttack:
		return m_nCompressorAttack[nTG];
	case TGParameterCompressorRelease:
		return m_nCompressorRelease[nTG];
	case TGParameterCompressorMakeupGain:
		return m_nCompressorMakeupGain[nTG];

	case TGParameterEQLow:
		return m_nEQLow[nTG];
		break;
	case TGParameterEQMid:
		return m_nEQMid[nTG];
		break;
	case TGParameterEQHigh:
		return m_nEQHigh[nTG];
		break;
	case TGParameterEQGain:
		return m_nEQGain[nTG];
		break;
	case TGParameterEQLowMidFreq:
		return m_nEQLowMidFreq[nTG];
		break;
	case TGParameterEQMidHighFreq:
		return m_nEQMidHighFreq[nTG];
		break;
	case TGParameterEQPreLowcut:
		return m_nEQPreLowcut[nTG];
		break;
	case TGParameterEQPreHighcut:
		return m_nEQPreHighcut[nTG];
		break;

	default:
		assert(0);
		return 0;
	}
}

void CMiniDexed::SetVoiceParameter(int nOffset, int nValue, int nOP, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	assert(nOP <= 6);

	if (nOP < 6)
	{
		nOP = 5 - nOP; // OPs are in reverse order
	}

	int nTGLink = m_nTGLink[nTG];

	for (int i = 0; i < m_nToneGenerators; i++)
	{
		if (i != nTG && (!nTGLink || m_nTGLink[i] != nTGLink || i / 8 != nTG / 8))
			continue;

		if (nOP < 6)
		{
			if (nOffset == DEXED_OP_ENABLE)
			{
				if (nValue)
				{
					setOPMask(m_uchOPMask[i] | static_cast<uint8_t>(1 << nOP), i);
				}
				else
				{
					setOPMask(m_uchOPMask[i] & ~(1 << nOP), i);
				}

				continue;
			}
		}

		int offset = nOffset + nOP * 21;
		assert(offset < 156);

		m_pTG[i]->setVoiceDataElement(static_cast<uint8_t>(offset), static_cast<uint8_t>(nValue));
	}
}

int CMiniDexed::GetVoiceParameter(int nOffset, int nOP, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return 0; // Not an active TG

	assert(m_pTG[nTG]);
	assert(nOP <= 6);

	if (nOP < 6)
	{
		nOP = 5 - nOP; // OPs are in reverse order

		if (nOffset == DEXED_OP_ENABLE)
		{
			return !!(m_uchOPMask[nTG] & (1 << nOP));
		}
	}

	nOffset += nOP * 21;
	assert(nOffset < 156);

	return m_pTG[nTG]->getVoiceDataElement(static_cast<uint8_t>(nOffset));
}

std::string CMiniDexed::GetVoiceName(int nTG)
{
	char VoiceName[11];
	memset(VoiceName, 0, sizeof VoiceName);
	VoiceName[0] = 32; // space
	assert(nTG < CConfig::AllToneGenerators);

	if (nTG < m_nToneGenerators)
	{
		assert(m_pTG[nTG]);
		m_pTG[nTG]->getName(VoiceName);
	}
	std::string Result(VoiceName);
	return Result;
}

#ifndef ARM_ALLOW_MULTI_CORE

void CMiniDexed::ProcessSound()
{
	assert(m_pSoundDevice);

	int nFrames = m_nQueueSizeFrames - static_cast<int>(m_pSoundDevice->GetQueueFramesAvail());
	if (nFrames >= m_nQueueSizeFrames / 2)
	{
		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Start();
		}

		float32_t SampleBuffer[nFrames];
		m_pTG[0]->getSamples(SampleBuffer, nFrames);

		// Convert single float array (mono) to int16 array
		int32_t tmp_int[nFrames];
		arm_float_to_q23(SampleBuffer, tmp_int, nFrames);

		if (m_pSoundDevice->Write(tmp_int, sizeof(tmp_int)) != ssizeof(tmp_int))
		{
			LOGERR("Sound data dropped");
		}

		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Stop();
		}
	}
}

#else // #ifdef ARM_ALLOW_MULTI_CORE

void CMiniDexed::ProcessSound()
{
	assert(m_pSoundDevice);
	assert(m_pConfig);

	int nFrames = m_nQueueSizeFrames - static_cast<int>(m_pSoundDevice->GetQueueFramesAvail());
	if (nFrames >= m_nQueueSizeFrames / 2)
	{
		// only process the minimum number of frames (== chunksize / 2)
		// as the tg_mixer cannot process more
		nFrames = m_nQueueSizeFrames / 2;

		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Start();
		}

		m_nFramesToProcess = nFrames;

		// kick secondary cores
		for (unsigned nCore = 2; nCore < CORES; nCore++)
		{
			assert(m_CoreStatus[nCore] == CoreStatusIdle);
			m_CoreStatus[nCore] = CoreStatusBusy;
			SendIPI(nCore, IPI_USER);
		}

		// process the TGs assigned to core 1
		assert(nFrames <= CConfig::MaxChunkSize);
		for (int i = 0; i < m_pConfig->GetTGsCore1(); i++)
		{
			assert(m_pTG[i]);
			m_pTG[i]->getSamples(m_OutputLevel[i], nFrames);
		}

		// wait for cores 2 and 3 to complete their work
		for (int nCore = 2; nCore < CORES; nCore++)
		{
			while (m_CoreStatus[nCore] != CoreStatusIdle)
			{
				WaitForEvent();
			}
		}

		//
		// Audio signal path after tone generators starts here
		//

		if (m_bQuadDAC8Chan)
		{
			// This is only supported when there are 8 TGs
			assert(m_nToneGenerators == 8);

			// No mixing is performed by MiniDexed, sound is output in 8 channels.
			// Note: one TG per audio channel; output=mono; no processing.
			const int Channels = 8; // One TG per channel
			float tmp_float[nFrames * Channels];
			int32_t tmp_int[nFrames * Channels];

			// Convert dual float array (8 chan) to single int16 array (8 chan)
			for (int i = 0; i < nFrames; i++)
			{
				// TGs will alternate on L/R channels for each output
				// reading directly from the TG OutputLevel buffer with
				// no additional processing.
				for (int tg = 0; tg < Channels; tg++)
				{
					tmp_float[(i * Channels) + tg] = m_OutputLevel[tg][i] * m_fMasterVolumeW;
				}
			}

			arm_float_to_q23(tmp_float, tmp_int, nFrames * Channels);

			// Prevent PCM510x analog mute from kicking in
			for (int tg = 0; tg < Channels; tg++)
			{
				if (tmp_int[(nFrames - 1) * Channels + tg] == 0)
				{
					tmp_int[(nFrames - 1) * Channels + tg]++;
				}
			}

			if (m_pSoundDevice->Write(tmp_int, sizeof(tmp_int)) != ssizeof(tmp_int))
			{
				LOGERR("Sound data dropped");
			}
		}
		else
		{
			// Mix everything down to stereo
			int indexL = 0, indexR = 1;

			// BEGIN TG mixing
			float tmp_float[nFrames * 2];
			int32_t tmp_int[nFrames * 2];

			// get the mix buffer of all TGs
			float *MasterBuffer[2];
			bus_mixer[0]->getBuffers(MasterBuffer);

			for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
			{
				if (m_nToneGenerators <= nBus * 8)
					continue;

				float *BusBuffer[2];
				bus_mixer[nBus]->getBuffers(BusBuffer);
				bus_mixer[nBus]->zeroFill();

				if (m_fBusGain[nBus] == 0.0f)
					continue;

				for (int i = nBus * 8; i < m_nToneGenerators; i++)
				{
					if (i < (nBus + 1) * 8)
					{
						bus_mixer[nBus]->doAddMix(i, m_OutputLevel[i]);
					}
				}

				// BEGIN adding sendFX
				float *FXSendBuffer[2];

				for (int idFX = 0; idFX < CConfig::BusFXChains; ++idFX)
				{
					int nFX = idFX + CConfig::BusFXChains * nBus;

					if (fx_chain[nFX]->get_level() == 0.0f) continue;

					sendfx_mixer[nFX]->getBuffers(FXSendBuffer);
					sendfx_mixer[nFX]->zeroFill();

					for (int i = nBus * 8; i < m_nToneGenerators; i++)
					{
						if (i < (nBus + 1) * 8)
						{
							sendfx_mixer[nFX]->doAddMix(i, m_OutputLevel[i]);
						}
					}

					if (!m_nBusParameter[nBus][Bus::Parameter::FXBypass])
					{
						m_FXSpinLock.Acquire();
						fx_chain[nFX]->process(FXSendBuffer[0], FXSendBuffer[1], nFrames);
						m_FXSpinLock.Release();
					}

					arm_add_f32(BusBuffer[0], FXSendBuffer[0], BusBuffer[0], static_cast<uint32_t>(nFrames));
					arm_add_f32(BusBuffer[1], FXSendBuffer[1], BusBuffer[1], static_cast<uint32_t>(nFrames));
				}
				// END adding sendFX

				if (m_fBusGain[nBus] != 1.0f)
				{
					arm_scale_f32(BusBuffer[0], m_fBusGain[nBus], BusBuffer[0], static_cast<uint32_t>(nFrames));
					arm_scale_f32(BusBuffer[1], m_fBusGain[nBus], BusBuffer[1], static_cast<uint32_t>(nFrames));
				}

				if (nBus != 0)
				{
					arm_add_f32(MasterBuffer[0], BusBuffer[0], MasterBuffer[0], static_cast<uint32_t>(nFrames));
					arm_add_f32(MasterBuffer[1], BusBuffer[1], MasterBuffer[1], static_cast<uint32_t>(nFrames));
				}
			}

			m_FXSpinLock.Acquire();
			fx_chain[CConfig::MasterFX]->process(MasterBuffer[0], MasterBuffer[1], nFrames);
			m_FXSpinLock.Release();

			// swap stereo channels if needed prior to writing back out
			if (m_bChannelsSwapped)
			{
				indexL = 1;
				indexR = 0;
			}

			// Convert dual float array (left, right) to single int32 (q23) array (left/right)
			if (m_bVolRampDownWait)
			{
				float targetVol = 0.0f;

				scale_ramp_f32(MasterBuffer[0], &m_fMasterVolume[0], targetVol, m_fRamp, MasterBuffer[0], nFrames);
				scale_ramp_f32(MasterBuffer[1], &m_fMasterVolume[1], targetVol, m_fRamp, MasterBuffer[1], nFrames);
				arm_zip_f32(MasterBuffer[indexL], MasterBuffer[indexR], tmp_float, nFrames);

				if (targetVol == m_fMasterVolume[0] && targetVol == m_fMasterVolume[1])
				{
					m_bVolRampDownWait = false;
					m_bVolRampedDown = true;
				}
			}
			else if (m_bVolRampedDown)
			{
				arm_scale_zip_f32(MasterBuffer[indexL], MasterBuffer[indexR], 0.0f, tmp_float, nFrames);
			}
			else if (m_fMasterVolume[0] == m_fMasterVolumeW && m_fMasterVolume[1] == m_fMasterVolumeW)
			{
				arm_scale_zip_f32(MasterBuffer[indexL], MasterBuffer[indexR], m_fMasterVolumeW, tmp_float, nFrames);
			}
			else
			{
				scale_ramp_f32(MasterBuffer[0], &m_fMasterVolume[0], m_fMasterVolumeW, m_fRamp, MasterBuffer[0], nFrames);
				scale_ramp_f32(MasterBuffer[1], &m_fMasterVolume[1], m_fMasterVolumeW, m_fRamp, MasterBuffer[1], nFrames);
				arm_zip_f32(MasterBuffer[indexL], MasterBuffer[indexR], tmp_float, nFrames);
			}

			arm_float_to_q23(tmp_float, tmp_int, nFrames * 2);

			// Prevent PCM510x analog mute from kicking in
			if (tmp_int[nFrames * 2 - 1] == 0)
			{
				tmp_int[nFrames * 2 - 1]++;
			}

			if (m_pSoundDevice->Write(tmp_int, sizeof(tmp_int)) != ssizeof(tmp_int))
			{
				LOGERR("Sound data dropped");
			}
		} // End of Stereo mixing

		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Stop();
		}
	}
}

#endif

int CMiniDexed::GetPerformanceSelectChannel()
{
	// Stores and returns Select Channel using MIDI Device Channel definitions
	return GetParameter(ParameterPerformanceSelectChannel);
}

void CMiniDexed::SetPerformanceSelectChannel(int nCh)
{
	// Turns a configuration setting to MIDI Device Channel definitions
	// Mirrors the logic in Performance Config for handling MIDI channel configuration
	if (nCh == 0)
	{
		SetParameter(ParameterPerformanceSelectChannel, CMIDIDevice::Disabled);
	}
	else if (nCh < CMIDIDevice::Channels)
	{
		SetParameter(ParameterPerformanceSelectChannel, nCh - 1);
	}
	else
	{
		SetParameter(ParameterPerformanceSelectChannel, CMIDIDevice::OmniMode);
	}
}

bool CMiniDexed::SavePerformance(bool bSaveAsDeault)
{
	if (m_PerformanceConfig.GetInternalFolderOk())
	{
		m_bSavePerformance = true;
		m_bSaveAsDeault = bSaveAsDeault;

		return true;
	}
	else
	{
		return false;
	}
}

bool CMiniDexed::DoSavePerformance()
{
	for (int nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
	{
		m_PerformanceConfig.SetBankNumber(m_nVoiceBankID[nTG], nTG);
		m_PerformanceConfig.SetVoiceNumber(m_nProgram[nTG], nTG);
		m_PerformanceConfig.SetMIDIChannel(m_nMIDIChannel[nTG], nTG);
		m_PerformanceConfig.SetSysExChannel(m_nSysExChannel[nTG], nTG);
		m_PerformanceConfig.SetSysExEnable(m_bSysExEnable[nTG], nTG);
		m_PerformanceConfig.SetMIDIRxSustain(m_bMIDIRxSustain[nTG], nTG);
		m_PerformanceConfig.SetMIDIRxPortamento(m_bMIDIRxPortamento[nTG], nTG);
		m_PerformanceConfig.SetMIDIRxSostenuto(m_bMIDIRxSostenuto[nTG], nTG);
		m_PerformanceConfig.SetMIDIRxHold2(m_bMIDIRxHold2[nTG], nTG);
		m_PerformanceConfig.SetVolume(m_nVolume[nTG], nTG);
		m_PerformanceConfig.SetPan(m_nPan[nTG], nTG);
		m_PerformanceConfig.SetDetune(m_nMasterTune[nTG], nTG);
		m_PerformanceConfig.SetCutoff(m_nCutoff[nTG], nTG);
		m_PerformanceConfig.SetResonance(m_nResonance[nTG], nTG);
		m_PerformanceConfig.SetPitchBendRange(m_nPitchBendRange[nTG], nTG);
		m_PerformanceConfig.SetPitchBendStep(m_nPitchBendStep[nTG], nTG);
		m_PerformanceConfig.SetPortamentoMode(m_nPortamentoMode[nTG], nTG);
		m_PerformanceConfig.SetPortamentoGlissando(m_nPortamentoGlissando[nTG], nTG);
		m_PerformanceConfig.SetPortamentoTime(m_nPortamentoTime[nTG], nTG);

		m_PerformanceConfig.SetNoteLimitLow(m_nNoteLimitLow[nTG], nTG);
		m_PerformanceConfig.SetNoteLimitHigh(m_nNoteLimitHigh[nTG], nTG);
		m_PerformanceConfig.SetNoteShift(m_nNoteShift[nTG], nTG);
		if (nTG < m_pConfig->GetToneGenerators())
		{
			m_pTG[nTG]->getVoiceData(m_nRawVoiceData);
		}
		else
		{
			// Not an active TG so provide default voice by asking for an invalid voice ID.
			m_SysExFileLoader.GetVoice(CSysExFileLoader::MaxVoiceBankID, CSysExFileLoader::VoicesPerBank + 1, m_nRawVoiceData);
		}
		m_PerformanceConfig.SetVoiceDataToTxt(m_nRawVoiceData, nTG);
		m_PerformanceConfig.SetMonoMode(m_bMonoMode[nTG], nTG);
		m_PerformanceConfig.SetTGLink(m_nTGLink[nTG], nTG);

		m_PerformanceConfig.SetModulationWheelRange(m_nModulationWheelRange[nTG], nTG);
		m_PerformanceConfig.SetModulationWheelTarget(m_nModulationWheelTarget[nTG], nTG);
		m_PerformanceConfig.SetFootControlRange(m_nFootControlRange[nTG], nTG);
		m_PerformanceConfig.SetFootControlTarget(m_nFootControlTarget[nTG], nTG);
		m_PerformanceConfig.SetBreathControlRange(m_nBreathControlRange[nTG], nTG);
		m_PerformanceConfig.SetBreathControlTarget(m_nBreathControlTarget[nTG], nTG);
		m_PerformanceConfig.SetAftertouchRange(m_nAftertouchRange[nTG], nTG);
		m_PerformanceConfig.SetAftertouchTarget(m_nAftertouchTarget[nTG], nTG);

		m_PerformanceConfig.SetFX1Send(m_nFX1Send[nTG], nTG);
		m_PerformanceConfig.SetFX2Send(m_nFX2Send[nTG], nTG);

		m_PerformanceConfig.SetCompressorEnable(m_bCompressorEnable[nTG], nTG);
		m_PerformanceConfig.SetCompressorPreGain(m_nCompressorPreGain[nTG], nTG);
		m_PerformanceConfig.SetCompressorThresh(m_nCompressorThresh[nTG], nTG);
		m_PerformanceConfig.SetCompressorRatio(m_nCompressorRatio[nTG], nTG);
		m_PerformanceConfig.SetCompressorAttack(m_nCompressorAttack[nTG], nTG);
		m_PerformanceConfig.SetCompressorRelease(m_nCompressorRelease[nTG], nTG);
		m_PerformanceConfig.SetCompressorMakeupGain(m_nCompressorMakeupGain[nTG], nTG);

		m_PerformanceConfig.SetEQLow(m_nEQLow[nTG], nTG);
		m_PerformanceConfig.SetEQMid(m_nEQMid[nTG], nTG);
		m_PerformanceConfig.SetEQHigh(m_nEQHigh[nTG], nTG);
		m_PerformanceConfig.SetEQGain(m_nEQGain[nTG], nTG);
		m_PerformanceConfig.SetEQLowMidFreq(m_nEQLowMidFreq[nTG], nTG);
		m_PerformanceConfig.SetEQMidHighFreq(m_nEQMidHighFreq[nTG], nTG);
		m_PerformanceConfig.SetEQPreLowcut(m_nEQPreLowcut[nTG], nTG);
		m_PerformanceConfig.SetEQPreHighcut(m_nEQPreHighcut[nTG], nTG);
	}

	for (int nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		for (int nParam = 0; nParam < FX::Parameter::Unknown; ++nParam)
		{
			FX::Parameter param = FX::Parameter(nParam);
			m_PerformanceConfig.SetFXParameter(param, GetFXParameter(param, nFX), nFX);
		}
	}

	for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
	{
		for (int nParam = 0; nParam < Bus::Parameter::Unknown; ++nParam)
		{
			Bus::Parameter param = Bus::Parameter(nParam);
			m_PerformanceConfig.SetBusParameter(param, GetBusParameter(param, nBus), nBus);
		}
	}

	if (m_bSaveAsDeault)
	{
		m_PerformanceConfig.SetNewPerformanceBank(0);
		m_PerformanceConfig.SetNewPerformance(0);
	}
	return m_PerformanceConfig.Save();
}

void CMiniDexed::SetCompressorEnable(bool compressor, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_bCompressorEnable[nTG] = compressor;
	m_pTG[nTG]->setCompressorEnable(compressor);
	m_UI.ParameterChanged();
}

void CMiniDexed::SetCompressorPreGain(int preGain, int nTG)
{
	preGain = constrain(preGain, -20, 20);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nCompressorPreGain[nTG] = preGain;
	m_pTG[nTG]->Compr.setPreGain_dB(preGain);
	m_UI.ParameterChanged();
}

void CMiniDexed::SetCompressorThresh(int thresh, int nTG)
{
	thresh = constrain(thresh, -60, 0);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nCompressorThresh[nTG] = thresh;
	m_pTG[nTG]->Compr.setThresh_dBFS(thresh);
	m_UI.ParameterChanged();
}

void CMiniDexed::SetCompressorRatio(int ratio, int nTG)
{
	ratio = constrain(ratio, 1, AudioEffectCompressor::CompressorRatioInf);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nCompressorRatio[nTG] = ratio;
	m_pTG[nTG]->Compr.setCompressionRatio(ratio == AudioEffectCompressor::CompressorRatioInf ? INFINITY : ratio);
	m_UI.ParameterChanged();
}

void CMiniDexed::SetCompressorAttack(int attack, int nTG)
{
	attack = constrain(attack, 0, 1000);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nCompressorAttack[nTG] = attack;
	m_pTG[nTG]->Compr.setAttack_sec((attack ?: 1) / 1000.0f, m_pConfig->GetSampleRate());
	m_UI.ParameterChanged();
}

void CMiniDexed::SetCompressorRelease(int release, int nTG)
{
	release = constrain(release, 0, 2000);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nCompressorRelease[nTG] = release;
	m_pTG[nTG]->Compr.setRelease_sec((release ?: 1) / 1000.0, m_pConfig->GetSampleRate());
	m_UI.ParameterChanged();
}

void CMiniDexed::SetCompressorMakeupGain(int makeupGain, int nTG)
{
	makeupGain = constrain(makeupGain, -20, 20);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nCompressorMakeupGain[nTG] = makeupGain;
	m_pTG[nTG]->Compr.setMakeupGain_dB(makeupGain);
	m_UI.ParameterChanged();
}

void CMiniDexed::SetEQLow(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQLow[nTG] = nValue;
	m_pTG[nTG]->EQ.setLow_dB(nValue);
}

void CMiniDexed::SetEQMid(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQMid[nTG] = nValue;
	m_pTG[nTG]->EQ.setMid_dB(nValue);
}

void CMiniDexed::SetEQHigh(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQHigh[nTG] = nValue;
	m_pTG[nTG]->EQ.setHigh_dB(nValue);
}

void CMiniDexed::SetEQGain(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQGain[nTG] = nValue;
	m_pTG[nTG]->EQ.setGain_dB(nValue);
}

void CMiniDexed::SetEQLowMidFreq(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, 0, 46);
	m_nEQLowMidFreq[nTG] = m_pTG[nTG]->EQ.setLowMidFreq_n(nValue);
}

void CMiniDexed::SetEQMidHighFreq(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, 28, 59);
	m_nEQMidHighFreq[nTG] = m_pTG[nTG]->EQ.setMidHighFreq_n(nValue);
}

void CMiniDexed::SetEQPreLowcut(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, 0, 60);
	m_nEQPreLowcut[nTG] = nValue;
	m_pTG[nTG]->EQ.setPreLowCut(MIDI_EQ_HZ[nValue]);
}

void CMiniDexed::SetEQPreHighcut(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	nValue = constrain(nValue, 0, 60);
	m_nEQPreHighcut[nTG] = nValue;
	m_pTG[nTG]->EQ.setPreHighCut(MIDI_EQ_HZ[nValue]);
}

void CMiniDexed::setMonoMode(bool mono, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_bMonoMode[nTG] = mono;
	m_pTG[nTG]->setMonoMode(mono);
	m_pTG[nTG]->doRefreshVoice();
	m_UI.ParameterChanged();
}

void CMiniDexed::setTGLink(int nTGLink, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nTGLink[nTG] = constrain(nTGLink, 0, 4);
	m_UI.ParameterChanged();
}

void CMiniDexed::setPitchbendRange(int range, int nTG)
{
	range = constrain(range, 0, 12);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nPitchBendRange[nTG] = range;

	m_pTG[nTG]->setPitchbendRange(static_cast<uint8_t>(range));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setPitchbendStep(int step, int nTG)
{
	step = constrain(step, 0, 12);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nPitchBendStep[nTG] = step;

	m_pTG[nTG]->setPitchbendStep(static_cast<uint8_t>(step));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setPortamentoMode(int mode, int nTG)
{
	mode = constrain(mode, 0, 1);

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nPortamentoMode[nTG] = mode;

	m_pTG[nTG]->setPortamentoMode(static_cast<uint8_t>(mode));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setPortamentoGlissando(int glissando, int nTG)
{
	glissando = constrain(glissando, 0, 1);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nPortamentoGlissando[nTG] = glissando;

	m_pTG[nTG]->setPortamentoGlissando(static_cast<uint8_t>(glissando));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setPortamentoTime(int time, int nTG)
{
	time = constrain(time, 0, 99);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	m_nPortamentoTime[nTG] = time;

	m_pTG[nTG]->setPortamentoTime(static_cast<uint8_t>(time));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setNoteLimitLow(int limit, int nTG)
{
	limit = constrain(limit, 0, 127);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nNoteLimitLow[nTG] = limit;

	// reset all notes, so they don't stay down
	m_pTG[nTG]->deactivate();
	m_UI.ParameterChanged();
}

void CMiniDexed::setNoteLimitHigh(int limit, int nTG)
{
	limit = constrain(limit, 0, 127);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nNoteLimitHigh[nTG] = limit;

	// reset all notes, so they don't stay down
	m_pTG[nTG]->deactivate();
	m_UI.ParameterChanged();
}

void CMiniDexed::setNoteShift(int shift, int nTG)
{
	shift = constrain(shift, -24, 24);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nNoteShift[nTG] = shift;

	// reset all notes, so they don't stay down
	m_pTG[nTG]->deactivate();
	m_UI.ParameterChanged();
}

void CMiniDexed::setModWheelRange(int range, int nTG)
{
	range = constrain(range, 0, 99);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nModulationWheelRange[nTG] = range;
	m_pTG[nTG]->setMWController(static_cast<uint8_t>(range), m_pTG[nTG]->getModWheelTarget(), 0);

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setModWheelTarget(int target, int nTG)
{
	target = constrain(target, 0, 7);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nModulationWheelTarget[nTG] = target;

	m_pTG[nTG]->setModWheelTarget(static_cast<uint8_t>(target));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setFootControllerRange(int range, int nTG)
{
	range = constrain(range, 0, 99);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nFootControlRange[nTG] = range;
	m_pTG[nTG]->setFCController(static_cast<uint8_t>(range), m_pTG[nTG]->getFootControllerTarget(), 0);

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setFootControllerTarget(int target, int nTG)
{
	target = constrain(target, 0, 7);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nFootControlTarget[nTG] = target;

	m_pTG[nTG]->setFootControllerTarget(static_cast<uint8_t>(target));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setBreathControllerRange(int range, int nTG)
{
	range = constrain(range, 0, 99);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nBreathControlRange[nTG] = range;
	m_pTG[nTG]->setBCController(static_cast<uint8_t>(range), m_pTG[nTG]->getBreathControllerTarget(), 0);

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setBreathControllerTarget(int target, int nTG)
{
	target = constrain(target, 0, 7);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nBreathControlTarget[nTG] = target;

	m_pTG[nTG]->setBreathControllerTarget(static_cast<uint8_t>(target));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setAftertouchRange(int range, int nTG)
{
	range = constrain(range, 0, 99);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	m_nAftertouchRange[nTG] = range;
	m_pTG[nTG]->setATController(static_cast<uint8_t>(range), m_pTG[nTG]->getAftertouchTarget(), 0);

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::setAftertouchTarget(int target, int nTG)
{
	target = constrain(target, 0, 7);
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);

	m_nAftertouchTarget[nTG] = target;

	m_pTG[nTG]->setAftertouchTarget(static_cast<uint8_t>(target));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged();
}

void CMiniDexed::loadVoiceParameters(const uint8_t *data, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	uint8_t voice[161];

	memcpy(voice, data, 161);

	// fix voice name
	for (int i = 0; i < 10; i++)
	{
		if (voice[151 + i] > 126) // filter characters
			voice[151 + i] = 32;
	}

	m_pTG[nTG]->loadVoiceParameters(&voice[6]);
	m_pTG[nTG]->doRefreshVoice();
	setOPMask(0b111111, nTG);

	m_UI.ParameterChanged();
}

void CMiniDexed::setVoiceDataElement(int address, int value, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	address = constrain(address, 0, 155);
	value = constrain(value, 0, 99);

	m_pTG[nTG]->setVoiceDataElement(static_cast<uint8_t>(address), static_cast<uint8_t>(value));
	m_UI.ParameterChanged();
}

int16_t CMiniDexed::checkSystemExclusive(const uint8_t *pMessage, int nLength, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return 0; // Not an active TG

	return m_pTG[nTG]->checkSystemExclusive(pMessage, static_cast<uint16_t>(nLength));
}

void CMiniDexed::getSysExVoiceDump(uint8_t *dest, int nTG)
{
	uint8_t checksum = 0;
	uint8_t data[156];

	assert(nTG < CConfig::AllToneGenerators);
	if (nTG < m_nToneGenerators)
	{
		assert(m_pTG[nTG]);
		m_pTG[nTG]->getVoiceData(data);
	}
	else
	{
		// Not an active TG so grab a default voice
		m_SysExFileLoader.GetVoice(CSysExFileLoader::MaxVoiceBankID, CSysExFileLoader::VoicesPerBank + 1, data);
	}

	dest[0] = 0xF0; // SysEx start
	dest[1] = 0x43; // ID=Yamaha
	dest[2] = static_cast<uint8_t>(m_nSysExChannel[nTG]); // 0x0c Sub-status 0 and MIDI channel
	dest[3] = 0x00; // Format number (0=1 voice)
	dest[4] = 0x01; // Byte count MSB
	dest[5] = 0x1B; // Byte count LSB
	for (int n = 0; n < 155; n++)
	{
		checksum -= data[n];
		dest[6 + n] = data[n];
	}
	dest[161] = checksum & 0x7f; // Checksum
	dest[162] = 0xF7; // SysEx end
}

void CMiniDexed::setOPMask(uint8_t uchOPMask, int nTG)
{
	if (nTG >= m_nToneGenerators) return; // Not an active TG
	m_uchOPMask[nTG] = uchOPMask;
	m_pTG[nTG]->setOPAll(m_uchOPMask[nTG]);
}

void CMiniDexed::setMasterVolume(float vol)
{
	if (vol < 0.0)
		vol = 0.0;
	else if (vol > 1.0)
		vol = 1.0;

	// Apply logarithmic scaling to match perceived loudness
	vol = powf(vol, 2.0f);

	m_fMasterVolumeW = vol;
}

bool CMiniDexed::SDFilterOut(int nTG)
{
	switch (m_SDFilter.type)
	{
	case SDFilter::Type::TGLink:
		return m_nTGLink[nTG] != m_SDFilter.param;
	case SDFilter::Type::TG:
		return nTG != m_SDFilter.param;
	case SDFilter::Type::MIDIChannel:
		return m_nMIDIChannel[nTG] != m_SDFilter.param;
	default:
		return false;
	}
}

std::string CMiniDexed::GetPerformanceFileName(int nID)
{
	return m_PerformanceConfig.GetPerformanceFileName(nID);
}

std::string CMiniDexed::GetPerformanceName(int nID)
{
	return m_PerformanceConfig.GetPerformanceName(nID);
}

int CMiniDexed::GetLastPerformance()
{
	return m_PerformanceConfig.GetLastPerformance();
}

int CMiniDexed::GetPerformanceBank()
{
	return m_PerformanceConfig.GetPerformanceBankID();
}

int CMiniDexed::GetLastPerformanceBank()
{
	return m_PerformanceConfig.GetLastPerformanceBank();
}

int CMiniDexed::GetActualPerformanceID()
{
	return m_PerformanceConfig.GetPerformanceID();
}

bool CMiniDexed::SetNewPerformance(int nID)
{
	m_bSetNewPerformance = true;
	m_nSetNewPerformanceID = nID;
	if (!m_bVolRampedDown)
		m_bVolRampDownWait = true;

	return true;
}

bool CMiniDexed::SetNewPerformanceBank(int nBankID)
{
	m_bSetNewPerformanceBank = true;
	m_nSetNewPerformanceBankID = nBankID;

	return true;
}

void CMiniDexed::SetFirstPerformance()
{
	m_bSetFirstPerformance = true;
	return;
}

bool CMiniDexed::DoSetNewPerformance()
{
	int nID = m_nSetNewPerformanceID;
	m_PerformanceConfig.SetNewPerformance(nID);

	if (m_PerformanceConfig.Load())
	{
		LoadPerformanceParameters();
		return true;
	}
	else
	{
		SetMIDIChannel(CMIDIDevice::OmniMode, 0);
		return false;
	}
}

void CMiniDexed::DoSetFirstPerformance()
{
	int nID = m_PerformanceConfig.FindFirstPerformance();
	SetNewPerformance(nID);
	m_bSetFirstPerformance = false;
	return;
}

bool CMiniDexed::SavePerformanceNewFile()
{
	m_bSavePerformanceNewFile = m_PerformanceConfig.GetInternalFolderOk() && m_PerformanceConfig.CheckFreePerformanceSlot();
	return m_bSavePerformanceNewFile;
}

bool CMiniDexed::DoSavePerformanceNewFile()
{
	if (m_PerformanceConfig.CreateNewPerformanceFile())
	{
		if (SavePerformance(false))
		{
			return true;
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}
}

void CMiniDexed::LoadPerformanceParameters(void)
{
	for (int nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
	{
		BankSelect(m_PerformanceConfig.GetBankNumber(nTG), nTG);
		ProgramChange(m_PerformanceConfig.GetVoiceNumber(nTG), nTG);
		SetMIDIChannel(m_PerformanceConfig.GetMIDIChannel(nTG), nTG);
		SetSysExChannel(m_PerformanceConfig.GetSysExChannel(nTG), nTG);
		SetSysExEnable(m_PerformanceConfig.GetSysExEnable(nTG), nTG);
		SetMIDIRxSustain(m_PerformanceConfig.GetMIDIRxSustain(nTG), nTG);
		SetMIDIRxPortamento(m_PerformanceConfig.GetMIDIRxPortamento(nTG), nTG);
		SetMIDIRxSostenuto(m_PerformanceConfig.GetMIDIRxSostenuto(nTG), nTG);
		SetMIDIRxHold2(m_PerformanceConfig.GetMIDIRxHold2(nTG), nTG);
		SetVolume(m_PerformanceConfig.GetVolume(nTG), nTG);
		SetPan(m_PerformanceConfig.GetPan(nTG), nTG);
		SetMasterTune(m_PerformanceConfig.GetDetune(nTG), nTG);
		SetCutoff(m_PerformanceConfig.GetCutoff(nTG), nTG);
		SetResonance(m_PerformanceConfig.GetResonance(nTG), nTG);
		setPitchbendRange(m_PerformanceConfig.GetPitchBendRange(nTG), nTG);
		setPitchbendStep(m_PerformanceConfig.GetPitchBendStep(nTG), nTG);
		setPortamentoMode(m_PerformanceConfig.GetPortamentoMode(nTG), nTG);
		setPortamentoGlissando(m_PerformanceConfig.GetPortamentoGlissando(nTG), nTG);
		setPortamentoTime(m_PerformanceConfig.GetPortamentoTime(nTG), nTG);

		setNoteLimitLow(m_PerformanceConfig.GetNoteLimitLow(nTG), nTG);
		setNoteLimitHigh(m_PerformanceConfig.GetNoteLimitHigh(nTG), nTG);
		setNoteShift(m_PerformanceConfig.GetNoteShift(nTG), nTG);

		if (m_PerformanceConfig.VoiceDataFilled(nTG) && nTG < m_nToneGenerators)
		{
			uint8_t tVoiceData[156];
			m_PerformanceConfig.GetVoiceDataFromTxt(tVoiceData, nTG);
			m_pTG[nTG]->loadVoiceParameters(tVoiceData);
			setOPMask(0b111111, nTG);
		}

		setMonoMode(m_PerformanceConfig.GetMonoMode(nTG) ? 1 : 0, nTG);
		setTGLink(m_PerformanceConfig.GetTGLink(nTG), nTG);

		SetFX1Send(m_PerformanceConfig.GetFX1Send(nTG), nTG);
		SetFX2Send(m_PerformanceConfig.GetFX2Send(nTG), nTG);

		setModWheelRange(m_PerformanceConfig.GetModulationWheelRange(nTG), nTG);
		setModWheelTarget(m_PerformanceConfig.GetModulationWheelTarget(nTG), nTG);
		setFootControllerRange(m_PerformanceConfig.GetFootControlRange(nTG), nTG);
		setFootControllerTarget(m_PerformanceConfig.GetFootControlTarget(nTG), nTG);
		setBreathControllerRange(m_PerformanceConfig.GetBreathControlRange(nTG), nTG);
		setBreathControllerTarget(m_PerformanceConfig.GetBreathControlTarget(nTG), nTG);
		setAftertouchRange(m_PerformanceConfig.GetAftertouchRange(nTG), nTG);
		setAftertouchTarget(m_PerformanceConfig.GetAftertouchTarget(nTG), nTG);

		SetCompressorEnable(m_PerformanceConfig.GetCompressorEnable(nTG), nTG);
		SetCompressorPreGain(m_PerformanceConfig.GetCompressorPreGain(nTG), nTG);
		SetCompressorThresh(m_PerformanceConfig.GetCompressorThresh(nTG), nTG);
		SetCompressorRatio(m_PerformanceConfig.GetCompressorRatio(nTG), nTG);
		SetCompressorAttack(m_PerformanceConfig.GetCompressorAttack(nTG), nTG);

		SetCompressorRelease(m_PerformanceConfig.GetCompressorRelease(nTG), nTG);
		SetCompressorMakeupGain(m_PerformanceConfig.GetCompressorMakeupGain(nTG), nTG);

		SetEQLow(m_PerformanceConfig.GetEQLow(nTG), nTG);
		SetEQMid(m_PerformanceConfig.GetEQMid(nTG), nTG);
		SetEQHigh(m_PerformanceConfig.GetEQHigh(nTG), nTG);
		SetEQGain(m_PerformanceConfig.GetEQGain(nTG), nTG);
		SetEQLowMidFreq(m_PerformanceConfig.GetEQLowMidFreq(nTG), nTG);
		SetEQMidHighFreq(m_PerformanceConfig.GetEQMidHighFreq(nTG), nTG);
		SetEQPreLowcut(m_PerformanceConfig.GetEQPreLowcut(nTG), nTG);
		SetEQPreHighcut(m_PerformanceConfig.GetEQPreHighcut(nTG), nTG);
	}

	for (int nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		for (int nParam = 0; nParam < FX::Parameter::Unknown; ++nParam)
		{
			FX::Parameter param = FX::Parameter(nParam);
			const FX::ParameterType &p = FX::s_Parameter[nParam];
			bool bSaveOnly = p.Flags & FX::Flag::Composite;
			SetFXParameter(param, m_PerformanceConfig.GetFXParameter(param, nFX), nFX, bSaveOnly);
		}
	}

	for (int nBus = 0; nBus < CConfig::Buses; ++nBus)
	{
		for (int nParam = 0; nParam < Bus::Parameter::Unknown; ++nParam)
		{
			Bus::Parameter param = Bus::Parameter(nParam);
			SetBusParameter(param, m_PerformanceConfig.GetBusParameter(param, nBus), nBus);
		}
	}

	m_UI.DisplayChanged();
}

std::string CMiniDexed::GetNewPerformanceDefaultName()
{
	return m_PerformanceConfig.GetNewPerformanceDefaultName();
}

void CMiniDexed::SetNewPerformanceName(const std::string &Name)
{
	m_PerformanceConfig.SetNewPerformanceName(Name);
}

bool CMiniDexed::IsValidPerformance(int nID)
{
	return m_PerformanceConfig.IsValidPerformance(nID);
}

bool CMiniDexed::IsValidPerformanceBank(int nBankID)
{
	return m_PerformanceConfig.IsValidPerformanceBank(nBankID);
}

int CMiniDexed::GetLastKeyDown()
{
	return m_nLastKeyDown;
}

void CMiniDexed::SetVoiceName(const std::string &VoiceName, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return; // Not an active TG

	assert(m_pTG[nTG]);
	char Name[11];
	strncpy(Name, VoiceName.c_str(), 10);
	Name[10] = '\0';
	m_pTG[nTG]->setName(Name);
}

bool CMiniDexed::DeletePerformance(int nID)
{
	if (m_PerformanceConfig.IsValidPerformance(nID) && m_PerformanceConfig.GetInternalFolderOk())
	{
		m_bDeletePerformance = true;
		m_nDeletePerformanceID = nID;

		return true;
	}
	else
	{
		return false;
	}
}

bool CMiniDexed::DoDeletePerformance()
{
	int nID = m_nDeletePerformanceID;
	if (m_PerformanceConfig.DeletePerformance(nID))
	{
		if (m_PerformanceConfig.Load())
		{
			LoadPerformanceParameters();
			return true;
		}
		else
		{
			SetMIDIChannel(CMIDIDevice::OmniMode, 0);
		}
	}

	return false;
}

bool CMiniDexed::GetPerformanceSelectToLoad()
{
	return m_pConfig->GetPerformanceSelectToLoad();
}

void CMiniDexed::setModController(int controller, int parameter, int value, int nTG)
{
	int nBits;

	switch (controller)
	{
	case 0:
		if (parameter == 0)
		{
			setModWheelRange(value, nTG);
		}
		else
		{
			value = constrain(value, 0, 1);
			nBits = m_nModulationWheelTarget[nTG];
			value == 1 ? nBits |= 1 << (parameter - 1) : nBits &= ~(1 << (parameter - 1));
			setModWheelTarget(nBits, nTG);
		}
		break;

	case 1:
		if (parameter == 0)
		{
			setFootControllerRange(value, nTG);
		}
		else
		{
			value = constrain(value, 0, 1);
			nBits = m_nFootControlTarget[nTG];
			value == 1 ? nBits |= 1 << (parameter - 1) : nBits &= ~(1 << (parameter - 1));
			setFootControllerTarget(nBits, nTG);
		}
		break;

	case 2:
		if (parameter == 0)
		{
			setBreathControllerRange(value, nTG);
		}
		else
		{
			value = constrain(value, 0, 1);
			nBits = m_nBreathControlTarget[nTG];
			value == 1 ? nBits |= 1 << (parameter - 1) : nBits &= ~(1 << (parameter - 1));
			setBreathControllerTarget(nBits, nTG);
		}
		break;

	case 3:
		if (parameter == 0)
		{
			setAftertouchRange(value, nTG);
		}
		else
		{
			value = constrain(value, 0, 1);
			nBits = m_nAftertouchTarget[nTG];
			value == 1 ? nBits |= 1 << (parameter - 1) : nBits &= ~(1 << (parameter - 1));
			setAftertouchTarget(nBits, nTG);
		}
		break;
	default:
		break;
	}
}

int CMiniDexed::getModController(int controller, int parameter, int nTG)
{
	int nBits;
	switch (controller)
	{
	case 0:
		if (parameter == 0)
		{
			return m_nModulationWheelRange[nTG];
		}
		else
		{
			nBits = m_nModulationWheelTarget[nTG];
			nBits &= 1 << (parameter - 1);
			return (nBits != 0 ? 1 : 0);
		}
		break;

	case 1:
		if (parameter == 0)
		{
			return m_nFootControlRange[nTG];
		}
		else
		{
			nBits = m_nFootControlTarget[nTG];
			nBits &= 1 << (parameter - 1);
			return (nBits != 0 ? 1 : 0);
		}
		break;

	case 2:
		if (parameter == 0)
		{
			return m_nBreathControlRange[nTG];
		}
		else
		{
			nBits = m_nBreathControlTarget[nTG];
			nBits &= 1 << (parameter - 1);
			return (nBits != 0 ? 1 : 0);
		}
		break;

	case 3:
		if (parameter == 0)
		{
			return m_nAftertouchRange[nTG];
		}
		else
		{
			nBits = m_nAftertouchTarget[nTG];
			nBits &= 1 << (parameter - 1);
			return (nBits != 0 ? 1 : 0);
		}
		break;

	default:
		return 0;
		break;
	}
}

void CMiniDexed::UpdateNetwork()
{
	if (!m_pNet)
	{
		LOGNOTE("CMiniDexed::UpdateNetwork: m_pNet is nullptr, returning early");
		return;
	}

	bool bNetIsRunning = m_pNet->IsRunning();
	if (m_pNetDevice->GetType() == NetDeviceTypeEthernet)
		bNetIsRunning &= m_pNetDevice->IsLinkUp();
	else if (m_pNetDevice->GetType() == NetDeviceTypeWLAN)
		bNetIsRunning &= (m_WPASupplicant && m_WPASupplicant->IsConnected());

	if (!m_bNetworkInit && bNetIsRunning)
	{
		LOGNOTE("CMiniDexed::UpdateNetwork: Network became ready, initializing network services");
		m_bNetworkInit = true;
		CString IPString;
		m_pNet->GetConfig()->GetIPAddress()->Format(&IPString);

		if (m_UDPMIDI)
		{
			m_UDPMIDI->Initialize();
		}

		if (m_pConfig->GetNetworkFTPEnabled())
		{
			m_pFTPDaemon = new CFTPDaemon(FTPUSERNAME, FTPPASSWORD, m_pmDNSPublisher, m_pConfig);

			if (!m_pFTPDaemon->Initialize())
			{
				LOGERR("Failed to init FTP daemon");
				delete m_pFTPDaemon;
				m_pFTPDaemon = nullptr;
			}
			else
			{
				LOGNOTE("FTP daemon initialized");
			}
		}
		else
		{
			LOGNOTE("FTP daemon not started (NetworkFTPEnabled=0)");
		}

		m_pmDNSPublisher = new CmDNSPublisher(m_pNet);
		assert(m_pmDNSPublisher);

		if (!m_pmDNSPublisher->PublishService(m_pConfig->GetNetworkHostname(), CmDNSPublisher::ServiceTypeAppleMIDI,
						      5004))
		{
			LOGPANIC("Cannot publish mdns service");
		}

		static constexpr const char *ServiceTypeFTP = "_ftp._tcp";
		static const char *ftpTxt[] = {"app=MiniDexed", nullptr};
		if (!m_pmDNSPublisher->PublishService(m_pConfig->GetNetworkHostname(), ServiceTypeFTP, 21, ftpTxt))
		{
			LOGPANIC("Cannot publish mdns service");
		}

		if (m_pConfig->GetSyslogEnabled())
		{
			LOGNOTE("Syslog server is enabled in configuration");
			const CIPAddress &ServerIP = m_pConfig->GetNetworkSyslogServerIPAddress();
			if (ServerIP.IsSet() && !ServerIP.IsNull())
			{
				static const uint16_t usServerPort = 8514;
				CString IPString;
				ServerIP.Format(&IPString);
				LOGNOTE("Sending log messages to syslog server %s:%d",
					IPString.c_str(), static_cast<int>(usServerPort));

				new CSysLogDaemon(m_pNet, ServerIP, usServerPort);
			}
			else
			{
				LOGNOTE("Syslog server IP not set");
			}
		}
		else
		{
			LOGNOTE("Syslog server is not enabled in configuration");
		}
		m_bNetworkReady = true;
	}

	if (m_bNetworkReady && !bNetIsRunning)
	{
		LOGNOTE("CMiniDexed::UpdateNetwork: Network disconnected");
		m_bNetworkReady = false;
		m_pmDNSPublisher->UnpublishService(m_pConfig->GetNetworkHostname());
		LOGNOTE("Network disconnected.");
	}
	else if (!m_bNetworkReady && bNetIsRunning)
	{
		LOGNOTE("CMiniDexed::UpdateNetwork: Network connection reestablished");
		m_bNetworkReady = true;

		if (!m_pmDNSPublisher->PublishService(m_pConfig->GetNetworkHostname(), CmDNSPublisher::ServiceTypeAppleMIDI,
						      5004))
		{
			LOGPANIC("Cannot publish mdns service");
		}

		static constexpr const char *ServiceTypeFTP = "_ftp._tcp";
		static const char *ftpTxt[] = {"app=DreamDexed", nullptr};
		if (!m_pmDNSPublisher->PublishService(m_pConfig->GetNetworkHostname(), ServiceTypeFTP, 21, ftpTxt))
		{
			LOGPANIC("Cannot publish mdns service");
		}

		m_bNetworkReady = true;

		LOGNOTE("Network connection reestablished.");
	}
}

bool CMiniDexed::InitNetwork()
{
	LOGNOTE("CMiniDexed::InitNetwork called");
	assert(m_pNet == nullptr);

	TNetDeviceType NetDeviceType = NetDeviceTypeUnknown;

	if (m_pConfig->GetNetworkEnabled())
	{
		LOGNOTE("CMiniDexed::InitNetwork: Network is enabled in configuration");

		LOGNOTE("CMiniDexed::InitNetwork: Network type set in configuration: %s", m_pConfig->GetNetworkType());

		if (strcmp(m_pConfig->GetNetworkType(), "wlan") == 0)
		{
			LOGNOTE("CMiniDexed::InitNetwork: Initializing WLAN");
			NetDeviceType = NetDeviceTypeWLAN;
			m_WLAN = new CBcm4343Device(WLANFirmwarePath);
			if (m_WLAN && m_WLAN->Initialize())
			{
				LOGNOTE("CMiniDexed::InitNetwork: WLAN initialized");
			}
			else
			{
				LOGERR("CMiniDexed::InitNetwork: Failed to initialize WLAN, maybe firmware files are missing?");
				delete m_WLAN;
				m_WLAN = nullptr;
				return false;
			}
		}
		else if (strcmp(m_pConfig->GetNetworkType(), "ethernet") == 0)
		{
			LOGNOTE("CMiniDexed::InitNetwork: Initializing Ethernet");
			NetDeviceType = NetDeviceTypeEthernet;
		}
		else
		{
			LOGERR("CMiniDexed::InitNetwork: Network type is not set, please check your minidexed configuration file.");
			NetDeviceType = NetDeviceTypeUnknown;
		}

		if (NetDeviceType != NetDeviceTypeUnknown)
		{
			if (m_pConfig->GetNetworkDHCP())
			{
				LOGNOTE("CMiniDexed::InitNetwork: Creating CNetSubSystem with DHCP (Hostname: %s)", m_pConfig->GetNetworkHostname());
				m_pNet = new CNetSubSystem(0, 0, 0, 0, m_pConfig->GetNetworkHostname(), NetDeviceType);
			}
			else if (m_pConfig->GetNetworkIPAddress().IsSet() && m_pConfig->GetNetworkSubnetMask().IsSet())
			{
				CString IPString, SubnetString;
				m_pConfig->GetNetworkIPAddress().Format(&IPString);
				m_pConfig->GetNetworkSubnetMask().Format(&SubnetString);
				LOGNOTE("CMiniDexed::InitNetwork: Creating CNetSubSystem with IP: %s / %s", IPString.c_str(), SubnetString.c_str());
				m_pNet = new CNetSubSystem(
				m_pConfig->GetNetworkIPAddress().Get(),
				m_pConfig->GetNetworkSubnetMask().Get(),
				m_pConfig->GetNetworkDefaultGateway().IsSet() ? m_pConfig->GetNetworkDefaultGateway().Get() : 0,
				m_pConfig->GetNetworkDNSServer().IsSet() ? m_pConfig->GetNetworkDNSServer().Get() : 0,
				m_pConfig->GetNetworkHostname(),
				NetDeviceType);
			}
			else
			{
				LOGNOTE("CMiniDexed::InitNetwork: Neither DHCP nor IP address/subnet mask is set, using DHCP (Hostname: %s)", m_pConfig->GetNetworkHostname());
				m_pNet = new CNetSubSystem(0, 0, 0, 0, m_pConfig->GetNetworkHostname(), NetDeviceType);
			}

			if (!m_pNet || !m_pNet->Initialize(false)) // Check if m_pNet allocation succeeded
			{
				LOGERR("CMiniDexed::InitNetwork: Failed to initialize network subsystem");
				delete m_pNet;
				m_pNet = nullptr; // Clean up if failed
				delete m_WLAN;
				m_WLAN = nullptr; // Clean up WLAN if allocated
				return false; // Return false as network init failed
			}
			// WPASupplicant needs to be started after netdevice available
			if (NetDeviceType == NetDeviceTypeWLAN)
			{
				LOGNOTE("CMiniDexed::InitNetwork: Initializing WPASupplicant");
				m_WPASupplicant = new CWPASupplicant(WLANConfigFile); // Allocate m_WPASupplicant
				if (!m_WPASupplicant || !m_WPASupplicant->Initialize())
				{
					LOGERR("CMiniDexed::InitNetwork: Failed to initialize WPASupplicant, maybe wlan config is missing?");
					delete m_WPASupplicant;
					m_WPASupplicant = nullptr; // Clean up if failed
					// Continue without supplicant? Or return false? Decided to continue for now.
				}
			}
			m_pNetDevice = CNetDevice::GetNetDevice(NetDeviceType);

			// Allocate UDP MIDI device now that network might be up
			m_UDPMIDI = new CUDPMIDIDevice(this, m_pConfig, &m_UI); // Allocate m_UDPMIDI
			if (!m_UDPMIDI)
			{
				LOGERR("CMiniDexed::InitNetwork: Failed to allocate UDP MIDI device");
				// Clean up other network resources if needed, or handle error appropriately
			}
			else
			{
				// Synchronize UDP MIDI channels with current assignments
				for (int nTG = 0; nTG < m_nToneGenerators; ++nTG)
					m_UDPMIDI->SetChannel(m_nMIDIChannel[nTG], nTG);
			}
		}
		LOGNOTE("CMiniDexed::InitNetwork: returning %d", m_pNet != nullptr);
		return m_pNet != nullptr;
	}
	else
	{
		LOGNOTE("CMiniDexed::InitNetwork: Network is not enabled in configuration");
		return false;
	}
}

const CIPAddress &CMiniDexed::GetNetworkIPAddress()
{
	if (m_pNet)
		return *m_pNet->GetConfig()->GetIPAddress();
	else
		return m_pConfig->GetNetworkIPAddress();
}

CStatus *CStatus::s_pThis = 0;
