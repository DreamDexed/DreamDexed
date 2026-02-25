//
// performanceconfig.cpp
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

#include "performanceconfig.h"

#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

#include <Properties/propertiesfatfsfile.h>
#include <circle/logger.h>
#include <circle/string.h>
#include <fatfs/ff.h>

#include "bus.h"
#include "common.h"
#include "config.h"
#include "effect.h"
#include "mididevice.h"

LOGMODULE("Performance");

// #define VERBOSE_DEBUG

#define PERFORMANCE_DIR "performance"
#define DEFAULT_PERFORMANCE_FILENAME "performance.ini"
#define DEFAULT_PERFORMANCE_NAME "Default"

CPerformanceConfig::CPerformanceConfig(FATFS *pFileSystem) :
m_Properties{DEFAULT_PERFORMANCE_FILENAME, pFileSystem},
m_pFileSystem{pFileSystem}
{
}

CPerformanceConfig::~CPerformanceConfig()
{
}

bool CPerformanceConfig::Init(int nToneGenerators)
{
	// Different versions of Pi allow different TG configurations.
	// On loading, performances will load up to the number of
	// supported/active TGs.
	//
	// On saving, the active/supported number of TGs is used.
	//
	// This means that if an 8TG performance is loaded into
	// a 16 TG system and then saved, the saved performance
	// will include all 16 TG configurations.
	//
	m_nToneGenerators = nToneGenerators;
	m_nBuses = nToneGenerators / 8;

	// Check intermal performance directory exists
	DIR Directory;
	FRESULT Result;
	// Check if internal "performance" directory exists
	Result = f_opendir(&Directory, PERFORMANCE_DIR);
	if (Result == FR_OK)
	{
		m_bPerformanceDirectoryExists = true;
		Result = f_closedir(&Directory);
	}
	else
	{
		m_bPerformanceDirectoryExists = false;
	}

	// List banks if present
	ListPerformanceBanks();

#ifdef VERBOSE_DEBUG
#warning "PerformanceConfig in verbose debug printing mode"
	LOGNOTE("Testing loading of banks");
	for (int i = 0; i < NUM_PERFORMANCE_BANKS; i++)
	{
		if (!m_PerformanceBankName[i].empty())
		{
			SetNewPerformanceBank(i);
			SetNewPerformance(0);
		}
	}
#endif
	// Set to default initial bank
	SetNewPerformanceBank(0);
	SetNewPerformance(0);

	LOGNOTE("Loaded Default Performance Bank - Last Performance: %d", m_nLastPerformance + 1); // Show "user facing" index

	return true;
}

bool CPerformanceConfig::Load()
{
	if (!m_Properties.Load())
	{
		return false;
	}

	bool bResult = false;

	for (int nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
	{
		CString PropertyName;

		PropertyName.Format("BankNumber%d", nTG + 1);
		m_nBankNumber[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("VoiceNumber%d", nTG + 1);
		m_nVoiceNumber[nTG] = m_Properties.GetSignedNumber(PropertyName, 1);
		if (m_nVoiceNumber[nTG] > 0)
		{
			m_nVoiceNumber[nTG]--;
		}

		PropertyName.Format("MIDIChannel%d", nTG + 1);
		int nMIDIChannel = m_Properties.GetSignedNumber(PropertyName, 0);
		if (nMIDIChannel == 0)
		{
			m_nMIDIChannel[nTG] = CMIDIDevice::Disabled;
		}
		else if (nMIDIChannel <= CMIDIDevice::Channels)
		{
			m_nMIDIChannel[nTG] = nMIDIChannel - 1;
			bResult = true;
		}
		else
		{
			m_nMIDIChannel[nTG] = CMIDIDevice::OmniMode;
			bResult = true;
		}

		PropertyName.Format("SysExChannel%d", nTG + 1);
		int nSysExChannel = m_Properties.GetSignedNumber(PropertyName, 1);
		if (nSysExChannel > 0 && nSysExChannel <= CMIDIDevice::Channels)
		{
			m_nSysExChannel[nTG] = nSysExChannel - 1;
		}
		else
		{
			m_nSysExChannel[nTG] = 0;
		}

		PropertyName.Format("SysExEnable%d", nTG + 1);
		m_bSysExEnable[nTG] = m_Properties.GetNumber(PropertyName, 1);

		PropertyName.Format("MIDIRxSustain%d", nTG + 1);
		m_bMIDIRxSustain[nTG] = m_Properties.GetNumber(PropertyName, 1);

		PropertyName.Format("MIDIRxPortamento%d", nTG + 1);
		m_bMIDIRxPortamento[nTG] = m_Properties.GetNumber(PropertyName, 1);

		PropertyName.Format("MIDIRxSostenuto%d", nTG + 1);
		m_bMIDIRxSostenuto[nTG] = m_Properties.GetNumber(PropertyName, 1);

		PropertyName.Format("MIDIRxHold2%d", nTG + 1);
		m_bMIDIRxHold2[nTG] = m_Properties.GetNumber(PropertyName, 1);

		PropertyName.Format("Volume%d", nTG + 1);
		m_nVolume[nTG] = m_Properties.GetSignedNumber(PropertyName, 100);

		PropertyName.Format("Pan%d", nTG + 1);
		m_nPan[nTG] = m_Properties.GetSignedNumber(PropertyName, 64);

		PropertyName.Format("Detune%d", nTG + 1);
		m_nDetune[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("Cutoff%d", nTG + 1);
		m_nCutoff[nTG] = m_Properties.GetSignedNumber(PropertyName, 99);

		PropertyName.Format("Resonance%d", nTG + 1);
		m_nResonance[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("NoteLimitLow%d", nTG + 1);
		m_nNoteLimitLow[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("NoteLimitHigh%d", nTG + 1);
		m_nNoteLimitHigh[nTG] = m_Properties.GetSignedNumber(PropertyName, 127);

		PropertyName.Format("NoteShift%d", nTG + 1);
		m_nNoteShift[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("FX1Send%d", nTG + 1);
		m_nFX1Send[nTG] = m_Properties.GetSignedNumber(PropertyName, 25);

		PropertyName.Format("FX2Send%d", nTG + 1);
		m_nFX2Send[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		// compatibility ReverbSend[n] => FX1Send[n]
		PropertyName.Format("ReverbSend%d", nTG + 1);
		if (m_Properties.IsSet(PropertyName))
		{
			// the volume calculated by x^4, but FX1Send uses x^2
			float reverbSend = m_Properties.GetNumber(PropertyName, 50);
			reverbSend = std::powf(mapfloat(reverbSend, 0.0f, 99.0f, 0.0f, 1.0f), 2);
			m_nFX1Send[nTG] = mapfloat(reverbSend, 0.0f, 1.0f, 0, 99);
		}

		PropertyName.Format("PitchBendRange%d", nTG + 1);
		m_nPitchBendRange[nTG] = m_Properties.GetSignedNumber(PropertyName, 2);

		PropertyName.Format("PitchBendStep%d", nTG + 1);
		m_nPitchBendStep[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("PortamentoMode%d", nTG + 1);
		m_nPortamentoMode[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("PortamentoGlissando%d", nTG + 1);
		m_nPortamentoGlissando[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("PortamentoTime%d", nTG + 1);
		m_nPortamentoTime[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("VoiceData%d", nTG + 1);
		m_nVoiceDataTxt[nTG] = m_Properties.GetString(PropertyName, "");

		PropertyName.Format("MonoMode%d", nTG + 1);
		m_bMonoMode[nTG] = m_Properties.GetNumber(PropertyName, 0) != 0;

		PropertyName.Format("TGLink%d", nTG + 1);
		m_nTGLink[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("ModulationWheelRange%d", nTG + 1);
		m_nModulationWheelRange[nTG] = m_Properties.GetSignedNumber(PropertyName, 99);

		PropertyName.Format("ModulationWheelTarget%d", nTG + 1);
		m_nModulationWheelTarget[nTG] = m_Properties.GetSignedNumber(PropertyName, 1);

		PropertyName.Format("FootControlRange%d", nTG + 1);
		m_nFootControlRange[nTG] = m_Properties.GetSignedNumber(PropertyName, 99);

		PropertyName.Format("FootControlTarget%d", nTG + 1);
		m_nFootControlTarget[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("BreathControlRange%d", nTG + 1);
		m_nBreathControlRange[nTG] = m_Properties.GetSignedNumber(PropertyName, 99);

		PropertyName.Format("BreathControlTarget%d", nTG + 1);
		m_nBreathControlTarget[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("AftertouchRange%d", nTG + 1);
		m_nAftertouchRange[nTG] = m_Properties.GetSignedNumber(PropertyName, 99);

		PropertyName.Format("AftertouchTarget%d", nTG + 1);
		m_nAftertouchTarget[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("CompressorEnable%d", nTG + 1);
		m_bCompressorEnable[nTG] = m_Properties.GetNumber(PropertyName, 0);

		PropertyName.Format("CompressorPreGain%d", nTG + 1);
		m_nCompressorPreGain[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("CompressorThresh%d", nTG + 1);
		m_nCompressorThresh[nTG] = m_Properties.GetSignedNumber(PropertyName, -20);

		PropertyName.Format("CompressorRatio%d", nTG + 1);
		m_nCompressorRatio[nTG] = m_Properties.GetSignedNumber(PropertyName, 5);

		PropertyName.Format("CompressorAttack%d", nTG + 1);
		m_nCompressorAttack[nTG] = m_Properties.GetSignedNumber(PropertyName, 5);

		PropertyName.Format("CompressorRelease%d", nTG + 1);
		m_nCompressorRelease[nTG] = m_Properties.GetSignedNumber(PropertyName, 200);

		PropertyName.Format("CompressorMakeupGain%d", nTG + 1);
		m_nCompressorMakeupGain[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("EQLow%d", nTG + 1);
		m_nEQLow[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("EQMid%d", nTG + 1);
		m_nEQMid[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("EQHigh%d", nTG + 1);
		m_nEQHigh[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("EQGain%d", nTG + 1);
		m_nEQGain[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("EQLowMidFreq%d", nTG + 1);
		m_nEQLowMidFreq[nTG] = m_Properties.GetSignedNumber(PropertyName, 24);

		PropertyName.Format("EQMidHighFreq%d", nTG + 1);
		m_nEQMidHighFreq[nTG] = m_Properties.GetSignedNumber(PropertyName, 44);

		PropertyName.Format("EQPreLowcut%d", nTG + 1);
		m_nEQPreLowcut[nTG] = m_Properties.GetSignedNumber(PropertyName, 0);

		PropertyName.Format("EQPreHighcut%d", nTG + 1);
		m_nEQPreHighcut[nTG] = m_Properties.GetSignedNumber(PropertyName, 60);
	}

	for (int nBus = 0; nBus < m_nBuses; ++nBus)
	{
		CString PropertyName;

		for (int nParam = 0; nParam < Bus::Parameter::Unknown; ++nParam)
		{
			const Bus::ParameterType &p = Bus::s_Parameter[nParam];

			PropertyName.Format("Bus%d%s", nBus + 1, p.Name);
			m_nBusParameter[nBus][nParam] = m_Properties.GetSignedNumber(PropertyName, p.Default);
		}
	}

	for (int nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		CString PropertyName;

		for (int nParam = 0; nParam < FX::Parameter::Unknown; ++nParam)
		{
			const FX::ParameterType &p = FX::s_Parameter[nParam];

			if (nFX == CConfig::MasterFX)
			{
				PropertyName.Format("Out1MasterFX%s", p.Name);
			}
			else
			{
				int idFX = nFX % CConfig::BusFXChains;
				int nBus = nFX / CConfig::BusFXChains;

				if (nBus >= m_nBuses)
					continue;

				PropertyName.Format("Bus%dSendFX%d%s", nBus + 1, idFX + 1, p.Name);
			}

			if (p.Flags & FX::Flag::SaveAsString)
				m_nFXParameter[nFX][nParam] = FX::getIDFromName(FX::Parameter(nParam), m_Properties.GetString(PropertyName, ""));
			else
				m_nFXParameter[nFX][nParam] = m_Properties.GetSignedNumber(PropertyName, p.Default);
		}
	}

	if (CConfig::FXChains)
	{
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::ReturnLevel] = FX::s_Parameter[FX::Parameter::ReturnLevel].Maximum;
	}

	// Compatibility
	if (m_Properties.IsSet("CompressorEnable") && CConfig::FXChains)
	{
		bool bHasCompressor = m_Properties.GetNumber("CompressorEnable", 0);

		m_nFXParameter[CConfig::MasterFX][FX::Parameter::Slot0] = bHasCompressor ? FX::getIDFromName(FX::Parameter::Slot0, "Compressor") : 0;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorPreGain] = 0;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorThresh] = -7;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorRatio] = 5;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorAttack] = 0;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorRelease] = 200;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorHPFilterEnable] = 1;
		m_nFXParameter[CConfig::MasterFX][FX::Parameter::CompressorBypass] = 0;

		m_nBusParameter[0][Bus::Parameter::ReturnLevel] = Bus::s_Parameter[Bus::Parameter::ReturnLevel].Maximum;
	}

	if (m_Properties.IsSet("ReverbEnable") && CConfig::FXChains)
	{
		// setup Reverb to FX1
		m_nFXParameter[0][FX::Parameter::Slot0] = FX::getIDFromName(FX::Parameter::Slot0, "PlateReverb");
		m_nFXParameter[0][FX::Parameter::PlateReverbMix] = m_Properties.GetNumber("ReverbEnable", 1) ? 100 : 0;
		m_nFXParameter[0][FX::Parameter::PlateReverbSize] = m_Properties.GetSignedNumber("ReverbSize", 70);
		m_nFXParameter[0][FX::Parameter::PlateReverbHighDamp] = m_Properties.GetSignedNumber("ReverbHighDamp", 50);
		m_nFXParameter[0][FX::Parameter::PlateReverbLowDamp] = m_Properties.GetSignedNumber("ReverbLowDamp", 50);
		m_nFXParameter[0][FX::Parameter::PlateReverbLowPass] = m_Properties.GetSignedNumber("ReverbLowPass", 30);
		m_nFXParameter[0][FX::Parameter::PlateReverbDiffusion] = m_Properties.GetSignedNumber("ReverbDiffusion", 65);
		m_nFXParameter[0][FX::Parameter::ReturnLevel] = m_Properties.GetNumber("ReverbEnable", 1) ? m_Properties.GetSignedNumber("ReverbLevel", 99) : 0;
	}

	return bResult;
}

bool CPerformanceConfig::Save()
{
	m_Properties.RemoveAll();

	for (int nTG = 0; nTG < m_nToneGenerators; nTG++)
	{
		CString PropertyName;

		PropertyName.Format("BankNumber%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nBankNumber[nTG]);

		PropertyName.Format("VoiceNumber%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nVoiceNumber[nTG] + 1);

		PropertyName.Format("MIDIChannel%d", nTG + 1);
		int nMIDIChannel = m_nMIDIChannel[nTG];
		if (nMIDIChannel < CMIDIDevice::Channels)
		{
			nMIDIChannel++;
		}
		else if (nMIDIChannel == CMIDIDevice::OmniMode)
		{
			nMIDIChannel = 255;
		}
		else
		{
			nMIDIChannel = 0;
		}
		m_Properties.SetSignedNumber(PropertyName, nMIDIChannel);

		if (m_nSysExChannel[nTG])
		{
			PropertyName.Format("SysExChannel%d", nTG + 1);
			m_Properties.SetSignedNumber(PropertyName, m_nSysExChannel[nTG] + 1);
		}

		if (!m_bSysExEnable[nTG])
		{
			PropertyName.Format("SysExEnable%d", nTG + 1);
			m_Properties.SetNumber(PropertyName, m_bSysExEnable[nTG]);
		}

		if (!m_bMIDIRxSustain[nTG])
		{
			PropertyName.Format("MIDIRxSustain%d", nTG + 1);
			m_Properties.SetNumber(PropertyName, m_bMIDIRxSustain[nTG]);
		}

		if (!m_bMIDIRxPortamento[nTG])
		{
			PropertyName.Format("MIDIRxPortamento%d", nTG + 1);
			m_Properties.SetNumber(PropertyName, m_bMIDIRxPortamento[nTG]);
		}

		if (!m_bMIDIRxSostenuto[nTG])
		{
			PropertyName.Format("MIDIRxSostenuto%d", nTG + 1);
			m_Properties.SetNumber(PropertyName, m_bMIDIRxSostenuto[nTG]);
		}

		if (!m_bMIDIRxHold2[nTG])
		{
			PropertyName.Format("MIDIRxHold2%d", nTG + 1);
			m_Properties.SetNumber(PropertyName, m_bMIDIRxHold2[nTG]);
		}

		PropertyName.Format("Volume%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nVolume[nTG]);

		PropertyName.Format("Pan%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nPan[nTG]);

		PropertyName.Format("Detune%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nDetune[nTG]);

		PropertyName.Format("Cutoff%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCutoff[nTG]);

		PropertyName.Format("Resonance%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nResonance[nTG]);

		PropertyName.Format("NoteLimitLow%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nNoteLimitLow[nTG]);

		PropertyName.Format("NoteLimitHigh%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nNoteLimitHigh[nTG]);

		PropertyName.Format("NoteShift%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nNoteShift[nTG]);

		PropertyName.Format("FX1Send%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nFX1Send[nTG]);

		PropertyName.Format("FX2Send%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nFX2Send[nTG]);

		PropertyName.Format("PitchBendRange%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nPitchBendRange[nTG]);

		PropertyName.Format("PitchBendStep%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nPitchBendStep[nTG]);

		PropertyName.Format("PortamentoMode%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nPortamentoMode[nTG]);

		PropertyName.Format("PortamentoGlissando%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nPortamentoGlissando[nTG]);

		PropertyName.Format("PortamentoTime%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nPortamentoTime[nTG]);

		PropertyName.Format("VoiceData%d", nTG + 1);
		m_Properties.SetString(PropertyName, m_nVoiceDataTxt[nTG].c_str());

		PropertyName.Format("MonoMode%d", nTG + 1);
		m_Properties.SetNumber(PropertyName, m_bMonoMode[nTG] ? 1 : 0);

		PropertyName.Format("TGLink%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nTGLink[nTG]);

		PropertyName.Format("ModulationWheelRange%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nModulationWheelRange[nTG]);

		PropertyName.Format("ModulationWheelTarget%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nModulationWheelTarget[nTG]);

		PropertyName.Format("FootControlRange%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nFootControlRange[nTG]);

		PropertyName.Format("FootControlTarget%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nFootControlTarget[nTG]);

		PropertyName.Format("BreathControlRange%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nBreathControlRange[nTG]);

		PropertyName.Format("BreathControlTarget%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nBreathControlTarget[nTG]);

		PropertyName.Format("AftertouchRange%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nAftertouchRange[nTG]);

		PropertyName.Format("AftertouchTarget%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nAftertouchTarget[nTG]);

		PropertyName.Format("CompressorEnable%d", nTG + 1);
		m_Properties.SetNumber(PropertyName, m_bCompressorEnable[nTG]);

		PropertyName.Format("CompressorPreGain%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCompressorPreGain[nTG]);

		PropertyName.Format("CompressorThresh%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCompressorThresh[nTG]);

		PropertyName.Format("CompressorRatio%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCompressorRatio[nTG]);

		PropertyName.Format("CompressorAttack%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCompressorAttack[nTG]);

		PropertyName.Format("CompressorRelease%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCompressorRelease[nTG]);

		PropertyName.Format("CompressorMakeupGain%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nCompressorMakeupGain[nTG]);

		PropertyName.Format("EQLow%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQLow[nTG]);

		PropertyName.Format("EQMid%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQMid[nTG]);

		PropertyName.Format("EQHigh%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQHigh[nTG]);

		PropertyName.Format("EQGain%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQGain[nTG]);

		PropertyName.Format("EQLowMidFreq%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQLowMidFreq[nTG]);

		PropertyName.Format("EQMidHighFreq%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQMidHighFreq[nTG]);

		PropertyName.Format("EQPreLowcut%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQPreLowcut[nTG]);

		PropertyName.Format("EQPreHighcut%d", nTG + 1);
		m_Properties.SetSignedNumber(PropertyName, m_nEQPreHighcut[nTG]);
	}

	for (int nBus = 0; nBus < m_nBuses; ++nBus)
	{
		CString PropertyName;

		for (int nParam = 0; nParam < Bus::Parameter::Unknown; ++nParam)
		{
			if (Bus::s_Parameter[nParam].Flags & Bus::Flag::UIOnly) continue;

			PropertyName.Format("Bus%d%s", nBus + 1, Bus::s_Parameter[nParam].Name);
			m_Properties.SetSignedNumber(PropertyName, m_nBusParameter[nBus][nParam]);
		}
	}

	for (int nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		CString PropertyName;

		CString FXName;
		if (nFX == CConfig::MasterFX)
		{
			FXName = "Out1MasterFX";
		}
		else
		{
			int idFX = nFX % CConfig::BusFXChains;
			int nBus = nFX / CConfig::BusFXChains;

			if (nBus >= m_nBuses)
				continue;

			FXName.Format("Bus%dSendFX%d", nBus + 1, idFX + 1);
		}

		for (int nSlot = 0; nSlot < 3; ++nSlot)
		{
			int nSlotParam = FX::Parameter::Slot0 + nSlot;
			int nEffectID = m_nFXParameter[nFX][nSlotParam];
			const FX::EffectType &effect = FX::s_effects[nEffectID];

			PropertyName.Format("%s%s", FXName.c_str(), FX::s_Parameter[nSlotParam].Name);
			m_Properties.SetString(PropertyName, effect.Name);
		}

		for (int nSlot = 0; nSlot < 3; ++nSlot)
		{
			int nSlotParam = FX::Parameter::Slot0 + nSlot;
			int nEffectID = m_nFXParameter[nFX][nSlotParam];
			const FX::EffectType &effect = FX::s_effects[nEffectID];

			if (nEffectID == 0) continue;

			for (int nParam = effect.MinID; nParam <= effect.MaxID; ++nParam)
			{
				const FX::ParameterType &p = FX::s_Parameter[nParam];
				PropertyName.Format("%s%s", FXName.c_str(), FX::s_Parameter[nParam].Name);

				if (p.Flags & FX::Flag::SaveAsString)
					m_Properties.SetString(PropertyName, FX::getNameFromID(FX::Parameter(nParam), m_nFXParameter[nFX][nParam]));
				else
					m_Properties.SetSignedNumber(PropertyName, m_nFXParameter[nFX][nParam]);
			}
		}

		if (nFX != CConfig::MasterFX)
		{
			PropertyName.Format("%s%s", FXName.c_str(), FX::s_Parameter[FX::Parameter::ReturnLevel].Name);
			m_Properties.SetSignedNumber(PropertyName, m_nFXParameter[nFX][FX::Parameter::ReturnLevel]);
		}

		PropertyName.Format("%s%s", FXName.c_str(), FX::s_Parameter[FX::Parameter::Bypass].Name);
		m_Properties.SetSignedNumber(PropertyName, m_nFXParameter[nFX][FX::Parameter::Bypass]);
	}

	return m_Properties.Save();
}

int CPerformanceConfig::GetBankNumber(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nBankNumber[nTG];
}

int CPerformanceConfig::GetVoiceNumber(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nVoiceNumber[nTG];
}

int CPerformanceConfig::GetMIDIChannel(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nMIDIChannel[nTG];
}

int CPerformanceConfig::GetSysExChannel(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nSysExChannel[nTG];
}

bool CPerformanceConfig::GetSysExEnable(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bSysExEnable[nTG];
}

bool CPerformanceConfig::GetMIDIRxSustain(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bMIDIRxSustain[nTG];
}

bool CPerformanceConfig::GetMIDIRxPortamento(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bMIDIRxPortamento[nTG];
}

bool CPerformanceConfig::GetMIDIRxSostenuto(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bMIDIRxSostenuto[nTG];
}

bool CPerformanceConfig::GetMIDIRxHold2(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bMIDIRxHold2[nTG];
}

int CPerformanceConfig::GetVolume(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nVolume[nTG];
}

int CPerformanceConfig::GetPan(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nPan[nTG];
}

int CPerformanceConfig::GetDetune(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nDetune[nTG];
}

int CPerformanceConfig::GetCutoff(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCutoff[nTG];
}

int CPerformanceConfig::GetResonance(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nResonance[nTG];
}

int CPerformanceConfig::GetNoteLimitLow(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nNoteLimitLow[nTG];
}

int CPerformanceConfig::GetNoteLimitHigh(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nNoteLimitHigh[nTG];
}

int CPerformanceConfig::GetNoteShift(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nNoteShift[nTG];
}

int CPerformanceConfig::GetFX1Send(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nFX1Send[nTG];
}

int CPerformanceConfig::GetFX2Send(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nFX2Send[nTG];
}

void CPerformanceConfig::SetBankNumber(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nBankNumber[nTG] = nValue;
}

void CPerformanceConfig::SetVoiceNumber(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nVoiceNumber[nTG] = nValue;
}

void CPerformanceConfig::SetMIDIChannel(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nMIDIChannel[nTG] = nValue;
}

void CPerformanceConfig::SetSysExChannel(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nSysExChannel[nTG] = nValue;
}

void CPerformanceConfig::SetSysExEnable(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bSysExEnable[nTG] = bValue;
}

void CPerformanceConfig::SetMIDIRxSustain(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bMIDIRxSustain[nTG] = bValue;
}

void CPerformanceConfig::SetMIDIRxPortamento(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bMIDIRxPortamento[nTG] = bValue;
}

void CPerformanceConfig::SetMIDIRxSostenuto(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bMIDIRxSostenuto[nTG] = bValue;
}

void CPerformanceConfig::SetMIDIRxHold2(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bMIDIRxHold2[nTG] = bValue;
}

void CPerformanceConfig::SetVolume(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nVolume[nTG] = nValue;
}

void CPerformanceConfig::SetPan(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nPan[nTG] = nValue;
}

void CPerformanceConfig::SetDetune(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nDetune[nTG] = nValue;
}

void CPerformanceConfig::SetCutoff(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCutoff[nTG] = nValue;
}

void CPerformanceConfig::SetResonance(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nResonance[nTG] = nValue;
}

void CPerformanceConfig::SetNoteLimitLow(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nNoteLimitLow[nTG] = nValue;
}

void CPerformanceConfig::SetNoteLimitHigh(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nNoteLimitHigh[nTG] = nValue;
}

void CPerformanceConfig::SetNoteShift(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nNoteShift[nTG] = nValue;
}

void CPerformanceConfig::SetFX1Send(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nFX1Send[nTG] = nValue;
}

void CPerformanceConfig::SetFX2Send(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nFX2Send[nTG] = nValue;
}

int CPerformanceConfig::GetEQLow(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQLow[nTG];
}

int CPerformanceConfig::GetEQMid(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQMid[nTG];
}

int CPerformanceConfig::GetEQHigh(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQHigh[nTG];
}

int CPerformanceConfig::GetEQGain(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQGain[nTG];
}

int CPerformanceConfig::GetEQLowMidFreq(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQLowMidFreq[nTG];
}

int CPerformanceConfig::GetEQMidHighFreq(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQMidHighFreq[nTG];
}

int CPerformanceConfig::GetEQPreLowcut(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQPreLowcut[nTG];
}

int CPerformanceConfig::GetEQPreHighcut(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nEQPreHighcut[nTG];
}

void CPerformanceConfig::SetEQLow(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQLow[nTG] = nValue;
}

void CPerformanceConfig::SetEQMid(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQMid[nTG] = nValue;
}

void CPerformanceConfig::SetEQHigh(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQHigh[nTG] = nValue;
}

void CPerformanceConfig::SetEQGain(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQGain[nTG] = nValue;
}

void CPerformanceConfig::SetEQLowMidFreq(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQLowMidFreq[nTG] = nValue;
}

void CPerformanceConfig::SetEQMidHighFreq(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQMidHighFreq[nTG] = nValue;
}

void CPerformanceConfig::SetEQPreLowcut(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQPreLowcut[nTG] = nValue;
}

void CPerformanceConfig::SetEQPreHighcut(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nEQPreHighcut[nTG] = nValue;
}

int CPerformanceConfig::GetFXParameter(FX::Parameter nParameter, int nFX) const
{
	assert(nFX < CConfig::FXChains);
	assert(nParameter < FX::Parameter::Unknown);
	return m_nFXParameter[nFX][nParameter];
}

void CPerformanceConfig::SetFXParameter(FX::Parameter nParameter, int nValue, int nFX)
{
	assert(nFX < CConfig::FXChains);
	assert(nParameter < FX::Parameter::Unknown);
	m_nFXParameter[nFX][nParameter] = nValue;
}

int CPerformanceConfig::GetBusParameter(Bus::Parameter nParameter, int nBus) const
{
	assert(nBus < CConfig::Buses);
	assert(nParameter < Bus::Parameter::Unknown);
	return m_nBusParameter[nBus][nParameter];
}

void CPerformanceConfig::SetBusParameter(Bus::Parameter nParameter, int nValue, int nBus)
{
	assert(nBus < CConfig::Buses);
	assert(nParameter < Bus::Parameter::Unknown);
	m_nBusParameter[nBus][nParameter] = nValue;
}

// Pitch bender and portamento:
void CPerformanceConfig::SetPitchBendRange(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nPitchBendRange[nTG] = nValue;
}

int CPerformanceConfig::GetPitchBendRange(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nPitchBendRange[nTG];
}

void CPerformanceConfig::SetPitchBendStep(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nPitchBendStep[nTG] = nValue;
}

int CPerformanceConfig::GetPitchBendStep(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nPitchBendStep[nTG];
}

void CPerformanceConfig::SetPortamentoMode(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nPortamentoMode[nTG] = nValue;
}

int CPerformanceConfig::GetPortamentoMode(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nPortamentoMode[nTG];
}

void CPerformanceConfig::SetPortamentoGlissando(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nPortamentoGlissando[nTG] = nValue;
}

int CPerformanceConfig::GetPortamentoGlissando(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nPortamentoGlissando[nTG];
}

void CPerformanceConfig::SetPortamentoTime(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nPortamentoTime[nTG] = nValue;
}

int CPerformanceConfig::GetPortamentoTime(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nPortamentoTime[nTG];
}

void CPerformanceConfig::SetMonoMode(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bMonoMode[nTG] = bValue;
}

bool CPerformanceConfig::GetMonoMode(int nTG) const
{
	return m_bMonoMode[nTG];
}

void CPerformanceConfig::SetTGLink(int nTGLink, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nTGLink[nTG] = nTGLink;
}

int CPerformanceConfig::GetTGLink(int nTG) const
{
	return m_nTGLink[nTG];
}

void CPerformanceConfig::SetModulationWheelRange(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nModulationWheelRange[nTG] = nValue;
}

int CPerformanceConfig::GetModulationWheelRange(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nModulationWheelRange[nTG];
}

void CPerformanceConfig::SetModulationWheelTarget(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nModulationWheelTarget[nTG] = nValue;
}

int CPerformanceConfig::GetModulationWheelTarget(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nModulationWheelTarget[nTG];
}

void CPerformanceConfig::SetFootControlRange(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nFootControlRange[nTG] = nValue;
}

int CPerformanceConfig::GetFootControlRange(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nFootControlRange[nTG];
}

void CPerformanceConfig::SetFootControlTarget(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nFootControlTarget[nTG] = nValue;
}

int CPerformanceConfig::GetFootControlTarget(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nFootControlTarget[nTG];
}

void CPerformanceConfig::SetBreathControlRange(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nBreathControlRange[nTG] = nValue;
}

int CPerformanceConfig::GetBreathControlRange(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nBreathControlRange[nTG];
}

void CPerformanceConfig::SetBreathControlTarget(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nBreathControlTarget[nTG] = nValue;
}

int CPerformanceConfig::GetBreathControlTarget(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nBreathControlTarget[nTG];
}

void CPerformanceConfig::SetAftertouchRange(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nAftertouchRange[nTG] = nValue;
}

int CPerformanceConfig::GetAftertouchRange(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nAftertouchRange[nTG];
}

void CPerformanceConfig::SetAftertouchTarget(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nAftertouchTarget[nTG] = nValue;
}

int CPerformanceConfig::GetAftertouchTarget(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nAftertouchTarget[nTG];
}

void CPerformanceConfig::SetCompressorEnable(bool bValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_bCompressorEnable[nTG] = bValue;
}

bool CPerformanceConfig::GetCompressorEnable(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_bCompressorEnable[nTG];
}

void CPerformanceConfig::SetCompressorPreGain(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCompressorPreGain[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorPreGain(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCompressorPreGain[nTG];
}

void CPerformanceConfig::SetCompressorThresh(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCompressorThresh[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorThresh(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCompressorThresh[nTG];
}

void CPerformanceConfig::SetCompressorRatio(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCompressorRatio[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorRatio(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCompressorRatio[nTG];
}

void CPerformanceConfig::SetCompressorAttack(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCompressorAttack[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorAttack(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCompressorAttack[nTG];
}

void CPerformanceConfig::SetCompressorRelease(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCompressorRelease[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorRelease(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCompressorRelease[nTG];
}

void CPerformanceConfig::SetCompressorMakeupGain(int nValue, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nCompressorMakeupGain[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorMakeupGain(int nTG) const
{
	assert(nTG < CConfig::AllToneGenerators);
	return m_nCompressorMakeupGain[nTG];
}

void CPerformanceConfig::SetVoiceDataToTxt(const uint8_t *pData, int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	m_nVoiceDataTxt[nTG] = "";
	char nDtoH[] = "0123456789ABCDEF";
	for (int i = 0; i < NUM_VOICE_PARAM; i++)
	{
		m_nVoiceDataTxt[nTG] += nDtoH[(pData[i] & 0xF0) / 16];
		m_nVoiceDataTxt[nTG] += nDtoH[pData[i] & 0x0F];
		if (i < (NUM_VOICE_PARAM - 1))
		{
			m_nVoiceDataTxt[nTG] += " ";
		}
	}
}

void CPerformanceConfig::GetVoiceDataFromTxt(uint8_t pData[156], int nTG)
{
	assert(nTG < CConfig::AllToneGenerators);
	std::string nHtoD = "0123456789ABCDEF";

	for (int i = 0; i < NUM_VOICE_PARAM; ++i)
	{
		pData[i] = static_cast<uint8_t>(strtol(m_nVoiceDataTxt[nTG].c_str() + i * 3, nullptr, 16));
	}
}

bool CPerformanceConfig::VoiceDataFilled(int nTG)
{
	return m_nVoiceDataTxt[nTG].length() >= 156 * 3 - 1;
}

std::string CPerformanceConfig::GetPerformanceFileName(int nID)
{
	assert(nID < NUM_PERFORMANCES);
	std::string FileN = "";
	if ((m_nPerformanceBank == 0) && (nID == 0)) // in order to assure retrocompatibility
	{
		FileN += DEFAULT_PERFORMANCE_FILENAME;
	}
	else
	{
		// Build up from the index, "_", performance name, and ".ini"
		// NB: Index on disk = index in memory + 1
		std::string nIndex = "000000";
		nIndex += std::to_string(nID + 1);
		nIndex = nIndex.substr(nIndex.length() - 6, 6);
		FileN += nIndex;
		FileN += "_";
		FileN += m_PerformanceFileName[nID];
		FileN += ".ini";
	}
	return FileN;
}

std::string CPerformanceConfig::GetPerformanceFullFilePath(int nID)
{
	assert(nID < NUM_PERFORMANCES);
	std::string FileN;
	if ((m_nPerformanceBank == 0) && (nID == 0))
	{
		// Special case for the legacy Bank 1/Default performance
		FileN += DEFAULT_PERFORMANCE_FILENAME;
	}
	else
	{
		if (m_bPerformanceDirectoryExists)
		{
			FileN += PERFORMANCE_DIR;
			FileN += AddPerformanceBankDirName(m_nPerformanceBank);
			FileN += "/";
			FileN += GetPerformanceFileName(nID);
		}
	}
	return FileN;
}

std::string CPerformanceConfig::GetPerformanceName(int nID)
{
	assert(nID < NUM_PERFORMANCES);
	if ((m_nPerformanceBank == 0) && (nID == 0)) // in order to assure retrocompatibility
	{
		return DEFAULT_PERFORMANCE_NAME;
	}
	else
	{
		return m_PerformanceFileName[nID];
	}
}

int CPerformanceConfig::GetLastPerformance()
{
	return m_nLastPerformance;
}

int CPerformanceConfig::GetLastPerformanceBank()
{
	return m_nLastPerformanceBank;
}

int CPerformanceConfig::GetPerformanceID()
{
	return m_nPerformance;
}

bool CPerformanceConfig::GetInternalFolderOk()
{
	return m_bPerformanceDirectoryExists;
}

bool CPerformanceConfig::IsValidPerformance(int nID)
{
	if (nID < NUM_PERFORMANCES)
	{
		if (!m_PerformanceFileName[nID].empty())
		{
			return true;
		}
	}

	return false;
}

bool CPerformanceConfig::CheckFreePerformanceSlot()
{
	if (m_nLastPerformance < NUM_PERFORMANCES - 1)
	{
		// There is a free slot...
		return true;
	}
	else
	{
		return false;
	}
}

bool CPerformanceConfig::CreateNewPerformanceFile()
{
	if (!m_bPerformanceDirectoryExists)
	{
		// Nothing can be done if there is no performance directory
		LOGNOTE("Performance directory does not exist");
		return false;
	}
	if (m_nLastPerformance >= NUM_PERFORMANCES)
	{
		// No space left for new performances
		LOGWARN("No space left for new performance");
		return false;
	}

	// Note: New performances are created at the end of the currently selected bank.
	//       Sould we default to a new bank just for user-performances?
	//
	//       There is the possibility of MIDI changing the Bank Number and the user
	//       not spotting the bank has changed...
	//
	//       Another option would be to find empty slots in the current bank
	//       rather than always add at the end.
	//
	//       Sorting this out is left for a future update.

	std::string sPerformanceName = NewPerformanceName;
	NewPerformanceName = "";
	int nNewPerformance = m_nLastPerformance + 1;
	std::string nFileName;
	std::string nPath;
	std::string nIndex = "000000";
	nIndex += std::to_string(nNewPerformance + 1); // Index on disk = index in memory+1
	nIndex = nIndex.substr(nIndex.length() - 6, 6);

	nFileName = nIndex;
	nFileName += "_";
	if (sPerformanceName.empty())
	{
		nFileName += "Perf";
		nFileName += nIndex;
	}
	else
	{
		nFileName += sPerformanceName.substr(0, 14);
	}
	nFileName += ".ini";
	m_PerformanceFileName[nNewPerformance] = sPerformanceName;

	nPath = PERFORMANCE_DIR;
	nPath += AddPerformanceBankDirName(m_nPerformanceBank);
	nPath += "/";
	nFileName = nPath + nFileName;

	FIL File;
	FRESULT Result = f_open(&File, nFileName.c_str(), FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		m_PerformanceFileName[nNewPerformance].clear();
		return false;
	}

	if (f_close(&File) != FR_OK)
	{
		m_PerformanceFileName[nNewPerformance].clear();
		return false;
	}

	m_nLastPerformance = nNewPerformance;
	m_nPerformance = nNewPerformance;
	new (&m_Properties) CPropertiesFatFsFile(nFileName.c_str(), m_pFileSystem);

	return true;
}

bool CPerformanceConfig::ListPerformances()
{
	// Clear any existing lists of performances
	for (int i = 0; i < NUM_PERFORMANCES; i++)
	{
		m_PerformanceFileName[i].clear();
	}
	m_nLastPerformance = 0;
	if (m_nPerformanceBank == 0)
	{
		// The first bank is the default performance directory
		m_PerformanceFileName[0] = DEFAULT_PERFORMANCE_NAME; // in order to assure retrocompatibility
	}

	if (m_bPerformanceDirectoryExists)
	{
		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result;
		std::string PerfDir = PERFORMANCE_DIR + AddPerformanceBankDirName(m_nPerformanceBank);
#ifdef VERBOSE_DEBUG
		LOGNOTE("Listing Performances from %s", PerfDir.c_str());
#endif
		Result = f_opendir(&Directory, PerfDir.c_str());
		if (Result != FR_OK)
		{
			return false;
		}
		int nPIndex;

		Result = f_findfirst(&Directory, &FileInfo, PerfDir.c_str(), "*.ini");
		while (Result == FR_OK && FileInfo.fname[0])
		{
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))
			{
				std::string OriFileName = FileInfo.fname;
				size_t nLen = OriFileName.length();
				if (nLen > 8 && nLen < 26 && OriFileName.substr(6, 1) == "_")
				{
					// Note: m_nLastPerformance - refers to the number (index) of the last performance in memory,
					//       which includes a default performance.
					//
					//       Filenames on the disk start from 1 to match what the user might see in MIDI.
					//       So this means that actually file 000001_ will correspond to index position [0].
					//       For the default bank though, ID 1 is the default performance, so will already exist.
					//          m_PerformanceFileName[0] = default performance (file 000001)
					//          m_PerformanceFileName[1] = first available on-disk performance (file 000002)
					//
					// Note2: filenames assume 6 digits, underscore, name, finally ".ini"
					//        i.e.   123456_Performance Name.ini
					//
					nPIndex = stoi(OriFileName.substr(0, 6));
					if ((nPIndex < 1) || (nPIndex >= (NUM_PERFORMANCES + 1)))
					{
						// Index is out of range - skip to next file
						LOGNOTE("Performance number out of range: %s (%d to %d)", FileInfo.fname, 1, NUM_PERFORMANCES);
					}
					else
					{
						// Convert from "user facing" 1..indexed number to internal 0..indexed
						nPIndex = nPIndex - 1;
						if (m_PerformanceFileName[nPIndex].empty())
						{
							if (nPIndex > m_nLastPerformance)
							{
								m_nLastPerformance = nPIndex;
							}

							std::string FileName = OriFileName.substr(0, OriFileName.length() - 4).substr(7, 14);

							m_PerformanceFileName[nPIndex] = FileName;
#ifdef VERBOSE_DEBUG
							LOGNOTE("Loading performance %s (%d, %s)", OriFileName.c_str(), nPIndex, FileName.c_str());
#endif
						}
						else
						{
							LOGNOTE("Duplicate performance %s", OriFileName.c_str());
						}
					}
				}
			}

			Result = f_findnext(&Directory, &FileInfo);
		}
		f_closedir(&Directory);
	}

	return true;
}

void CPerformanceConfig::SetNewPerformance(int nID)
{
	assert(nID < NUM_PERFORMANCES);
	m_nPerformance = nID;
	std::string FileN = GetPerformanceFullFilePath(nID);

	new (&m_Properties) CPropertiesFatFsFile(FileN.c_str(), m_pFileSystem);
#ifdef VERBOSE_DEBUG
	LOGNOTE("Selecting Performance: %d (%s)", nID + 1, FileN.c_str());
#endif
}

int CPerformanceConfig::FindFirstPerformance()
{
	for (int nID = 0; nID < NUM_PERFORMANCES; nID++)
	{
		if (IsValidPerformance(nID))
		{
			return nID;
		}
	}

	return 0; // Even though 0 is a valid performance, not much else to do
}

std::string CPerformanceConfig::GetNewPerformanceDefaultName()
{
	std::string nIndex = "000000";
	nIndex += std::to_string(m_nLastPerformance + 1 + 1); // Convert from internal 0.. index to a file-based 1.. index to show the user
	nIndex = nIndex.substr(nIndex.length() - 6, 6);
	return "Perf" + nIndex;
}

void CPerformanceConfig::SetNewPerformanceName(const std::string &Name)
{
	NewPerformanceName = Name.substr(0, Name.find_last_not_of(' ') + 1);
}

bool CPerformanceConfig::DeletePerformance(int nID)
{
	if (!m_bPerformanceDirectoryExists)
	{
		// Nothing can be done if there is no performance directory
		LOGNOTE("Performance directory does not exist");
		return false;
	}
	bool bOK = false;
	if ((m_nPerformanceBank == 0) && (nID == 0)) { return bOK; } // default (performance.ini at root directory) can't be deleted
	DIR Directory;
	FILINFO FileInfo;
	std::string FileN = PERFORMANCE_DIR;
	FileN += AddPerformanceBankDirName(m_nPerformanceBank);

	FRESULT Result = f_findfirst(&Directory, &FileInfo, FileN.c_str(), GetPerformanceFileName(nID).c_str());
	if (Result == FR_OK && FileInfo.fname[0])
	{
		FileN += "/";
		FileN += GetPerformanceFileName(nID);
		Result = f_unlink(FileN.c_str());
		if (Result == FR_OK)
		{
			SetNewPerformance(0);
			m_nPerformance = 0;
			// nMenuSelectedPerformance=0;
			m_PerformanceFileName[nID].clear();
			// If this was the last performance in the bank...
			if (nID == m_nLastPerformance)
			{
				do
				{
					// Find the new last performance
					m_nLastPerformance--;
				} while (!IsValidPerformance(m_nLastPerformance) && (m_nLastPerformance > 0));
			}
			bOK = true;
		}
		else
		{
			LOGNOTE("Failed to delete %s", FileN.c_str());
		}
	}
	return bOK;
}

bool CPerformanceConfig::ListPerformanceBanks()
{
	m_nPerformanceBank = 0;
	m_nLastPerformance = 0;
	m_nLastPerformanceBank = 0;

	// Open performance directory
	DIR Directory;
	FILINFO FileInfo;
	FRESULT Result;
	Result = f_opendir(&Directory, PERFORMANCE_DIR);
	if (Result != FR_OK)
	{
		// No performance directory, so no performance banks.
		// So nothing else to do here
		LOGNOTE("No performance banks detected");
		m_bPerformanceDirectoryExists = false;
		return false;
	}

	int nNumBanks = 0;
	m_nLastPerformanceBank = 0;

	// List directories with names in format 01_Perf Bank Name
	Result = f_findfirst(&Directory, &FileInfo, PERFORMANCE_DIR, "*");
	while (Result == FR_OK && FileInfo.fname[0])
	{
		// Check to see if it is a directory
		if ((FileInfo.fattrib & AM_DIR) != 0)
		{
			// Looking for Performance banks of the form: 01_Perf Bank Name
			// So positions 0,1,2 = decimal digit
			//    position  3   = "_"
			//    positions 4.. = actual name
			//
			std::string OriFileName = FileInfo.fname;
			size_t nLen = OriFileName.length();
			if (nLen > 4 && nLen < 26 && OriFileName.substr(3, 1) == "_")
			{
				int nBankIndex = stoi(OriFileName.substr(0, 3));
				// Recall user index numbered 002..NUM_PERFORMANCE_BANKS
				// NB: Bank 001 is reserved for the default performance directory
				if ((nBankIndex > 0) && (nBankIndex <= NUM_PERFORMANCE_BANKS))
				{
					// Convert from "user facing" 1..indexed number to internal 0..indexed
					nBankIndex = nBankIndex - 1;
					if (m_PerformanceBankName[nBankIndex].empty())
					{
						std::string BankName = OriFileName.substr(4, nLen);

						m_PerformanceBankName[nBankIndex] = BankName;
#ifdef VERBOSE_DEBUG
						LOGNOTE("Found performance bank %s (%d, %s)", OriFileName.c_str(), nBankIndex, BankName.c_str());
#endif
						nNumBanks++;
						if (nBankIndex > m_nLastPerformanceBank)
						{
							m_nLastPerformanceBank = nBankIndex;
						}
					}
					else
					{
						LOGNOTE("Duplicate Performance Bank: %s", FileInfo.fname);
						if (nBankIndex == 0)
						{
							LOGNOTE("(Bank 001 is the default performance directory)");
						}
					}
				}
				else
				{
					LOGNOTE("Performance Bank number out of range: %s (%d to %d)", FileInfo.fname, 1, NUM_PERFORMANCE_BANKS);
				}
			}
			else
			{
#ifdef VERBOSE_DEBUG
				LOGNOTE("Skipping: %s", FileInfo.fname);
#endif
			}
		}

		Result = f_findnext(&Directory, &FileInfo);
	}

	if (nNumBanks > 0)
	{
		LOGNOTE("Number of Performance Banks: %d (last = %d)", nNumBanks, m_nLastPerformanceBank + 1);
	}

	f_closedir(&Directory);
	return true;
}

void CPerformanceConfig::SetNewPerformanceBank(int nBankID)
{
	assert(nBankID < NUM_PERFORMANCE_BANKS);
	if (IsValidPerformanceBank(nBankID))
	{
#ifdef VERBOSE_DEBUG
		LOGNOTE("Selecting Performance Bank: %d", nBankID + 1);
#endif
		m_nPerformanceBank = nBankID;
		ListPerformances();
		m_nPerformance = NUM_PERFORMANCES;
	}
	else
	{
#ifdef VERBOSE_DEBUG
		LOGNOTE("Not selecting invalid Performance Bank: %d", nBankID + 1);
#endif
	}
}

int CPerformanceConfig::GetPerformanceBankID()
{
	return m_nPerformanceBank;
}

std::string CPerformanceConfig::GetPerformanceBankName(int nBankID)
{
	assert(nBankID < NUM_PERFORMANCE_BANKS);
	if (IsValidPerformanceBank(nBankID))
	{
		return m_PerformanceBankName[nBankID];
	}
	return "";
}

std::string CPerformanceConfig::AddPerformanceBankDirName(int nBankID)
{
	assert(nBankID < NUM_PERFORMANCE_BANKS);
	if (IsValidPerformanceBank(nBankID))
	{
		// Performance Banks directories in format "001_Bank Name"
		std::string Index;
		if (nBankID < 9)
		{
			Index = "00" + std::to_string(nBankID + 1);
		}
		else if (nBankID < 99)
		{
			Index = "0" + std::to_string(nBankID + 1);
		}
		else
		{
			Index = std::to_string(nBankID + 1);
		}

		return "/" + Index + "_" + m_PerformanceBankName[nBankID];
	}
	else
	{
		return "";
	}
}

bool CPerformanceConfig::IsValidPerformanceBank(int nBankID)
{
	if (nBankID >= NUM_PERFORMANCE_BANKS)
	{
		return false;
	}
	if (m_PerformanceBankName[nBankID].empty())
	{
		return false;
	}
	return true;
}
