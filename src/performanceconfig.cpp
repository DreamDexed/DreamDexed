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
#include <circle/logger.h>
#include <arm_math.h>
#include "performanceconfig.h"
#include "mididevice.h"
#include "common.h"
#include <cstring> 
#include <algorithm>

LOGMODULE ("Performance");

//#define VERBOSE_DEBUG

#define PERFORMANCE_DIR "performance" 
#define DEFAULT_PERFORMANCE_FILENAME "performance.ini"
#define DEFAULT_PERFORMANCE_NAME "Default"

CPerformanceConfig::CPerformanceConfig (FATFS *pFileSystem)
:	m_Properties (DEFAULT_PERFORMANCE_FILENAME, pFileSystem)
{
	m_pFileSystem = pFileSystem; 
}

CPerformanceConfig::~CPerformanceConfig (void)
{
}

bool CPerformanceConfig::Init (unsigned nToneGenerators)
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

	// Check intermal performance directory exists
	DIR Directory;
	FRESULT Result;
	//Check if internal "performance" directory exists
	Result = f_opendir (&Directory, "SD:/" PERFORMANCE_DIR);
	if (Result == FR_OK)
	{
		m_bPerformanceDirectoryExists=true;		
		Result = f_closedir (&Directory);
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
	for (unsigned i=0; i<NUM_PERFORMANCE_BANKS; i++)
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

	LOGNOTE ("Loaded Default Performance Bank - Last Performance: %d", m_nLastPerformance + 1); // Show "user facing" index

	return true;
}

bool CPerformanceConfig::Load (void)
{
	if (!m_Properties.Load ())
	{
		return false;
	}

	bool bResult = false;

	for (unsigned nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
	{
		CString PropertyName;

		PropertyName.Format ("BankNumber%u", nTG+1);
		m_nBankNumber[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("VoiceNumber%u", nTG+1);
		m_nVoiceNumber[nTG] = m_Properties.GetNumber (PropertyName, 1);
		if (m_nVoiceNumber[nTG] > 0)
		{
			m_nVoiceNumber[nTG]--;
		}

		PropertyName.Format ("MIDIChannel%u", nTG+1);
		unsigned nMIDIChannel = m_Properties.GetNumber (PropertyName, 0);
		if (nMIDIChannel == 0)
		{
			m_nMIDIChannel[nTG] = CMIDIDevice::Disabled;
		}
		else if (nMIDIChannel <= CMIDIDevice::Channels)
		{
			m_nMIDIChannel[nTG] = nMIDIChannel-1;
			bResult = true;
		}
		else
		{
			m_nMIDIChannel[nTG] = CMIDIDevice::OmniMode;
			bResult = true;
		}

		PropertyName.Format ("Volume%u", nTG+1);
		m_nVolume[nTG] = m_Properties.GetNumber (PropertyName, 100);

		PropertyName.Format ("Pan%u", nTG+1);
		m_nPan[nTG] = m_Properties.GetNumber (PropertyName, 64);

		PropertyName.Format ("Detune%u", nTG+1);
		m_nDetune[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("Cutoff%u", nTG+1);
		m_nCutoff[nTG] = m_Properties.GetNumber (PropertyName, 99);

		PropertyName.Format ("Resonance%u", nTG+1);
		m_nResonance[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("NoteLimitLow%u", nTG+1);
		m_nNoteLimitLow[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("NoteLimitHigh%u", nTG+1);
		m_nNoteLimitHigh[nTG] = m_Properties.GetNumber (PropertyName, 127);

		PropertyName.Format ("NoteShift%u", nTG+1);
		m_nNoteShift[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("FX1Send%u", nTG+1);
		m_nFXSend[nTG][0] = m_Properties.GetNumber (PropertyName, 25);

		PropertyName.Format ("FX2Send%u", nTG+1);
		m_nFXSend[nTG][1] = m_Properties.GetNumber (PropertyName, 0);

		// compatibility ReverbSend[n] => FX1Send[n]
		PropertyName.Format ("ReverbSend%u", nTG+1);
		if (m_Properties.IsSet (PropertyName) && CConfig::FXChains)
		{
			// the volume calculated by x^4, but FxSend uses x^2
			float32_t reverbSend = m_Properties.GetNumber (PropertyName, 50);
			reverbSend = pow(mapfloat(reverbSend, 0.0f, 99.0f, 0.0f, 1.0f), 2);
			m_nFXSend[nTG][0] = mapfloat(reverbSend, 0.0f, 1.0f, 0, 99);
		}
	
		PropertyName.Format ("PitchBendRange%u", nTG+1);
		m_nPitchBendRange[nTG] = m_Properties.GetNumber (PropertyName, 2);

		PropertyName.Format ("PitchBendStep%u", nTG+1);
		m_nPitchBendStep[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("PortamentoMode%u", nTG+1);
		m_nPortamentoMode[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("PortamentoGlissando%u", nTG+1);
		m_nPortamentoGlissando[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("PortamentoTime%u", nTG+1);
		m_nPortamentoTime[nTG] = m_Properties.GetNumber (PropertyName, 0);
		
		PropertyName.Format ("VoiceData%u", nTG+1); 
		m_nVoiceDataTxt[nTG] = m_Properties.GetString (PropertyName, "");
		
		PropertyName.Format ("MonoMode%u", nTG+1);
		m_bMonoMode[nTG] = m_Properties.GetNumber (PropertyName, 0) != 0;
		
		PropertyName.Format ("TGLink%u", nTG+1);
		m_nTGLink[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("ModulationWheelRange%u", nTG+1);
		m_nModulationWheelRange[nTG] = m_Properties.GetNumber (PropertyName, 99); 
		
		PropertyName.Format ("ModulationWheelTarget%u", nTG+1);
		m_nModulationWheelTarget[nTG] = m_Properties.GetNumber (PropertyName, 1);
		
		PropertyName.Format ("FootControlRange%u", nTG+1);
		m_nFootControlRange[nTG] = m_Properties.GetNumber (PropertyName, 99); 
		
		PropertyName.Format ("FootControlTarget%u", nTG+1);
		m_nFootControlTarget[nTG] = m_Properties.GetNumber (PropertyName, 0);
		
		PropertyName.Format ("BreathControlRange%u", nTG+1);
		m_nBreathControlRange[nTG] = m_Properties.GetNumber (PropertyName, 99); 
		
		PropertyName.Format ("BreathControlTarget%u", nTG+1);
		m_nBreathControlTarget[nTG] = m_Properties.GetNumber (PropertyName, 0);
		
		PropertyName.Format ("AftertouchRange%u", nTG+1);
		m_nAftertouchRange[nTG] = m_Properties.GetNumber (PropertyName, 99); 
		
		PropertyName.Format ("AftertouchTarget%u", nTG+1);
		m_nAftertouchTarget[nTG] = m_Properties.GetNumber (PropertyName, 0);

		PropertyName.Format ("CompressorEnable%u", nTG+1);
		m_bCompressorEnable[nTG] = m_Properties.GetNumber (PropertyName, 1);
		
		PropertyName.Format ("CompressorPreGain%u", nTG+1);
		m_nCompressorPreGain[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("CompressorThresh%u", nTG+1);
		m_nCompressorThresh[nTG] = m_Properties.GetSignedNumber (PropertyName, -20);

		PropertyName.Format ("CompressorRatio%u", nTG+1);
		m_nCompressorRatio[nTG] = m_Properties.GetNumber (PropertyName, 5);

		PropertyName.Format ("CompressorAttack%u", nTG+1);
		m_nCompressorAttack[nTG] = m_Properties.GetNumber (PropertyName, 5);

		PropertyName.Format ("CompressorRelease%u", nTG+1);
		m_nCompressorRelease[nTG] = m_Properties.GetNumber (PropertyName, 200);

		PropertyName.Format ("CompressorMakeupGain%u", nTG+1);
		m_nCompressorMakeupGain[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("EQLow%u", nTG+1);
		m_nEQLow[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("EQMid%u", nTG+1);
		m_nEQMid[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("EQHigh%u", nTG+1);
		m_nEQHigh[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("EQGain%u", nTG+1);
		m_nEQGain[nTG] = m_Properties.GetSignedNumber (PropertyName, 0);

		PropertyName.Format ("EQLowMidFreq%u", nTG+1);
		m_nEQLowMidFreq[nTG] = m_Properties.GetNumber (PropertyName, 24);

		PropertyName.Format ("EQMidHighFreq%u", nTG+1);
		m_nEQMidHighFreq[nTG] = m_Properties.GetNumber (PropertyName, 44);
	}

	for (unsigned nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		CString PropertyName;

		for (unsigned nParam = 0; nParam < FX::FXParameterUnknown; ++nParam)
		{
			const FX::FXParameterType &p = FX::s_FXParameter[nParam];

			PropertyName.Format ("FX%u%s", nFX+1, p.Name);

			if (nParam >= FX::FXParameterSlot0 && nParam <= FX::FXParameterSlot2)
				m_nFXParameter[nFX][nParam] = FX::getIDFromEffectName(m_Properties.GetString (PropertyName, ""));
			else if (nParam == FX::FXParameterCloudSeed2Preset)
				m_nFXParameter[nFX][nParam] = FX::getIDFromCS2PresetName(m_Properties.GetString (PropertyName, ""));
			else
				m_nFXParameter[nFX][nParam] = m_Properties.GetSignedNumber (PropertyName, p.Default);
		}
	}

	m_nMasterEQLow = m_Properties.GetSignedNumber ("MasterEQLow", 0);
	m_nMasterEQMid = m_Properties.GetSignedNumber ("MasterEQMid", 0);
	m_nMasterEQHigh = m_Properties.GetSignedNumber ("MasterEQHigh", 0);
	m_nMasterEQGain = m_Properties.GetSignedNumber ("MasterEQGain", 0);
	m_nMasterEQLowMidFreq = m_Properties.GetNumber ("MasterEQLowMidFreq", 24);
	m_nMasterEQMidHighFreq = m_Properties.GetNumber ("MasterEQMidHighFreq", 44);

	m_bMasterCompressorEnable = m_Properties.GetNumber ("MasterCompressorEnable", 1);
	m_nMasterCompressorPreGain = m_Properties.GetSignedNumber ("MasterCompressorPreGain", 0);
	m_nMasterCompressorThresh = m_Properties.GetSignedNumber ("MasterCompressorThresh", -3);
	m_nMasterCompressorRatio = m_Properties.GetNumber ("MasterCompressorRatio", 20);
	m_nMasterCompressorAttack = m_Properties.GetNumber ("MasterCompressorAttack", 5);
	m_nMasterCompressorRelease = m_Properties.GetNumber ("MasterCompressorRelease", 5);
	m_bMasterCompressorHPFilterEnable = m_Properties.GetNumber ("MasterCompressorHPFilterEnable", 0);

	m_nMixerDryLevel = m_Properties.GetNumber ("MixerDryLevel", 99);

	// Compatibility
	if (m_Properties.IsSet ("CompressorEnable") && m_Properties.GetNumber ("CompressorEnable", 1) == 0)
	{
		for (unsigned nTG = 0; nTG < CConfig::AllToneGenerators; nTG++)
		{
			m_bCompressorEnable[nTG] = 0;
		}
	}

	if (m_Properties.IsSet ("ReverbEnable") && CConfig::FXChains)
	{
		// setup Reverb to FX1
		m_nFXParameter[0][FX::FXParameterSlot0] = FX::getIDFromEffectName("PlateReverb");
		m_nFXParameter[0][FX::FXParameterPlateReverbMix] = m_Properties.GetNumber ("ReverbEnable", 1) ? 100 : 0;
		m_nFXParameter[0][FX::FXParameterPlateReverbSize] = m_Properties.GetNumber ("ReverbSize", 70);
		m_nFXParameter[0][FX::FXParameterPlateReverbHighDamp] = m_Properties.GetNumber ("ReverbHighDamp", 50);
		m_nFXParameter[0][FX::FXParameterPlateReverbLowDamp] = m_Properties.GetNumber ("ReverbLowDamp", 50);
		m_nFXParameter[0][FX::FXParameterPlateReverbLowPass] = m_Properties.GetNumber ("ReverbLowPass", 30);
		m_nFXParameter[0][FX::FXParameterPlateReverbDiffusion] = m_Properties.GetNumber ("ReverbDiffusion", 65);
		m_nFXParameter[0][FX::FXParameterReturnLevel] = m_Properties.GetNumber ("ReverbEnable", 1) ? m_Properties.GetNumber ("ReverbLevel", 99) : 0;
	}

	return bResult;
}

bool CPerformanceConfig::Save (void)
{
	m_Properties.RemoveAll ();

	for (unsigned nTG = 0; nTG < m_nToneGenerators; nTG++)
	{
		CString PropertyName;

		PropertyName.Format ("BankNumber%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nBankNumber[nTG]);

		PropertyName.Format ("VoiceNumber%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nVoiceNumber[nTG]+1);

		PropertyName.Format ("MIDIChannel%u", nTG+1);
		unsigned nMIDIChannel = m_nMIDIChannel[nTG];
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
		m_Properties.SetNumber (PropertyName, nMIDIChannel);

		PropertyName.Format ("Volume%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nVolume[nTG]);

		PropertyName.Format ("Pan%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nPan[nTG]);

		PropertyName.Format ("Detune%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nDetune[nTG]);

		PropertyName.Format ("Cutoff%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nCutoff[nTG]);

		PropertyName.Format ("Resonance%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nResonance[nTG]);

		PropertyName.Format ("NoteLimitLow%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nNoteLimitLow[nTG]);

		PropertyName.Format ("NoteLimitHigh%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nNoteLimitHigh[nTG]);

		PropertyName.Format ("NoteShift%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nNoteShift[nTG]);

		for (unsigned nFX = 0; nFX < CConfig::FXChains; ++nFX)
		{
			PropertyName.Format ("FX%uSend%u", nFX+1, nTG+1);
			m_Properties.SetNumber (PropertyName, m_nFXSend[nTG][nFX]);
		}

		PropertyName.Format ("PitchBendRange%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nPitchBendRange[nTG]);

		PropertyName.Format ("PitchBendStep%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nPitchBendStep[nTG]);

		PropertyName.Format ("PortamentoMode%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nPortamentoMode[nTG]);

		PropertyName.Format ("PortamentoGlissando%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nPortamentoGlissando[nTG]);

		PropertyName.Format ("PortamentoTime%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nPortamentoTime[nTG]);
		
		PropertyName.Format ("VoiceData%u", nTG+1);
		char *cstr = &m_nVoiceDataTxt[nTG][0];
		m_Properties.SetString (PropertyName, cstr);
		
		PropertyName.Format ("MonoMode%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_bMonoMode[nTG] ? 1 : 0);

		PropertyName.Format ("TGLink%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nTGLink[nTG]);

		PropertyName.Format ("ModulationWheelRange%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nModulationWheelRange[nTG]);
	
		PropertyName.Format ("ModulationWheelTarget%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nModulationWheelTarget[nTG]);	
			
		PropertyName.Format ("FootControlRange%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nFootControlRange[nTG]);	
		
		PropertyName.Format ("FootControlTarget%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nFootControlTarget[nTG]);	
		
		PropertyName.Format ("BreathControlRange%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nBreathControlRange[nTG]);	
		
		PropertyName.Format ("BreathControlTarget%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nBreathControlTarget[nTG]);	
		
		PropertyName.Format ("AftertouchRange%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nAftertouchRange[nTG]);	
		
		PropertyName.Format ("AftertouchTarget%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nAftertouchTarget[nTG]);			

		PropertyName.Format ("CompressorEnable%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_bCompressorEnable[nTG]);

		PropertyName.Format ("CompressorPreGain%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nCompressorPreGain[nTG]);

		PropertyName.Format ("CompressorThresh%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nCompressorThresh[nTG]);

		PropertyName.Format ("CompressorRatio%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nCompressorRatio[nTG]);

		PropertyName.Format ("CompressorAttack%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nCompressorAttack[nTG]);

		PropertyName.Format ("CompressorRelease%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nCompressorRelease[nTG]);

		PropertyName.Format ("CompressorMakeupGain%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nCompressorMakeupGain[nTG]);

		PropertyName.Format ("EQLow%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nEQLow[nTG]);

		PropertyName.Format ("EQMid%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nEQMid[nTG]);

		PropertyName.Format ("EQHigh%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nEQHigh[nTG]);

		PropertyName.Format ("EQGain%u", nTG+1);
		m_Properties.SetSignedNumber (PropertyName, m_nEQGain[nTG]);

		PropertyName.Format ("EQLowMidFreq%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nEQLowMidFreq[nTG]);

		PropertyName.Format ("EQMidHighFreq%u", nTG+1);
		m_Properties.SetNumber (PropertyName, m_nEQMidHighFreq[nTG]);

	}

	for (unsigned nFX = 0; nFX < CConfig::FXChains; ++nFX)
	{
		CString PropertyName;

		for (unsigned nSlot = 0; nSlot < 3; ++nSlot)
		{
			unsigned nSlotParam = FX::FXParameterSlot0 + nSlot;
			unsigned nEffectID = m_nFXParameter[nFX][nSlotParam];
			const FX::EffectType &effect = FX::s_effects[nEffectID];

			PropertyName.Format ("FX%u%s", nFX+1, FX::s_FXParameter[nSlotParam].Name);
			m_Properties.SetString (PropertyName, effect.Name);
		}

		for (unsigned nSlot = 0; nSlot < 3; ++nSlot)
		{
			unsigned nSlotParam = FX::FXParameterSlot0 + nSlot;
			unsigned nEffectID = m_nFXParameter[nFX][nSlotParam];
			const FX::EffectType &effect = FX::s_effects[nEffectID];

			if (nEffectID == 0) continue;
		
			for (unsigned nParam = effect.MinID; nParam <= effect.MaxID; ++nParam)
			{
				PropertyName.Format ("FX%u%s", nFX+1, FX::s_FXParameter[nParam].Name);

				if (nParam == FX::FXParameterCloudSeed2Preset)
					m_Properties.SetString (PropertyName, FX::getCS2PresetName(m_nFXParameter[nFX][nParam], 0).c_str());
				else
					m_Properties.SetSignedNumber (PropertyName, m_nFXParameter[nFX][nParam]);
			}
		}

		PropertyName.Format ("FX%u%s", nFX+1, FX::s_FXParameter[FX::FXParameterReturnLevel].Name);
		m_Properties.SetSignedNumber (PropertyName, m_nFXParameter[nFX][FX::FXParameterReturnLevel]);
	}

	m_Properties.SetSignedNumber ("MasterEQLow", m_nMasterEQLow);
	m_Properties.SetSignedNumber ("MasterEQMid", m_nMasterEQMid);
	m_Properties.SetSignedNumber ("MasterEQHigh", m_nMasterEQHigh);
	m_Properties.SetSignedNumber ("MasterEQGain", m_nMasterEQGain);
	m_Properties.SetNumber ("MasterEQLowMidFreq", m_nMasterEQLowMidFreq);
	m_Properties.SetNumber ("MasterEQMidHighFreq", m_nMasterEQMidHighFreq);

	m_Properties.SetNumber ("MasterCompressorEnable", m_bMasterCompressorEnable);
	m_Properties.SetSignedNumber ("MasterCompressorPreGain", m_nMasterCompressorPreGain);
	m_Properties.SetSignedNumber ("MasterCompressorThresh", m_nMasterCompressorThresh);
	m_Properties.SetNumber ("MasterCompressorRatio", m_nMasterCompressorRatio);
	m_Properties.SetNumber ("MasterCompressorAttack", m_nMasterCompressorAttack);
	m_Properties.SetNumber ("MasterCompressorRelease", m_nMasterCompressorRelease);
	m_Properties.SetNumber ("MasterCompressorHPFilterEnable", m_bMasterCompressorHPFilterEnable);

	m_Properties.SetNumber ("MixerDryLevel", m_nMixerDryLevel);

	return m_Properties.Save ();
}

unsigned CPerformanceConfig::GetBankNumber (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nBankNumber[nTG];
}

unsigned CPerformanceConfig::GetVoiceNumber (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nVoiceNumber[nTG];
}

unsigned CPerformanceConfig::GetMIDIChannel (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nMIDIChannel[nTG];
}

unsigned CPerformanceConfig::GetVolume (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nVolume[nTG];
}

unsigned CPerformanceConfig::GetPan (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nPan[nTG];
}

int CPerformanceConfig::GetDetune (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nDetune[nTG];
}

unsigned CPerformanceConfig::GetCutoff (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCutoff[nTG];
}

unsigned CPerformanceConfig::GetResonance (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nResonance[nTG];
}

unsigned CPerformanceConfig::GetNoteLimitLow (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nNoteLimitLow[nTG];
}

unsigned CPerformanceConfig::GetNoteLimitHigh (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nNoteLimitHigh[nTG];
}

int CPerformanceConfig::GetNoteShift (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nNoteShift[nTG];
}

unsigned CPerformanceConfig::GetFXSend (unsigned nTG, unsigned nFX) const
{
	assert (nTG < CConfig::AllToneGenerators);
	assert (nFX < CConfig::FXChains);
	return m_nFXSend[nTG][nFX];
}

void CPerformanceConfig::SetBankNumber (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nBankNumber[nTG] = nValue;
}

void CPerformanceConfig::SetVoiceNumber (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nVoiceNumber[nTG] = nValue;
}

void CPerformanceConfig::SetMIDIChannel (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nMIDIChannel[nTG] = nValue;
}

void CPerformanceConfig::SetVolume (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nVolume[nTG] = nValue;
}

void CPerformanceConfig::SetPan (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nPan[nTG] = nValue;
}

void CPerformanceConfig::SetDetune (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nDetune[nTG] = nValue;
}

void CPerformanceConfig::SetCutoff (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCutoff[nTG] = nValue;
}

void CPerformanceConfig::SetResonance (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nResonance[nTG] = nValue;
}

void CPerformanceConfig::SetNoteLimitLow (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nNoteLimitLow[nTG] = nValue;
}

void CPerformanceConfig::SetNoteLimitHigh (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nNoteLimitHigh[nTG] = nValue;
}

void CPerformanceConfig::SetNoteShift (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nNoteShift[nTG] = nValue;
}

void CPerformanceConfig::SetFXSend (unsigned nValue, unsigned nTG, unsigned nFX)
{
	assert (nTG < CConfig::AllToneGenerators);
	assert (nFX < CConfig::FXChains);
	m_nFXSend[nTG][nFX] = nValue;
}

int CPerformanceConfig::GetEQLow (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nEQLow[nTG];
}

int CPerformanceConfig::GetEQMid (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nEQMid[nTG];
}

int CPerformanceConfig::GetEQHigh (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nEQHigh[nTG];
}

int CPerformanceConfig::GetEQGain (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nEQGain[nTG];
}

unsigned CPerformanceConfig::GetEQLowMidFreq (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nEQLowMidFreq[nTG];
}

unsigned CPerformanceConfig::GetEQMidHighFreq (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nEQMidHighFreq[nTG];
}

void CPerformanceConfig::SetEQLow (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nEQLow[nTG] = nValue;
}

void CPerformanceConfig::SetEQMid (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nEQMid[nTG] = nValue;
}

void CPerformanceConfig::SetEQHigh (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nEQHigh[nTG] = nValue;
}

void CPerformanceConfig::SetEQGain (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nEQGain[nTG] = nValue;
}

void CPerformanceConfig::SetEQLowMidFreq (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nEQLowMidFreq[nTG] = nValue;
}

void CPerformanceConfig::SetEQMidHighFreq (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nEQMidHighFreq[nTG] = nValue;
}

int CPerformanceConfig::GetFXParameter (FX::TFXParameter nParameter, unsigned nFX) const
{
	assert (nFX < CConfig::FXChains);
	assert (nParameter < FX::FXParameterUnknown);
	return m_nFXParameter[nFX][nParameter];
}

void CPerformanceConfig::SetFXParameter (FX::TFXParameter nParameter, int nValue, unsigned nFX)
{
	assert (nFX < CConfig::FXChains);
	assert (nParameter < FX::FXParameterUnknown);
	m_nFXParameter[nFX][nParameter] = nValue;
}

int CPerformanceConfig::GetMasterEQLow () const
{
	return m_nMasterEQLow;
}

int CPerformanceConfig::GetMasterEQMid () const
{
	return m_nMasterEQMid;
}

int CPerformanceConfig::GetMasterEQHigh () const
{
	return m_nMasterEQHigh;
}

int CPerformanceConfig::GetMasterEQGain () const
{
	return m_nMasterEQGain;
}

unsigned CPerformanceConfig::GetMasterEQLowMidFreq () const
{
	return m_nMasterEQLowMidFreq;
}

unsigned CPerformanceConfig::GetMasterEQMidHighFreq () const
{
	return m_nMasterEQMidHighFreq;
}

void CPerformanceConfig::SetMasterEQLow (int nValue)
{
	m_nMasterEQLow = nValue;
}

void CPerformanceConfig::SetMasterEQMid (int nValue)
{
	m_nMasterEQMid = nValue;
}

void CPerformanceConfig::SetMasterEQHigh (int nValue)
{
	m_nMasterEQHigh = nValue;
}

void CPerformanceConfig::SetMasterEQGain (int nValue)
{
	m_nMasterEQGain = nValue;
}

void CPerformanceConfig::SetMasterEQLowMidFreq (unsigned nValue)
{
	m_nMasterEQLowMidFreq = nValue;
}

void CPerformanceConfig::SetMasterEQMidHighFreq (unsigned nValue)
{
	m_nMasterEQMidHighFreq = nValue;
}

bool CPerformanceConfig::GetMasterCompressorEnable () const
{
	return m_bMasterCompressorEnable;
}

int CPerformanceConfig::GetMasterCompressorPreGain () const
{
	return m_nMasterCompressorPreGain;
}

int CPerformanceConfig::GetMasterCompressorThresh () const
{
	return m_nMasterCompressorThresh;
}

unsigned CPerformanceConfig::GetMasterCompressorRatio () const
{
	return m_nMasterCompressorRatio;
}

unsigned CPerformanceConfig::GetMasterCompressorAttack () const
{
	return m_nMasterCompressorAttack;
}

unsigned CPerformanceConfig::GetMasterCompressorRelease () const
{
	return m_nMasterCompressorRelease;
}

bool CPerformanceConfig::GetMasterCompressorHPFilterEnable () const
{
	return m_bMasterCompressorHPFilterEnable;
}

void CPerformanceConfig::SetMasterCompressorEnable (bool nValue)
{
	m_bMasterCompressorEnable = nValue;
}

void CPerformanceConfig::SetMasterCompressorPreGain (int nValue)
{
	m_nMasterCompressorPreGain= nValue;
}

void CPerformanceConfig::SetMasterCompressorThresh (int nValue)
{
	m_nMasterCompressorThresh = nValue;
}

void CPerformanceConfig::SetMasterCompressorRatio (unsigned nValue)
{
	m_nMasterCompressorRatio = nValue;
}

void CPerformanceConfig::SetMasterCompressorAttack (unsigned nValue)
{
	m_nMasterCompressorAttack = nValue;
}

void CPerformanceConfig::SetMasterCompressorRelease (unsigned nValue)
{
	m_nMasterCompressorRelease = nValue;
}

void CPerformanceConfig::SetMasterCompressorHPFilterEnable (bool nValue)
{
	m_bMasterCompressorHPFilterEnable = nValue;
}

unsigned CPerformanceConfig::GetMixerDryLevel () const
{
	return m_nMixerDryLevel;
}

void CPerformanceConfig::SetMixerDryLevel (unsigned nValue)
{
	m_nMixerDryLevel = nValue;
}

// Pitch bender and portamento:
void CPerformanceConfig::SetPitchBendRange (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nPitchBendRange[nTG] = nValue;
}

unsigned CPerformanceConfig::GetPitchBendRange (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nPitchBendRange[nTG];
}


void CPerformanceConfig::SetPitchBendStep (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nPitchBendStep[nTG] = nValue;
}

unsigned CPerformanceConfig::GetPitchBendStep (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nPitchBendStep[nTG];
}


void CPerformanceConfig::SetPortamentoMode (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nPortamentoMode[nTG] = nValue;
}

unsigned CPerformanceConfig::GetPortamentoMode (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nPortamentoMode[nTG];
}


void CPerformanceConfig::SetPortamentoGlissando (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nPortamentoGlissando[nTG] = nValue;
}

unsigned CPerformanceConfig::GetPortamentoGlissando (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nPortamentoGlissando[nTG];
}


void CPerformanceConfig::SetPortamentoTime (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nPortamentoTime[nTG] = nValue;
}

unsigned CPerformanceConfig::GetPortamentoTime (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nPortamentoTime[nTG];
}

void CPerformanceConfig::SetMonoMode (bool bValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_bMonoMode[nTG] = bValue;
}

bool CPerformanceConfig::GetMonoMode (unsigned nTG) const
{
	return m_bMonoMode[nTG];
}

void CPerformanceConfig::SetTGLink (unsigned nTGLink, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nTGLink[nTG] = nTGLink;
}

unsigned CPerformanceConfig::GetTGLink (unsigned nTG) const
{
	return m_nTGLink[nTG];
}

void CPerformanceConfig::SetModulationWheelRange (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nModulationWheelRange[nTG] = nValue;
}

unsigned CPerformanceConfig::GetModulationWheelRange (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nModulationWheelRange[nTG];
}

void CPerformanceConfig::SetModulationWheelTarget (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nModulationWheelTarget[nTG] = nValue;
}

unsigned CPerformanceConfig::GetModulationWheelTarget (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nModulationWheelTarget[nTG];
}

void CPerformanceConfig::SetFootControlRange (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nFootControlRange[nTG] = nValue;
}

unsigned CPerformanceConfig::GetFootControlRange (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nFootControlRange[nTG];
}

void CPerformanceConfig::SetFootControlTarget (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nFootControlTarget[nTG] = nValue;
}

unsigned CPerformanceConfig::GetFootControlTarget (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nFootControlTarget[nTG];
}

void CPerformanceConfig::SetBreathControlRange (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nBreathControlRange[nTG] = nValue;
}

unsigned CPerformanceConfig::GetBreathControlRange (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nBreathControlRange[nTG];
}

void CPerformanceConfig::SetBreathControlTarget (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nBreathControlTarget[nTG] = nValue;
}

unsigned CPerformanceConfig::GetBreathControlTarget (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nBreathControlTarget[nTG];
}

void CPerformanceConfig::SetAftertouchRange (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nAftertouchRange[nTG] = nValue;
}

unsigned CPerformanceConfig::GetAftertouchRange (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nAftertouchRange[nTG];
}

void CPerformanceConfig::SetAftertouchTarget (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nAftertouchTarget[nTG] = nValue;
}

unsigned CPerformanceConfig::GetAftertouchTarget (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nAftertouchTarget[nTG];
}

void CPerformanceConfig::SetCompressorEnable (bool bValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_bCompressorEnable[nTG] = bValue;
}

bool CPerformanceConfig::GetCompressorEnable (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_bCompressorEnable[nTG];
}

void CPerformanceConfig::SetCompressorPreGain (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCompressorPreGain[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorPreGain (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCompressorPreGain[nTG];
}

void CPerformanceConfig::SetCompressorThresh (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCompressorThresh[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorThresh (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCompressorThresh[nTG];
}

void CPerformanceConfig::SetCompressorRatio (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCompressorRatio[nTG] = nValue;
}

unsigned CPerformanceConfig::GetCompressorRatio (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCompressorRatio[nTG];
}

void CPerformanceConfig::SetCompressorAttack (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCompressorAttack[nTG] = nValue;
}

unsigned CPerformanceConfig::GetCompressorAttack (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCompressorAttack[nTG];
}

void CPerformanceConfig::SetCompressorRelease (unsigned nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCompressorRelease[nTG] = nValue;
}

unsigned CPerformanceConfig::GetCompressorRelease (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCompressorRelease[nTG];
}

void CPerformanceConfig::SetCompressorMakeupGain (int nValue, unsigned nTG)
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nCompressorMakeupGain[nTG] = nValue;
}

int CPerformanceConfig::GetCompressorMakeupGain (unsigned nTG) const
{
	assert (nTG < CConfig::AllToneGenerators);
	return m_nCompressorMakeupGain[nTG];
}


void CPerformanceConfig::SetVoiceDataToTxt (const uint8_t *pData, unsigned nTG)  
{
	assert (nTG < CConfig::AllToneGenerators);
	m_nVoiceDataTxt[nTG] = "";
	char nDtoH[]="0123456789ABCDEF";
	for (int i = 0; i < NUM_VOICE_PARAM; i++)
	{
	m_nVoiceDataTxt[nTG]  += nDtoH[(pData[i] & 0xF0)/16];
	m_nVoiceDataTxt[nTG]  += nDtoH[pData[i] & 0x0F]  ;
	if ( i < (NUM_VOICE_PARAM-1)  ) 
    	{
     	 m_nVoiceDataTxt[nTG] += " ";
    	}
	}
}

uint8_t *CPerformanceConfig::GetVoiceDataFromTxt (unsigned nTG) 
{
	assert (nTG < CConfig::AllToneGenerators);
	static uint8_t pData[NUM_VOICE_PARAM];
	std::string nHtoD="0123456789ABCDEF";
	 
 	for (int i=0; i<NUM_VOICE_PARAM * 3; i=i+3)
	{
  	pData[i/3] = ((nHtoD.find(toupper(m_nVoiceDataTxt[nTG][i]),0) * 16 + nHtoD.find(toupper(m_nVoiceDataTxt[nTG][i+1]),0))) & 0xFF ;
	} 
  
	return pData;
}

bool CPerformanceConfig::VoiceDataFilled(unsigned nTG) 
{
	return (strcmp(m_nVoiceDataTxt[nTG].c_str(),"") != 0) ;
}

std::string CPerformanceConfig::GetPerformanceFileName(unsigned nID)
{
	assert (nID < NUM_PERFORMANCES);
	std::string FileN = "";
	if ((m_nPerformanceBank==0) && (nID == 0)) // in order to assure retrocompatibility
	{
		FileN += DEFAULT_PERFORMANCE_FILENAME;
	}
	else
	{
		// Build up from the index, "_", performance name, and ".ini"
		// NB: Index on disk = index in memory + 1
		std::string nIndex = "000000";
		nIndex += std::to_string(nID+1);
		nIndex = nIndex.substr(nIndex.length()-6,6);
		FileN += nIndex;
		FileN += "_";
		FileN += m_PerformanceFileName[nID];
		FileN += ".ini";
	}
	return FileN;
}

std::string CPerformanceConfig::GetPerformanceFullFilePath(unsigned nID)
{
	assert (nID < NUM_PERFORMANCES);
	std::string FileN = "SD:/";
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

std::string CPerformanceConfig::GetPerformanceName(unsigned nID)
{
	assert (nID < NUM_PERFORMANCES);
	if ((m_nPerformanceBank==0) && (nID == 0)) // in order to assure retrocompatibility
	{
		return DEFAULT_PERFORMANCE_NAME;
	}
	else
	{
		return m_PerformanceFileName[nID];
	}
}

unsigned CPerformanceConfig::GetLastPerformance()
{
	return m_nLastPerformance;
}

unsigned CPerformanceConfig::GetLastPerformanceBank()
{
	return m_nLastPerformanceBank;
}

unsigned CPerformanceConfig::GetActualPerformanceID()
{
	return m_nActualPerformance;
}

void CPerformanceConfig::SetActualPerformanceID(unsigned nID)
{
	assert (nID < NUM_PERFORMANCES);
	m_nActualPerformance = nID;
}

unsigned CPerformanceConfig::GetActualPerformanceBankID()
{
	return m_nActualPerformanceBank;
}

void CPerformanceConfig::SetActualPerformanceBankID(unsigned nBankID)
{
	assert (nBankID < NUM_PERFORMANCE_BANKS);
	m_nActualPerformanceBank = nBankID;
}

bool CPerformanceConfig::GetInternalFolderOk()
{
	return m_bPerformanceDirectoryExists;
}

bool CPerformanceConfig::IsValidPerformance(unsigned nID)
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


bool CPerformanceConfig::CheckFreePerformanceSlot(void)
{
	if (m_nLastPerformance < NUM_PERFORMANCES-1)
	{
		// There is a free slot...
		return true;
	}
	else
	{
		return false;
	}
}

bool CPerformanceConfig::CreateNewPerformanceFile(void)
{
	if (!m_bPerformanceDirectoryExists)
	{
		// Nothing can be done if there is no performance directory
		LOGNOTE("Performance directory does not exist");
		return false;
	}
	if (m_nLastPerformance >= NUM_PERFORMANCES) {
		// No space left for new performances
		LOGWARN ("No space left for new performance");
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
	NewPerformanceName=""; 
	unsigned nNewPerformance = m_nLastPerformance + 1;
	std::string nFileName;
	std::string nPath;
	std::string nIndex = "000000";
	nIndex += std::to_string(nNewPerformance+1);  // Index on disk = index in memory+1
	nIndex = nIndex.substr(nIndex.length()-6,6);
	

	nFileName = nIndex;
	nFileName += "_";
	if (strcmp(sPerformanceName.c_str(),"") == 0)
	{
		nFileName += "Perf";
		nFileName += nIndex;
	}		
	else
	{
		nFileName +=sPerformanceName.substr(0,14);
	}
	nFileName += ".ini";
	m_PerformanceFileName[nNewPerformance]= sPerformanceName;
	
	nPath = "SD:/" ;
	nPath += PERFORMANCE_DIR;
	nPath += AddPerformanceBankDirName(m_nPerformanceBank);
	nPath += "/";
	nFileName = nPath + nFileName;
	
	FIL File;
	FRESULT Result = f_open (&File, nFileName.c_str(), FA_WRITE | FA_CREATE_ALWAYS);
	if (Result != FR_OK)
	{
		m_PerformanceFileName[nNewPerformance].clear();
		return false;
	}

	if (f_close (&File) != FR_OK)
	{
		m_PerformanceFileName[nNewPerformance].clear();
		return false;
	}
	
	m_nLastPerformance = nNewPerformance;
	m_nActualPerformance = nNewPerformance;
	new (&m_Properties) CPropertiesFatFsFile(nFileName.c_str(), m_pFileSystem);
	
	return true;
}

bool CPerformanceConfig::ListPerformances()
{
	// Clear any existing lists of performances
	for (unsigned i=0; i<NUM_PERFORMANCES; i++)
	{
		m_PerformanceFileName[i].clear();
	}
	m_nLastPerformance=0;
	if (m_nPerformanceBank == 0)
	{
		// The first bank is the default performance directory
	   	m_PerformanceFileName[0]=DEFAULT_PERFORMANCE_NAME; // in order to assure retrocompatibility
	}
	
	if (m_bPerformanceDirectoryExists)
	{
		DIR Directory;
		FILINFO FileInfo;
		FRESULT Result;
		std::string PerfDir = "SD:/" PERFORMANCE_DIR + AddPerformanceBankDirName(m_nPerformanceBank);
#ifdef VERBOSE_DEBUG
		LOGNOTE("Listing Performances from %s", PerfDir.c_str());
#endif
		Result = f_opendir (&Directory, PerfDir.c_str());
		if (Result != FR_OK)
		{
			return false;
		}
		unsigned nPIndex;

		Result = f_findfirst (&Directory, &FileInfo, PerfDir.c_str(), "*.ini");
		for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
		{
			if (!(FileInfo.fattrib & (AM_HID | AM_SYS)))  
			{
				std::string OriFileName = FileInfo.fname;
				size_t nLen = OriFileName.length();
				if (   nLen > 8 && nLen <26	 && strcmp(OriFileName.substr(6,1).c_str(), "_")==0)
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
					nPIndex=stoi(OriFileName.substr(0,6));
					if ((nPIndex < 1) || (nPIndex >= (NUM_PERFORMANCES+1)))
					{
						// Index is out of range - skip to next file
						LOGNOTE ("Performance number out of range: %s (%d to %d)", FileInfo.fname, 1, NUM_PERFORMANCES);
					}
					else
					{
						// Convert from "user facing" 1..indexed number to internal 0..indexed
						nPIndex = nPIndex-1;
						if (m_PerformanceFileName[nPIndex].empty())
						{
							if(nPIndex > m_nLastPerformance)
							{
								m_nLastPerformance=nPIndex;
							}

							std::string FileName = OriFileName.substr(0,OriFileName.length()-4).substr(7,14);

							m_PerformanceFileName[nPIndex] = FileName;
#ifdef VERBOSE_DEBUG
							LOGNOTE ("Loading performance %s (%d, %s)", OriFileName.c_str(), nPIndex, FileName.c_str());
#endif
						}
						else
						{
							LOGNOTE ("Duplicate performance %s", OriFileName.c_str());
						}
					}
				}
			}

			Result = f_findnext (&Directory, &FileInfo);
		}
		f_closedir (&Directory);
	}
	
	return true;
}

void CPerformanceConfig::SetNewPerformance (unsigned nID)
{
	assert (nID < NUM_PERFORMANCES);
	m_nActualPerformance=nID;
	std::string FileN = GetPerformanceFullFilePath(nID);

	new (&m_Properties) CPropertiesFatFsFile(FileN.c_str(), m_pFileSystem);
#ifdef VERBOSE_DEBUG
	LOGNOTE("Selecting Performance: %d (%s)", nID+1, FileN.c_str());
#endif
}

unsigned CPerformanceConfig::FindFirstPerformance (void)
{
	for (int nID=0; nID < NUM_PERFORMANCES; nID++)
	{
		if (IsValidPerformance(nID))
		{
			return nID;
		}
	}

	return 0; // Even though 0 is a valid performance, not much else to do
}

std::string CPerformanceConfig::GetNewPerformanceDefaultName(void)
{
	std::string nIndex = "000000";
	nIndex += std::to_string(m_nLastPerformance+1+1); // Convert from internal 0.. index to a file-based 1.. index to show the user
	nIndex = nIndex.substr(nIndex.length()-6,6);
	return "Perf" + nIndex;
}

void CPerformanceConfig::SetNewPerformanceName(const std::string &Name)
{
	NewPerformanceName = Name.substr(0, Name.find_last_not_of(' ') + 1);
}

bool CPerformanceConfig::DeletePerformance(unsigned nID)
{
	if (!m_bPerformanceDirectoryExists)
	{
		// Nothing can be done if there is no performance directory
		LOGNOTE("Performance directory does not exist");
		return false;
	}
	bool bOK = false;
	if((m_nPerformanceBank == 0) && (nID == 0)){return bOK;} // default (performance.ini at root directory) can't be deleted
	DIR Directory;
	FILINFO FileInfo;
	std::string FileN = "SD:/";
	FileN += PERFORMANCE_DIR;
	FileN += AddPerformanceBankDirName(m_nPerformanceBank);
	
	FRESULT Result = f_findfirst (&Directory, &FileInfo, FileN.c_str(), GetPerformanceFileName(nID).c_str());
	if (Result == FR_OK && FileInfo.fname[0])
	{
		FileN += "/";
		FileN += GetPerformanceFileName(nID);
		Result=f_unlink (FileN.c_str());
		if (Result == FR_OK)
		{
			SetNewPerformance(0);
			m_nActualPerformance =0;
			//nMenuSelectedPerformance=0;
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
			bOK=true;
		}
		else
		{
			LOGNOTE ("Failed to delete %s", FileN.c_str());
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
	Result = f_opendir (&Directory, "SD:/" PERFORMANCE_DIR);
	if (Result != FR_OK)
	{
		// No performance directory, so no performance banks.
		// So nothing else to do here
		LOGNOTE ("No performance banks detected");
		m_bPerformanceDirectoryExists = false;
		return false;
	}

	unsigned nNumBanks = 0;
	m_nLastPerformanceBank = 0;

	// List directories with names in format 01_Perf Bank Name
	Result = f_findfirst (&Directory, &FileInfo, "SD:/" PERFORMANCE_DIR, "*");
	for (unsigned i = 0; Result == FR_OK && FileInfo.fname[0]; i++)
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
			if (   nLen > 4 && nLen <26	 && strcmp(OriFileName.substr(3,1).c_str(), "_")==0)
			{
				unsigned nBankIndex = stoi(OriFileName.substr(0,3));
				// Recall user index numbered 002..NUM_PERFORMANCE_BANKS
				// NB: Bank 001 is reserved for the default performance directory
				if ((nBankIndex > 0) && (nBankIndex <= NUM_PERFORMANCE_BANKS))
				{
					// Convert from "user facing" 1..indexed number to internal 0..indexed
					nBankIndex = nBankIndex-1;
					if (m_PerformanceBankName[nBankIndex].empty())
					{
						std::string BankName = OriFileName.substr(4,nLen);

						m_PerformanceBankName[nBankIndex] = BankName;
#ifdef VERBOSE_DEBUG
						LOGNOTE ("Found performance bank %s (%d, %s)", OriFileName.c_str(), nBankIndex, BankName.c_str());
#endif
						nNumBanks++;
						if (nBankIndex > m_nLastPerformanceBank)
						{
							m_nLastPerformanceBank = nBankIndex;
						}
					}
					else
					{
						LOGNOTE ("Duplicate Performance Bank: %s", FileInfo.fname);
						if (nBankIndex==0)
						{
							LOGNOTE ("(Bank 001 is the default performance directory)");
						}
					}
				}
				else
				{
					LOGNOTE ("Performance Bank number out of range: %s (%d to %d)", FileInfo.fname, 1, NUM_PERFORMANCE_BANKS);
				}
			}
			else
			{
#ifdef VERBOSE_DEBUG
				LOGNOTE ("Skipping: %s", FileInfo.fname);
#endif
			}
		}
		
		Result = f_findnext (&Directory, &FileInfo);
	}
	
	if (nNumBanks > 0)
	{
		LOGNOTE ("Number of Performance Banks: %d (last = %d)", nNumBanks, m_nLastPerformanceBank+1);
	}
	
	f_closedir (&Directory);
	return true;
}

void CPerformanceConfig::SetNewPerformanceBank(unsigned nBankID)
{
	assert (nBankID < NUM_PERFORMANCE_BANKS);
	if (IsValidPerformanceBank(nBankID))
	{
#ifdef VERBOSE_DEBUG
		LOGNOTE("Selecting Performance Bank: %d", nBankID+1);
#endif
		m_nPerformanceBank = nBankID;
		m_nActualPerformanceBank = nBankID;
		ListPerformances();
	}
	else
	{
#ifdef VERBOSE_DEBUG
		LOGNOTE("Not selecting invalid Performance Bank: %d", nBankID+1);
#endif
	}
}

unsigned CPerformanceConfig::GetPerformanceBank(void)
{
	return m_nPerformanceBank;
}

std::string CPerformanceConfig::GetPerformanceBankName(unsigned nBankID)
{
	assert (nBankID < NUM_PERFORMANCE_BANKS);
	if (IsValidPerformanceBank(nBankID))
	{
		return m_PerformanceBankName[nBankID];
	}
	return "";
}

std::string CPerformanceConfig::AddPerformanceBankDirName(unsigned nBankID)
{
	assert (nBankID < NUM_PERFORMANCE_BANKS);
	if (IsValidPerformanceBank(nBankID))
	{
		// Performance Banks directories in format "001_Bank Name"
		std::string Index;
		if (nBankID < 9)
		{
			Index = "00" + std::to_string(nBankID+1);
		}
		else if (nBankID < 99)
		{
			Index = "0" + std::to_string(nBankID+1);
		}
		else
		{
			Index = std::to_string(nBankID+1);
		}

		return "/" + Index + "_" + m_PerformanceBankName[nBankID];
	}
	else
	{
		return "";
	}
}

bool CPerformanceConfig::IsValidPerformanceBank(unsigned nBankID)
{
	if (nBankID >= NUM_PERFORMANCE_BANKS) {
		return false;
	}
	if (m_PerformanceBankName[nBankID].empty())
	{
		return false;
	}
	return true;
}
