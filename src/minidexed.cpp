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
#include <circle/logger.h>
#include <circle/memory.h>
#include <circle/sound/pwmsoundbasedevice.h>
#include <circle/sound/i2ssoundbasedevice.h>
#include <circle/sound/hdmisoundbasedevice.h>
#include <circle/net/syslogdaemon.h>
#include <circle/net/ipaddress.h>
#include <circle/gpiopin.h>
#include <cstring>
#include <cstdio>
#include <cassert>
#include "arm_float_to_q23.h"
#include "arm_scale_zip_f32.h"
#include "arm_zip_f32.h"

const char WLANFirmwarePath[] = "SD:firmware/";
const char WLANConfigFile[]   = "SD:wpa_supplicant.conf";
#define FTPUSERNAME "admin"
#define FTPPASSWORD "admin"

LOGMODULE ("minidexed");

CMiniDexed::CMiniDexed (CConfig *pConfig, CInterruptSystem *pInterrupt,
			CGPIOManager *pGPIOManager, CI2CMaster *pI2CMaster, CSPIMaster *pSPIMaster, FATFS *pFileSystem)
:
#ifdef ARM_ALLOW_MULTI_CORE
	CMultiCoreSupport (CMemorySystem::Get ()),
#endif
	m_pConfig (pConfig),
	m_pTG {},
	m_UI (this, pGPIOManager, pI2CMaster, pSPIMaster, pConfig),
	m_PerformanceConfig (pFileSystem),
	m_pMIDIKeyboard {},
	m_PCKeyboard (this, pConfig, &m_UI),
	m_SerialMIDI (this, pInterrupt, pConfig, &m_UI),
	m_fMasterVolume{},
	m_fMasterVolumeW(0),
	m_bUseSerial (false),
	m_bQuadDAC8Chan (false),
	m_pSoundDevice (nullptr),
	m_bChannelsSwapped (pConfig->GetChannelsSwapped ()),
#ifdef ARM_ALLOW_MULTI_CORE
//	m_nActiveTGsLog2 (0),
#endif
	m_nLastKeyDown{},
	m_GetChunkTimer ("GetChunk",
			 1000000U * pConfig->GetChunkSize ()/2 / pConfig->GetSampleRate ()),
	m_bProfileEnabled (m_pConfig->GetProfileEnabled ()),
	fx_chain {},
	tg_mixer {},
	sendfx_mixer {},
	m_pNet(nullptr),
	m_pNetDevice(nullptr),
	m_WLAN(nullptr),
	m_WPASupplicant(nullptr),
	m_bNetworkReady(false),
	m_bNetworkInit(false),
	m_UDPMIDI(nullptr),
	m_pFTPDaemon(nullptr),
	m_pmDNSPublisher (nullptr),
	m_bSavePerformance (false),
	m_bSavePerformanceNewFile (false),
	m_bSetNewPerformance (false),
	m_bSetNewPerformanceBank (false),
	m_bSetFirstPerformance (false),
	m_bDeletePerformance (false),
	m_bLoadPerformanceBusy(false),
	m_bLoadPerformanceBankBusy(false),
	m_bVolRampDownWait{},
	m_bVolRampedDown{},
	m_fRamp{10.0f / pConfig->GetSampleRate()}
{
	assert (m_pConfig);
		
	m_nToneGenerators = m_pConfig->GetToneGenerators();
	m_nPolyphony = m_pConfig->GetPolyphony();
	LOGNOTE("Tone Generators=%d, Polyphony=%d", m_nToneGenerators, m_nPolyphony);

	for (unsigned i = 0; i < CConfig::AllToneGenerators; i++)
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
		m_nPitchBendRange[i] = 2;
		m_nPitchBendStep[i] = 0;
		m_nPortamentoMode[i] = 0;
		m_nPortamentoGlissando[i] = 0;
		m_nPortamentoTime[i] = 0;
		m_bMonoMode[i]=0; 
		m_nTGLink[i]=0;
		m_nNoteLimitLow[i] = 0;
		m_nNoteLimitHigh[i] = 127;
		m_nNoteShift[i] = 0;
		
		m_nModulationWheelRange[i]=99;
		m_nModulationWheelTarget[i]=7;
		m_nFootControlRange[i]=99;
		m_nFootControlTarget[i]=0;	
		m_nBreathControlRange[i]=99;	
		m_nBreathControlTarget[i]=0;	
		m_nAftertouchRange[i]=99;	
		m_nAftertouchTarget[i]=0;
		
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
		if (i<m_nToneGenerators)
		{
			m_uchOPMask[i] = 0b111111;	// All operators on

			m_pTG[i] = new CDexedAdapter (m_nPolyphony, pConfig->GetSampleRate ());
			assert (m_pTG[i]);

			m_pTG[i]->setEngineType(pConfig->GetEngineType ());
			m_pTG[i]->activate ();
		}
	}

	unsigned nUSBGadgetPin = pConfig->GetUSBGadgetPin();
	bool bUSBGadget = pConfig->GetUSBGadget();
	bool bUSBGadgetMode = pConfig->GetUSBGadgetMode();
		
	if (bUSBGadgetMode)
	{
#if RASPPI==5
		LOGNOTE ("USB Gadget (Device) Mode NOT supported on RPI 5");
#else
		if (nUSBGadgetPin == 0)
		{
			LOGNOTE ("USB In Gadget (Device) Mode");
		}
		else
		{
			LOGNOTE ("USB In Gadget (Device) Mode [USBGadgetPin %d = LOW]", nUSBGadgetPin);
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
				LOGNOTE ("USB State Unknown");
			}
			else
			{
				LOGNOTE ("USB In Host Mode [USBGadgetPin %d = HIGH]", nUSBGadgetPin);
			}
		}
		else
		{
			LOGNOTE ("USB In Host Mode");
		}
	}

	for (unsigned i = 0; i < CConfig::MaxUSBMIDIDevices; i++)
	{
		m_pMIDIKeyboard[i] = new CMIDIKeyboard (this, pConfig, &m_UI, i);
		assert (m_pMIDIKeyboard[i]);
	}

	// select the sound device
	const char *pDeviceName = pConfig->GetSoundDevice ();
	if (strcmp (pDeviceName, "i2s") == 0)
	{
		LOGNOTE ("I2S mode");
#if RASPPI==5
		// Quad DAC 8-channel mono only an option for RPI 5
		m_bQuadDAC8Chan = pConfig->GetQuadDAC8Chan ();
#endif
		if (m_bQuadDAC8Chan && (m_nToneGenerators != 8))
		{
			LOGNOTE("ERROR: Quad DAC Mode is only valid when number of TGs = 8.  Defaulting to non-Quad DAC mode,");
			m_bQuadDAC8Chan = false;
		}
		if (m_bQuadDAC8Chan) {
			LOGNOTE ("Configured for Quad DAC 8-channel Mono audio");
			m_pSoundDevice = new CI2SSoundBaseDevice (pInterrupt, pConfig->GetSampleRate (),
								  pConfig->GetChunkSize (), false,
								  pI2CMaster, pConfig->GetDACI2CAddress (),
								  CI2SSoundBaseDevice::DeviceModeTXOnly,
								  8);  // 8 channels - L+R x4 across 4 I2S lanes
		}
		else
		{
			m_pSoundDevice = new CI2SSoundBaseDevice (pInterrupt, pConfig->GetSampleRate (),
								  pConfig->GetChunkSize (), false,
								  pI2CMaster, pConfig->GetDACI2CAddress (),
								  CI2SSoundBaseDevice::DeviceModeTXOnly,
								  2);  // 2 channels - L+R
		}
	}
	else if (strcmp (pDeviceName, "hdmi") == 0)
	{
#if RASPPI==5
		LOGNOTE ("HDMI mode NOT supported on RPI 5.");
#else
		LOGNOTE ("HDMI mode");

		m_pSoundDevice = new CHDMISoundBaseDevice (pInterrupt, pConfig->GetSampleRate (),
							   pConfig->GetChunkSize ());
#endif
	}
	else
	{
		LOGNOTE ("PWM mode");

		m_pSoundDevice = new CPWMSoundBaseDevice (pInterrupt, pConfig->GetSampleRate (),
							  pConfig->GetChunkSize ());
	}

#ifdef ARM_ALLOW_MULTI_CORE
	for (unsigned nCore = 0; nCore < CORES; nCore++)
	{
		m_CoreStatus[nCore] = CoreStatusInit;
	}
#endif

	// BEGIN setup tg_mixer
	tg_mixer = new AudioStereoMixer<CConfig::AllToneGenerators>(pConfig->GetChunkSize()/2, pConfig->GetSampleRate());
	// END setup tgmixer

	for (unsigned nFX = 0; nFX < CConfig::FXMixers; nFX++)
	{
		sendfx_mixer[nFX] = new AudioStereoMixer<CConfig::AllToneGenerators>(pConfig->GetChunkSize()/2, pConfig->GetSampleRate());
	}

	for (unsigned nFX = 0; nFX < CConfig::FXChains; nFX++)
	{
		fx_chain[nFX] = new AudioFXChain(pConfig->GetSampleRate());

		for (unsigned nParam = 0; nParam < FX::FXParameterUnknown; ++nParam)
		{
			const FX::FXParameterType &p = FX::s_FXParameter[nParam];
			bool bSaveOnly = p.Flags & FX::FXComposite;
			SetFXParameter (FX::TFXParameter(nParam), p.Default, nFX, bSaveOnly);
		}
	}

	// END setup reverb

	SetParameter (ParameterMasterVolume, pConfig->GetMasterVolume());
	SetParameter (ParameterMixerDryLevel, 99);
	SetParameter (ParameterFXBypass, 0);

	SetPerformanceSelectChannel(m_pConfig->GetPerformanceSelectChannel());
		
	SetParameter (ParameterPerformanceBank, 0);
};

CMiniDexed::~CMiniDexed (void)
{
	delete m_WLAN;
	delete m_WPASupplicant;
	delete m_UDPMIDI;
	delete m_pFTPDaemon;
	delete m_pmDNSPublisher;
}

bool CMiniDexed::Initialize (void)
{
	LOGNOTE("CMiniDexed::Initialize called");
	assert (m_pConfig);
	assert (m_pSoundDevice);

	if (!m_UI.Initialize ())
	{
		return false;
	}

	m_SysExFileLoader.Load (m_pConfig->GetHeaderlessSysExVoices ());

	if (m_SerialMIDI.Initialize ())
	{
		LOGNOTE ("Serial MIDI interface enabled");

		m_bUseSerial = true;
	}
	
	if (m_pConfig->GetMIDIRXProgramChange())
	{
		int nPerfCh = GetParameter(ParameterPerformanceSelectChannel);
		if (nPerfCh == CMIDIDevice::Disabled) {
			LOGNOTE("Program Change: Enabled for Voices");
		} else if (nPerfCh == CMIDIDevice::OmniMode) {
			LOGNOTE("Program Change: Enabled for Performances (Omni)");
		} else {
			LOGNOTE("Program Change: Enabled for Performances (CH %d)", nPerfCh+1);
		}
	} else {
		LOGNOTE("Program Change: Disabled");
	}

	for (unsigned i = 0; i < m_nToneGenerators; i++)
	{
		assert (m_pTG[i]);

		SetVolume (100, i);
		SetExpression (127, i);
		ProgramChange (0, i);

		m_pTG[i]->setTranspose (24);

		m_pTG[i]->setPBController (2, 0);
		m_pTG[i]->setMWController (99, 1, 0); 

		m_pTG[i]->setFCController (99, 1, 0); 
		m_pTG[i]->setBCController (99, 1, 0);
		m_pTG[i]->setATController (99, 1, 0);
		
		tg_mixer->pan(i,mapfloat(m_nPan[i],0,127,0.0f,1.0f));
		tg_mixer->gain(i,1.0f);

		for (unsigned nFX = 0; nFX < CConfig::FXMixers; ++nFX)
		{
			sendfx_mixer[nFX]->pan(i,mapfloat(m_nPan[i],0,127,0.0f,1.0f));
			sendfx_mixer[nFX]->gain(i,mapfloat(nFX == 0 ? m_nFX1Send[i] : m_nFX2Send[i],0,99,0.0f,1.0f));
		}
	}

	m_PerformanceConfig.Init(m_nToneGenerators);
	if (m_PerformanceConfig.Load ())
	{
		LoadPerformanceParameters(); 
	}
	else
	{
		SetMIDIChannel (CMIDIDevice::OmniMode, 0);
	}
	
	// setup and start the sound device
	int Channels = 1;	// 16-bit Mono
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_bQuadDAC8Chan)
	{
		Channels = 8;	// 16-bit 8-channel mono
	}
	else
	{
		Channels = 2;	// 16-bit Stereo
	}
#endif
	// Need 2 x ChunkSize / Channel queue frames as the audio driver uses
	// two DMA channels each of ChunkSize and one single single frame
	// contains a sample for each of all the channels.
	//
	// See discussion here: https://github.com/rsta2/circle/discussions/453
	if (!m_pSoundDevice->AllocateQueueFrames (2 * m_pConfig->GetChunkSize () / Channels))
	{
		LOGERR ("Cannot allocate sound queue");

		return false;
	}

	m_pSoundDevice->SetWriteFormat (SoundFormatSigned24_32, Channels);

	m_nQueueSizeFrames = m_pSoundDevice->GetQueueSizeFrames ();

	m_pSoundDevice->Start ();

	m_UI.LoadDefaultScreen ();

#ifdef ARM_ALLOW_MULTI_CORE
	// start secondary cores
	if (!CMultiCoreSupport::Initialize ())
	{
		return false;
	}

	InitNetwork();  // returns bool but we continue even if something goes wrong
	LOGNOTE("CMiniDexed::Initialize: InitNetwork() called");
#endif

	return true;
}

void CMiniDexed::Process (bool bPlugAndPlayUpdated)
{
	CScheduler* const pScheduler = CScheduler::Get();
#ifndef ARM_ALLOW_MULTI_CORE
	ProcessSound ();
	pScheduler->Yield();
#endif

	for (unsigned i = 0; i < CConfig::MaxUSBMIDIDevices; i++)
	{
		assert (m_pMIDIKeyboard[i]);
		m_pMIDIKeyboard[i]->Process (bPlugAndPlayUpdated);
		pScheduler->Yield();
	}

	m_PCKeyboard.Process (bPlugAndPlayUpdated);

	if (m_bUseSerial)
	{
		m_SerialMIDI.Process ();
		pScheduler->Yield();
	}

	m_UI.Process ();

	if (m_bSavePerformance)
	{
		DoSavePerformance ();

		m_bSavePerformance = false;
		pScheduler->Yield();
	}

	if (m_bSavePerformanceNewFile)
	{
		DoSavePerformanceNewFile ();
		m_bSavePerformanceNewFile = false;
		pScheduler->Yield();
	}
	
	if (m_bSetNewPerformanceBank && !m_bLoadPerformanceBusy && !m_bLoadPerformanceBankBusy)
	{
		DoSetNewPerformanceBank ();
		if (m_nSetNewPerformanceBankID == GetActualPerformanceBankID())
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
		pScheduler->Yield();
	}
	
	if (m_bSetNewPerformance && m_bVolRampedDown && !m_bSetNewPerformanceBank && !m_bLoadPerformanceBusy && !m_bLoadPerformanceBankBusy)
	{
		for (unsigned i = 0; i < m_nToneGenerators; ++i)
		{
			m_pTG[i]->resetState();
		}

		DoSetNewPerformance ();

		for (unsigned nFX = 0; nFX < CConfig::FXChains; ++nFX)
		{
			fx_chain[nFX]->resetState();
		}

		if (m_nSetNewPerformanceID == GetActualPerformanceID())
		{
			m_bSetNewPerformance = false;
			m_bVolRampedDown = false;
		}
		pScheduler->Yield();
	}
	
	if(m_bDeletePerformance)
	{
		DoDeletePerformance ();
		m_bDeletePerformance = false;
		pScheduler->Yield();
	}
		
	if (m_bProfileEnabled)
	{
		m_GetChunkTimer.Dump ();
		pScheduler->Yield();
	}

	m_Status.Update();

	if (m_pNet) {
		UpdateNetwork();
	}
	// Allow other tasks to run
	pScheduler->Yield();
}

#ifdef ARM_ALLOW_MULTI_CORE

void CMiniDexed::Run (unsigned nCore)
{
	assert (1 <= nCore && nCore < CORES);

	if (nCore == 1)
	{
		m_CoreStatus[nCore] = CoreStatusIdle;			// core 1 ready

		// wait for cores 2 and 3 to be ready
		for (unsigned nCore = 2; nCore < CORES; nCore++)
		{
			while (m_CoreStatus[nCore] != CoreStatusIdle)
			{
				WaitForEvent ();
			}
		}

		while (m_CoreStatus[nCore] != CoreStatusExit)
		{
			ProcessSound ();
		}
	}
	else								// core 2 and 3
	{
		while (1)
		{
			m_CoreStatus[nCore] = CoreStatusIdle;		// ready to be kicked
			SendIPI (1, IPI_USER);

			while (m_CoreStatus[nCore] == CoreStatusIdle)
			{
				WaitForEvent ();
			}

			// now kicked from core 1

			if (m_CoreStatus[nCore] == CoreStatusExit)
			{
				m_CoreStatus[nCore] = CoreStatusUnknown;

				break;
			}

			assert (m_CoreStatus[nCore] == CoreStatusBusy);

			// process the TGs, assigned to this core (2 or 3)

			assert (m_nFramesToProcess <= m_pConfig->MaxChunkSize);
			unsigned nTG = m_pConfig->GetTGsCore1() + (nCore-2)*m_pConfig->GetTGsCore23();
			for (unsigned i = 0; i < m_pConfig->GetTGsCore23(); i++, nTG++)
			{
				assert (nTG < CConfig::AllToneGenerators);
				if (nTG < m_pConfig->GetToneGenerators())
				{
					assert (m_pTG[nTG]);
					m_pTG[nTG]->getSamples (m_OutputLevel[nTG],m_nFramesToProcess);
				}
			}
		}
	}
}

#endif

CSysExFileLoader *CMiniDexed::GetSysExFileLoader (void)
{
	return &m_SysExFileLoader;
}

CPerformanceConfig *CMiniDexed::GetPerformanceConfig (void)
{
	return &m_PerformanceConfig;
}

void CMiniDexed::BankSelect (unsigned nBank, unsigned nTG)
{
	nBank=constrain((int)nBank,0,16383);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG
	
	if (GetSysExFileLoader ()->IsValidBank(nBank))
	{
		// Only change if we have the bank loaded
		m_nVoiceBankID[nTG] = nBank;

		m_UI.ParameterChanged ();
	}
}

void CMiniDexed::BankSelectPerformance (unsigned nBank)
{
	nBank=constrain((int)nBank,0,16383);

	if (GetPerformanceConfig ()->IsValidPerformanceBank(nBank))
	{
		// Only change if we have the bank loaded
		m_nVoiceBankIDPerformance = nBank;
		SetNewPerformanceBank (nBank);

		m_UI.ParameterChanged ();
	}
}

void CMiniDexed::BankSelectMSB (unsigned nBankMSB, unsigned nTG)
{
	nBankMSB=constrain((int)nBankMSB,0,127);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	// MIDI Spec 1.0 "BANK SELECT" states:
	//   "The transmitter must transmit the MSB and LSB as a pair,
	//   and the Program Change must be sent immediately after
	//   the Bank Select pair."
	//
	// So it isn't possible to validate the selected bank ID until
	// we receive both MSB and LSB so just store the MSB for now.
	m_nVoiceBankIDMSB[nTG] = nBankMSB;
}

void CMiniDexed::BankSelectMSBPerformance (unsigned nBankMSB)
{
	nBankMSB=constrain((int)nBankMSB,0,127);
	m_nVoiceBankIDMSBPerformance = nBankMSB;
}

void CMiniDexed::BankSelectLSB (unsigned nBankLSB, unsigned nTG)
{
	nBankLSB=constrain((int)nBankLSB,0,127);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	unsigned nBank = m_nVoiceBankID[nTG];
	unsigned nBankMSB = m_nVoiceBankIDMSB[nTG];
	nBank = (nBankMSB << 7) + nBankLSB;

	// Now should have both MSB and LSB so enable the BankSelect
	BankSelect(nBank, nTG);
}

void CMiniDexed::BankSelectLSBPerformance (unsigned nBankLSB)
{
	nBankLSB=constrain((int)nBankLSB,0,127);

	unsigned nBank = m_nVoiceBankIDPerformance;
	unsigned nBankMSB = m_nVoiceBankIDMSBPerformance;
	nBank = (nBankMSB << 7) + nBankLSB;

	// Now should have both MSB and LSB so enable the BankSelect
	BankSelectPerformance(nBank);
}

void CMiniDexed::ProgramChange (unsigned nProgram, unsigned nTG)
{
	assert (m_pConfig);

	unsigned nBankOffset;
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
		nProgram=constrain((int)nProgram,0,127);
		nBankOffset = nProgram >> 5;
		nProgram = nProgram % 32;
	}
	else
	{
		nBankOffset = 0;
		nProgram=constrain((int)nProgram,0,31);
	}

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nProgram[nTG] = nProgram;

	uint8_t Buffer[156];
	m_SysExFileLoader.GetVoice (m_nVoiceBankID[nTG]+nBankOffset, nProgram, Buffer);

	assert (m_pTG[nTG]);
	m_pTG[nTG]->loadVoiceParameters (Buffer);
	setOPMask(0b111111, nTG);

	if (m_pConfig->GetMIDIAutoVoiceDumpOnPC())
	{
		// Only do the voice dump back out over MIDI if we have a specific
		// MIDI channel configured for this TG
		if (m_nMIDIChannel[nTG] < CMIDIDevice::Channels)
		{
			m_SerialMIDI.SendSystemExclusiveVoice(nProgram, m_SerialMIDI.GetDeviceName(), 0, nTG);
		}
	}

	m_UI.ParameterChanged ();
}

void CMiniDexed::ProgramChangePerformance (unsigned nProgram)
{
	if (m_nParameter[ParameterPerformanceSelectChannel] != CMIDIDevice::Disabled)
	{
		// Program Change messages change Performances.
		if (m_PerformanceConfig.IsValidPerformance(nProgram))
		{
			SetNewPerformance(nProgram);
		}
		m_UI.ParameterChanged ();
	}
}

void CMiniDexed::SetVolume (unsigned nVolume, unsigned nTG)
{
	nVolume=constrain((int)nVolume,0,127);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nVolume[nTG] = nVolume;

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setGain ((m_nVolume[nTG] * m_nExpression[nTG]) / (127.0f * 127.0f));

	m_UI.ParameterChanged ();
}

void CMiniDexed::SetExpression (unsigned nExpression, unsigned nTG)
{
	nExpression=constrain((int)nExpression,0,127);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nExpression[nTG] = nExpression;

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setGain ((m_nVolume[nTG] * m_nExpression[nTG]) / (127.0f * 127.0f));

	// Expression is a "live performance" parameter only set
	// via MIDI and not via the UI.
	//m_UI.ParameterChanged ();
}

void CMiniDexed::SetPan (unsigned nPan, unsigned nTG)
{
	nPan=constrain((int)nPan,0,127);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nPan[nTG] = nPan;
	
	tg_mixer->pan(nTG,mapfloat(nPan,0,127,0.0f,1.0f));

	for (unsigned nFX = 0; nFX < CConfig::FXMixers; ++nFX)
	{
		sendfx_mixer[nFX]->pan(nTG,mapfloat(nPan,0,127,0.0f,1.0f));
	}

	m_UI.ParameterChanged ();
}

void CMiniDexed::SetFX1Send (unsigned nFX1Send, unsigned nTG)
{
	nFX1Send=constrain((int)nFX1Send,0,99);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG
	if (0 >= CConfig::FXMixers) return;

	m_nFX1Send[nTG] = nFX1Send;

	sendfx_mixer[0]->gain(nTG,mapfloat(nFX1Send,0,99,0.0f,1.0f));

	m_UI.ParameterChanged ();
}

void CMiniDexed::SetFX2Send (unsigned nFX2Send, unsigned nTG)
{
	nFX2Send=constrain((int)nFX2Send,0,99);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG
	if (1 >= CConfig::FXMixers) return;

	m_nFX2Send[nTG] = nFX2Send;

	sendfx_mixer[1]->gain(nTG,mapfloat(nFX2Send,0,99,0.0f,1.0f));

	m_UI.ParameterChanged ();
}

void CMiniDexed::SetMasterTune (int nMasterTune, unsigned nTG)
{
	nMasterTune=constrain((int)nMasterTune,-99,99);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nMasterTune[nTG] = nMasterTune;

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setMasterTune ((int8_t) nMasterTune);

	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCutoff (int nCutoff, unsigned nTG)
{
	nCutoff = constrain (nCutoff, 0, 99);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nCutoff[nTG] = nCutoff;

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setFilterCutoff (mapfloat (nCutoff, 0, 99, 0.0f, 1.0f));

	m_UI.ParameterChanged ();
}

void CMiniDexed::SetResonance (int nResonance, unsigned nTG)
{
	nResonance = constrain (nResonance, 0, 99);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	m_nResonance[nTG] = nResonance;

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setFilterResonance (mapfloat (nResonance, 0, 99, 0.0f, 1.0f));

	m_UI.ParameterChanged ();
}



void CMiniDexed::SetMIDIChannel (uint8_t uchChannel, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (uchChannel < CMIDIDevice::ChannelUnknown);

	m_nMIDIChannel[nTG] = uchChannel;

	for (unsigned i = 0; i < CConfig::MaxUSBMIDIDevices; i++)
	{
		assert (m_pMIDIKeyboard[i]);
		m_pMIDIKeyboard[i]->SetChannel (uchChannel, nTG);
	}

	m_PCKeyboard.SetChannel (uchChannel, nTG);

	if (m_bUseSerial)
	{
		m_SerialMIDI.SetChannel (uchChannel, nTG);
	}
	if (m_UDPMIDI)
	{
		m_UDPMIDI->SetChannel (uchChannel, nTG);
	}

#ifdef ARM_ALLOW_MULTI_CORE
/* This doesn't appear to be used anywhere...
	unsigned nActiveTGs = 0;
	for (unsigned nTG = 0; nTG < CConfig::ToneGenerators; nTG++)
	{
		if (m_nMIDIChannel[nTG] != CMIDIDevice::Disabled)
		{
			nActiveTGs++;
		}
	}

	assert (nActiveTGs <= 8);
	static const unsigned Log2[] = {0, 0, 1, 2, 2, 3, 3, 3, 3};
		m_nActiveTGsLog2 = Log2[nActiveTGs];
*/
#endif

	m_UI.ParameterChanged ();
}

void CMiniDexed::keyup (int16_t pitch, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	pitch = ApplyNoteLimits (pitch, nTG);
	if (pitch >= 0)
	{
		m_pTG[nTG]->keyup (pitch);
	}
}

void CMiniDexed::keydown (int16_t pitch, uint8_t velocity, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nLastKeyDown = pitch;

	pitch = ApplyNoteLimits (pitch, nTG);
	if (pitch >= 0)
	{
		m_pTG[nTG]->keydown (pitch, velocity);
	}
}

int16_t CMiniDexed::ApplyNoteLimits (int16_t pitch, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return -1;  // Not an active TG

	if (   pitch < (int16_t) m_nNoteLimitLow[nTG]
	    || pitch > (int16_t) m_nNoteLimitHigh[nTG])
	{
		return -1;
	}

	pitch += m_nNoteShift[nTG];

	if (   pitch < 0
	    || pitch > 127)
	{
		return -1;
	}

	return pitch;
}

void CMiniDexed::setSustain(bool sustain, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setSustain (sustain);

	// TODO: use the bus MIDI channel for sustain
	if (nTG == 0)
		for (unsigned i = 0; i < CConfig::FXChains; ++i)
			fx_chain[i]->zyn_sympathetic.sustain(sustain);
}

void CMiniDexed::setSostenuto(bool sostenuto, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setSostenuto (sostenuto);
}

void CMiniDexed::setHoldMode(bool holdmode, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setHold (holdmode);
}

void CMiniDexed::panic(uint8_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	if (value == 0) {
		m_pTG[nTG]->panic ();
	}
}

void CMiniDexed::notesOff(uint8_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	if (value == 0) {
		m_pTG[nTG]->notesOff ();
	}
}

void CMiniDexed::setModWheel (uint8_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setModWheel (value);
}


void CMiniDexed::setFootController (uint8_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setFootController (value);
}

void CMiniDexed::setBreathController (uint8_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setBreathController (value);
}

void CMiniDexed::setAftertouch (uint8_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setAftertouch (value);
}

void CMiniDexed::setPitchbend (int16_t value, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->setPitchbend (value);
}

void CMiniDexed::ControllersRefresh (unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_pTG[nTG]->ControllersRefresh ();
}

void CMiniDexed::SetParameter (TParameter Parameter, int nValue)
{
	assert (Parameter < ParameterUnknown);

	switch (Parameter)
	{
	case ParameterPerformanceSelectChannel:
		// Nothing more to do
		break;

	case ParameterPerformanceBank:
		BankSelectPerformance(nValue);
		break;

	case ParameterMasterVolume:
		nValue=constrain((int)nValue,0,127);
		setMasterVolume (nValue / 127.0f);
		m_UI.ParameterChanged ();
		break;

	case ParameterMixerDryLevel:
		nValue = constrain(nValue, 0, 99);
		tg_mixer->gain(mapfloat(nValue,0,99,0.0f,1.0f));
		break;

	case ParameterFXBypass:
		break;

	default:
		assert (0);
		break;
	}

	m_nParameter[Parameter] = nValue;
}

int CMiniDexed::GetParameter (TParameter Parameter)
{
	assert (Parameter < ParameterUnknown);
	return m_nParameter[Parameter];
}

void CMiniDexed::SetFXParameter (FX::TFXParameter Parameter, int nValue, unsigned nFX, bool bSaveOnly)
{
	assert (nFX < CConfig::FXChains);
	assert (Parameter < FX::FXParameterUnknown);

	const FX::FXParameterType &p = FX::s_FXParameter[Parameter];
	nValue = constrain((int)nValue, p.Minimum, p.Maximum);

	m_nFXParameter[nFX][Parameter] = nValue;

	if (bSaveOnly) return;

	switch (Parameter)
	{
	case FX::FXParameterSlot0:
	case FX::FXParameterSlot1:
	case FX::FXParameterSlot2:
		fx_chain[nFX]->setSlot(Parameter - FX::FXParameterSlot0, nValue);
		break;

	case FX::FXParameterZynDistortionPreset:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_distortion.loadpreset (nValue);
		m_FXSpinLock.Release ();
		break;
	
	case FX::FXParameterZynDistortionMix:
	case FX::FXParameterZynDistortionPanning:
	case FX::FXParameterZynDistortionDrive:
	case FX::FXParameterZynDistortionLevel:
	case FX::FXParameterZynDistortionType:
	case FX::FXParameterZynDistortionNegate:
	case FX::FXParameterZynDistortionFiltering:
	case FX::FXParameterZynDistortionLowcut:
	case FX::FXParameterZynDistortionHighcut:
	case FX::FXParameterZynDistortionStereo:
	case FX::FXParameterZynDistortionLRCross:
	case FX::FXParameterZynDistortionShape:
	case FX::FXParameterZynDistortionOffset:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_distortion.changepar (Parameter - FX::FXParameterZynDistortionMix, nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynDistortionBypass:
		fx_chain[nFX]->zyn_distortion.bypass = nValue;
		break;

	case FX::FXParameterYKChorusMix:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->yk_chorus.setMix (nValue / 100.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterYKChorusEnable1:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->yk_chorus.setChorus1 (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterYKChorusEnable2:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->yk_chorus.setChorus2 (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterYKChorusLFORate1:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->yk_chorus.setChorus1LFORate (nValue / 100.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterYKChorusLFORate2:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->yk_chorus.setChorus2LFORate (nValue / 100.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterYKChorusBypass:
		fx_chain[nFX]->yk_chorus.bypass = nValue;
		break;

	case FX::FXParameterZynChorusPreset:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_chorus.loadpreset (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynChorusMix:
	case FX::FXParameterZynChorusPanning:
	case FX::FXParameterZynChorusLFOFreq:
	case FX::FXParameterZynChorusLFORandomness:
	case FX::FXParameterZynChorusLFOType:
	case FX::FXParameterZynChorusLFOLRDelay:
	case FX::FXParameterZynChorusDepth:
	case FX::FXParameterZynChorusDelay:
	case FX::FXParameterZynChorusFeedback:
	case FX::FXParameterZynChorusLRCross:
	case FX::FXParameterZynChorusMode:
	case FX::FXParameterZynChorusSubtractive:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_chorus.changepar(Parameter - FX::FXParameterZynChorusMix, nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynChorusBypass:
		fx_chain[nFX]->zyn_chorus.bypass = nValue;
		break;

	case FX::FXParameterZynSympatheticPreset:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_sympathetic.loadpreset (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynSympatheticMix:
	case FX::FXParameterZynSympatheticPanning:
	case FX::FXParameterZynSympatheticQ:
	case FX::FXParameterZynSympatheticQSustain:
	case FX::FXParameterZynSympatheticDrive:
	case FX::FXParameterZynSympatheticLevel:
	case FX::FXParameterZynSympatheticType:
	case FX::FXParameterZynSympatheticUnisonSize:
	case FX::FXParameterZynSympatheticUnisonSpread:
	case FX::FXParameterZynSympatheticStrings:
	case FX::FXParameterZynSympatheticInterval:
	case FX::FXParameterZynSympatheticBaseNote:
	case FX::FXParameterZynSympatheticLowcut:
	case FX::FXParameterZynSympatheticHighcut:
	case FX::FXParameterZynSympatheticNegate:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_sympathetic.changepar(Parameter - FX::FXParameterZynSympatheticMix, nValue, true);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynSympatheticBypass:
		fx_chain[nFX]->zyn_sympathetic.bypass = nValue;
		break;

	case FX::FXParameterZynAPhaserPreset:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_aphaser.loadpreset (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynAPhaserMix:
	case FX::FXParameterZynAPhaserPanning:
	case FX::FXParameterZynAPhaserLFOFreq:
	case FX::FXParameterZynAPhaserLFORandomness:
	case FX::FXParameterZynAPhaserLFOType:
	case FX::FXParameterZynAPhaserLFOLRDelay:
	case FX::FXParameterZynAPhaserDepth:
	case FX::FXParameterZynAPhaserFeedback:
	case FX::FXParameterZynAPhaserStages:
	case FX::FXParameterZynAPhaserLRCross:
	case FX::FXParameterZynAPhaserSubtractive:
	case FX::FXParameterZynAPhaserWidth:
	case FX::FXParameterZynAPhaserDistortion:
	case FX::FXParameterZynAPhaserMismatch:
	case FX::FXParameterZynAPhaserHyper:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_aphaser.changepar(Parameter - FX::FXParameterZynAPhaserMix, nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynAPhaserBypass:
		fx_chain[nFX]->zyn_aphaser.bypass = nValue;
		break;

	case FX::FXParameterZynPhaserPreset:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_phaser.loadpreset (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynPhaserMix:
	case FX::FXParameterZynPhaserPanning:
	case FX::FXParameterZynPhaserLFOFreq:
	case FX::FXParameterZynPhaserLFORandomness:
	case FX::FXParameterZynPhaserLFOType:
	case FX::FXParameterZynPhaserLFOLRDelay:
	case FX::FXParameterZynPhaserDepth:
	case FX::FXParameterZynPhaserFeedback:
	case FX::FXParameterZynPhaserStages:
	case FX::FXParameterZynPhaserLRCross:
	case FX::FXParameterZynPhaserSubtractive:
	case FX::FXParameterZynPhaserPhase:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->zyn_phaser.changepar(Parameter - FX::FXParameterZynPhaserMix, nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterZynPhaserBypass:
		fx_chain[nFX]->zyn_phaser.bypass = nValue;
		break;

	case FX::FXParameterDreamDelayMix:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->dream_delay.setMix (nValue / 100.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayMode:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->dream_delay.setMode ((AudioEffectDreamDelay::Mode)nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayTime:
		SetFXParameter (FX::FXParameterDreamDelayTimeL, nValue, nFX);
		SetFXParameter (FX::FXParameterDreamDelayTimeR, nValue, nFX);
		break;

	case FX::FXParameterDreamDelayTimeL:
		m_FXSpinLock.Acquire ();

		if (nValue <= 100)
		{
			fx_chain[nFX]->dream_delay.setTimeL (nValue / 100.f);
			fx_chain[nFX]->dream_delay.setTimeLSync (AudioEffectDreamDelay::SYNC_NONE);
		}
		else
		{
			fx_chain[nFX]->dream_delay.setTimeLSync ((AudioEffectDreamDelay::Sync)(nValue - 100));
		}

		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayTimeR:
		m_FXSpinLock.Acquire ();

		if (nValue <= 100)
		{
			fx_chain[nFX]->dream_delay.setTimeR (nValue / 100.f);
			fx_chain[nFX]->dream_delay.setTimeRSync (AudioEffectDreamDelay::SYNC_NONE);
		}
		else
		{
			fx_chain[nFX]->dream_delay.setTimeRSync ((AudioEffectDreamDelay::Sync)(nValue - 100));
		}

		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayTempo:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->dream_delay.setTempo (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayFeedback:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->dream_delay.setFeedback (nValue / 100.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayHighCut:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->dream_delay.setHighCut (MIDI_EQ_HZ[nValue]);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterDreamDelayBypass:
		fx_chain[nFX]->dream_delay.bypass = nValue;
		break;

	case FX::FXParameterPlateReverbMix:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->plate_reverb.set_mix (nValue / 100.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterPlateReverbSize:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->plate_reverb.size (nValue / 99.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterPlateReverbHighDamp:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->plate_reverb.hidamp (nValue / 99.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterPlateReverbLowDamp:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->plate_reverb.lodamp (nValue / 99.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterPlateReverbLowPass:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->plate_reverb.lowpass (nValue / 99.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterPlateReverbDiffusion:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->plate_reverb.diffusion (nValue / 99.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterPlateReverbBypass:
		fx_chain[nFX]->plate_reverb.bypass = nValue;
		break;

	case FX::FXParameterCloudSeed2Preset:
		fx_chain[nFX]->cloudseed2.loadPreset (nValue);
		break;

	case FX::FXParameterCloudSeed2Interpolation:
	case FX::FXParameterCloudSeed2LowCutEnabled:
	case FX::FXParameterCloudSeed2HighCutEnabled:
	case FX::FXParameterCloudSeed2InputMix:
	case FX::FXParameterCloudSeed2LowCut:
	case FX::FXParameterCloudSeed2HighCut:
	case FX::FXParameterCloudSeed2DryOut:
	case FX::FXParameterCloudSeed2EarlyOut:
	case FX::FXParameterCloudSeed2LateOut:
	case FX::FXParameterCloudSeed2TapEnabled:
	case FX::FXParameterCloudSeed2TapCount:
	case FX::FXParameterCloudSeed2TapDecay:
	case FX::FXParameterCloudSeed2TapPredelay:
	case FX::FXParameterCloudSeed2TapLength:
	case FX::FXParameterCloudSeed2EarlyDiffuseEnabled:
	case FX::FXParameterCloudSeed2EarlyDiffuseCount:
	case FX::FXParameterCloudSeed2EarlyDiffuseDelay:
	case FX::FXParameterCloudSeed2EarlyDiffuseModAmount:
	case FX::FXParameterCloudSeed2EarlyDiffuseFeedback:
	case FX::FXParameterCloudSeed2EarlyDiffuseModRate:
	case FX::FXParameterCloudSeed2LateMode:
	case FX::FXParameterCloudSeed2LateLineCount:
	case FX::FXParameterCloudSeed2LateDiffuseEnabled:
	case FX::FXParameterCloudSeed2LateDiffuseCount:
	case FX::FXParameterCloudSeed2LateLineSize:
	case FX::FXParameterCloudSeed2LateLineModAmount:
	case FX::FXParameterCloudSeed2LateDiffuseDelay:
	case FX::FXParameterCloudSeed2LateDiffuseModAmount:
	case FX::FXParameterCloudSeed2LateLineDecay:
	case FX::FXParameterCloudSeed2LateLineModRate:
	case FX::FXParameterCloudSeed2LateDiffuseFeedback:
	case FX::FXParameterCloudSeed2LateDiffuseModRate:
	case FX::FXParameterCloudSeed2EqLowShelfEnabled:
	case FX::FXParameterCloudSeed2EqHighShelfEnabled:
	case FX::FXParameterCloudSeed2EqLowpassEnabled:
	case FX::FXParameterCloudSeed2EqLowFreq:
	case FX::FXParameterCloudSeed2EqHighFreq:
	case FX::FXParameterCloudSeed2EqCutoff:
	case FX::FXParameterCloudSeed2EqLowGain:
	case FX::FXParameterCloudSeed2EqHighGain:
	case FX::FXParameterCloudSeed2EqCrossSeed:
	case FX::FXParameterCloudSeed2SeedTap:
	case FX::FXParameterCloudSeed2SeedDiffusion:
	case FX::FXParameterCloudSeed2SeedDelay:
	case FX::FXParameterCloudSeed2SeedPostDiffusion:
		fx_chain[nFX]->cloudseed2.setParameter (Parameter - FX::FXParameterCloudSeed2Interpolation, mapfloat(nValue, p.Minimum, p.Maximum, 0.0f, 1.0f));
		break;

	case FX::FXParameterCloudSeed2Bypass:
		fx_chain[nFX]->cloudseed2.bypass = nValue;
		break;

	case FX::FXParameterCompressorPreGain:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.setPreGain_dB (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorThresh:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.setThresh_dBFS (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorRatio:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.setCompressionRatio (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorAttack:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.setAttack_sec ((nValue ?: 1) / 1000.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorRelease:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.setRelease_sec ((nValue ?: 1) / 1000.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorMakeupGain:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.setMakeupGain_dB (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorHPFilterEnable:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->compressor.enableHPFilter (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterCompressorBypass:
		fx_chain[nFX]->compressor.bypass = nValue;
		break;

	case FX::FXParameterEQLow:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->eq.setLow_dB (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQMid:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->eq.setMid_dB (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQHigh:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->eq.setHigh_dB (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQGain:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->eq.setGain_dB (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQLowMidFreq:
		m_FXSpinLock.Acquire ();
		m_nFXParameter[nFX][Parameter] = fx_chain[nFX]->eq.setLowMidFreq_n (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQMidHighFreq:
		m_FXSpinLock.Acquire ();
		m_nFXParameter[nFX][Parameter] = fx_chain[nFX]->eq.setMidHighFreq_n (nValue);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQPreLowCut:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->eq.setPreLowCut(MIDI_EQ_HZ[nValue]);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQPreHighCut:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->eq.setPreHighCut(MIDI_EQ_HZ[nValue]);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterEQBypass:
		fx_chain[nFX]->eq.bypass = nValue;
		break;

	case FX::FXParameterReturnLevel:
		m_FXSpinLock.Acquire ();
		fx_chain[nFX]->set_level (nValue / 99.0f);
		m_FXSpinLock.Release ();
		break;

	case FX::FXParameterBypass:
		fx_chain[nFX]->bypass = nValue;
		break;

	default:
		assert (0);
		break;
	}
}

int CMiniDexed::GetFXParameter (FX::TFXParameter Parameter, unsigned nFX)
{
	assert (nFX < CConfig::FXChains);
	assert (Parameter < FX::FXParameterUnknown);

	if (Parameter >= FX::FXParameterZynDistortionMix && Parameter <= FX::FXParameterZynDistortionOffset)
	{
		return fx_chain[nFX]->zyn_distortion.getpar (Parameter - FX::FXParameterZynDistortionMix);
	}

	if (Parameter >= FX::FXParameterZynChorusMix && Parameter <= FX::FXParameterZynChorusSubtractive)
	{
		return fx_chain[nFX]->zyn_chorus.getpar (Parameter - FX::FXParameterZynChorusMix);
	}

	if (Parameter >= FX::FXParameterZynSympatheticMix && Parameter <= FX::FXParameterZynSympatheticNegate)
	{
		return fx_chain[nFX]->zyn_sympathetic.getpar (Parameter - FX::FXParameterZynSympatheticMix);
	}

	if (Parameter >= FX::FXParameterZynAPhaserMix && Parameter <= FX::FXParameterZynAPhaserHyper)
	{
		return fx_chain[nFX]->zyn_aphaser.getpar (Parameter - FX::FXParameterZynAPhaserMix);
	}

	if (Parameter >= FX::FXParameterZynPhaserMix && Parameter <= FX::FXParameterZynPhaserPhase)
	{
		return fx_chain[nFX]->zyn_phaser.getpar (Parameter - FX::FXParameterZynPhaserMix);
	}

	if (Parameter >= FX::FXParameterCloudSeed2Interpolation && Parameter <= FX::FXParameterCloudSeed2SeedPostDiffusion)
	{
		const FX::FXParameterType &p = FX::s_FXParameter[Parameter];
		return mapfloat(fx_chain[nFX]->cloudseed2.getParameter (Parameter - FX::FXParameterCloudSeed2Interpolation), 0.0f, 1.0f, p.Minimum, p.Maximum);
	}

	return m_nFXParameter[nFX][Parameter];
}

void CMiniDexed::SetTGParameter (TTGParameter Parameter, int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);

	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	unsigned nTGLink = m_nTGLink[nTG];

	for (unsigned i = 0; i < m_nToneGenerators; i++)
	{
		if (i != nTG && (!nTGLink || m_nTGLink[i] != nTGLink))
			continue;

		if (i != nTG && Parameter == TGParameterTGLink)
			continue;

		if (i != nTG && Parameter == TGParameterPan)
			continue;

		if (i != nTG && Parameter == TGParameterMasterTune)
			continue;

		switch (Parameter)
		{
		case TGParameterVoiceBank:	BankSelect (nValue, i);	break;
		case TGParameterVoiceBankMSB:	BankSelectMSB (nValue, i);	break;
		case TGParameterVoiceBankLSB:	BankSelectLSB (nValue, i);	break;
		case TGParameterProgram:	ProgramChange (nValue, i);	break;
		case TGParameterVolume:		SetVolume (nValue, i);	break;
		case TGParameterPan:		SetPan (nValue, i);		break;
		case TGParameterMasterTune:	SetMasterTune (nValue, i);	break;
		case TGParameterCutoff:		SetCutoff (nValue, i);	break;
		case TGParameterResonance:	SetResonance (nValue, i);	break;
		case TGParameterPitchBendRange:	setPitchbendRange (nValue, i);	break;
		case TGParameterPitchBendStep:	setPitchbendStep (nValue, i);	break;
		case TGParameterPortamentoMode:		setPortamentoMode (nValue, i);	break;
		case TGParameterPortamentoGlissando:	setPortamentoGlissando (nValue, i);	break;
		case TGParameterPortamentoTime:		setPortamentoTime (nValue, i);	break;
		case TGParameterNoteLimitLow:		setNoteLimitLow (nValue, i);		break;
		case TGParameterNoteLimitHigh:		setNoteLimitHigh (nValue, i);		break;
		case TGParameterNoteShift:		setNoteShift (nValue, i);		break;
		case TGParameterMonoMode:		setMonoMode (nValue , i);	break; 
		case TGParameterTGLink:			setTGLink(nValue, i);		break;

		case TGParameterMWRange:					setModController(0, 0, nValue, i); break;
		case TGParameterMWPitch:					setModController(0, 1, nValue, i); break;
		case TGParameterMWAmplitude:				setModController(0, 2, nValue, i); break;
		case TGParameterMWEGBias:					setModController(0, 3, nValue, i); break;
		
		case TGParameterFCRange:					setModController(1, 0, nValue, i); break;
		case TGParameterFCPitch:					setModController(1, 1, nValue, i); break;
		case TGParameterFCAmplitude:				setModController(1, 2, nValue, i); break;
		case TGParameterFCEGBias:					setModController(1, 3, nValue, i); break;
		
		case TGParameterBCRange:					setModController(2, 0, nValue, i); break;
		case TGParameterBCPitch:					setModController(2, 1, nValue, i); break;
		case TGParameterBCAmplitude:				setModController(2, 2, nValue, i); break;
		case TGParameterBCEGBias:					setModController(2, 3, nValue, i); break;
		
		case TGParameterATRange:					setModController(3, 0, nValue, i); break;
		case TGParameterATPitch:					setModController(3, 1, nValue, i); break;
		case TGParameterATAmplitude:				setModController(3, 2, nValue, i); break;
		case TGParameterATEGBias:					setModController(3, 3, nValue, i); break;
		
		
		case TGParameterMIDIChannel:
			assert (0 <= nValue && nValue <= 255);
			SetMIDIChannel ((uint8_t) nValue, i);
			break;

		case TGParameterFX1Send:			SetFX1Send (nValue, i); break;
		case TGParameterFX2Send:			SetFX2Send (nValue, i); break;

		case TGParameterCompressorEnable:	SetCompressorEnable (nValue, i); break;
		case TGParameterCompressorPreGain:	SetCompressorPreGain (nValue, i); break;
		case TGParameterCompressorThresh:	SetCompressorThresh (nValue, i); break;
		case TGParameterCompressorRatio:	SetCompressorRatio (nValue, i); break;
		case TGParameterCompressorAttack:	SetCompressorAttack (nValue, i); break;
		case TGParameterCompressorRelease:	SetCompressorRelease (nValue, i); break;
		case TGParameterCompressorMakeupGain:	SetCompressorMakeupGain (nValue, i); break;

		case TGParameterEQLow:			SetEQLow (nValue, i); break;
		case TGParameterEQMid:			SetEQMid (nValue, i); break;
		case TGParameterEQHigh:			SetEQHigh (nValue, i); break;
		case TGParameterEQGain:			SetEQGain (nValue, i); break;
		case TGParameterEQLowMidFreq:		SetEQLowMidFreq (nValue, i); break;
		case TGParameterEQMidHighFreq:		SetEQMidHighFreq (nValue, i); break;
		case TGParameterEQPreLowcut:		SetEQPreLowcut (nValue, i); break;
		case TGParameterEQPreHighcut:		SetEQPreHighcut (nValue, i); break;

		default:
			assert (0);
			break;
		}
	}
}

int CMiniDexed::GetTGParameter (TTGParameter Parameter, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);

	switch (Parameter)
	{
	case TGParameterVoiceBank:	return m_nVoiceBankID[nTG];
	case TGParameterVoiceBankMSB:	return m_nVoiceBankID[nTG] >> 7;
	case TGParameterVoiceBankLSB:	return m_nVoiceBankID[nTG] & 0x7F;
	case TGParameterProgram:	return m_nProgram[nTG];
	case TGParameterVolume:		return m_nVolume[nTG];
	case TGParameterPan:		return m_nPan[nTG];
	case TGParameterMasterTune:	return m_nMasterTune[nTG];
	case TGParameterCutoff:		return m_nCutoff[nTG];
	case TGParameterResonance:	return m_nResonance[nTG];
	case TGParameterMIDIChannel:	return m_nMIDIChannel[nTG];
	case TGParameterFX1Send:	return m_nFX1Send[nTG];
	case TGParameterFX2Send:	return m_nFX2Send[nTG];
	case TGParameterPitchBendRange:	return m_nPitchBendRange[nTG];
	case TGParameterPitchBendStep:	return m_nPitchBendStep[nTG];
	case TGParameterPortamentoMode:		return m_nPortamentoMode[nTG];
	case TGParameterPortamentoGlissando:	return m_nPortamentoGlissando[nTG];
	case TGParameterPortamentoTime:		return m_nPortamentoTime[nTG];
	case TGParameterNoteLimitLow:		return m_nNoteLimitLow[nTG];
	case TGParameterNoteLimitHigh:		return m_nNoteLimitHigh[nTG];
	case TGParameterNoteShift:		return m_nNoteShift[nTG];
	case TGParameterMonoMode:		return m_bMonoMode[nTG] ? 1 : 0; 

	case TGParameterTGLink:			return m_nTGLink[nTG];
	
	case TGParameterMWRange:					return getModController(0, 0, nTG);
	case TGParameterMWPitch:					return getModController(0, 1, nTG);
	case TGParameterMWAmplitude:				return getModController(0, 2, nTG); 
	case TGParameterMWEGBias:					return getModController(0, 3, nTG); 
	
	case TGParameterFCRange:					return getModController(1, 0,  nTG); 
	case TGParameterFCPitch:					return getModController(1, 1,  nTG); 
	case TGParameterFCAmplitude:				return getModController(1, 2,  nTG); 
	case TGParameterFCEGBias:					return getModController(1, 3,  nTG); 
	
	case TGParameterBCRange:					return getModController(2, 0,  nTG); 
	case TGParameterBCPitch:					return getModController(2, 1,  nTG); 
	case TGParameterBCAmplitude:				return getModController(2, 2,  nTG); 
	case TGParameterBCEGBias:					return getModController(2, 3,  nTG); 
	
	case TGParameterATRange:					return getModController(3, 0,  nTG); 
	case TGParameterATPitch:					return getModController(3, 1,  nTG); 
	case TGParameterATAmplitude:				return getModController(3, 2,  nTG); 
	case TGParameterATEGBias:					return getModController(3, 3,  nTG); 
	
	case TGParameterCompressorEnable:	return m_bCompressorEnable[nTG];
	case TGParameterCompressorPreGain:	return m_nCompressorPreGain[nTG];
	case TGParameterCompressorThresh:	return m_nCompressorThresh[nTG];
	case TGParameterCompressorRatio:	return m_nCompressorRatio[nTG];	
	case TGParameterCompressorAttack:	return m_nCompressorAttack[nTG];
	case TGParameterCompressorRelease:	return m_nCompressorRelease[nTG];
	case TGParameterCompressorMakeupGain:	return m_nCompressorMakeupGain[nTG];

	case TGParameterEQLow:			return m_nEQLow[nTG]; break;
	case TGParameterEQMid:			return m_nEQMid[nTG]; break;
	case TGParameterEQHigh:			return m_nEQHigh[nTG]; break;
	case TGParameterEQGain:			return m_nEQGain[nTG]; break;
	case TGParameterEQLowMidFreq:		return m_nEQLowMidFreq[nTG]; break;
	case TGParameterEQMidHighFreq:		return m_nEQMidHighFreq[nTG]; break;
	case TGParameterEQPreLowcut:		return m_nEQPreLowcut[nTG]; break;
	case TGParameterEQPreHighcut:		return m_nEQPreHighcut[nTG]; break;

	default:
		assert (0);
		return 0;
	}
}

void CMiniDexed::SetVoiceParameter (uint8_t uchOffset, uint8_t uchValue, unsigned nOP, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	assert (nOP <= 6);

	if (nOP < 6)
	{
		nOP = 5 - nOP;		// OPs are in reverse order
	}

	unsigned nTGLink = m_nTGLink[nTG];

	for (unsigned i = 0; i < m_nToneGenerators; i++)
	{
		if (i != nTG && (!nTGLink || m_nTGLink[i] != nTGLink))
			continue;

		if (nOP < 6)
		{
			if (uchOffset == DEXED_OP_ENABLE)
			{
				if (uchValue)
				{
					setOPMask(m_uchOPMask[i] | 1 << nOP, i);
				}
				else
				{
					setOPMask(m_uchOPMask[i] & ~(1 << nOP), i);
				}


				continue;
			}		
		}

		uint8_t offset = uchOffset + nOP * 21;
		assert (offset < 156);

		m_pTG[i]->setVoiceDataElement (offset, uchValue);
	}
}

uint8_t CMiniDexed::GetVoiceParameter (uint8_t uchOffset, unsigned nOP, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return 0;  // Not an active TG

	assert (m_pTG[nTG]);
	assert (nOP <= 6);

	if (nOP < 6)
	{
		nOP = 5 - nOP;		// OPs are in reverse order

		if (uchOffset == DEXED_OP_ENABLE)
		{
			return !!(m_uchOPMask[nTG] & (1 << nOP));
		}
	}

	uchOffset += nOP * 21;
	assert (uchOffset < 156);

	return m_pTG[nTG]->getVoiceDataElement (uchOffset);
}

std::string CMiniDexed::GetVoiceName (unsigned nTG)
{
	char VoiceName[11];
	memset (VoiceName, 0, sizeof VoiceName);
	VoiceName[0] = 32; // space
	assert (nTG < CConfig::AllToneGenerators);

	if (nTG < m_nToneGenerators)
	{
		assert (m_pTG[nTG]);
		m_pTG[nTG]->getName (VoiceName);
	}
	std::string Result (VoiceName);
	return Result;
}

#ifndef ARM_ALLOW_MULTI_CORE

void CMiniDexed::ProcessSound (void)
{
	assert (m_pSoundDevice);

	unsigned nFrames = m_nQueueSizeFrames - m_pSoundDevice->GetQueueFramesAvail ();
	if (nFrames >= m_nQueueSizeFrames/2)
	{
		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Start ();
		}

		float32_t SampleBuffer[nFrames];
		m_pTG[0]->getSamples (SampleBuffer, nFrames);

		// Convert single float array (mono) to int16 array
		int32_t tmp_int[nFrames];
		arm_float_to_q23(SampleBuffer,tmp_int,nFrames);

		if (m_pSoundDevice->Write (tmp_int, sizeof(tmp_int)) != (int) sizeof(tmp_int))
		{
			LOGERR ("Sound data dropped");
		}

		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Stop ();
		}
	}
}

#else	// #ifdef ARM_ALLOW_MULTI_CORE

void CMiniDexed::ProcessSound (void)
{
	assert (m_pSoundDevice);
	assert (m_pConfig);

	unsigned nFrames = m_nQueueSizeFrames - m_pSoundDevice->GetQueueFramesAvail ();
	if (nFrames >= m_nQueueSizeFrames/2)
	{
		// only process the minimum number of frames (== chunksize / 2)
		// as the tg_mixer cannot process more
		nFrames = m_nQueueSizeFrames / 2;

		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Start ();
		}

		m_nFramesToProcess = nFrames;

		// kick secondary cores
		for (unsigned nCore = 2; nCore < CORES; nCore++)
		{
			assert (m_CoreStatus[nCore] == CoreStatusIdle);
			m_CoreStatus[nCore] = CoreStatusBusy;
			SendIPI (nCore, IPI_USER);
		}

		// process the TGs assigned to core 1
		assert (nFrames <= CConfig::MaxChunkSize);
		for (unsigned i = 0; i < m_pConfig->GetTGsCore1(); i++)
		{
			assert (m_pTG[i]);
			m_pTG[i]->getSamples (m_OutputLevel[i], nFrames);
		}

		// wait for cores 2 and 3 to complete their work
		for (unsigned nCore = 2; nCore < CORES; nCore++)
		{
			while (m_CoreStatus[nCore] != CoreStatusIdle)
			{
				WaitForEvent ();
			}
		}

		//
		// Audio signal path after tone generators starts here
		//

		if (m_bQuadDAC8Chan) {
			// This is only supported when there are 8 TGs
			assert (m_nToneGenerators == 8);

			// No mixing is performed by MiniDexed, sound is output in 8 channels.
			// Note: one TG per audio channel; output=mono; no processing.
			const int Channels = 8;  // One TG per channel
			float32_t tmp_float[nFrames*Channels];
			int32_t tmp_int[nFrames*Channels];

			// Convert dual float array (8 chan) to single int16 array (8 chan)
			for(uint16_t i=0; i<nFrames;i++)
			{
				// TGs will alternate on L/R channels for each output
				// reading directly from the TG OutputLevel buffer with
				// no additional processing.
				for (uint8_t tg = 0; tg < Channels; tg++)
				{
					tmp_float[(i*Channels)+tg]=m_OutputLevel[tg][i] * m_fMasterVolumeW;
				}
			}

			arm_float_to_q23(tmp_float,tmp_int,nFrames*Channels);

			// Prevent PCM510x analog mute from kicking in
			for (uint8_t tg = 0; tg < Channels; tg++) 
			{
				if (tmp_int[(nFrames - 1) * Channels + tg] == 0)
				{
					tmp_int[(nFrames - 1) * Channels + tg]++;
				}
			}
			
			if (m_pSoundDevice->Write (tmp_int, sizeof(tmp_int)) != (int) sizeof(tmp_int))
			{
				LOGERR ("Sound data dropped");
			}
		}
		else
		{
			// Mix everything down to stereo		
			uint8_t indexL=0, indexR=1;

			// BEGIN TG mixing
			float32_t tmp_float[nFrames*2];
			int32_t tmp_int[nFrames*2];

			// get the mix buffer of all TGs
			float32_t *SampleBuffer[2];
			tg_mixer->getBuffers(SampleBuffer);

			tg_mixer->zeroFill();

			for (uint8_t i = 0; i < m_nToneGenerators; i++)
			{
				tg_mixer->doAddMix(i,m_OutputLevel[i]);
			}
			// END TG mixing

			// BEGIN adding sendFX
			float32_t *FXSendBuffer[2];

			for (unsigned nFX = 0; nFX < CConfig::FXMixers; ++nFX)
			{
				if (fx_chain[nFX]->get_level() == 0.0f) continue;

				sendfx_mixer[nFX]->getBuffers(FXSendBuffer);
				sendfx_mixer[nFX]->zeroFill();

				for (uint8_t i = 0; i < m_nToneGenerators; i++)
				{
					sendfx_mixer[nFX]->doAddMix(i,m_OutputLevel[i]);
				}

				if (!m_nParameter[ParameterFXBypass])
				{
					m_FXSpinLock.Acquire ();
					fx_chain[nFX]->process(FXSendBuffer[0], FXSendBuffer[1], nFrames);
					m_FXSpinLock.Release ();
				}

				arm_add_f32(SampleBuffer[0], FXSendBuffer[0], SampleBuffer[0], nFrames);
				arm_add_f32(SampleBuffer[1], FXSendBuffer[1], SampleBuffer[1], nFrames);
			}
			// END adding sendFX

			if (!m_nParameter[ParameterFXBypass])
			{
				m_FXSpinLock.Acquire ();
				fx_chain[CConfig::MasterFX]->process(SampleBuffer[0], SampleBuffer[1], nFrames);
				m_FXSpinLock.Release ();
			}

			// swap stereo channels if needed prior to writing back out
			if (m_bChannelsSwapped)
			{
				indexL=1;
				indexR=0;
			}

			// Convert dual float array (left, right) to single int32 (q23) array (left/right)
			if (m_bVolRampDownWait)
			{
				float targetVol = 0.0f;

				scale_ramp_f32(SampleBuffer[0], &m_fMasterVolume[0], targetVol, m_fRamp, SampleBuffer[0], nFrames);
				scale_ramp_f32(SampleBuffer[1], &m_fMasterVolume[1], targetVol, m_fRamp, SampleBuffer[1], nFrames);
				arm_zip_f32(SampleBuffer[indexL], SampleBuffer[indexR], tmp_float, nFrames);

				if (targetVol == m_fMasterVolume[0] && targetVol == m_fMasterVolume[1])
				{
					m_bVolRampDownWait = false;
					m_bVolRampedDown = true;
				}
			}
			else if (m_bVolRampedDown)
			{
				arm_scale_zip_f32(SampleBuffer[indexL], SampleBuffer[indexR], 0.0f, tmp_float, nFrames);
			}
			else if (m_fMasterVolume[0] == m_fMasterVolumeW && m_fMasterVolume[1] == m_fMasterVolumeW)
			{
				arm_scale_zip_f32(SampleBuffer[indexL], SampleBuffer[indexR], m_fMasterVolumeW, tmp_float, nFrames);
			}
			else
			{
				scale_ramp_f32(SampleBuffer[0], &m_fMasterVolume[0], m_fMasterVolumeW, m_fRamp, SampleBuffer[0], nFrames);
				scale_ramp_f32(SampleBuffer[1], &m_fMasterVolume[1], m_fMasterVolumeW, m_fRamp, SampleBuffer[1], nFrames);
				arm_zip_f32(SampleBuffer[indexL], SampleBuffer[indexR], tmp_float, nFrames);
			}

			arm_float_to_q23(tmp_float,tmp_int,nFrames*2);

			// Prevent PCM510x analog mute from kicking in
			if (tmp_int[nFrames * 2 - 1] == 0)
			{
				tmp_int[nFrames * 2 - 1]++;
			}
			
			if (m_pSoundDevice->Write (tmp_int, sizeof(tmp_int)) != (int) sizeof(tmp_int))
			{
				LOGERR ("Sound data dropped");
			}
		} // End of Stereo mixing

		if (m_bProfileEnabled)
		{
			m_GetChunkTimer.Stop ();
		}
	}
}

#endif

unsigned CMiniDexed::GetPerformanceSelectChannel (void)
{
	// Stores and returns Select Channel using MIDI Device Channel definitions
	return (unsigned) GetParameter (ParameterPerformanceSelectChannel);
}

void CMiniDexed::SetPerformanceSelectChannel (unsigned uCh)
{
	// Turns a configuration setting to MIDI Device Channel definitions
	// Mirrors the logic in Performance Config for handling MIDI channel configuration
	if (uCh == 0)
	{
		SetParameter (ParameterPerformanceSelectChannel, CMIDIDevice::Disabled);
	}
	else if (uCh < CMIDIDevice::Channels)
	{
		SetParameter (ParameterPerformanceSelectChannel, uCh - 1);
	}
	else
	{
		SetParameter (ParameterPerformanceSelectChannel, CMIDIDevice::OmniMode);
	}
}

bool CMiniDexed::SavePerformance (bool bSaveAsDeault)
{
	if (m_PerformanceConfig.GetInternalFolderOk())
	{
		m_bSavePerformance = true;
		m_bSaveAsDeault=bSaveAsDeault;

		return true;
	}
	else
	{
		return false;
	}
}

bool CMiniDexed::DoSavePerformance (void)
{
	for (unsigned nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
	{
		m_PerformanceConfig.SetBankNumber (m_nVoiceBankID[nTG], nTG);
		m_PerformanceConfig.SetVoiceNumber (m_nProgram[nTG], nTG);
		m_PerformanceConfig.SetMIDIChannel (m_nMIDIChannel[nTG], nTG);
		m_PerformanceConfig.SetVolume (m_nVolume[nTG], nTG);
		m_PerformanceConfig.SetPan (m_nPan[nTG], nTG);
		m_PerformanceConfig.SetDetune (m_nMasterTune[nTG], nTG);
		m_PerformanceConfig.SetCutoff (m_nCutoff[nTG], nTG);
		m_PerformanceConfig.SetResonance (m_nResonance[nTG], nTG);
		m_PerformanceConfig.SetPitchBendRange (m_nPitchBendRange[nTG], nTG);
		m_PerformanceConfig.SetPitchBendStep	(m_nPitchBendStep[nTG], nTG);
		m_PerformanceConfig.SetPortamentoMode (m_nPortamentoMode[nTG], nTG);
		m_PerformanceConfig.SetPortamentoGlissando (m_nPortamentoGlissando[nTG], nTG);
		m_PerformanceConfig.SetPortamentoTime (m_nPortamentoTime[nTG], nTG);

		m_PerformanceConfig.SetNoteLimitLow (m_nNoteLimitLow[nTG], nTG);
		m_PerformanceConfig.SetNoteLimitHigh (m_nNoteLimitHigh[nTG], nTG);
		m_PerformanceConfig.SetNoteShift (m_nNoteShift[nTG], nTG);
		if (nTG < m_pConfig->GetToneGenerators())
		{
			m_pTG[nTG]->getVoiceData(m_nRawVoiceData);
		} else {
			// Not an active TG so provide default voice by asking for an invalid voice ID.
			m_SysExFileLoader.GetVoice(CSysExFileLoader::MaxVoiceBankID, CSysExFileLoader::VoicesPerBank+1, m_nRawVoiceData);
		}
		m_PerformanceConfig.SetVoiceDataToTxt (m_nRawVoiceData, nTG); 
		m_PerformanceConfig.SetMonoMode (m_bMonoMode[nTG], nTG); 
		m_PerformanceConfig.SetTGLink (m_nTGLink[nTG], nTG);

		m_PerformanceConfig.SetModulationWheelRange (m_nModulationWheelRange[nTG], nTG);
		m_PerformanceConfig.SetModulationWheelTarget (m_nModulationWheelTarget[nTG], nTG);
		m_PerformanceConfig.SetFootControlRange (m_nFootControlRange[nTG], nTG);
		m_PerformanceConfig.SetFootControlTarget (m_nFootControlTarget[nTG], nTG);
		m_PerformanceConfig.SetBreathControlRange (m_nBreathControlRange[nTG], nTG);
		m_PerformanceConfig.SetBreathControlTarget (m_nBreathControlTarget[nTG], nTG);
		m_PerformanceConfig.SetAftertouchRange (m_nAftertouchRange[nTG], nTG);
		m_PerformanceConfig.SetAftertouchTarget (m_nAftertouchTarget[nTG], nTG);
		
		m_PerformanceConfig.SetFX1Send (m_nFX1Send[nTG], nTG);
		m_PerformanceConfig.SetFX2Send (m_nFX2Send[nTG], nTG);

		m_PerformanceConfig.SetCompressorEnable (m_bCompressorEnable[nTG], nTG);
		m_PerformanceConfig.SetCompressorPreGain (m_nCompressorPreGain[nTG], nTG);
		m_PerformanceConfig.SetCompressorThresh (m_nCompressorThresh[nTG], nTG);
		m_PerformanceConfig.SetCompressorRatio (m_nCompressorRatio[nTG], nTG);		
		m_PerformanceConfig.SetCompressorAttack (m_nCompressorAttack[nTG], nTG);
		m_PerformanceConfig.SetCompressorRelease (m_nCompressorRelease[nTG], nTG);	
		m_PerformanceConfig.SetCompressorMakeupGain (m_nCompressorMakeupGain[nTG], nTG);

		m_PerformanceConfig.SetEQLow (m_nEQLow[nTG], nTG);
		m_PerformanceConfig.SetEQMid (m_nEQMid[nTG], nTG);
		m_PerformanceConfig.SetEQHigh (m_nEQHigh[nTG], nTG);
		m_PerformanceConfig.SetEQGain (m_nEQGain[nTG], nTG);
		m_PerformanceConfig.SetEQLowMidFreq (m_nEQLowMidFreq[nTG], nTG);
		m_PerformanceConfig.SetEQMidHighFreq (m_nEQMidHighFreq[nTG], nTG);
		m_PerformanceConfig.SetEQPreLowcut (m_nEQPreLowcut[nTG], nTG);
		m_PerformanceConfig.SetEQPreHighcut (m_nEQPreHighcut[nTG], nTG);
	}

	for (unsigned nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		for (unsigned nParam = 0; nParam < FX::FXParameterUnknown; ++nParam)
		{
			FX::TFXParameter param = FX::TFXParameter (nParam);
			m_PerformanceConfig.SetFXParameter (param, GetFXParameter (param, nFX), nFX);
		}
	}

	m_PerformanceConfig.SetMixerDryLevel (m_nParameter[ParameterMixerDryLevel]);
	m_PerformanceConfig.SetFXBypass (m_nParameter[ParameterFXBypass]);

	if(m_bSaveAsDeault)
	{
		m_PerformanceConfig.SetNewPerformanceBank(0);
		m_PerformanceConfig.SetNewPerformance(0);
		
	}
	return m_PerformanceConfig.Save ();
}

void CMiniDexed::SetCompressorEnable(bool compressor, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_bCompressorEnable[nTG] = compressor;
	m_pTG[nTG]->setCompressorEnable (compressor);
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCompressorPreGain (int preGain, unsigned nTG)
{
	preGain = constrain (preGain, -20, 20);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nCompressorPreGain[nTG] = preGain;
	m_pTG[nTG]->Compr.setPreGain_dB (preGain);
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCompressorThresh (int thresh, unsigned nTG)
{
	thresh = constrain (thresh, -60, 0);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nCompressorThresh[nTG] = thresh;
	m_pTG[nTG]->Compr.setThresh_dBFS (thresh);
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCompressorRatio (unsigned ratio, unsigned nTG)
{
	ratio = constrain (ratio, 1u, CMiniDexed::CompressorRatioInf);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nCompressorRatio[nTG] = ratio;
	m_pTG[nTG]->Compr.setCompressionRatio (ratio == CompressorRatioInf ? INFINITY : ratio);
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCompressorAttack (unsigned attack, unsigned nTG)
{
	attack = constrain (attack, 0u, 1000u);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nCompressorAttack[nTG] = attack;
	m_pTG[nTG]->Compr.setAttack_sec ((attack ?: 1) / 1000.0f, m_pConfig->GetSampleRate ());
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCompressorRelease (unsigned release, unsigned nTG)
{
	release = constrain (release, 0u, 2000u);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nCompressorRelease[nTG] = release;
	m_pTG[nTG]->Compr.setRelease_sec ((release ?: 1) / 1000.0, m_pConfig->GetSampleRate ());
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetCompressorMakeupGain (int makeupGain, unsigned nTG)
{
	makeupGain = constrain (makeupGain, -20, 20);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nCompressorMakeupGain[nTG] = makeupGain;
	m_pTG[nTG]->Compr.setMakeupGain_dB (makeupGain);
	m_UI.ParameterChanged ();
}

void CMiniDexed::SetEQLow (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQLow[nTG] = nValue;
	m_pTG[nTG]->EQ.setLow_dB(nValue);
}

void CMiniDexed::SetEQMid (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQMid[nTG] = nValue;
	m_pTG[nTG]->EQ.setMid_dB(nValue);
}

void CMiniDexed::SetEQHigh (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQHigh[nTG] = nValue;
	m_pTG[nTG]->EQ.setHigh_dB(nValue);
}

void CMiniDexed::SetEQGain (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, -24, 24);
	m_nEQGain[nTG] = nValue;
	m_pTG[nTG]->EQ.setGain_dB(nValue);
}

void CMiniDexed::SetEQLowMidFreq (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, 0u, 46u);
	m_nEQLowMidFreq[nTG] = m_pTG[nTG]->EQ.setLowMidFreq_n(nValue);
}

void CMiniDexed::SetEQMidHighFreq (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, 28u, 59u);
	m_nEQMidHighFreq[nTG] = m_pTG[nTG]->EQ.setMidHighFreq_n(nValue);
}

void CMiniDexed::SetEQPreLowcut (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, 0u, 60u);
	m_nEQPreLowcut[nTG] = nValue;
	m_pTG[nTG]->EQ.setPreLowCut(MIDI_EQ_HZ[nValue]);
}

void CMiniDexed::SetEQPreHighcut (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	nValue = constrain(nValue, 0u, 60u);
	m_nEQPreHighcut[nTG] = nValue;
	m_pTG[nTG]->EQ.setPreHighCut(MIDI_EQ_HZ[nValue]);
}

void CMiniDexed::setMonoMode(uint8_t mono, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_bMonoMode[nTG]= mono != 0; 
	m_pTG[nTG]->setMonoMode(constrain(mono, 0, 1));
	m_pTG[nTG]->doRefreshVoice();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setTGLink(uint8_t nTGLink, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nTGLink[nTG]= constrain(nTGLink, 0, 4);
	m_UI.ParameterChanged ();
}

void CMiniDexed::setPitchbendRange(uint8_t range, uint8_t nTG)
{
	range = constrain (range, 0, 12);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nPitchBendRange[nTG] = range;
	
	m_pTG[nTG]->setPitchbendRange(range);
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setPitchbendStep(uint8_t step, uint8_t nTG)
{
	step= constrain (step, 0, 12);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nPitchBendStep[nTG] = step;
	
	m_pTG[nTG]->setPitchbendStep(step);
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setPortamentoMode(uint8_t mode, uint8_t nTG)
{
	mode= constrain (mode, 0, 1);

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nPortamentoMode[nTG] = mode;
	
	m_pTG[nTG]->setPortamentoMode(mode);
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setPortamentoGlissando(uint8_t glissando, uint8_t nTG)
{
	glissando = constrain (glissando, 0, 1);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nPortamentoGlissando[nTG] = glissando;
	
	m_pTG[nTG]->setPortamentoGlissando(glissando);
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setPortamentoTime(uint8_t time, uint8_t nTG)
{
	time = constrain (time, 0, 99);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	m_nPortamentoTime[nTG] = time;
	
	m_pTG[nTG]->setPortamentoTime(time);
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setNoteLimitLow(unsigned limit, uint8_t nTG)
{
	limit = constrain (limit, 0u, 127u);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nNoteLimitLow[nTG] = limit;

	// reset all notes, so they don't stay down
	m_pTG[nTG]->deactivate();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setNoteLimitHigh(unsigned limit, uint8_t nTG)
{
	limit = constrain (limit, 0u, 127u);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nNoteLimitHigh[nTG] = limit;

	// reset all notes, so they don't stay down
	m_pTG[nTG]->deactivate();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setNoteShift(int shift, uint8_t nTG)
{
	shift = constrain (shift, -24, 24);
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nNoteShift[nTG] = shift;

	// reset all notes, so they don't stay down
	m_pTG[nTG]->deactivate();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setModWheelRange(uint8_t range, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nModulationWheelRange[nTG] = range;
	m_pTG[nTG]->setMWController(range, m_pTG[nTG]->getModWheelTarget(), 0);
//	m_pTG[nTG]->setModWheelRange(constrain(range, 0, 99));  replaces with the above due to wrong constrain on dexed_synth module. 

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setModWheelTarget(uint8_t target, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nModulationWheelTarget[nTG] = target;

	m_pTG[nTG]->setModWheelTarget(constrain(target, 0, 7));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setFootControllerRange(uint8_t range, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nFootControlRange[nTG]=range;
	m_pTG[nTG]->setFCController(range, m_pTG[nTG]->getFootControllerTarget(), 0);
//	m_pTG[nTG]->setFootControllerRange(constrain(range, 0, 99));  replaces with the above due to wrong constrain on dexed_synth module. 

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setFootControllerTarget(uint8_t target, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nFootControlTarget[nTG] = target;

	m_pTG[nTG]->setFootControllerTarget(constrain(target, 0, 7));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setBreathControllerRange(uint8_t range, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nBreathControlRange[nTG]=range;
	m_pTG[nTG]->setBCController(range, m_pTG[nTG]->getBreathControllerTarget(), 0);
	//m_pTG[nTG]->setBreathControllerRange(constrain(range, 0, 99));

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setBreathControllerTarget(uint8_t target, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nBreathControlTarget[nTG] = target;

	m_pTG[nTG]->setBreathControllerTarget(constrain(target, 0, 7));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setAftertouchRange(uint8_t range, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nAftertouchRange[nTG]=range;
	m_pTG[nTG]->setATController(range, m_pTG[nTG]->getAftertouchTarget(), 0);
//	m_pTG[nTG]->setAftertouchRange(constrain(range, 0, 99));

	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::setAftertouchTarget(uint8_t target, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_nAftertouchTarget[nTG] = target;

	m_pTG[nTG]->setAftertouchTarget(constrain(target, 0, 7));
	m_pTG[nTG]->ControllersRefresh();
	m_UI.ParameterChanged ();
}

void CMiniDexed::loadVoiceParameters(const uint8_t* data, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	uint8_t voice[161];

	memcpy(voice, data, sizeof(uint8_t)*161);

	// fix voice name
	for (uint8_t i = 0; i < 10; i++)
	{
		if (voice[151 + i] > 126) // filter characters
			voice[151 + i] = 32;
	}

	m_pTG[nTG]->loadVoiceParameters(&voice[6]);
	m_pTG[nTG]->doRefreshVoice();
	setOPMask(0b111111, nTG);

	m_UI.ParameterChanged ();
}

void CMiniDexed::setVoiceDataElement(uint8_t data, uint8_t number, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);

	m_pTG[nTG]->setVoiceDataElement(constrain(data, 0, 155),constrain(number, 0, 99));
	m_UI.ParameterChanged ();
}

int16_t CMiniDexed::checkSystemExclusive(const uint8_t* pMessage,const  uint16_t nLength, uint8_t nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return 0;  // Not an active TG

	assert (m_pTG[nTG]);

	return(m_pTG[nTG]->checkSystemExclusive(pMessage, nLength));
}

void CMiniDexed::getSysExVoiceDump(uint8_t* dest, uint8_t nTG)
{
	uint8_t checksum = 0;
	uint8_t data[155];

	assert (nTG < CConfig::AllToneGenerators);
	if (nTG < m_nToneGenerators)
	{
		assert (m_pTG[nTG]);
		m_pTG[nTG]->getVoiceData(data);
	}
	else
	{
		// Not an active TG so grab a default voice
		m_SysExFileLoader.GetVoice(CSysExFileLoader::MaxVoiceBankID, CSysExFileLoader::VoicesPerBank+1, data);
	}

	dest[0] = 0xF0; // SysEx start
	dest[1] = 0x43; // ID=Yamaha
	dest[2] = 0x00 | m_nMIDIChannel[nTG]; // 0x0c Sub-status 0 and MIDI channel
	dest[3] = 0x00; // Format number (0=1 voice)
	dest[4] = 0x01; // Byte count MSB
	dest[5] = 0x1B; // Byte count LSB
	for (uint8_t n = 0; n < 155; n++)
	{
		checksum -= data[n];
		dest[6 + n] = data[n];
	}
	dest[161] = checksum & 0x7f; // Checksum
	dest[162] = 0xF7; // SysEx end
}

void CMiniDexed::setOPMask(uint8_t uchOPMask, uint8_t nTG)
{
	if (nTG >= m_nToneGenerators) return;  // Not an active TG
	m_uchOPMask[nTG] = uchOPMask;
	m_pTG[nTG]->setOPAll (m_uchOPMask[nTG]);
}

void CMiniDexed::setMasterVolume(float32_t vol)
{
    if (vol < 0.0)
        vol = 0.0;
    else if (vol > 1.0)
        vol = 1.0;

    // Apply logarithmic scaling to match perceived loudness
    vol = powf(vol, 2.0f);

    m_fMasterVolumeW = vol;
}

std::string CMiniDexed::GetPerformanceFileName(unsigned nID)
{
	return m_PerformanceConfig.GetPerformanceFileName(nID);
}

std::string CMiniDexed::GetPerformanceName(unsigned nID)
{
	return m_PerformanceConfig.GetPerformanceName(nID);
}

unsigned CMiniDexed::GetLastPerformance()
{
	return m_PerformanceConfig.GetLastPerformance();
}

unsigned CMiniDexed::GetPerformanceBank()
{
	return m_PerformanceConfig.GetPerformanceBank();
}

unsigned CMiniDexed::GetLastPerformanceBank()
{
	return m_PerformanceConfig.GetLastPerformanceBank();
}

unsigned CMiniDexed::GetActualPerformanceID()
{
	return m_PerformanceConfig.GetActualPerformanceID();
}

void CMiniDexed::SetActualPerformanceID(unsigned nID)
{
	m_PerformanceConfig.SetActualPerformanceID(nID);
}

unsigned CMiniDexed::GetActualPerformanceBankID()
{
	return m_PerformanceConfig.GetActualPerformanceBankID();
}

void CMiniDexed::SetActualPerformanceBankID(unsigned nBankID)
{
	m_PerformanceConfig.SetActualPerformanceBankID(nBankID);
}

bool CMiniDexed::SetNewPerformance(unsigned nID)
{
	m_bSetNewPerformance = true;
	m_nSetNewPerformanceID = nID;
	if (!m_bVolRampedDown)
		m_bVolRampDownWait = true;

	return true;
}

bool CMiniDexed::SetNewPerformanceBank(unsigned nBankID)
{
	m_bSetNewPerformanceBank = true;
	m_nSetNewPerformanceBankID = nBankID;

	return true;
}

void CMiniDexed::SetFirstPerformance(void)
{
	m_bSetFirstPerformance = true;
	return;
}

bool CMiniDexed::DoSetNewPerformance (void)
{
	m_bLoadPerformanceBusy = true;
	
	unsigned nID = m_nSetNewPerformanceID;
	m_PerformanceConfig.SetNewPerformance(nID);
	
	if (m_PerformanceConfig.Load ())
	{
		LoadPerformanceParameters();
		m_bLoadPerformanceBusy = false;
		return true;
	}
	else
	{
		SetMIDIChannel (CMIDIDevice::OmniMode, 0);
		m_bLoadPerformanceBusy = false;
		return false;
	}
}

bool CMiniDexed::DoSetNewPerformanceBank (void)
{
	m_bLoadPerformanceBankBusy = true;
	
	unsigned nBankID = m_nSetNewPerformanceBankID;
	m_PerformanceConfig.SetNewPerformanceBank(nBankID);
	
	m_bLoadPerformanceBankBusy = false;
	return true;
}

void CMiniDexed::DoSetFirstPerformance(void)
{
	unsigned nID = m_PerformanceConfig.FindFirstPerformance();
	SetNewPerformance(nID);
	m_bSetFirstPerformance = false;
	return;
}

bool CMiniDexed::SavePerformanceNewFile ()
{
	m_bSavePerformanceNewFile = m_PerformanceConfig.GetInternalFolderOk() && m_PerformanceConfig.CheckFreePerformanceSlot();
	return m_bSavePerformanceNewFile;
}

bool CMiniDexed::DoSavePerformanceNewFile (void)
{
	if (m_PerformanceConfig.CreateNewPerformanceFile())
	{
		if(SavePerformance(false))
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
	for (unsigned nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
	{
		BankSelect (m_PerformanceConfig.GetBankNumber (nTG), nTG);
		ProgramChange (m_PerformanceConfig.GetVoiceNumber (nTG), nTG);
		SetMIDIChannel (m_PerformanceConfig.GetMIDIChannel (nTG), nTG);
		SetVolume (m_PerformanceConfig.GetVolume (nTG), nTG);
		SetPan (m_PerformanceConfig.GetPan (nTG), nTG);
		SetMasterTune (m_PerformanceConfig.GetDetune (nTG), nTG);
		SetCutoff (m_PerformanceConfig.GetCutoff (nTG), nTG);
		SetResonance (m_PerformanceConfig.GetResonance (nTG), nTG);
		setPitchbendRange (m_PerformanceConfig.GetPitchBendRange (nTG), nTG);
		setPitchbendStep (m_PerformanceConfig.GetPitchBendStep (nTG), nTG);
		setPortamentoMode (m_PerformanceConfig.GetPortamentoMode (nTG), nTG);
		setPortamentoGlissando (m_PerformanceConfig.GetPortamentoGlissando  (nTG), nTG);
		setPortamentoTime (m_PerformanceConfig.GetPortamentoTime (nTG), nTG);

		setNoteLimitLow(m_PerformanceConfig.GetNoteLimitLow (nTG), nTG);
		setNoteLimitHigh(m_PerformanceConfig.GetNoteLimitHigh (nTG), nTG);
		setNoteShift(m_PerformanceConfig.GetNoteShift (nTG), nTG);
		
		if(m_PerformanceConfig.VoiceDataFilled(nTG) && nTG < m_nToneGenerators) 
		{
			uint8_t* tVoiceData = m_PerformanceConfig.GetVoiceDataFromTxt(nTG);
			m_pTG[nTG]->loadVoiceParameters(tVoiceData); 
			setOPMask(0b111111, nTG);
		}

		setMonoMode(m_PerformanceConfig.GetMonoMode(nTG) ? 1 : 0, nTG); 
		setTGLink(m_PerformanceConfig.GetTGLink(nTG), nTG);

		SetFX1Send (m_PerformanceConfig.GetFX1Send (nTG), nTG);
		SetFX2Send (m_PerformanceConfig.GetFX2Send (nTG), nTG);

		setModWheelRange (m_PerformanceConfig.GetModulationWheelRange (nTG),  nTG);
		setModWheelTarget (m_PerformanceConfig.GetModulationWheelTarget (nTG),  nTG);
		setFootControllerRange (m_PerformanceConfig.GetFootControlRange (nTG),  nTG);
		setFootControllerTarget (m_PerformanceConfig.GetFootControlTarget (nTG),  nTG);
		setBreathControllerRange (m_PerformanceConfig.GetBreathControlRange (nTG),  nTG);
		setBreathControllerTarget (m_PerformanceConfig.GetBreathControlTarget (nTG),  nTG);
		setAftertouchRange (m_PerformanceConfig.GetAftertouchRange (nTG),  nTG);
		setAftertouchTarget (m_PerformanceConfig.GetAftertouchTarget (nTG),  nTG);

		SetCompressorEnable (m_PerformanceConfig.GetCompressorEnable (nTG), nTG);
		SetCompressorPreGain (m_PerformanceConfig.GetCompressorPreGain (nTG), nTG);
		SetCompressorThresh (m_PerformanceConfig.GetCompressorThresh (nTG), nTG);
		SetCompressorRatio (m_PerformanceConfig.GetCompressorRatio (nTG), nTG);		
		SetCompressorAttack (m_PerformanceConfig.GetCompressorAttack (nTG), nTG);;
		SetCompressorRelease (m_PerformanceConfig.GetCompressorRelease (nTG), nTG);
		SetCompressorMakeupGain (m_PerformanceConfig.GetCompressorMakeupGain (nTG), nTG);

		SetEQLow (m_PerformanceConfig.GetEQLow (nTG), nTG);
		SetEQMid (m_PerformanceConfig.GetEQMid (nTG), nTG);
		SetEQHigh (m_PerformanceConfig.GetEQHigh (nTG), nTG);
		SetEQGain (m_PerformanceConfig.GetEQGain (nTG), nTG);
		SetEQLowMidFreq (m_PerformanceConfig.GetEQLowMidFreq (nTG), nTG);
		SetEQMidHighFreq (m_PerformanceConfig.GetEQMidHighFreq (nTG), nTG);
		SetEQPreLowcut (m_PerformanceConfig.GetEQPreLowcut (nTG), nTG);
		SetEQPreHighcut (m_PerformanceConfig.GetEQPreHighcut (nTG), nTG);
	}

	for (unsigned nFX=0; nFX < CConfig::FXChains; ++nFX)
	{
		for (unsigned nParam = 0; nParam < FX::FXParameterUnknown; ++nParam)
		{
			FX::TFXParameter param = FX::TFXParameter(nParam);
			const FX::FXParameterType &p = FX::s_FXParameter[nParam];
			bool bSaveOnly = p.Flags & FX::FXComposite;
			SetFXParameter (param, m_PerformanceConfig.GetFXParameter (param, nFX), nFX, bSaveOnly);
		}
	}

	SetParameter (ParameterMixerDryLevel, m_PerformanceConfig.GetMixerDryLevel ());
	SetParameter (ParameterFXBypass, m_PerformanceConfig.GetFXBypass ());

	m_UI.DisplayChanged ();
}

std::string CMiniDexed::GetNewPerformanceDefaultName(void)	
{
	return m_PerformanceConfig.GetNewPerformanceDefaultName();
}

void CMiniDexed::SetNewPerformanceName(const std::string &Name)
{
	m_PerformanceConfig.SetNewPerformanceName(Name);
}

bool CMiniDexed::IsValidPerformance(unsigned nID)
{
	return m_PerformanceConfig.IsValidPerformance(nID);
}

bool CMiniDexed::IsValidPerformanceBank(unsigned nBankID)
{
	return m_PerformanceConfig.IsValidPerformanceBank(nBankID);
}

int CMiniDexed::GetLastKeyDown()
{
	return m_nLastKeyDown;
}

void CMiniDexed::SetVoiceName (const std::string &VoiceName, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	if (nTG >= m_nToneGenerators) return;  // Not an active TG

	assert (m_pTG[nTG]);
	char Name[11];
	strncpy(Name, VoiceName.c_str(),10);
	Name[10] = '\0';
	m_pTG[nTG]->setName (Name);
}

bool CMiniDexed::DeletePerformance(unsigned nID)
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

bool CMiniDexed::DoDeletePerformance(void)
{
	unsigned nID = m_nDeletePerformanceID;
	if(m_PerformanceConfig.DeletePerformance(nID))
	{
		if (m_PerformanceConfig.Load ())
		{
			LoadPerformanceParameters();
			return true;
		}
		else
		{
			SetMIDIChannel (CMIDIDevice::OmniMode, 0);
		}
	}
	
	return false;
}

bool CMiniDexed::GetPerformanceSelectToLoad(void)
{
	return m_pConfig->GetPerformanceSelectToLoad();
}

void CMiniDexed::setModController (unsigned controller, unsigned parameter, uint8_t value, uint8_t nTG)
{
	 uint8_t nBits;
	
	switch (controller)
	{
		case 0:
			if (parameter == 0)
			{
				setModWheelRange(value, nTG);
			}
			else
			{
				value=constrain(value, 0, 1);
				nBits=m_nModulationWheelTarget[nTG];
				value == 1 ?  nBits |= 1 << (parameter-1) : nBits &= ~(1 << (parameter-1)); 
				setModWheelTarget(nBits , nTG); 
			}
		break;
		
		case 1:
			if (parameter == 0)
			{
				setFootControllerRange(value, nTG);
			}
			else
			{
				value=constrain(value, 0, 1);
				nBits=m_nFootControlTarget[nTG];
				value == 1 ?  nBits |= 1 << (parameter-1) : nBits &= ~(1 << (parameter-1)); 
				setFootControllerTarget(nBits , nTG); 
			}
		break;	

		case 2:
			if (parameter == 0)
			{
				setBreathControllerRange(value, nTG);
			}
			else
			{
				value=constrain(value, 0, 1);
				nBits=m_nBreathControlTarget[nTG];
				value == 1 ?  nBits |= 1 << (parameter-1) : nBits &= ~(1 << (parameter-1));
				setBreathControllerTarget(nBits , nTG); 
			}
		break;			
		
		case 3:
			if (parameter == 0)
			{
				setAftertouchRange(value, nTG);
			}
			else
			{
				value=constrain(value, 0, 1);
				nBits=m_nAftertouchTarget[nTG];
				value == 1 ?  nBits |= 1 << (parameter-1) : nBits &= ~(1 << (parameter-1));
				setAftertouchTarget(nBits , nTG); 
			}
		break;	
		default:
		break;
	}
}

unsigned CMiniDexed::getModController (unsigned controller, unsigned parameter, uint8_t nTG)
{
	unsigned nBits;
	switch (controller)
	{
		case 0:
			if (parameter == 0)
			{
			    return m_nModulationWheelRange[nTG];
			}
			else
			{
	
				nBits=m_nModulationWheelTarget[nTG];
				nBits &= 1 << (parameter-1);				
				return (nBits != 0 ? 1 : 0) ; 
			}
		break;
		
		case 1:
			if (parameter == 0)
			{
				return m_nFootControlRange[nTG];
			}
			else
			{
				nBits=m_nFootControlTarget[nTG];
				nBits &= 1 << (parameter-1)	;			
				return (nBits != 0 ? 1 : 0) ; 
			}
		break;	

		case 2:
			if (parameter == 0)
			{
				return m_nBreathControlRange[nTG];
			}
			else
			{
				nBits=m_nBreathControlTarget[nTG];	
				nBits &= 1 << (parameter-1)	;			
				return (nBits != 0 ? 1 : 0) ; 
			}
		break;			
		
		case 3:
			if (parameter == 0)
			{
				return m_nAftertouchRange[nTG];
			}
			else
			{
				nBits=m_nAftertouchTarget[nTG];
				nBits &= 1 << (parameter-1)	;			
				return (nBits != 0 ? 1 : 0) ; 
			}
		break;	
		
		default:
			return 0;
		break;
	}
	
}

void CMiniDexed::UpdateNetwork()
{
	if (!m_pNet) {
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

		if (m_pConfig->GetNetworkFTPEnabled()) {
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
		} else {
			LOGNOTE("FTP daemon not started (NetworkFTPEnabled=0)");
		}

		m_pmDNSPublisher = new CmDNSPublisher (m_pNet);
		assert (m_pmDNSPublisher);
		
		if (!m_pmDNSPublisher->PublishService (m_pConfig->GetNetworkHostname(), CmDNSPublisher::ServiceTypeAppleMIDI,
						     5004))
		{
			LOGPANIC ("Cannot publish mdns service");
		}

		static constexpr const char *ServiceTypeFTP = "_ftp._tcp";
		static const char *ftpTxt[] = { "app=MiniDexed", nullptr };
		if (!m_pmDNSPublisher->PublishService (m_pConfig->GetNetworkHostname(), ServiceTypeFTP, 21, ftpTxt))
		{
			LOGPANIC ("Cannot publish mdns service");
		}

		if (m_pConfig->GetSyslogEnabled())
		{
			LOGNOTE ("Syslog server is enabled in configuration");
			const CIPAddress& ServerIP = m_pConfig->GetNetworkSyslogServerIPAddress();
			if (ServerIP.IsSet () && !ServerIP.IsNull ())
			{
				static const u16 usServerPort = 8514;
				CString IPString;
				ServerIP.Format (&IPString);
				LOGNOTE ("Sending log messages to syslog server %s:%u",
					(const char *) IPString, (unsigned) usServerPort);

				new CSysLogDaemon (m_pNet, ServerIP, usServerPort);
			}
			else
			{
				LOGNOTE ("Syslog server IP not set");
			}	
		}
		else
		{
			LOGNOTE ("Syslog server is not enabled in configuration");
		}
		m_bNetworkReady = true;
	}

	if (m_bNetworkReady && !bNetIsRunning)
	{
		LOGNOTE("CMiniDexed::UpdateNetwork: Network disconnected");
		m_bNetworkReady = false;
		m_pmDNSPublisher->UnpublishService (m_pConfig->GetNetworkHostname());
		LOGNOTE("Network disconnected.");
	}
	else if (!m_bNetworkReady && bNetIsRunning)
	{
		LOGNOTE("CMiniDexed::UpdateNetwork: Network connection reestablished");
		m_bNetworkReady = true;
		
		if (!m_pmDNSPublisher->PublishService (m_pConfig->GetNetworkHostname(), CmDNSPublisher::ServiceTypeAppleMIDI,
						     5004))
		{
			LOGPANIC ("Cannot publish mdns service");
		}

		static constexpr const char *ServiceTypeFTP = "_ftp._tcp";
		static const char *ftpTxt[] = { "app=MiniDexed", nullptr };
		if (!m_pmDNSPublisher->PublishService (m_pConfig->GetNetworkHostname(), ServiceTypeFTP, 21, ftpTxt))
		{
			LOGPANIC ("Cannot publish mdns service");
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
				delete m_WLAN; m_WLAN = nullptr;
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
				m_pConfig->GetNetworkIPAddress().Format (&IPString);
				m_pConfig->GetNetworkSubnetMask().Format (&SubnetString);
				LOGNOTE("CMiniDexed::InitNetwork: Creating CNetSubSystem with IP: %s / %s", (const char*)IPString, (const char*)SubnetString);
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
				LOGNOTE ("CMiniDexed::InitNetwork: Neither DHCP nor IP address/subnet mask is set, using DHCP (Hostname: %s)", m_pConfig->GetNetworkHostname());
				m_pNet = new CNetSubSystem(0, 0, 0, 0, m_pConfig->GetNetworkHostname(), NetDeviceType);
			}

			if (!m_pNet || !m_pNet->Initialize(false)) // Check if m_pNet allocation succeeded
			{
				LOGERR("CMiniDexed::InitNetwork: Failed to initialize network subsystem");
				delete m_pNet; m_pNet = nullptr; // Clean up if failed
				delete m_WLAN; m_WLAN = nullptr; // Clean up WLAN if allocated
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
					delete m_WPASupplicant; m_WPASupplicant = nullptr; // Clean up if failed
					// Continue without supplicant? Or return false? Decided to continue for now.
				}
			}
			m_pNetDevice = CNetDevice::GetNetDevice(NetDeviceType);

			// Allocate UDP MIDI device now that network might be up
			m_UDPMIDI = new CUDPMIDIDevice(this, m_pConfig, &m_UI); // Allocate m_UDPMIDI
			if (!m_UDPMIDI) {
				LOGERR("CMiniDexed::InitNetwork: Failed to allocate UDP MIDI device");
				// Clean up other network resources if needed, or handle error appropriately
			} else {
				// Synchronize UDP MIDI channels with current assignments
				for (unsigned nTG = 0; nTG < m_nToneGenerators; ++nTG)
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

const CIPAddress& CMiniDexed::GetNetworkIPAddress()
{
	if (m_pNet)
		return *m_pNet->GetConfig()->GetIPAddress();
	else
		return m_pConfig->GetNetworkIPAddress();
}

CStatus *CStatus::s_pThis = 0;
