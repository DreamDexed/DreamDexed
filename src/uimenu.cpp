//
// uimenu.cpp
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

#include "uimenu.h"

#include <cassert>
#include <cmath>
#include <cstdio>
#include <string>

#include <circle/cputhrottle.h>
#include <circle/logger.h>
#include <circle/net/ipaddress.h>
#include <circle/string.h>
#include <circle/sysconfig.h>
#include <circle/timer.h>

#include "bus.h"
#include "config.h"
#include "dexed.h"
#include "dexedadapter.h"
#include "effect.h"
#include "effect_compressor.h"
#include "mididevice.h"
#include "minidexed.h"
#include "performanceconfig.h"
#include "sdfilter.h"
#include "status.h"
#include "sysexfileloader.h"
#include "uitostring.h"
#include "userinterface.h"
#include "util.h"

// LOGMODULE("uimenu");

const CUIMenu::TMenuItem CUIMenu::s_MenuRoot[] =
{
	{"DreamDexed", MenuHandler, s_MainMenu},
	{0},
};

// inserting menu items before "TG1" affect TGShortcutHandler()
CUIMenu::TMenuItem CUIMenu::s_MainMenu[] =
{
	{"TG1", MenuHandler, s_TGMenu, 0},
#ifdef ARM_ALLOW_MULTI_CORE
	{"TG2", MenuHandler, s_TGMenu, 1},
	{"TG3", MenuHandler, s_TGMenu, 2},
	{"TG4", MenuHandler, s_TGMenu, 3},
	{"TG5", MenuHandler, s_TGMenu, 4},
	{"TG6", MenuHandler, s_TGMenu, 5},
	{"TG7", MenuHandler, s_TGMenu, 6},
	{"TG8", MenuHandler, s_TGMenu, 7},
	{"TG9", MenuHandler, s_TGMenu, 8},
	{"TG10", MenuHandler, s_TGMenu, 9},
	{"TG11", MenuHandler, s_TGMenu, 10},
	{"TG12", MenuHandler, s_TGMenu, 11},
	{"TG13", MenuHandler, s_TGMenu, 12},
	{"TG14", MenuHandler, s_TGMenu, 13},
	{"TG15", MenuHandler, s_TGMenu, 14},
	{"TG16", MenuHandler, s_TGMenu, 15},
	{"TG17", MenuHandler, s_TGMenu, 16},
	{"TG18", MenuHandler, s_TGMenu, 17},
	{"TG19", MenuHandler, s_TGMenu, 18},
	{"TG20", MenuHandler, s_TGMenu, 19},
	{"TG21", MenuHandler, s_TGMenu, 20},
	{"TG22", MenuHandler, s_TGMenu, 21},
	{"TG23", MenuHandler, s_TGMenu, 22},
	{"TG24", MenuHandler, s_TGMenu, 23},
	{"TG25", MenuHandler, s_TGMenu, 24},
	{"TG26", MenuHandler, s_TGMenu, 25},
	{"TG27", MenuHandler, s_TGMenu, 26},
	{"TG28", MenuHandler, s_TGMenu, 27},
	{"TG29", MenuHandler, s_TGMenu, 28},
	{"TG30", MenuHandler, s_TGMenu, 29},
	{"TG31", MenuHandler, s_TGMenu, 30},
	{"TG32", MenuHandler, s_TGMenu, 31},
#endif
	{"Status", MenuHandler, s_StatusMenu},
	{"Mixer", MenuHandler, s_MixerMenu},
#ifdef ARM_ALLOW_MULTI_CORE
	{"Effects", MenuHandler, s_EffectsMenu},
	{"Bus1", MenuHandler, s_BusMenu, 0},
	{"Bus2", MenuHandler, s_BusMenu, 1},
	{"Bus3", MenuHandler, s_BusMenu, 2},
	{"Bus4", MenuHandler, s_BusMenu, 3},
	{"Out1", MenuHandler, s_OutputMenu, CConfig::Buses},
#endif
	{"Performance", MenuHandler, s_PerformanceMenu},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_TGMenu[] =
{
	{"Voice", EditProgramNumber},
	{"Bank", EditVoiceBankNumber},
	{"Volume", EditTGParameter, 0, CMiniDexed::TGParameterVolume},
#ifdef ARM_ALLOW_MULTI_CORE
	{"Pan", EditTGParameter, 0, CMiniDexed::TGParameterPan},
	{"FX1-Send", EditTGParameter, 0, CMiniDexed::TGParameterFX1Send},
	{"FX2-Send", EditTGParameter, 0, CMiniDexed::TGParameterFX2Send},
#endif
	{"Detune", EditTGParameter, 0, CMiniDexed::TGParameterMasterTune},
	{"Cutoff", EditTGParameter, 0, CMiniDexed::TGParameterCutoff},
	{"Resonance", EditTGParameter, 0, CMiniDexed::TGParameterResonance},
	{"Pitch Bend", MenuHandler, s_EditPitchBendMenu},
	{"Portamento", MenuHandler, s_EditPortamentoMenu},
	{"Note Limit", MenuHandler, s_EditNoteLimitMenu},
	{"Poly/Mono", EditTGParameter, 0, CMiniDexed::TGParameterMonoMode},
	{"TG-Link", EditTGParameter, 0, CMiniDexed::TGParameterTGLink},
	{"Modulation", MenuHandler, s_ModulationMenu},
	{"MIDI", MenuHandler, s_MIDIMenu},
	{"EQ", MenuHandler, s_EQMenu},
	{"Compressor", MenuHandler, s_EditCompressorMenu},
	{"Edit Voice", MenuHandler, s_EditVoiceMenu},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_EditCompressorMenu[] =
{
	{"Enable", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorEnable},
	{"Pre Gain", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorPreGain},
	{"Threshold", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorThresh},
	{"Ratio", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorRatio},
	{"Attack", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorAttack},
	{"Release", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorRelease},
	{"Makeup Gain", EditTGParameter2, 0, CMiniDexed::TGParameterCompressorMakeupGain},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_EditPitchBendMenu[] =
{
	{"Bend Range", EditTGParameter2, 0, CMiniDexed::TGParameterPitchBendRange},
	{"Bend Step", EditTGParameter2, 0, CMiniDexed::TGParameterPitchBendStep},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_EditPortamentoMenu[] =
{
	{"Mode", EditTGParameter2, 0, CMiniDexed::TGParameterPortamentoMode},
	{"Glissando", EditTGParameter2, 0, CMiniDexed::TGParameterPortamentoGlissando},
	{"Time", EditTGParameter2, 0, CMiniDexed::TGParameterPortamentoTime},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_EditNoteLimitMenu[] =
{
	{"Limit Low", EditTGParameter2, 0, CMiniDexed::TGParameterNoteLimitLow, .OnSelect = InputKeyDown},
	{"Limit High", EditTGParameter2, 0, CMiniDexed::TGParameterNoteLimitHigh, .OnSelect = InputKeyDown},
	{"Shift", EditTGParameter2, 0, CMiniDexed::TGParameterNoteShift, .OnSelect = InputShiftKeyDown},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ModulationMenu[] =
{
	{"Mod. Wheel", MenuHandler, s_ModulationMenuParameters, CMiniDexed::TGParameterMWRange},
	{"Foot Control", MenuHandler, s_ModulationMenuParameters, CMiniDexed::TGParameterFCRange},
	{"Breath Control", MenuHandler, s_ModulationMenuParameters, CMiniDexed::TGParameterBCRange},
	{"Aftertouch", MenuHandler, s_ModulationMenuParameters, CMiniDexed::TGParameterATRange},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ModulationMenuParameters[] =
{
	{"Range", EditTGParameterModulation, 0, 0},
	{"Pitch", EditTGParameterModulation, 0, 1},
	{"Amplitude", EditTGParameterModulation, 0, 2},
	{"EG Bias", EditTGParameterModulation, 0, 3},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_MIDIMenu[] =
{
	{"Channel", EditTGParameter2, 0, CMiniDexed::TGParameterMIDIChannel},
	{"SysEx Channel", EditTGParameter2, 0, CMiniDexed::TGParameterSysExChannel},
	{"SysEx Enable", EditTGParameter2, 0, CMiniDexed::TGParameterSysExEnable},
	{"Sustain Rx", EditTGParameter2, 0, CMiniDexed::TGParameterMIDIRxSustain},
	{"Portamento Rx", EditTGParameter2, 0, CMiniDexed::TGParameterMIDIRxPortamento},
	{"Sostenuto Rx", EditTGParameter2, 0, CMiniDexed::TGParameterMIDIRxSostenuto},
	{"Hold2 Rx", EditTGParameter2, 0, CMiniDexed::TGParameterMIDIRxHold2},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_EQMenu[] =
{
	{"Low Level", EditTGParameter2, 0, CMiniDexed::TGParameterEQLow},
	{"Mid Level", EditTGParameter2, 0, CMiniDexed::TGParameterEQMid},
	{"High Level", EditTGParameter2, 0, CMiniDexed::TGParameterEQHigh},
	{"Gain", EditTGParameter2, 0, CMiniDexed::TGParameterEQGain},
	{"Low-Mid Freq", EditTGParameter2, 0, CMiniDexed::TGParameterEQLowMidFreq},
	{"Mid-High Freq", EditTGParameter2, 0, CMiniDexed::TGParameterEQMidHighFreq},
	{"Pre Lowcut", EditTGParameter2, 0, CMiniDexed::TGParameterEQPreLowcut},
	{"Pre Highcut", EditTGParameter2, 0, CMiniDexed::TGParameterEQPreHighcut},
	{0},
};

CUIMenu::TMenuItem CUIMenu::s_MixerMenu[] =
{
	{"Master Volume", EditGlobalParameter, 0, CMiniDexed::ParameterMasterVolume},
#ifdef ARM_ALLOW_MULTI_CORE
	{"B1 Dry Level", EditBusParameterG, 0, Bus::Parameter::MixerDryLevel, .nBus = 0},
	{"B1 FX1 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 0, .idFX = 0},
	{"B1 FX2 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 0, .idFX = 1},
	{"B1 Return", EditBusParameterG, 0, Bus::Parameter::ReturnLevel, .nBus = 0},
	{"B2 Dry Level", EditBusParameterG, 0, Bus::Parameter::MixerDryLevel, .nBus = 1},
	{"B2 FX1 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 1, .idFX = 0},
	{"B2 FX2 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 1, .idFX = 1},
	{"B2 Return", EditBusParameterG, 0, Bus::Parameter::ReturnLevel, .nBus = 1},
	{"B3 Dry Level", EditBusParameterG, 0, Bus::Parameter::MixerDryLevel, .nBus = 2},
	{"B3 FX1 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 2, .idFX = 0},
	{"B3 FX2 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 2, .idFX = 1},
	{"B3 Return", EditBusParameterG, 0, Bus::Parameter::ReturnLevel, .nBus = 2},
	{"B4 Dry Level", EditBusParameterG, 0, Bus::Parameter::MixerDryLevel, .nBus = 3},
	{"B4 FX1 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 3, .idFX = 0},
	{"B4 FX2 Return", EditFXParameterG, 0, FX::Parameter::ReturnLevel, .nBus = 3, .idFX = 1},
	{"B4 Return", EditBusParameterG, 0, Bus::Parameter::ReturnLevel, .nBus = 3},
#endif
	{0},
};

#ifdef ARM_ALLOW_MULTI_CORE

const CUIMenu::TMenuItem CUIMenu::s_EffectsMenu[] =
{
	{"Dry Level", EditBusParameter, 0, Bus::Parameter::MixerDryLevel},
	{"SendFX1", MenuHandler, s_SendFXMenu, 0},
	{"SendFX2", MenuHandler, s_SendFXMenu, 1},
	{"MasterFX", MenuHandler, s_MasterFXMenu, CConfig::MasterFX},
	{"SendFX Bypass", EditBusParameter, 0, Bus::Parameter::FXBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_BusMenu[] =
{
	{"Dry Level", EditBusParameter, 0, Bus::Parameter::MixerDryLevel},
	{"SendFX1", MenuHandler, s_SendFXMenu, 0},
	{"SendFX2", MenuHandler, s_SendFXMenu, 1},
	{"Return Level", EditBusParameter, 0, Bus::Parameter::ReturnLevel},
	{"SendFX Bypass", EditBusParameter, 0, Bus::Parameter::FXBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_OutputMenu[] =
{
	{"MasterFX", MenuHandler, s_MasterFXMenu, 0},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_SendFXMenu[] =
{
	{"Slot1", MenuHandler, s_FXListMenu, FX::Parameter::Slot0, .OnSelect = SelectCurrentEffect, .StepDown = StepDownEffect, .StepUp = StepUpEffect},
	{"Slot2", MenuHandler, s_FXListMenu, FX::Parameter::Slot1, .OnSelect = SelectCurrentEffect, .StepDown = StepDownEffect, .StepUp = StepUpEffect},
	{"Slot3", MenuHandler, s_FXListMenu, FX::Parameter::Slot2, .OnSelect = SelectCurrentEffect, .StepDown = StepDownEffect, .StepUp = StepUpEffect},
	{"Return Level", EditFXParameter2, 0, FX::Parameter::ReturnLevel},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::Bypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_MasterFXMenu[] =
{
	{"Slot1", MenuHandler, s_FXListMenu, FX::Parameter::Slot0, .OnSelect = SelectCurrentEffect, .StepDown = StepDownEffect, .StepUp = StepUpEffect},
	{"Slot2", MenuHandler, s_FXListMenu, FX::Parameter::Slot1, .OnSelect = SelectCurrentEffect, .StepDown = StepDownEffect, .StepUp = StepUpEffect},
	{"Slot3", MenuHandler, s_FXListMenu, FX::Parameter::Slot2, .OnSelect = SelectCurrentEffect, .StepDown = StepDownEffect, .StepUp = StepUpEffect},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::Bypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_FXListMenu[] =
{
	{"None"},
	{"ZynDistortion", MenuHandler, s_ZynDistortionMenu},
	{"YKChorus", MenuHandler, s_YKChorusMenu},
	{"ZynChorus", MenuHandler, s_ZynChorusMenu},
	{"ZynSympathetic", MenuHandler, s_ZynSympatheticMenu},
	{"ZynAPhaser", MenuHandler, s_ZynAPhaserMenu},
	{"ZynPhaser", MenuHandler, s_ZynPhaserMenu},
	{"DreamDelay", MenuHandler, s_DreamDelayMenu},
	{"PlateReverb", MenuHandler, s_PlateReverbMenu},
	{"CloudSeed2", MenuHandler, s_CloudSeed2Menu},
	{"Compressor", MenuHandler, s_CompressorMenu},
	{"EQ", MenuHandler, s_FXEQMenu},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ZynDistortionMenu[] =
{
	{"Load Preset", EditFXParameter2, 0, FX::Parameter::ZynDistortionPreset},
	{"Mix", EditFXParameter2, 0, FX::Parameter::ZynDistortionMix},
	{"Panning", EditFXParameter2, 0, FX::Parameter::ZynDistortionPanning},
	{"Drive", EditFXParameter2, 0, FX::Parameter::ZynDistortionDrive},
	{"Level", EditFXParameter2, 0, FX::Parameter::ZynDistortionLevel},
	{"Type", EditFXParameter2, 0, FX::Parameter::ZynDistortionType},
	{"Negate", EditFXParameter2, 0, FX::Parameter::ZynDistortionNegate},
	{"Filtering", EditFXParameter2, 0, FX::Parameter::ZynDistortionFiltering},
	{"Lowcut", EditFXParameter2, 0, FX::Parameter::ZynDistortionLowcut},
	{"Highcut", EditFXParameter2, 0, FX::Parameter::ZynDistortionHighcut},
	{"Stereo", EditFXParameter2, 0, FX::Parameter::ZynDistortionStereo},
	{"LR Cross", EditFXParameter2, 0, FX::Parameter::ZynDistortionLRCross},
	{"Shape", EditFXParameter2, 0, FX::Parameter::ZynDistortionShape},
	{"Offset", EditFXParameter2, 0, FX::Parameter::ZynDistortionOffset},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::ZynDistortionBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_YKChorusMenu[] =
{
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::YKChorusMix},
	{"Enable I", EditFXParameter2, 0, FX::Parameter::YKChorusEnable1},
	{"Enable II", EditFXParameter2, 0, FX::Parameter::YKChorusEnable2},
	{"LFO Rate I", EditFXParameter2, 0, FX::Parameter::YKChorusLFORate1},
	{"LFO Rate II", EditFXParameter2, 0, FX::Parameter::YKChorusLFORate2},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::YKChorusBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ZynChorusMenu[] =
{
	{"Load Preset", EditFXParameter2, 0, FX::Parameter::ZynChorusPreset},
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::ZynChorusMix},
	{"Panning", EditFXParameter2, 0, FX::Parameter::ZynChorusPanning},
	{"LFO Freq", EditFXParameter2, 0, FX::Parameter::ZynChorusLFOFreq},
	{"LFO Rndness", EditFXParameter2, 0, FX::Parameter::ZynChorusLFORandomness},
	{"LFO Type", EditFXParameter2, 0, FX::Parameter::ZynChorusLFOType},
	{"LFO LR Delay", EditFXParameter2, 0, FX::Parameter::ZynChorusLFOLRDelay},
	{"Depth", EditFXParameter2, 0, FX::Parameter::ZynChorusDepth},
	{"Delay", EditFXParameter2, 0, FX::Parameter::ZynChorusDelay},
	{"Feedback", EditFXParameter2, 0, FX::Parameter::ZynChorusFeedback},
	{"LR Cross", EditFXParameter2, 0, FX::Parameter::ZynChorusLRCross},
	{"Mode", EditFXParameter2, 0, FX::Parameter::ZynChorusMode},
	{"Subtractive", EditFXParameter2, 0, FX::Parameter::ZynChorusSubtractive},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::ZynChorusBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ZynSympatheticMenu[] =
{
	{"Load Preset", EditFXParameter2, 0, FX::Parameter::ZynSympatheticPreset},
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::ZynSympatheticMix},
	{"Panning", EditFXParameter2, 0, FX::Parameter::ZynSympatheticPanning},
	{"Q", EditFXParameter2, 0, FX::Parameter::ZynSympatheticQ},
	{"Q Sustain", EditFXParameter2, 0, FX::Parameter::ZynSympatheticQSustain},
	{"Drive", EditFXParameter2, 0, FX::Parameter::ZynSympatheticDrive},
	{"Level", EditFXParameter2, 0, FX::Parameter::ZynSympatheticLevel},
	{"Type", EditFXParameter2, 0, FX::Parameter::ZynSympatheticType},
	{"Unison Size", EditFXParameter2, 0, FX::Parameter::ZynSympatheticUnisonSize},
	{"Unison Spread", EditFXParameter2, 0, FX::Parameter::ZynSympatheticUnisonSpread},
	{"Strings", EditFXParameter2, 0, FX::Parameter::ZynSympatheticStrings},
	{"Interval", EditFXParameter2, 0, FX::Parameter::ZynSympatheticInterval},
	{"Base Note", EditFXParameter2, 0, FX::Parameter::ZynSympatheticBaseNote},
	{"Lowcut", EditFXParameter2, 0, FX::Parameter::ZynSympatheticLowcut},
	{"Highcut", EditFXParameter2, 0, FX::Parameter::ZynSympatheticHighcut},
	{"Negate", EditFXParameter2, 0, FX::Parameter::ZynSympatheticNegate},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::ZynSympatheticBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ZynAPhaserMenu[] =
{
	{"Load Preset", EditFXParameter2, 0, FX::Parameter::ZynAPhaserPreset},
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::ZynAPhaserMix},
	{"Panning", EditFXParameter2, 0, FX::Parameter::ZynAPhaserPanning},
	{"LFO Freq", EditFXParameter2, 0, FX::Parameter::ZynAPhaserLFOFreq},
	{"LFO Rndness", EditFXParameter2, 0, FX::Parameter::ZynAPhaserLFORandomness},
	{"LFO Type", EditFXParameter2, 0, FX::Parameter::ZynAPhaserLFOType},
	{"LFO LR Delay", EditFXParameter2, 0, FX::Parameter::ZynAPhaserLFOLRDelay},
	{"Depth", EditFXParameter2, 0, FX::Parameter::ZynAPhaserDepth},
	{"Feedback", EditFXParameter2, 0, FX::Parameter::ZynAPhaserFeedback},
	{"Stages", EditFXParameter2, 0, FX::Parameter::ZynAPhaserStages},
	{"LR Cross", EditFXParameter2, 0, FX::Parameter::ZynAPhaserLRCross},
	{"Subtractive", EditFXParameter2, 0, FX::Parameter::ZynAPhaserSubtractive},
	{"Width", EditFXParameter2, 0, FX::Parameter::ZynAPhaserWidth},
	{"Distortion", EditFXParameter2, 0, FX::Parameter::ZynAPhaserDistortion},
	{"Mismatch", EditFXParameter2, 0, FX::Parameter::ZynAPhaserMismatch},
	{"Hyper", EditFXParameter2, 0, FX::Parameter::ZynAPhaserHyper},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::ZynAPhaserBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_ZynPhaserMenu[] =
{
	{"Load Preset", EditFXParameter2, 0, FX::Parameter::ZynPhaserPreset},
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::ZynPhaserMix},
	{"Panning", EditFXParameter2, 0, FX::Parameter::ZynPhaserPanning},
	{"LFO Freq", EditFXParameter2, 0, FX::Parameter::ZynPhaserLFOFreq},
	{"LFO Rndness", EditFXParameter2, 0, FX::Parameter::ZynPhaserLFORandomness},
	{"LFO Type", EditFXParameter2, 0, FX::Parameter::ZynPhaserLFOType},
	{"LFO LR Delay", EditFXParameter2, 0, FX::Parameter::ZynPhaserLFOLRDelay},
	{"Depth", EditFXParameter2, 0, FX::Parameter::ZynPhaserDepth},
	{"Feedback", EditFXParameter2, 0, FX::Parameter::ZynPhaserFeedback},
	{"Stages", EditFXParameter2, 0, FX::Parameter::ZynPhaserStages},
	{"LR Cross", EditFXParameter2, 0, FX::Parameter::ZynPhaserLRCross},
	{"Subtractive", EditFXParameter2, 0, FX::Parameter::ZynPhaserSubtractive},
	{"Phase", EditFXParameter2, 0, FX::Parameter::ZynPhaserPhase},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::ZynPhaserBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_DreamDelayMenu[] =
{
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::DreamDelayMix},
	{"Mode", EditFXParameter2, 0, FX::Parameter::DreamDelayMode},
	{"Time", EditFXParameter2, 0, FX::Parameter::DreamDelayTime},
	{"Time Left", EditFXParameter2, 0, FX::Parameter::DreamDelayTimeL},
	{"Time Right", EditFXParameter2, 0, FX::Parameter::DreamDelayTimeR},
	{"Tempo", EditFXParameter2, 0, FX::Parameter::DreamDelayTempo},
	{"Feedback", EditFXParameter2, 0, FX::Parameter::DreamDelayFeedback},
	{"HighCut", EditFXParameter2, 0, FX::Parameter::DreamDelayHighCut},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::DreamDelayBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_PlateReverbMenu[] =
{
	{"Mix Dry:Wet", EditFXParameter2, 0, FX::Parameter::PlateReverbMix},
	{"Size", EditFXParameter2, 0, FX::Parameter::PlateReverbSize},
	{"High damp", EditFXParameter2, 0, FX::Parameter::PlateReverbHighDamp},
	{"Low damp", EditFXParameter2, 0, FX::Parameter::PlateReverbLowDamp},
	{"Low pass", EditFXParameter2, 0, FX::Parameter::PlateReverbLowPass},
	{"Diffusion", EditFXParameter2, 0, FX::Parameter::PlateReverbDiffusion},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::PlateReverbBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2Menu[] =
{
	{"Load Preset", EditFXParameter2, 0, FX::Parameter::CloudSeed2Preset},
	{"Dry Out", EditFXParameter2, 0, FX::Parameter::CloudSeed2DryOut},
	{"Early Out", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyOut},
	{"Late Out", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateOut},
	{"Early FB", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseFeedback},
	{"Late FB", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseFeedback},
	{"Tap Decay", EditFXParameter2, 0, FX::Parameter::CloudSeed2TapDecay},
	{"Late Decay", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineDecay},
	{"Late Lines", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineCount},
	{"Input", MenuHandler, s_CloudSeed2InputMenu},
	{"Multitap Delay", MenuHandler, s_CloudSeed2MultitapMenu},
	{"Early Diffusion", MenuHandler, s_CloudSeed2EarlyDiffusionMenu},
	{"Late Diffusion", MenuHandler, s_CloudSeed2LateDiffusionMenu},
	{"Late Lines", MenuHandler, s_CloudSeed2LateLineMenu},
	{"Low Shelf", MenuHandler, s_CloudSeed2LowShelfMenu},
	{"High Shelf", MenuHandler, s_CloudSeed2HighShelfMenu},
	{"Low Pass", MenuHandler, s_CloudSeed2LowPassMenu},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::CloudSeed2Bypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2InputMenu[] =
{
	{"Interpolation", EditFXParameter2, 0, FX::Parameter::CloudSeed2Interpolation},
	{"L/R Input Mix", EditFXParameter2, 0, FX::Parameter::CloudSeed2InputMix},
	{"High Cut Enabled", EditFXParameter2, 0, FX::Parameter::CloudSeed2HighCutEnabled},
	{"High Cut", EditFXParameter2, 0, FX::Parameter::CloudSeed2HighCut},
	{"Low Cut Enabled", EditFXParameter2, 0, FX::Parameter::CloudSeed2LowCutEnabled},
	{"Low Cut", EditFXParameter2, 0, FX::Parameter::CloudSeed2LowCut},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2MultitapMenu[] =
{
	{"Enabled", EditFXParameter2, 0, FX::Parameter::CloudSeed2TapEnabled},
	{"Count", EditFXParameter2, 0, FX::Parameter::CloudSeed2TapCount},
	{"Decay", EditFXParameter2, 0, FX::Parameter::CloudSeed2TapDecay},
	{"Predelay", EditFXParameter2, 0, FX::Parameter::CloudSeed2TapPredelay},
	{"Length", EditFXParameter2, 0, FX::Parameter::CloudSeed2TapLength},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2EarlyDiffusionMenu[] =
{
	{"Enabled", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseEnabled},
	{"Stage Count", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseCount},
	{"Delay", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseDelay},
	{"Feedback", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseFeedback},
	{"Mod Amount", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseModAmount},
	{"Mod Rate", EditFXParameter2, 0, FX::Parameter::CloudSeed2EarlyDiffuseModRate},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2LateDiffusionMenu[] =
{
	{"Enabled", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseEnabled},
	{"Stage Count", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseCount},
	{"Delay", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseDelay},
	{"Feedback", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseFeedback},
	{"Mod Amount", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseModAmount},
	{"Mod Rate", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateDiffuseModRate},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2LateLineMenu[] =
{
	{"Mode", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateMode},
	{"Count", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineCount},
	{"Size", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineSize},
	{"Decay", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineDecay},
	{"Mod Amt", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineModAmount},
	{"Mod Rate", EditFXParameter2, 0, FX::Parameter::CloudSeed2LateLineModRate},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2LowShelfMenu[] =
{
	{"Enable", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqLowShelfEnabled},
	{"Freq", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqLowFreq},
	{"Gain", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqLowGain},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2HighShelfMenu[] =
{
	{"Enable", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqHighShelfEnabled},
	{"Freq", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqHighFreq},
	{"Gain", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqHighGain},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CloudSeed2LowPassMenu[] =
{
	{"Enable", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqLowpassEnabled},
	{"Cutoff", EditFXParameter2, 0, FX::Parameter::CloudSeed2EqCutoff},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_CompressorMenu[] =
{
	{"Pre Gain", EditFXParameter2, 0, FX::Parameter::CompressorPreGain},
	{"Threshold", EditFXParameter2, 0, FX::Parameter::CompressorThresh},
	{"Ratio", EditFXParameter2, 0, FX::Parameter::CompressorRatio},
	{"Attack", EditFXParameter2, 0, FX::Parameter::CompressorAttack},
	{"Release", EditFXParameter2, 0, FX::Parameter::CompressorRelease},
	{"Makeup Gain", EditFXParameter2, 0, FX::Parameter::CompressorMakeupGain},
	{"HPFilter", EditFXParameter2, 0, FX::Parameter::CompressorHPFilterEnable},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::CompressorBypass},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_FXEQMenu[] =
{
	{"Low Level", EditFXParameter2, 0, FX::Parameter::EQLow},
	{"Mid Level", EditFXParameter2, 0, FX::Parameter::EQMid},
	{"High Level", EditFXParameter2, 0, FX::Parameter::EQHigh},
	{"Gain", EditFXParameter2, 0, FX::Parameter::EQGain},
	{"Low-Mid Freq", EditFXParameter2, 0, FX::Parameter::EQLowMidFreq},
	{"Mid-High Freq", EditFXParameter2, 0, FX::Parameter::EQMidHighFreq},
	{"Pre Lowcut", EditFXParameter2, 0, FX::Parameter::EQPreLowCut},
	{"Pre Highcut", EditFXParameter2, 0, FX::Parameter::EQPreHighCut},
	{"Bypass", EditFXParameter2, 0, FX::Parameter::EQBypass},
	{0},
};

#endif

// inserting menu items before "OP1" affect OPShortcutHandler()
const CUIMenu::TMenuItem CUIMenu::s_EditVoiceMenu[] =
{
	{"OP1", MenuHandler, s_OperatorMenu, 0},
	{"OP2", MenuHandler, s_OperatorMenu, 1},
	{"OP3", MenuHandler, s_OperatorMenu, 2},
	{"OP4", MenuHandler, s_OperatorMenu, 3},
	{"OP5", MenuHandler, s_OperatorMenu, 4},
	{"OP6", MenuHandler, s_OperatorMenu, 5},
	{"Algorithm", EditVoiceParameter, 0, DEXED_ALGORITHM},
	{"Feedback", EditVoiceParameter, 0, DEXED_FEEDBACK},
	{"P EG Rate 1", EditVoiceParameter, 0, DEXED_PITCH_EG_R1},
	{"P EG Rate 2", EditVoiceParameter, 0, DEXED_PITCH_EG_R2},
	{"P EG Rate 3", EditVoiceParameter, 0, DEXED_PITCH_EG_R3},
	{"P EG Rate 4", EditVoiceParameter, 0, DEXED_PITCH_EG_R4},
	{"P EG Level 1", EditVoiceParameter, 0, DEXED_PITCH_EG_L1},
	{"P EG Level 2", EditVoiceParameter, 0, DEXED_PITCH_EG_L2},
	{"P EG Level 3", EditVoiceParameter, 0, DEXED_PITCH_EG_L3},
	{"P EG Level 4", EditVoiceParameter, 0, DEXED_PITCH_EG_L4},
	{"Osc Key Sync", EditVoiceParameter, 0, DEXED_OSC_KEY_SYNC},
	{"LFO Speed", EditVoiceParameter, 0, DEXED_LFO_SPEED},
	{"LFO Delay", EditVoiceParameter, 0, DEXED_LFO_DELAY},
	{"LFO PMD", EditVoiceParameter, 0, DEXED_LFO_PITCH_MOD_DEP},
	{"LFO AMD", EditVoiceParameter, 0, DEXED_LFO_AMP_MOD_DEP},
	{"LFO Sync", EditVoiceParameter, 0, DEXED_LFO_SYNC},
	{"LFO Wave", EditVoiceParameter, 0, DEXED_LFO_WAVE},
	{"P Mod Sens.", EditVoiceParameter, 0, DEXED_LFO_PITCH_MOD_SENS},
	{"Transpose", EditVoiceParameter, 0, DEXED_TRANSPOSE},
	{"Name", InputTxt, 0, 3},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_OperatorMenu[] =
{
	{"Output Level", EditOPParameter, 0, DEXED_OP_OUTPUT_LEV},
	{"Freq Coarse", EditOPParameter, 0, DEXED_OP_FREQ_COARSE},
	{"Freq Fine", EditOPParameter, 0, DEXED_OP_FREQ_FINE},
	{"Osc Detune", EditOPParameter, 0, DEXED_OP_OSC_DETUNE},
	{"Osc Mode", EditOPParameter, 0, DEXED_OP_OSC_MODE},
	{"EG Rate 1", EditOPParameter, 0, DEXED_OP_EG_R1},
	{"EG Rate 2", EditOPParameter, 0, DEXED_OP_EG_R2},
	{"EG Rate 3", EditOPParameter, 0, DEXED_OP_EG_R3},
	{"EG Rate 4", EditOPParameter, 0, DEXED_OP_EG_R4},
	{"EG Level 1", EditOPParameter, 0, DEXED_OP_EG_L1},
	{"EG Level 2", EditOPParameter, 0, DEXED_OP_EG_L2},
	{"EG Level 3", EditOPParameter, 0, DEXED_OP_EG_L3},
	{"EG Level 4", EditOPParameter, 0, DEXED_OP_EG_L4},
	{"Break Point", EditOPParameter, 0, DEXED_OP_LEV_SCL_BRK_PT},
	{"L Key Depth", EditOPParameter, 0, DEXED_OP_SCL_LEFT_DEPTH},
	{"R Key Depth", EditOPParameter, 0, DEXED_OP_SCL_RGHT_DEPTH},
	{"L Key Scale", EditOPParameter, 0, DEXED_OP_SCL_LEFT_CURVE},
	{"R Key Scale", EditOPParameter, 0, DEXED_OP_SCL_RGHT_CURVE},
	{"Rate Scaling", EditOPParameter, 0, DEXED_OP_OSC_RATE_SCALE},
	{"A Mod Sens.", EditOPParameter, 0, DEXED_OP_AMP_MOD_SENS},
	{"K Vel. Sens.", EditOPParameter, 0, DEXED_OP_KEY_VEL_SENS},
	{"Enable", EditOPParameter, 0, DEXED_OP_ENABLE},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_SaveMenu[] =
{
	{"Overwrite", SavePerformance, 0, 0},
	{"New", InputTxt, 0, 1},
	{"Save as default", SavePerformance, 0, 1},
	{0},
};

// must match CMiniDexed::TParameter
CUIMenu::TParameter CUIMenu::s_GlobalParameter[CMiniDexed::ParameterUnknown] =
{
	{0, CMIDIDevice::ChannelUnknown - 1, 1, ToMIDIChannel}, // ParameterPerformanceSelectChannel
	{0, NUM_PERFORMANCE_BANKS, 1}, // ParameterPerformanceBank
	{0, 127, 8, ToVolume}, // ParameterMasterVolume
	{0, SDFilter::get_maximum(CConfig::AllToneGenerators), 1, ToSDFilter}, // ParameterSDFilter (Maximum updated in the constructor)
};

// must match CMiniDexed::TTGParameter
CUIMenu::TParameter CUIMenu::s_TGParameter[CMiniDexed::TGParameterUnknown] =
{
	{0, CSysExFileLoader::MaxVoiceBankID, 1}, // TGParameterVoiceBank
	{0, 0, 0}, // TGParameterVoiceBankMSB (not used in menus)
	{0, 0, 0}, // TGParameterVoiceBankLSB (not used in menus)
	{0, CSysExFileLoader::VoicesPerBank - 1, 1}, // TGParameterProgram
	{0, 127, 8, ToVolume}, // TGParameterVolume
	{0, 127, 8, ToPan}, // TGParameterPan
	{-99, 99, 1}, // TGParameterMasterTune
	{0, 99, 1}, // TGParameterCutoff
	{0, 99, 1}, // TGParameterResonance
	{0, CMIDIDevice::ChannelUnknown - 1, 1, ToMIDIChannel}, // TGParameterMIDIChannel
	{0, CMIDIDevice::Channels - 1, 1, ToMIDIChannel}, // TGParameterSysExChannel
	{0, 1, 1, ToOnOff}, // TGParameterSysExEnable
	{0, 1, 1, ToOnOff}, // TGParameterMIDIRxSustain
	{0, 1, 1, ToOnOff}, // TGParameterMIDIRxPortamento
	{0, 1, 1, ToOnOff}, // TGParameterMIDIRxSostenuto
	{0, 1, 1, ToOnOff}, // TGParameterMIDIRxHold2
	{0, 99, 1}, // TGParameterFX1Send
	{0, 99, 1}, // TGParameterFX2Send
	{0, 12, 1}, // TGParameterPitchBendRange
	{0, 12, 1}, // TGParameterPitchBendStep
	{0, 1, 1, ToPortaMode}, // TGParameterPortamentoMode
	{0, 1, 1, ToPortaGlissando}, // TGParameterPortamentoGlissando
	{0, 99, 1}, // TGParameterPortamentoTime
	{0, 127, 1, ToMIDINote}, // TGParameterNoteLimitLow
	{0, 127, 1, ToMIDINote}, // TGParameterNoteLimitHigh
	{-24, 24, 1, ToMIDINoteShift}, // TGParameterNoteShift
	{0, 1, 1, ToPolyMono}, // TGParameterMonoMode
	{0, 4, 1, ToTGLinkName}, // TGParameterTGLink
	{0, 99, 1}, // MW Range
	{0, 1, 1, ToOnOff}, // MW Pitch
	{0, 1, 1, ToOnOff}, // MW Amp
	{0, 1, 1, ToOnOff}, // MW EGBias
	{0, 99, 1}, // FC Range
	{0, 1, 1, ToOnOff}, // FC Pitch
	{0, 1, 1, ToOnOff}, // FC Amp
	{0, 1, 1, ToOnOff}, // FC EGBias
	{0, 99, 1}, // BC Range
	{0, 1, 1, ToOnOff}, // BC Pitch
	{0, 1, 1, ToOnOff}, // BC Amp
	{0, 1, 1, ToOnOff}, // BC EGBias
	{0, 99, 1}, // AT Range
	{0, 1, 1, ToOnOff}, // AT Pitch
	{0, 1, 1, ToOnOff}, // AT Amp
	{0, 1, 1, ToOnOff}, // AT EGBias
	{0, 1, 1, ToOnOff}, // TGParameterCompressorEnable
	{-20, 20, 1, TodB}, // TGParameterCompressorPreGain
	{-60, 0, 1, TodBFS}, // TGParameterCompressorThresh
	{1, AudioEffectCompressor::CompressorRatioInf, 1, ToRatio}, // TGParameterCompressorRatio
	{0, 1000, 5, ToMillisec}, // TGParameterCompressorAttack
	{0, 2000, 5, ToMillisec}, // TGParameterCompressorRelease
	{-20, 20, 1, TodB}, // TGParameterCompressorMakeupGain
	{-24, 24, 1, TodB}, // TGParameterEQLow
	{-24, 24, 1, TodB}, // TGParameterEQMid
	{-24, 24, 1, TodB}, // TGParameterEQHigh
	{-24, 24, 1, TodB}, // TGParameterEQGain
	{0, 46, 1, ToHz}, // TGParameterEQLowMidFreq
	{28, 59, 1, ToHz}, // TGParameterEQMidHighFreq
	{0, 60, 1, ToHz}, // TGParameterEQPreLowcut
	{0, 60, 1, ToHz}, // TGParameterEQPreHighcut
};

// must match DexedVoiceParameters in Synth_Dexed
const CUIMenu::TParameter CUIMenu::s_VoiceParameter[] =
{
	{0, 99, 1}, // DEXED_PITCH_EG_R1
	{0, 99, 1}, // DEXED_PITCH_EG_R2
	{0, 99, 1}, // DEXED_PITCH_EG_R3
	{0, 99, 1}, // DEXED_PITCH_EG_R4
	{0, 99, 1}, // DEXED_PITCH_EG_L1
	{0, 99, 1}, // DEXED_PITCH_EG_L2
	{0, 99, 1}, // DEXED_PITCH_EG_L3
	{0, 99, 1}, // DEXED_PITCH_EG_L4
	{0, 31, 1, ToAlgorithm}, // DEXED_ALGORITHM
	{0, 7, 1}, // DEXED_FEEDBACK
	{0, 1, 1, ToOnOff}, // DEXED_OSC_KEY_SYNC
	{0, 99, 1}, // DEXED_LFO_SPEED
	{0, 99, 1}, // DEXED_LFO_DELAY
	{0, 99, 1}, // DEXED_LFO_PITCH_MOD_DEP
	{0, 99, 1}, // DEXED_LFO_AMP_MOD_DEP
	{0, 1, 1, ToOnOff}, // DEXED_LFO_SYNC
	{0, 5, 1, ToLFOWaveform}, // DEXED_LFO_WAVE
	{0, 7, 1}, // DEXED_LFO_PITCH_MOD_SENS
	{0, 48, 1, ToTransposeNote}, // DEXED_TRANSPOSE
	{0, 1, 1} // Voice Name - Dummy parameters for in case new item would be added in future
};

// must match DexedVoiceOPParameters in Synth_Dexed
const CUIMenu::TParameter CUIMenu::s_OPParameter[] =
{
	{0, 99, 1}, // DEXED_OP_EG_R1
	{0, 99, 1}, // DEXED_OP_EG_R2
	{0, 99, 1}, // DEXED_OP_EG_R3
	{0, 99, 1}, // DEXED_OP_EG_R4
	{0, 99, 1}, // DEXED_OP_EG_L1
	{0, 99, 1}, // DEXED_OP_EG_L2
	{0, 99, 1}, // DEXED_OP_EG_L3
	{0, 99, 1}, // DEXED_OP_EG_L4
	{0, 99, 1, ToBreakpointNote}, // DEXED_OP_LEV_SCL_BRK_PT
	{0, 99, 1}, // DEXED_OP_SCL_LEFT_DEPTH
	{0, 99, 1}, // DEXED_OP_SCL_RGHT_DEPTH
	{0, 3, 1, ToKeyboardCurve}, // DEXED_OP_SCL_LEFT_CURVE
	{0, 3, 1, ToKeyboardCurve}, // DEXED_OP_SCL_RGHT_CURVE
	{0, 7, 1}, // DEXED_OP_OSC_RATE_SCALE
	{0, 3, 1}, // DEXED_OP_AMP_MOD_SENS
	{0, 7, 1}, // DEXED_OP_KEY_VEL_SENS
	{0, 99, 1}, // DEXED_OP_OUTPUT_LEV
	{0, 1, 1, ToOscillatorMode}, // DEXED_OP_OSC_MODE
	{0, 31, 1}, // DEXED_OP_FREQ_COARSE
	{0, 99, 1}, // DEXED_OP_FREQ_FINE
	{0, 14, 1, ToOscillatorDetune}, // DEXED_OP_OSC_DETUNE
	{0, 1, 1, ToOnOff} // DEXED_OP_ENABLE
};

const CUIMenu::TMenuItem CUIMenu::s_PerformanceMenu[] =
{
	{"Load", PerformanceMenu, 0, 0},
	{"Save", MenuHandler, s_SaveMenu},
	{"Delete", PerformanceMenu, 0, 1},
	{"Bank", EditPerformanceBankNumber, 0, 0},
	{"PCCH", EditGlobalParameter, 0, CMiniDexed::ParameterPerformanceSelectChannel},
	{"Design Filter", EditGlobalParameter, 0, CMiniDexed::ParameterSDFilter},
	{0},
};

const CUIMenu::TMenuItem CUIMenu::s_StatusMenu[] =
{
	{"CPU Temp", ShowCPUTemp, 0, 0, .ShowDirect = true},
	{"CPU Speed", ShowCPUSpeed, 0, 0, .ShowDirect = true},
	{"Net IP", ShowIPAddr, 0, 0, .ShowDirect = true},
	{"Version", ShowVersion, 0, 0, .ShowDirect = true},
	{0},
};

void CUIMenu::ShowCPUTemp(CUIMenu *pUIMenu, TMenuEvent Event)
{
	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	default:
		return;
	}

	CStatus *pStatus = CStatus::Get();

	char info[17];
	snprintf(info, sizeof(info), "%u/%u C", pStatus->nCPUTemp.load(), pStatus->nCPUMaxTemp);

	pUIMenu->m_pUI->DisplayWrite(pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Name,
				     info,
				     pUIMenu->m_nCurrentSelection > 0, !!pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection + 1].Name);

	static TKernelTimerHandle timer = 0;
	if (timer) CTimer::Get()->CancelKernelTimer(timer);
	timer = CTimer::Get()->StartKernelTimer(MSEC2HZ(3000), TimerHandlerUpdate, 0, pUIMenu);
}

void CUIMenu::ShowCPUSpeed(CUIMenu *pUIMenu, TMenuEvent Event)
{
	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	default:
		return;
	}

	CStatus *pStatus = CStatus::Get();

	char info[17];
	snprintf(info, sizeof(info), "%u/%u MHz", pStatus->nCPUClockRate.load() / 1000000, pStatus->nCPUMaxClockRate / 1000000);

	pUIMenu->m_pUI->DisplayWrite(pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Name,
				     info,
				     pUIMenu->m_nCurrentSelection > 0, !!pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection + 1].Name);

	static TKernelTimerHandle timer = 0;
	if (timer) CTimer::Get()->CancelKernelTimer(timer);
	timer = CTimer::Get()->StartKernelTimer(MSEC2HZ(3000), TimerHandlerUpdate, 0, pUIMenu);
}

void CUIMenu::ShowIPAddr(CUIMenu *pUIMenu, TMenuEvent Event)
{
	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	default:
		return;
	}

	CString IPString("-");
	const CIPAddress &IPAddr = pUIMenu->m_pMiniDexed->GetNetworkIPAddress();

	if (IPAddr.IsSet())
	{
		IPAddr.Format(&IPString);
	}

	pUIMenu->m_pUI->DisplayWrite(pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Name,
				     IPString.c_str(),
				     pUIMenu->m_nCurrentSelection > 0, !!pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection + 1].Name);

	static TKernelTimerHandle timer = 0;
	if (timer) CTimer::Get()->CancelKernelTimer(timer);
	timer = CTimer::Get()->StartKernelTimer(MSEC2HZ(3000), TimerHandlerUpdate, 0, pUIMenu);
}

void CUIMenu::ShowVersion(CUIMenu *pUIMenu, TMenuEvent Event)
{
	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	default:
		return;
	}

	pUIMenu->m_pUI->DisplayWrite(pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Name,
				     VERSION,
				     pUIMenu->m_nCurrentSelection > 0, !!pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection + 1].Name);
}

CUIMenu::CUIMenu(CUserInterface *pUI, CMiniDexed *pMiniDexed, CConfig *pConfig) :
m_pUI{pUI},
m_pMiniDexed{pMiniDexed},
m_pConfig{pConfig},
m_pParentMenu{s_MenuRoot},
m_pCurrentMenu{s_MainMenu},
m_nCurrentMenuItem{},
m_nCurrentSelection{},
m_nCurrentParameter{},
m_nCurrentMenuDepth{}
{
	assert(m_pConfig);
	m_nToneGenerators = m_pConfig->GetToneGenerators();

	if (m_nToneGenerators == 1)
	{
		// If there is just one core, then there is only a single
		// tone generator so start on the TG1 menu...
		m_pParentMenu = s_MainMenu;
		m_pCurrentMenu = s_TGMenu;
		m_nCurrentMenuItem = 0;
		m_nCurrentSelection = 0;
		m_nCurrentParameter = 0;
		m_nCurrentMenuDepth = 1;

		// Place the "root" menu at the top of the stack
		m_MenuStackParent[0] = s_MenuRoot;
		m_MenuStackMenu[0] = s_MainMenu;
		m_nMenuStackItem[0] = 0;
		m_nMenuStackSelection[0] = 0;
		m_nMenuStackParameter[0] = 0;
	}

	for (int n = 0; n < ARRAY_LENGTH(s_MainMenu); ++n)
	{
		TMenuItem *pItem = &s_MainMenu[n];

		if (pItem->MenuItem == s_TGMenu && pItem->Parameter >= m_nToneGenerators)
			pItem->Skip = true;

#ifdef ARM_ALLOW_MULTI_CORE
		if (pItem->MenuItem == s_BusMenu && (m_nToneGenerators <= 8 || pItem->Parameter >= m_nToneGenerators / 8))
			pItem->Skip = true;

		if (pItem->MenuItem == s_EffectsMenu && m_nToneGenerators > 8)
			pItem->Skip = true;

		if (pItem->MenuItem == s_OutputMenu && m_nToneGenerators <= 8)
			pItem->Skip = true;
#endif
	}

	for (int n = 0; n < ARRAY_LENGTH(s_MixerMenu); ++n)
	{
		TMenuItem *pItem = &s_MixerMenu[n];

		if (pItem->nBus >= m_nToneGenerators / 8)
			pItem->Name = 0;
	}

	if (m_pConfig->GetEncoderEnabled())
	{
		s_GlobalParameter[CMiniDexed::ParameterMasterVolume].Increment = 1;
		s_TGParameter[CMiniDexed::TGParameterVolume].Increment = 1;
		s_TGParameter[CMiniDexed::TGParameterPan].Increment = 1;
	}

	s_GlobalParameter[CMiniDexed::ParameterSDFilter].Maximum = SDFilter::get_maximum(m_nToneGenerators);
}

void CUIMenu::EventHandler(TMenuEvent Event)
{
	switch (Event)
	{
	case MenuEventBack: // pop menu
		if (m_nCurrentMenuDepth)
		{
			m_nCurrentMenuDepth--;

			m_pParentMenu = m_MenuStackParent[m_nCurrentMenuDepth];
			m_pCurrentMenu = m_MenuStackMenu[m_nCurrentMenuDepth];
			m_nCurrentMenuItem = m_nMenuStackItem[m_nCurrentMenuDepth];
			m_nCurrentSelection = m_nMenuStackSelection[m_nCurrentMenuDepth];
			m_nCurrentParameter = m_nMenuStackParameter[m_nCurrentMenuDepth];

			EventHandler(MenuEventUpdate);
		}
		break;

	case MenuEventHome:
		if (m_nToneGenerators == 1)
		{
			// "Home" is the TG0 menu if only one TG active
			m_pParentMenu = s_MainMenu;
			m_pCurrentMenu = s_TGMenu;
			m_nCurrentMenuItem = 0;
			m_nCurrentSelection = 0;
			m_nCurrentParameter = 0;
			m_nCurrentMenuDepth = 1;
			// Place the "root" menu at the top of the stack
			m_MenuStackParent[0] = s_MenuRoot;
			m_MenuStackMenu[0] = s_MainMenu;
			m_nMenuStackItem[0] = 0;
			m_nMenuStackSelection[0] = 0;
			m_nMenuStackParameter[0] = 0;
		}
		else
		{
			m_pParentMenu = s_MenuRoot;
			m_pCurrentMenu = s_MainMenu;
			m_nCurrentMenuItem = 0;
			m_nCurrentSelection = 0;
			m_nCurrentParameter = 0;
			m_nCurrentMenuDepth = 0;
		}
		EventHandler(MenuEventUpdate);
		break;

	case MenuEventPgmUp:
	case MenuEventPgmDown:
		PgmUpDownHandler(Event);
		break;

	case MenuEventBankUp:
	case MenuEventBankDown:
		BankUpDownHandler(Event);
		break;

	case MenuEventTGUp:
	case MenuEventTGDown:
		TGUpDownHandler(Event);
		break;

	default:
		if (m_pParentMenu[m_nCurrentMenuItem].Handler)
			(*m_pParentMenu[m_nCurrentMenuItem].Handler)(this, Event);
		break;
	}
}

void CUIMenu::MenuHandler(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nCurrentSelection = pUIMenu->m_nCurrentSelection;

	switch (Event)
	{
	case MenuEventUpdate:
		break;

	case MenuEventSelect: // push menu
		assert(pUIMenu->m_nCurrentMenuDepth < MaxMenuDepth);

		if (pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].ShowDirect)
			break;

		if (!pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Handler)
			break;

		pUIMenu->m_MenuStackParent[pUIMenu->m_nCurrentMenuDepth] = pUIMenu->m_pParentMenu;
		pUIMenu->m_MenuStackMenu[pUIMenu->m_nCurrentMenuDepth] = pUIMenu->m_pCurrentMenu;
		pUIMenu->m_nMenuStackItem[pUIMenu->m_nCurrentMenuDepth] = pUIMenu->m_nCurrentMenuItem;
		pUIMenu->m_nMenuStackSelection[pUIMenu->m_nCurrentMenuDepth] = pUIMenu->m_nCurrentSelection;
		pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth] = pUIMenu->m_nCurrentParameter;
		pUIMenu->m_nCurrentMenuDepth++;

		pUIMenu->m_pParentMenu = pUIMenu->m_pCurrentMenu;
		pUIMenu->m_nCurrentParameter =
		pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Parameter;
		pUIMenu->m_pCurrentMenu =
		pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].MenuItem;
		pUIMenu->m_nCurrentMenuItem = pUIMenu->m_nCurrentSelection;
		pUIMenu->m_nCurrentSelection = 0;

		if (pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].OnSelect)
			pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].OnSelect(pUIMenu, Event);
		break;

	case MenuEventStepDown:
		if (pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].StepDown)
		{
			pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].StepDown(pUIMenu, Event);
			break;
		}

		while (true)
		{
			if (nCurrentSelection == 0)
			{
				while (pUIMenu->m_pCurrentMenu[nCurrentSelection].Name)
				{
					nCurrentSelection++;
				}
				break;
			}

			nCurrentSelection--;

			// Might need to trim menu if number of TGs is configured to be less than the maximum supported
			if (!pUIMenu->m_pCurrentMenu[nCurrentSelection].Skip)
				break;
		}

		if (pUIMenu->m_pCurrentMenu[nCurrentSelection].Name)
		{
			pUIMenu->m_nCurrentSelection = nCurrentSelection;
		}
		else if (pUIMenu->m_pCurrentMenu == s_MainMenu)
		{
			// If in main menu, wrap around
			pUIMenu->m_nCurrentSelection = nCurrentSelection - 1;
		}
		break;

	case MenuEventStepUp:
		if (pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].StepUp)
		{
			pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].StepUp(pUIMenu, Event);
			break;
		}

		while (true)
		{
			nCurrentSelection++;

			if (!pUIMenu->m_pCurrentMenu[nCurrentSelection].Name)
				break;

			// Might need to trim menu if number of TGs is configured to be less than the maximum supported
			if (!pUIMenu->m_pCurrentMenu[nCurrentSelection].Skip)
				break;
		}

		if (pUIMenu->m_pCurrentMenu[nCurrentSelection].Name)
		{
			pUIMenu->m_nCurrentSelection = nCurrentSelection;
		}
		else if (pUIMenu->m_pCurrentMenu == s_MainMenu)
		{
			// If in main mennu, wrap around
			pUIMenu->m_nCurrentSelection = 0;
		}
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->GlobalShortcutHandler(Event);
		return;

	default:
		return;
	}

	if (pUIMenu->m_pCurrentMenu) // if this is another menu?
	{
		if (pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].ShowDirect)
		{
			pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Handler(pUIMenu, MenuEventUpdate);
			return;
		}

		bool bIsMainMenu = pUIMenu->m_pCurrentMenu == s_MainMenu;
		bool bIsTGMenu = pUIMenu->m_pCurrentMenu == s_TGMenu;

		std::string menuName = pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name;

		if (bIsTGMenu)
		{
			int nTG = pUIMenu->m_nCurrentParameter;
			int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);
			if (nTGLink) menuName += ToTGLinkName(nTGLink, 0);
		}

		std::string selectionName = pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Name;

		if (pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].MenuItem == s_TGMenu)
		{
			int nTG = pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection].Parameter;
			int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);
			if (nTGLink) selectionName += ToTGLinkName(nTGLink, 0);
		}

		pUIMenu->m_pUI->DisplayWrite(menuName.c_str(), "", selectionName.c_str(),
					     pUIMenu->m_nCurrentSelection > 0 || bIsMainMenu,
					     !!pUIMenu->m_pCurrentMenu[pUIMenu->m_nCurrentSelection + 1].Name || bIsMainMenu);
	}
	else
	{
		pUIMenu->EventHandler(MenuEventUpdate); // no, update parameter display
	}
}

void CUIMenu::EditGlobalParameter(CUIMenu *pUIMenu, TMenuEvent Event)
{
	CMiniDexed::TParameter Param = (CMiniDexed::TParameter)pUIMenu->m_nCurrentParameter;
	const TParameter &rParam = s_GlobalParameter[Param];

	int nValue = pUIMenu->m_pMiniDexed->GetParameter(Param);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetParameter(Param, nValue);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetParameter(Param, nValue);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->GlobalShortcutHandler(Event);
		return;

	default:
		return;
	}

	const char *pMenuName = pUIMenu->m_nCurrentMenuDepth == 1 ? "" : pUIMenu->m_MenuStackParent[pUIMenu->m_nCurrentMenuDepth - 1][pUIMenu->m_nMenuStackItem[pUIMenu->m_nCurrentMenuDepth - 1]].Name;

	std::string Value = GetGlobalValueString(Param,
						 pUIMenu->m_pMiniDexed->GetParameter(Param),
						 pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(pMenuName,
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditVoiceBankNumber(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 1];

	int nValue = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);
	int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue = pUIMenu->m_pMiniDexed->GetSysExFileLoader()->GetNextBankDown(nValue);
		pUIMenu->m_pMiniDexed->SetTGParameter(
		CMiniDexed::TGParameterVoiceBank, nValue, nTG);
		break;

	case MenuEventStepUp:
		nValue = pUIMenu->m_pMiniDexed->GetSysExFileLoader()->GetNextBankUp(nValue);
		pUIMenu->m_pMiniDexed->SetTGParameter(
		CMiniDexed::TGParameterVoiceBank, nValue, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->TGShortcutHandler(Event);
		return;

	default:
		return;
	}

	std::string TG("TG");
	TG += std::to_string(nTG + 1);
	if (nTGLink) TG += ToTGLinkName(nTGLink, 0);

	std::string Value = std::to_string(nValue + 1) + "=" + pUIMenu->m_pMiniDexed->GetSysExFileLoader()->GetBankName(nValue);

	pUIMenu->m_pUI->DisplayWrite(TG.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > 0, nValue < CSysExFileLoader::MaxVoiceBankID);
}

void CUIMenu::EditProgramNumber(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 1];

	int nValue = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterProgram, nTG);
	int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		if (--nValue < 0)
		{
			// Switch down a voice bank and set to the last voice
			nValue = CSysExFileLoader::VoicesPerBank - 1;
			int nVB = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);
			nVB = pUIMenu->m_pMiniDexed->GetSysExFileLoader()->GetNextBankDown(nVB);
			pUIMenu->m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterVoiceBank, nVB, nTG);
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterProgram, nValue, nTG);
		break;

	case MenuEventStepUp:
		if (++nValue > CSysExFileLoader::VoicesPerBank - 1)
		{
			// Switch up a voice bank and reset to voice 0
			nValue = 0;
			int nVB = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);
			nVB = pUIMenu->m_pMiniDexed->GetSysExFileLoader()->GetNextBankUp(nVB);
			pUIMenu->m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterVoiceBank, nVB, nTG);
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterProgram, nValue, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->TGShortcutHandler(Event);
		return;

	default:
		return;
	}

	// Skip empty voices.
	// Use same criteria in PgmUpDownHandler() too.
	std::string voiceName = pUIMenu->m_pMiniDexed->GetVoiceName(nTG);
	if (voiceName == "EMPTY     " || voiceName == "          " || voiceName == "----------" || voiceName == "~~~~~~~~~~")
	{
		if (Event == MenuEventStepUp)
		{
			CUIMenu::EditProgramNumber(pUIMenu, MenuEventStepUp);
		}
		if (Event == MenuEventStepDown)
		{
			CUIMenu::EditProgramNumber(pUIMenu, MenuEventStepDown);
		}
	}
	else
	{
		// Format: 000:000      TG1 (bank:voice padded, TGx right-aligned)
		int nBank = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);
		std::string voice = "000";
		voice += std::to_string(nBank + 1);
		voice = voice.substr(voice.length() - 3, 3);
		voice += ":";
		std::string voiceNum = "000";
		voiceNum += std::to_string(nValue + 1);
		voiceNum = voiceNum.substr(voiceNum.length() - 3, 3);
		voice += voiceNum;

		std::string TG = "TG" + std::to_string(nTG + 1);
		if (nTGLink) TG += ToTGLinkName(nTGLink, 0);

		std::string Value = pUIMenu->m_pMiniDexed->GetVoiceName(nTG);

		pUIMenu->m_pUI->DisplayWrite(TG.c_str(),
					     voice.c_str(),
					     Value.c_str(),
					     nValue > 0, nValue < CSysExFileLoader::VoicesPerBank);
	}
}

void CUIMenu::EditTGParameter(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 1];

	CMiniDexed::TTGParameter Param = (CMiniDexed::TTGParameter)pUIMenu->m_nCurrentParameter;
	const TParameter &rParam = s_TGParameter[Param];

	int nValue = pUIMenu->m_pMiniDexed->GetTGParameter(Param, nTG);
	int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nValue, nTG);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nValue, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->TGShortcutHandler(Event);
		return;

	default:
		return;
	}

	std::string TG("TG");
	TG += std::to_string(nTG + 1);
	if (nTGLink &&
	    Param != CMiniDexed::TGParameterTGLink &&
	    Param != CMiniDexed::TGParameterPan &&
	    Param != CMiniDexed::TGParameterMasterTune)
		TG += ToTGLinkName(nTGLink, 0);

	std::string Value = GetTGValueString(Param,
					     pUIMenu->m_pMiniDexed->GetTGParameter(Param, nTG),
					     pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(TG.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditTGParameter2(CUIMenu *pUIMenu, TMenuEvent Event) // second menu level. Redundant code but in order to not modified original code
{
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 2];

	CMiniDexed::TTGParameter Param = (CMiniDexed::TTGParameter)pUIMenu->m_nCurrentParameter;
	const TParameter &rParam = s_TGParameter[Param];

	int nValue = pUIMenu->m_pMiniDexed->GetTGParameter(Param, nTG);
	int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventSelect:
		if (pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].OnSelect)
			pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].OnSelect(pUIMenu, Event);
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nValue, nTG);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nValue, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->TGShortcutHandler(Event);
		return;

	default:
		return;
	}

	std::string TG("TG");
	TG += std::to_string(nTG + 1);
	if (nTGLink) TG += ToTGLinkName(nTGLink, 0);

	std::string Value = GetTGValueString(Param,
					     pUIMenu->m_pMiniDexed->GetTGParameter(Param, nTG),
					     pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(TG.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditFXParameter2(CUIMenu *pUIMenu, TMenuEvent Event)
{
	FX::Parameter Param = (FX::Parameter)pUIMenu->m_nCurrentParameter;
	const FX::ParameterType &rParam = FX::s_Parameter[Param];
	int nBus = pUIMenu->m_nMenuStackParameter[1];
	int idFX = pUIMenu->m_nMenuStackParameter[2];
	int nFX = idFX + CConfig::BusFXChains * nBus;

	int nValue = pUIMenu->m_pMiniDexed->GetFXParameter(Param, nFX);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetFXParameter(Param, nValue, nFX);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetFXParameter(Param, nValue, nFX);
		break;

	default:
		return;
	}

	std::string FX;
	if (nFX == CConfig::MasterFX)
		FX = "MFX";
	else
		FX = std::string("FX") + std::to_string(idFX + 1);

	std::string Value = GetFXValueString(Param,
					     pUIMenu->m_pMiniDexed->GetFXParameter(Param, nFX),
					     pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(FX.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditFXParameterG(CUIMenu *pUIMenu, TMenuEvent Event)
{
	FX::Parameter Param = (FX::Parameter)pUIMenu->m_nCurrentParameter;
	const FX::ParameterType &rParam = FX::s_Parameter[Param];
	int nBus = pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].nBus;
	int idFX = pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].idFX;
	int nFX = idFX + CConfig::BusFXChains * nBus;

	int nValue = pUIMenu->m_pMiniDexed->GetFXParameter(Param, nFX);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetFXParameter(Param, nValue, nFX);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetFXParameter(Param, nValue, nFX);
		break;

	default:
		return;
	}

	std::string FX;
	if (nFX == CConfig::MasterFX)
		FX = "MFX";
	else
		FX = std::string("FX") + std::to_string(idFX + 1);

	std::string Value = GetFXValueString(Param,
					     pUIMenu->m_pMiniDexed->GetFXParameter(Param, nFX),
					     pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(FX.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditBusParameter(CUIMenu *pUIMenu, TMenuEvent Event)
{
	Bus::Parameter Param = (Bus::Parameter)pUIMenu->m_nCurrentParameter;
	const Bus::ParameterType &rParam = Bus::s_Parameter[Param];
	int nBus = pUIMenu->m_nMenuStackParameter[1];

	int nValue = pUIMenu->m_pMiniDexed->GetBusParameter(Param, nBus);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetBusParameter(Param, nValue, nBus);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetBusParameter(Param, nValue, nBus);
		break;

	default:
		return;
	}

	std::string Bus = std::string("Bus") + std::to_string(nBus + 1);

	std::string Value = GetBusValueString(Param,
					      pUIMenu->m_pMiniDexed->GetBusParameter(Param, nBus),
					      pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(Bus.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditBusParameterG(CUIMenu *pUIMenu, TMenuEvent Event)
{
	Bus::Parameter Param = (Bus::Parameter)pUIMenu->m_nCurrentParameter;
	const Bus::ParameterType &rParam = Bus::s_Parameter[Param];
	int nBus = pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].nBus;

	int nValue = pUIMenu->m_pMiniDexed->GetBusParameter(Param, nBus);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetBusParameter(Param, nValue, nBus);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetBusParameter(Param, nValue, nBus);
		break;

	default:
		return;
	}

	std::string Bus = std::string("Bus") + std::to_string(nBus + 1);

	std::string Value = GetBusValueString(Param,
					      pUIMenu->m_pMiniDexed->GetBusParameter(Param, nBus),
					      pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(Bus.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditVoiceParameter(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 2];

	int nParam = pUIMenu->m_nCurrentParameter;
	const TParameter &rParam = s_VoiceParameter[nParam];

	int nValue = pUIMenu->m_pMiniDexed->GetVoiceParameter(nParam, CMiniDexed::NoOP, nTG);
	int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetVoiceParameter(nParam, nValue, CMiniDexed::NoOP, nTG);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetVoiceParameter(nParam, nValue, CMiniDexed::NoOP, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->TGShortcutHandler(Event);
		return;

	default:
		return;
	}

	std::string TG("TG");
	TG += std::to_string(nTG + 1);
	if (nTGLink) TG += ToTGLinkName(nTGLink, 0);

	std::string Value = GetVoiceValueString(nParam, nValue, pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(TG.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::EditOPParameter(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 3];
	int nOP = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 1];

	int nParam = pUIMenu->m_nCurrentParameter;
	const TParameter &rParam = s_OPParameter[nParam];

	int nValue = pUIMenu->m_pMiniDexed->GetVoiceParameter(nParam, nOP, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
	case MenuEventUpdateParameter:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetVoiceParameter(nParam, nValue, nOP, nTG);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetVoiceParameter(nParam, nValue, nOP, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->OPShortcutHandler(Event);
		return;

	default:
		return;
	}

	std::string OP("OP");
	OP += std::to_string(nOP + 1);

	std::string Value;

	static const int FixedMultiplier[4] = {1, 10, 100, 1000};
	if (nParam == DEXED_OP_FREQ_COARSE)
	{
		if (!pUIMenu->m_pMiniDexed->GetVoiceParameter(DEXED_OP_OSC_MODE, nOP, nTG))
		{
			// Ratio
			if (!nValue)
			{
				Value = "0.50";
			}
			else
			{
				Value = std::to_string(nValue);
				Value += ".00";
			}
		}
		else
		{
			// Fixed
			Value = std::to_string(FixedMultiplier[nValue % 4]);
		}
	}
	else if (nParam == DEXED_OP_FREQ_FINE)
	{
		int nCoarse = pUIMenu->m_pMiniDexed->GetVoiceParameter(
		DEXED_OP_FREQ_COARSE, nOP, nTG);

		char Buffer[20];
		if (!pUIMenu->m_pMiniDexed->GetVoiceParameter(DEXED_OP_OSC_MODE, nOP, nTG))
		{
			// Ratio
			float fValue = 1.0f + nValue / 100.0f;
			fValue *= !nCoarse ? 0.5f : float(nCoarse);
			sprintf(Buffer, "%.2f", fValue);
		}
		else
		{
			// Fixed
			float fValue = powf(1.023293f, nValue);
			fValue *= FixedMultiplier[nCoarse % 4];
			sprintf(Buffer, "%.3fHz", fValue);
		}

		Value = Buffer;
	}
	else
	{
		Value = GetOPValueString(nParam, nValue, pUIMenu->m_pConfig->GetLCDColumns() - 2);
	}

	pUIMenu->m_pUI->DisplayWrite(OP.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}

void CUIMenu::SavePerformance(CUIMenu *pUIMenu, TMenuEvent Event)
{
	if (Event != MenuEventUpdate)
	{
		return;
	}

	bool bOK = pUIMenu->m_pMiniDexed->SavePerformance(pUIMenu->m_nCurrentParameter == 1);

	const char *pMenuName =
	pUIMenu->m_MenuStackParent[pUIMenu->m_nCurrentMenuDepth - 1]
				  [pUIMenu->m_nMenuStackItem[pUIMenu->m_nCurrentMenuDepth - 1]]
				  .Name;

	pUIMenu->m_pUI->DisplayWrite(pMenuName,
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     bOK ? "Completed" : "Error",
				     false, false);

	CTimer::Get()->StartKernelTimer(MSEC2HZ(1500), TimerHandler, 0, pUIMenu);
}

std::string CUIMenu::GetGlobalValueString(int nParameter, int nValue, int nWidth)
{
	std::string Result;

	assert(nParameter < ARRAY_LENGTH(CUIMenu::s_GlobalParameter));

	CUIMenu::TToString *pToString = CUIMenu::s_GlobalParameter[nParameter].ToString;
	if (pToString)
	{
		Result = (*pToString)(nValue, nWidth);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string CUIMenu::GetTGValueString(int nTGParameter, int nValue, int nWidth)
{
	std::string Result;

	assert(nTGParameter < ARRAY_LENGTH(CUIMenu::s_TGParameter));

	CUIMenu::TToString *pToString = CUIMenu::s_TGParameter[nTGParameter].ToString;
	if (pToString)
	{
		Result = (*pToString)(nValue, nWidth);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string CUIMenu::GetFXValueString(int nFXParameter, int nValue, int nWidth)
{
	std::string Result;

	assert(nFXParameter < FX::Parameter::Unknown);

	CUIMenu::TToString *pToString = FX::s_Parameter[nFXParameter].ToString;
	if (pToString)
	{
		Result = (*pToString)(nValue, nWidth);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string CUIMenu::GetBusValueString(int nBusParameter, int nValue, int nWidth)
{
	std::string Result;

	assert(nBusParameter < Bus::Parameter::Unknown);

	CUIMenu::TToString *pToString = Bus::s_Parameter[nBusParameter].ToString;
	if (pToString)
	{
		Result = (*pToString)(nValue, nWidth);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string CUIMenu::GetVoiceValueString(int nVoiceParameter, int nValue, int nWidth)
{
	std::string Result;

	assert(nVoiceParameter < ARRAY_LENGTH(CUIMenu::s_VoiceParameter));

	CUIMenu::TToString *pToString = CUIMenu::s_VoiceParameter[nVoiceParameter].ToString;
	if (pToString)
	{
		Result = (*pToString)(nValue, nWidth);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string CUIMenu::GetOPValueString(int nOPParameter, int nValue, int nWidth)
{
	std::string Result;

	assert(nOPParameter < ARRAY_LENGTH(CUIMenu::s_OPParameter));

	CUIMenu::TToString *pToString = CUIMenu::s_OPParameter[nOPParameter].ToString;
	if (pToString)
	{
		Result = (*pToString)(nValue, nWidth);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string CUIMenu::ToSDFilter(int nValue, int nWidth)
{
	int nTG = s_GlobalParameter[CMiniDexed::ParameterSDFilter].Maximum - SDFilter::get_maximum(0);

	return SDFilter::to_filter(nValue, nTG).to_string();
}

void CUIMenu::GlobalShortcutHandler(TMenuEvent Event)
{
#ifdef ARM_ALLOW_MULTI_CORE
	if (m_pParentMenu == s_PlateReverbMenu ||
	    m_pParentMenu == s_YKChorusMenu ||
	    m_pParentMenu == s_DreamDelayMenu ||
	    m_pParentMenu == s_FXEQMenu ||
	    m_pParentMenu == s_CompressorMenu ||
	    m_pParentMenu == s_EditCompressorMenu ||
	    m_pParentMenu == s_EQMenu ||
	    m_pCurrentMenu == s_TGMenu)
	{
		bool bSaveCurrentSelection = m_pCurrentMenu == s_TGMenu;

		int nSavedSelection = m_nCurrentSelection;

		if (Event == MenuEventPressAndStepDown)
		{
			int nTG = m_nMenuStackSelection[0];
			if (m_pCurrentMenu == s_TGMenu && nTG == 0) return;

			EventHandler(MenuEventBack);
			EventHandler(MenuEventStepDown);
			EventHandler(MenuEventSelect);
		}
		else
		{
			int nTG = m_nMenuStackSelection[0];
			if (m_pCurrentMenu == s_TGMenu && nTG == m_nToneGenerators - 1) return;

			EventHandler(MenuEventBack);
			EventHandler(MenuEventStepUp);
			EventHandler(MenuEventSelect);
		}

		if (bSaveCurrentSelection)
		{
			m_nCurrentSelection = nSavedSelection;
			EventHandler(MenuEventUpdate);
		}
	}
#endif
}

void CUIMenu::TGShortcutHandler(TMenuEvent Event)
{
	assert(m_nCurrentMenuDepth >= 2);
	assert(m_MenuStackMenu[0] = s_MainMenu);
	int nTG = m_nMenuStackSelection[0];
	assert(nTG >= 0 && nTG < CConfig::AllToneGenerators);
	assert(m_nMenuStackItem[1] == nTG);
	assert(m_nMenuStackParameter[1] == nTG);

	assert(Event == MenuEventPressAndStepDown || Event == MenuEventPressAndStepUp);

	if (m_pParentMenu == s_EQMenu || m_pParentMenu == s_EditCompressorMenu)
	{
		if (Event == MenuEventPressAndStepDown)
		{
			EventHandler(MenuEventBack);
			EventHandler(MenuEventStepDown);
			EventHandler(MenuEventSelect);
		}
		else
		{
			EventHandler(MenuEventBack);
			EventHandler(MenuEventStepUp);
			EventHandler(MenuEventSelect);
		}
	}
	else
	{
		if (Event == MenuEventPressAndStepDown)
		{
			nTG--;
		}
		else
		{
			nTG++;
		}

		if (nTG >= 0 && nTG < m_nToneGenerators)
		{
			m_nMenuStackSelection[0] = nTG;
			m_nMenuStackItem[1] = nTG;
			m_nMenuStackParameter[1] = nTG;

			EventHandler(MenuEventUpdate);
		}
	}
}

void CUIMenu::OPShortcutHandler(TMenuEvent Event)
{
	assert(m_nCurrentMenuDepth >= 3);
	assert(m_MenuStackMenu[m_nCurrentMenuDepth - 2] = s_EditVoiceMenu);
	int nOP = m_nMenuStackSelection[m_nCurrentMenuDepth - 2];
	assert(nOP >= 0 && nOP < 6);
	assert(m_nMenuStackItem[m_nCurrentMenuDepth - 1] == nOP);
	assert(m_nMenuStackParameter[m_nCurrentMenuDepth - 1] == nOP);

	assert(Event == MenuEventPressAndStepDown || Event == MenuEventPressAndStepUp);
	if (Event == MenuEventPressAndStepDown)
	{
		nOP--;
	}
	else
	{
		nOP++;
	}

	if (nOP >= 0 && nOP < 6)
	{
		m_nMenuStackSelection[m_nCurrentMenuDepth - 2] = nOP;
		m_nMenuStackItem[m_nCurrentMenuDepth - 1] = nOP;
		m_nMenuStackParameter[m_nCurrentMenuDepth - 1] = nOP;

		EventHandler(MenuEventUpdate);
	}
}

void CUIMenu::PgmUpDownHandler(TMenuEvent Event)
{
	if (m_pMiniDexed->GetParameter(CMiniDexed::ParameterPerformanceSelectChannel) != CMIDIDevice::Disabled)
	{
		// Program Up/Down acts on performances
		int nLastPerformance = m_pMiniDexed->GetLastPerformance();
		int nPerformance = m_pMiniDexed->GetActualPerformanceID();
		int nStart = nPerformance;
		// LOGNOTE("Performance actual=%d, last=%d", nPerformance, nLastPerformance);
		if (Event == MenuEventPgmDown)
		{
			do
			{
				if (nPerformance == 0)
				{
					// Wrap around
					nPerformance = nLastPerformance;
				}
				else if (nPerformance > 0)
				{
					--nPerformance;
				}
			} while ((m_pMiniDexed->IsValidPerformance(nPerformance) != true) && (nPerformance != nStart));
			m_nSelectedPerformanceID = nPerformance;
			m_pMiniDexed->SetNewPerformance(m_nSelectedPerformanceID);
			// LOGNOTE("Performance new=%d, last=%d", m_nSelectedPerformanceID, nLastPerformance);
		}
		else // MenuEventPgmUp
		{
			do
			{
				if (nPerformance == nLastPerformance)
				{
					// Wrap around
					nPerformance = 0;
				}
				else if (nPerformance < nLastPerformance)
				{
					++nPerformance;
				}
			} while ((m_pMiniDexed->IsValidPerformance(nPerformance) != true) && (nPerformance != nStart));
			m_nSelectedPerformanceID = nPerformance;
			m_pMiniDexed->SetNewPerformance(m_nSelectedPerformanceID);
			// LOGNOTE("Performance new=%d, last=%d", m_nSelectedPerformanceID, nLastPerformance);
		}
	}
	else
	{
		// Program Up/Down acts on voices within a TG.

		// If we're not in the root menu, then see if we are already in a TG menu,
		// then find the current TG number. Otherwise assume TG1 (nTG=0).
		int nTG = 0;
		if (m_MenuStackMenu[0] == s_MainMenu && (m_pCurrentMenu == s_TGMenu) || (m_MenuStackMenu[1] == s_TGMenu))
		{
			nTG = m_nMenuStackSelection[0];
		}
		assert(nTG < CConfig::AllToneGenerators);
		if (nTG < m_nToneGenerators)
		{
			int nPgm = m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterProgram, nTG);

			assert(Event == MenuEventPgmDown || Event == MenuEventPgmUp);
			if (Event == MenuEventPgmDown)
			{
				// LOGNOTE("PgmDown");
				if (--nPgm < 0)
				{
					// Switch down a voice bank and set to the last voice
					nPgm = CSysExFileLoader::VoicesPerBank - 1;
					int nVB = m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);
					nVB = m_pMiniDexed->GetSysExFileLoader()->GetNextBankDown(nVB);
					m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterVoiceBank, nVB, nTG);
				}
				m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterProgram, nPgm, nTG);
			}
			else
			{
				// LOGNOTE("PgmUp");
				if (++nPgm > CSysExFileLoader::VoicesPerBank - 1)
				{
					// Switch up a voice bank and reset to voice 0
					nPgm = 0;
					int nVB = m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);
					nVB = m_pMiniDexed->GetSysExFileLoader()->GetNextBankUp(nVB);
					m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterVoiceBank, nVB, nTG);
				}
				m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterProgram, nPgm, nTG);
			}

			// Skip empty voices.
			// Use same criteria in EditProgramNumber () too.
			std::string voiceName = m_pMiniDexed->GetVoiceName(nTG);
			if (voiceName == "EMPTY     " || voiceName == "          " || voiceName == "----------" || voiceName == "~~~~~~~~~~")
			{
				if (Event == MenuEventStepUp)
				{
					PgmUpDownHandler(MenuEventStepUp);
				}
				if (Event == MenuEventStepDown)
				{
					PgmUpDownHandler(MenuEventStepDown);
				}
			}
		}
	}
}

void CUIMenu::BankUpDownHandler(TMenuEvent Event)
{
	if (m_pMiniDexed->GetParameter(CMiniDexed::ParameterPerformanceSelectChannel) != CMIDIDevice::Disabled)
	{
		// Bank Up/Down acts on performances
		int nLastPerformanceBank = m_pMiniDexed->GetLastPerformanceBank();
		int nPerformanceBank = m_nSelectedPerformanceBankID;
		int nStartBank = nPerformanceBank;
		// LOGNOTE("Performance Bank actual=%d, last=%d", nPerformanceBank, nLastPerformanceBank);
		if (Event == MenuEventBankDown)
		{
			do
			{
				if (nPerformanceBank == 0)
				{
					// Wrap around
					nPerformanceBank = nLastPerformanceBank;
				}
				else if (nPerformanceBank > 0)
				{
					--nPerformanceBank;
				}
			} while ((m_pMiniDexed->IsValidPerformanceBank(nPerformanceBank) != true) && (nPerformanceBank != nStartBank));
			m_nSelectedPerformanceBankID = nPerformanceBank;
			// Switch to the new bank and select the first performance voice
			m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nPerformanceBank);
			m_pMiniDexed->SetFirstPerformance();
			// LOGNOTE("Performance Bank new=%d, last=%d", m_nSelectedPerformanceBankID, nLastPerformanceBank);
		}
		else // MenuEventBankUp
		{
			do
			{
				if (nPerformanceBank == nLastPerformanceBank)
				{
					// Wrap around
					nPerformanceBank = 0;
				}
				else if (nPerformanceBank < nLastPerformanceBank)
				{
					++nPerformanceBank;
				}
			} while ((m_pMiniDexed->IsValidPerformanceBank(nPerformanceBank) != true) && (nPerformanceBank != nStartBank));
			m_nSelectedPerformanceBankID = nPerformanceBank;
			m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nPerformanceBank);
			m_pMiniDexed->SetFirstPerformance();
			// LOGNOTE("Performance Bank new=%d, last=%d", m_nSelectedPerformanceBankID, nLastPerformanceBank);
		}
	}
	else
	{
		// Bank Up/Down acts on voices within a TG.

		// If we're not in the root menu, then see if we are already in a TG menu,
		// then find the current TG number. Otherwise assume TG1 (nTG=0).
		int nTG = 0;
		if (m_MenuStackMenu[0] == s_MainMenu && (m_pCurrentMenu == s_TGMenu) || (m_MenuStackMenu[1] == s_TGMenu))
		{
			nTG = m_nMenuStackSelection[0];
		}
		assert(nTG < CConfig::AllToneGenerators);
		if (nTG < m_nToneGenerators)
		{
			int nBank = m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterVoiceBank, nTG);

			assert(Event == MenuEventBankDown || Event == MenuEventBankUp);
			if (Event == MenuEventBankDown)
			{
				// LOGNOTE("BankDown");
				nBank = m_pMiniDexed->GetSysExFileLoader()->GetNextBankDown(nBank);
				m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterVoiceBank, nBank, nTG);
			}
			else
			{
				// LOGNOTE("BankUp");
				nBank = m_pMiniDexed->GetSysExFileLoader()->GetNextBankUp(nBank);
				m_pMiniDexed->SetTGParameter(CMiniDexed::TGParameterVoiceBank, nBank, nTG);
			}
		}
	}
}

void CUIMenu::TGUpDownHandler(TMenuEvent Event)
{
	// This will update the menus to position it for the next TG up or down
	int nTG = 0;

	if (m_nToneGenerators <= 1)
	{
		// Nothing to do if only a single TG
		return;
	}

	// If we're not in the root menu, then see if we are already in a TG menu,
	// then find the current TG number. Otherwise assume TG1 (nTG=0).
	if (m_MenuStackMenu[0] == s_MainMenu && (m_pCurrentMenu == s_TGMenu) || (m_MenuStackMenu[1] == s_TGMenu))
	{
		nTG = m_nMenuStackSelection[0];
	}

	assert(nTG >= 0 && nTG < CConfig::AllToneGenerators);
	assert(Event == MenuEventTGDown || Event == MenuEventTGUp);
	if (Event == MenuEventTGDown)
	{
		// LOGNOTE("TGDown");
		if (nTG > 0)
		{
			nTG--;
		}
	}
	else
	{
		// LOGNOTE("TGUp");
		if (nTG < m_nToneGenerators - 1)
		{
			nTG++;
		}
	}

	// Set menu to the appropriate TG menu as follows:
	//  Top = Root
	//  Menu [0] = Main
	//  Menu [1] = TG Menu
	m_pParentMenu = s_MainMenu;
	m_pCurrentMenu = s_TGMenu;
	m_nCurrentMenuItem = nTG;
	m_nCurrentSelection = 0;
	m_nCurrentParameter = nTG;
	m_nCurrentMenuDepth = 1;

	// Place the main menu on the stack with Root as the parent
	m_MenuStackParent[0] = s_MenuRoot;
	m_MenuStackMenu[0] = s_MainMenu;
	m_nMenuStackItem[0] = 0;
	m_nMenuStackSelection[0] = nTG;
	m_nMenuStackParameter[0] = 0;

	EventHandler(MenuEventUpdate);
}

void CUIMenu::TimerHandler(TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CUIMenu *pThis = static_cast<CUIMenu *>(pContext);
	assert(pThis);

	pThis->EventHandler(MenuEventBack);
}

void CUIMenu::TimerHandlerUpdate(TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CUIMenu *pThis = static_cast<CUIMenu *>(pContext);
	assert(pThis);

	pThis->EventHandler(MenuEventUpdate);
}

void CUIMenu::TimerHandlerNoBack(TKernelTimerHandle hTimer, void *pParam, void *pContext)
{
	CUIMenu *pThis = static_cast<CUIMenu *>(pContext);
	assert(pThis);

	pThis->m_bSplashShow = false;

	pThis->EventHandler(MenuEventUpdate);
}

void CUIMenu::InputKeyDown(CUIMenu *pUIMenu, TMenuEvent Event)
{
	assert(pUIMenu);

	int nLastKeyDown = pUIMenu->m_pMiniDexed->GetLastKeyDown();
	CMiniDexed::TTGParameter Param = (CMiniDexed::TTGParameter)pUIMenu->m_nCurrentParameter;
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 2];
	pUIMenu->m_pMiniDexed->SetTGParameter(Param, nLastKeyDown, nTG);
}

void CUIMenu::InputShiftKeyDown(CUIMenu *pUIMenu, TMenuEvent Event)
{
	assert(pUIMenu);

	static const int MIDINoteC3 = 60;

	int nLastKeyDown = pUIMenu->m_pMiniDexed->GetLastKeyDown() - MIDINoteC3;
	CMiniDexed::TTGParameter Param = (CMiniDexed::TTGParameter)pUIMenu->m_nCurrentParameter;
	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 2];
	if (nLastKeyDown >= -24 && nLastKeyDown <= 24)
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nLastKeyDown, nTG);
}

#ifdef ARM_ALLOW_MULTI_CORE

void CUIMenu::SelectCurrentEffect(CUIMenu *pUIMenu, TMenuEvent Event)
{
	assert(pUIMenu);

	FX::Parameter Param = (FX::Parameter)pUIMenu->m_nCurrentParameter;
	int nBus = pUIMenu->m_nMenuStackParameter[1];
	int idFX = pUIMenu->m_nMenuStackParameter[2];
	int nFX = idFX + CConfig::BusFXChains * nBus;
	int nValue = pUIMenu->m_pMiniDexed->GetFXParameter(Param, nFX);

	if (!nValue) return;

	assert(nValue < ARRAY_LENGTH(s_FXListMenu));

	pUIMenu->m_nCurrentSelection = nValue;
}

void CUIMenu::StepDownEffect(CUIMenu *pUIMenu, TMenuEvent Event)
{
	assert(pUIMenu);

	FX::Parameter Param = (FX::Parameter)pUIMenu->m_nCurrentParameter;
	int nBus = pUIMenu->m_nMenuStackParameter[1];
	int idFX = pUIMenu->m_nMenuStackParameter[2];
	int nFX = idFX + CConfig::BusFXChains * nBus;
	const FX::ParameterType &rParam = FX::s_Parameter[Param];
	int nValue = pUIMenu->m_nCurrentSelection;
	int increment = rParam.Increment;

	while (true)
	{
		if (nValue - increment < rParam.Minimum)
		{
			increment = 0;
			break;
		}

		if (!FXSlotFilter(pUIMenu, Event, nValue - increment))
			break;

		increment += rParam.Increment;
	}

	pUIMenu->m_nCurrentSelection -= increment;

	pUIMenu->m_pMiniDexed->SetFXParameter(Param, pUIMenu->m_nCurrentSelection, nFX);
}

void CUIMenu::StepUpEffect(CUIMenu *pUIMenu, TMenuEvent Event)
{
	assert(pUIMenu);

	FX::Parameter Param = (FX::Parameter)pUIMenu->m_nCurrentParameter;
	int nBus = pUIMenu->m_nMenuStackParameter[1];
	int idFX = pUIMenu->m_nMenuStackParameter[2];
	int nFX = idFX + CConfig::BusFXChains * nBus;
	const FX::ParameterType &rParam = FX::s_Parameter[Param];
	int nValue = pUIMenu->m_nCurrentSelection;
	int increment = rParam.Increment;

	while (true)
	{
		if (nValue + increment > rParam.Maximum)
		{
			increment = 0;
			break;
		}

		if (!FXSlotFilter(pUIMenu, Event, nValue + increment))
			break;

		increment += rParam.Increment;
	}

	pUIMenu->m_nCurrentSelection += increment;

	pUIMenu->m_pMiniDexed->SetFXParameter(Param, pUIMenu->m_nCurrentSelection, nFX);
}

bool CUIMenu::FXSlotFilter(CUIMenu *pUIMenu, TMenuEvent Event, int nValue)
{
	assert(pUIMenu);
	FX::Parameter Param = (FX::Parameter)pUIMenu->m_nCurrentParameter;

	int nBus = pUIMenu->m_nMenuStackParameter[1];
	int idFX = pUIMenu->m_nMenuStackParameter[2];
	int nFX = idFX + CConfig::BusFXChains * nBus;

	if (nValue == 0) return false;

	if (Param != FX::Parameter::Slot0 && nValue == pUIMenu->m_pMiniDexed->GetFXParameter(FX::Parameter::Slot0, nFX))
		return true;

	if (Param != FX::Parameter::Slot1 && nValue == pUIMenu->m_pMiniDexed->GetFXParameter(FX::Parameter::Slot1, nFX))
		return true;

	if (Param != FX::Parameter::Slot2 && nValue == pUIMenu->m_pMiniDexed->GetFXParameter(FX::Parameter::Slot2, nFX))
		return true;

	return false;
}

#endif

void CUIMenu::PerformanceMenu(CUIMenu *pUIMenu, TMenuEvent Event)
{
	bool bPerformanceSelectToLoad = pUIMenu->m_pMiniDexed->GetPerformanceSelectToLoad();
	int nLastPerformance = pUIMenu->m_pMiniDexed->GetLastPerformance();
	int nValue = pUIMenu->m_nSelectedPerformanceID;
	int nStart = nValue;

	int nLastPerformanceBank = pUIMenu->m_pMiniDexed->GetLastPerformanceBank();
	int nBankValue = pUIMenu->m_nSelectedPerformanceBankID;
	int nBankStart = nBankValue;

	if (pUIMenu->m_pMiniDexed->IsValidPerformance(nValue) != true)
	{
		// A bank change has left the selected performance out of sync
		nValue = pUIMenu->m_pMiniDexed->GetActualPerformanceID();
		pUIMenu->m_nSelectedPerformanceID = nValue;
	}
	std::string Value;

	if (Event == MenuEventUpdate || Event == MenuEventUpdateParameter)
	{
		pUIMenu->m_bPerformanceDeleteMode = false;
		// Ensure selected performance matches the actual loaded one
		pUIMenu->m_nSelectedPerformanceID = pUIMenu->m_pMiniDexed->GetActualPerformanceID();
	}

	if (pUIMenu->m_bSplashShow)
	{
		return;
	}

	if (!pUIMenu->m_bPerformanceDeleteMode)
	{
		switch (Event)
		{
		case MenuEventUpdate:
			break;

		case MenuEventStepDown:
			do
			{
				if (nValue == 0)
				{
					// Wrap around
					nValue = nLastPerformance;
				}
				else if (nValue > 0)
				{
					--nValue;
				}
			} while ((pUIMenu->m_pMiniDexed->IsValidPerformance(nValue) != true) && (nValue != nStart));
			pUIMenu->m_nSelectedPerformanceID = nValue;
			if (!bPerformanceSelectToLoad && pUIMenu->m_nCurrentParameter == 0)
			{
				pUIMenu->m_pMiniDexed->SetNewPerformance(nValue);
			}
			break;

		case MenuEventStepUp:
			do
			{
				if (nValue == nLastPerformance)
				{
					// Wrap around
					nValue = 0;
				}
				else if (nValue < nLastPerformance)
				{
					++nValue;
				}
			} while ((pUIMenu->m_pMiniDexed->IsValidPerformance(nValue) != true) && (nValue != nStart));
			pUIMenu->m_nSelectedPerformanceID = nValue;
			if (!bPerformanceSelectToLoad && pUIMenu->m_nCurrentParameter == 0)
			{
				pUIMenu->m_pMiniDexed->SetNewPerformance(nValue);
			}
			break;

		case MenuEventPressAndStepDown:
			do
			{
				if (nBankValue == 0)
				{
					// Wrap around
					nBankValue = nLastPerformanceBank;
				}
				else if (nBankValue > 0)
				{
					--nBankValue;
				}
			} while ((pUIMenu->m_pMiniDexed->IsValidPerformanceBank(nBankValue) != true) && (nBankValue != nBankStart));
			pUIMenu->m_nSelectedPerformanceBankID = nBankValue;
			pUIMenu->m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nBankValue);
			pUIMenu->m_pMiniDexed->SetFirstPerformance();
			break;

		case MenuEventPressAndStepUp:
			do
			{
				if (nBankValue == nLastPerformanceBank)
				{
					// Wrap around
					nBankValue = 0;
				}
				else if (nBankValue < nLastPerformanceBank)
				{
					++nBankValue;
				}
			} while ((pUIMenu->m_pMiniDexed->IsValidPerformanceBank(nBankValue) != true) && (nBankValue != nBankStart));
			pUIMenu->m_nSelectedPerformanceBankID = nBankValue;
			pUIMenu->m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nBankValue);
			pUIMenu->m_pMiniDexed->SetFirstPerformance();
			break;

		case MenuEventSelect:
			switch (pUIMenu->m_nCurrentParameter)
			{
			case 0:
				if (bPerformanceSelectToLoad)
				{
					pUIMenu->m_pMiniDexed->SetNewPerformance(nValue);
				}

				break;
			case 1:
				if (pUIMenu->m_pMiniDexed->IsValidPerformance(pUIMenu->m_nSelectedPerformanceID))
				{
					pUIMenu->m_bPerformanceDeleteMode = true;
					pUIMenu->m_bConfirmDeletePerformance = false;
				}
				break;
			default:
				break;
			}
			break;
		default:
			return;
		}
	}
	else
	{
		switch (Event)
		{
		case MenuEventUpdate:
			break;

		case MenuEventStepDown:
			pUIMenu->m_bConfirmDeletePerformance = false;
			break;

		case MenuEventStepUp:
			pUIMenu->m_bConfirmDeletePerformance = true;
			break;

		case MenuEventSelect:
			pUIMenu->m_bPerformanceDeleteMode = false;
			if (pUIMenu->m_bConfirmDeletePerformance)
			{
				pUIMenu->m_nSelectedPerformanceID = 0;
				pUIMenu->m_bConfirmDeletePerformance = false;
				pUIMenu->m_pUI->DisplayWrite("", "Delete", pUIMenu->m_pMiniDexed->DeletePerformance(nValue) ? "Completed" : "Error", false, false);
				pUIMenu->m_bSplashShow = true;
				CTimer::Get()->StartKernelTimer(MSEC2HZ(1500), TimerHandlerNoBack, 0, pUIMenu);
				return;
			}
			else
			{
				break;
			}

		default:
			return;
		}
	}

	if (!pUIMenu->m_bPerformanceDeleteMode)
	{
		Value = pUIMenu->m_pMiniDexed->GetPerformanceName(nValue);
		int nBankNum = pUIMenu->m_pMiniDexed->GetPerformanceBank();

		std::string nPSelected = "000";
		nPSelected += std::to_string(nBankNum + 1); // Convert to user-facing bank number rather than index
		nPSelected = nPSelected.substr(nPSelected.length() - 3, 3);
		std::string nPPerf = "000";
		nPPerf += std::to_string(nValue + 1); // Convert to user-facing performance number rather than index
		nPPerf = nPPerf.substr(nPPerf.length() - 3, 3);

		nPSelected += ":" + nPPerf;
		if (bPerformanceSelectToLoad && nValue == pUIMenu->m_pMiniDexed->GetActualPerformanceID())
		{
			nPSelected += " [L]";
		}

		pUIMenu->m_pUI->DisplayWrite(pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name, nPSelected.c_str(),
					     Value.c_str(), true, true);
		//						 nValue > 0, nValue < pUIMenu->m_pMiniDexed->GetLastPerformance());
	}
	else
	{
		pUIMenu->m_pUI->DisplayWrite("", "Delete?", pUIMenu->m_bConfirmDeletePerformance ? "Yes" : "No", false, false);
	}
}

void CUIMenu::EditPerformanceBankNumber(CUIMenu *pUIMenu, TMenuEvent Event)
{
	bool bPerformanceSelectToLoad = pUIMenu->m_pMiniDexed->GetPerformanceSelectToLoad();
	int nLastPerformanceBank = pUIMenu->m_pMiniDexed->GetLastPerformanceBank();
	int nValue = pUIMenu->m_nSelectedPerformanceBankID;
	int nStart = nValue;
	std::string Value;

	switch (Event)
	{
	case MenuEventUpdate:
		break;

	case MenuEventStepDown:
		do
		{
			if (nValue == 0)
			{
				// Wrap around
				nValue = nLastPerformanceBank;
			}
			else if (nValue > 0)
			{
				--nValue;
			}
		} while ((pUIMenu->m_pMiniDexed->IsValidPerformanceBank(nValue) != true) && (nValue != nStart));
		pUIMenu->m_nSelectedPerformanceBankID = nValue;
		if (!bPerformanceSelectToLoad)
		{
			// Switch to the new bank and select the first performance voice
			pUIMenu->m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nValue);
			pUIMenu->m_pMiniDexed->SetFirstPerformance();
		}
		break;

	case MenuEventStepUp:
		do
		{
			if (nValue == nLastPerformanceBank)
			{
				// Wrap around
				nValue = 0;
			}
			else if (nValue < nLastPerformanceBank)
			{
				++nValue;
			}
		} while ((pUIMenu->m_pMiniDexed->IsValidPerformanceBank(nValue) != true) && (nValue != nStart));
		pUIMenu->m_nSelectedPerformanceBankID = nValue;
		if (!bPerformanceSelectToLoad)
		{
			pUIMenu->m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nValue);
			pUIMenu->m_pMiniDexed->SetFirstPerformance();
		}
		break;

	case MenuEventSelect:
		if (bPerformanceSelectToLoad)
		{
			pUIMenu->m_pMiniDexed->SetParameter(CMiniDexed::ParameterPerformanceBank, nValue);
			pUIMenu->m_pMiniDexed->SetFirstPerformance();
		}
		break;

	default:
		return;
	}

	Value = pUIMenu->m_pMiniDexed->GetPerformanceConfig()->GetPerformanceBankName(nValue);
	std::string nPSelected = "000";
	nPSelected += std::to_string(nValue + 1); // Convert to user-facing number rather than index
	nPSelected = nPSelected.substr(nPSelected.length() - 3, 3);

	if (bPerformanceSelectToLoad && nValue == pUIMenu->m_pMiniDexed->GetParameter(CMiniDexed::ParameterPerformanceBank))
	{
		nPSelected += " [L]";
	}

	pUIMenu->m_pUI->DisplayWrite(pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name, nPSelected.c_str(),
				     Value.c_str(), true, true);
}

void CUIMenu::InputTxt(CUIMenu *pUIMenu, TMenuEvent Event)
{
	int nTG = 0;
	std::string TG("TG");

	std::string MsgOk;
	std::string NoValidChars;
	unsigned MaxChars;
	std::string MenuTitleR;
	std::string MenuTitleL;
	std::string OkTitleL;
	std::string OkTitleR;

	switch (pUIMenu->m_nCurrentParameter)
	{
	case 1: // save new performance
		NoValidChars = {92, 47, 58, 42, 63, 34, 60, 62, 124};
		MaxChars = 14;
		MenuTitleL = "Performance Name";
		MenuTitleR = "";
		OkTitleL = "New Performance"; // \E[?25l
		OkTitleR = "";
		break;

	case 2: // Rename performance - NOT Implemented yet
		NoValidChars = {92, 47, 58, 42, 63, 34, 60, 62, 124};
		MaxChars = 14;
		MenuTitleL = "Performance Name";
		MenuTitleR = "";
		OkTitleL = "Rename Perf."; // \E[?25l
		OkTitleR = "";
		break;

	case 3: // Voice name
		nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 2];
		NoValidChars = {127};
		MaxChars = 10;
		MenuTitleL = "Name";
		TG += std::to_string(nTG + 1);
		MenuTitleR = TG;
		OkTitleL = "";
		OkTitleR = "";
		break;

	default:
		return;
	}

	bool bOK;
	unsigned nPosition = pUIMenu->m_InputTextPosition;
	char nChar = pUIMenu->m_InputText[nPosition];

	switch (Event)
	{
	case MenuEventUpdate:
		if (pUIMenu->m_nCurrentParameter == 1 || pUIMenu->m_nCurrentParameter == 2)
		{
			pUIMenu->m_InputText = pUIMenu->m_pMiniDexed->GetNewPerformanceDefaultName();
			pUIMenu->m_InputText += "              ";
			pUIMenu->m_InputText = pUIMenu->m_InputText.substr(0, 14);
			pUIMenu->m_InputTextPosition = 0;
			nPosition = pUIMenu->m_InputTextPosition;
			nChar = pUIMenu->m_InputText[nPosition];
		}
		else
		{

			pUIMenu->m_InputText = pUIMenu->m_pMiniDexed->GetVoiceName(nTG);
			pUIMenu->m_InputText += "          ";
			pUIMenu->m_InputText = pUIMenu->m_InputText.substr(0, 10);
			pUIMenu->m_InputTextPosition = 0;
			nPosition = pUIMenu->m_InputTextPosition;
			nChar = pUIMenu->m_InputText[nPosition];
		}
		break;

	case MenuEventStepDown:
		if (nChar > 32)
		{
			do
			{
				--nChar;
			} while (NoValidChars.find(nChar) != std::string::npos);
		}
		pUIMenu->m_InputTextChar = nChar;
		break;

	case MenuEventStepUp:
		if (nChar < 126)
		{
			do
			{
				++nChar;
			} while (NoValidChars.find(nChar) != std::string::npos);
		}
		pUIMenu->m_InputTextChar = nChar;
		break;

	case MenuEventSelect:
		if (pUIMenu->m_nCurrentParameter == 1)
		{
			pUIMenu->m_pMiniDexed->SetNewPerformanceName(pUIMenu->m_InputText);
			bOK = pUIMenu->m_pMiniDexed->SavePerformanceNewFile();
			MsgOk = bOK ? "Completed" : "Error";
			pUIMenu->m_pUI->DisplayWrite(OkTitleR.c_str(), OkTitleL.c_str(), MsgOk.c_str(), false, false);
			CTimer::Get()->StartKernelTimer(MSEC2HZ(1500), TimerHandler, 0, pUIMenu);
			return;
		}
		else
		{
			break; // Voice Name Edit
		}

	case MenuEventPressAndStepDown:
		if (nPosition > 0)
		{
			--nPosition;
		}
		pUIMenu->m_InputTextPosition = nPosition;
		nChar = pUIMenu->m_InputText[nPosition];
		break;

	case MenuEventPressAndStepUp:
		if (nPosition < MaxChars - 1)
		{
			++nPosition;
		}
		pUIMenu->m_InputTextPosition = nPosition;
		nChar = pUIMenu->m_InputText[nPosition];
		break;

	default:
		return;
	}

	// \E[2;%dH	Cursor move to row %1 and column %2 (starting at 1)
	// \E[?25h	Normal cursor visible
	// \E[?25l	Cursor invisible

	std::string escCursor = "\E[?25h\E[2;"; // this is to locate cursor
	escCursor += std::to_string(nPosition + 2);
	escCursor += "H";

	std::string Value = pUIMenu->m_InputText;
	Value[nPosition] = nChar;
	pUIMenu->m_InputText = Value;

	if (pUIMenu->m_nCurrentParameter == 3)
	{
		pUIMenu->m_pMiniDexed->SetVoiceName(pUIMenu->m_InputText, nTG);
	}

	Value = Value + " " + escCursor;
	pUIMenu->m_pUI->DisplayWrite(MenuTitleR.c_str(), MenuTitleL.c_str(), Value.c_str(), false, false);
}

void CUIMenu::EditTGParameterModulation(CUIMenu *pUIMenu, TMenuEvent Event)
{

	int nTG = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 3];
	int nController = pUIMenu->m_nMenuStackParameter[pUIMenu->m_nCurrentMenuDepth - 1];
	int nParameter = pUIMenu->m_nCurrentParameter + nController;

	CMiniDexed::TTGParameter Param = (CMiniDexed::TTGParameter)nParameter;
	const TParameter &rParam = s_TGParameter[Param];

	int nValue = pUIMenu->m_pMiniDexed->GetTGParameter(Param, nTG);
	int nTGLink = pUIMenu->m_pMiniDexed->GetTGParameter(CMiniDexed::TGParameterTGLink, nTG);

	switch (Event)
	{
	case MenuEventUpdate:
		break;

	case MenuEventStepDown:
		nValue -= rParam.Increment;
		if (nValue < rParam.Minimum)
		{
			nValue = rParam.Minimum;
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nValue, nTG);
		break;

	case MenuEventStepUp:
		nValue += rParam.Increment;
		if (nValue > rParam.Maximum)
		{
			nValue = rParam.Maximum;
		}
		pUIMenu->m_pMiniDexed->SetTGParameter(Param, nValue, nTG);
		break;

	case MenuEventPressAndStepDown:
	case MenuEventPressAndStepUp:
		pUIMenu->TGShortcutHandler(Event);
		return;

	default:
		return;
	}

	std::string TG("TG");
	TG += std::to_string(nTG + 1);
	if (nTGLink) TG += ToTGLinkName(nTGLink, 0);

	std::string Value = GetTGValueString(Param,
					     pUIMenu->m_pMiniDexed->GetTGParameter(Param, nTG),
					     pUIMenu->m_pConfig->GetLCDColumns() - 2);

	pUIMenu->m_pUI->DisplayWrite(TG.c_str(),
				     pUIMenu->m_pParentMenu[pUIMenu->m_nCurrentMenuItem].Name,
				     Value.c_str(),
				     nValue > rParam.Minimum, nValue < rParam.Maximum);
}
