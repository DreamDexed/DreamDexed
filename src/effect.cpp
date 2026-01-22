#include <cassert>

#include "effect.h"
#include "midi.h"
#include "effect_cloudseed2.h"
#include "effect_compressor.h"
#include "zyn/APhaser.h"
#include "zyn/Chorus.h"
#include "zyn/Sympathetic.h"
#include "zyn/Distortion.h"
#include "zyn/Phaser.h"
#include "zyn/EffectLFO.h"

static std::string ToOnOff (int nValue, int nWidth)
{
	static const char *OnOff[] = {"Off", "On"};

	assert ((unsigned) nValue < sizeof OnOff / sizeof OnOff[0]);

	return OnOff[nValue];
}

static std::string ToDelayMode (int nValue, int nWidth)
{
	static const char *Mode[] = {"Dual", "Crossover", "PingPong"};

	assert ((unsigned) nValue < sizeof Mode / sizeof Mode[0]);

	return Mode[nValue];
}

static std::string ToDelayTime (int nValue, int nWidth)
{
	static const char *Sync[] = {"1/1", "1/1T", "1/2", "1/2T", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T"};

	assert (nValue >= 0 && nValue <= 112);

	if (nValue <= 100)
		return std::to_string(nValue * 10) + " ms";

	return Sync[nValue - 100 - 1];
}

static std::string ToBPM(int nValue, int nWidth)
{
	return std::to_string(nValue) + " BPM";
}

static const char MIDINoteName[128][9] =
{
"0   C-2", "1   C#-2", "2   D-2", "3   D#-2", "4   E-2", "5   F-2", "6   F#-2", "7   G-2", "8   G#-2", "9   A-2", "10  A#-2", "11  B-2",
"12  C-1", "13  C#-1", "14  D-1", "15  D#-1", "16  E-1", "17  F-1", "18  F#-1", "19  G-1", "20  G#-1", "21  A-1", "22  A#-1", "23  B-1",
"24  C0",  "25  C#0",  "26  D0",  "27  D#0",  "28  E0",  "29  F0",  "30  F#0",  "31  G0",  "32  G#0",  "33  A0",  "34  A#0",  "35  B0",
"36  C1",  "37  C#1",  "38  D1",  "39  D#1",  "40  E1",  "41  F1",  "42  F#1",  "43  G1",  "44  G#1",  "45  A1",  "46  A#1",  "47  B1",
"48  C2",  "49  C#2",  "50  D2",  "51  D#2",  "52  E2",  "53  F2",  "54  F#2",  "55  G2",  "56  G#2",  "57  A2",  "58  A#2",  "59  B2",
"60  C3",  "61  C#3",  "62  D3",  "63  D#3",  "64  E3",  "65  F3",  "66  F#3",  "67  G3",  "68  G#3",  "69  A3",  "70  A#3",  "71  B3",
"72  C4",  "73  C#4",  "74  D4",  "75  D#4",  "76  E4",  "77  F4",  "78  F#4",  "79  G4",  "80  G#4",  "81  A4",  "82  A#4",  "83  B4",
"84  C5",  "85  C#5",  "86  D5",  "87  D#5",  "88  E5",  "89  F5",  "90  F#5",  "91  G5",  "92  G#5",  "93  A5",  "94  A#5",  "95  B5",
"96  C6",  "97  C#6",  "98  D6",  "99  D#6",  "100 E6",  "101 F6",  "102 F#6",  "103 G6",  "104 G#6",  "105 A6",  "106 A#6",  "107 B6",
"108 C7",  "109 C#7",  "110 D7",  "111 D#7",  "112 E7",  "113 F7",  "114 F#7",  "115 G7",  "116 G#7",  "117 A7",  "118 A#7",  "119 B7",
"120 C8",  "121 C#8",  "122 D8",  "123 D#8",  "124 E8",  "125 F8",  "126 F#8",  "127 G8"
};

static std::string ToMIDINote (int nValue, int nWidth)
{
	assert ((unsigned) nValue < sizeof MIDINoteName / sizeof *MIDINoteName);

	return MIDINoteName[nValue];
}

static std::string ToHz (int nValue, int nWidth)
{
	uint16_t hz = MIDI_EQ_HZ[nValue];
	char buf[20] = {};

	if (hz < 1000)
		return std::to_string (hz) + " Hz";

	std::snprintf (buf, sizeof(buf), "%.1f kHz", hz/1000.0);
	return buf;
}

static std::string ToSemitones(int nValue, int nWidth)
{
	return std::to_string(nValue) + " semitone" + (nValue > 1 ? "s" : "");
}

static std::string ToDryWet (int nValue, int nWidth)
{
	unsigned dry, wet;
	if (nValue <= 50)
	{
		dry = 100;
		wet = nValue * 2;
	}
	else
	{
		dry = 100 - ((nValue - 50) * 2);
		wet = 100;
	}

	return std::to_string (dry) + ":" + std::to_string(wet) + (wet == 0 ? " Off" : "");
}

static std::string ToEffectName (int nValue, int nWidth)
{
	assert (nValue >= 0 && nValue < FX::effects_num);
	return FX::s_effects[nValue].Name;
}

static std::string TodB (int nValue, int nWidth)
{
	return std::to_string (nValue) + " dB";
}

static std::string TodBFS (int nValue, int nWidth)
{
	return std::to_string (nValue) + " dBFS";
}

static std::string ToMillisec (int nValue, int nWidth)
{
	return std::to_string (nValue) + " ms";
}

static std::string ToRatio (int nValue, int nWidth)
{
	if (nValue == AudioEffectCompressor::CompressorRatioInf)
		return "INF:1";

	return std::to_string (nValue) + ":1";
}

static std::string ToPan (int nValue, int nWidth)
{
	char buf[nWidth + 1];
	unsigned nBarWidth = nWidth - 3;
	unsigned nIndex = std::min(nValue * nBarWidth / 127, nBarWidth - 1);
	std::fill_n(buf, nBarWidth, '.');
	buf[nBarWidth / 2] = ':';
	buf[nIndex] = 0xFF; // 0xFF is the block character
	snprintf(buf + nBarWidth, 4, "%3d", nValue);
	return buf;
}

static std::string ToLRDelay (int nValue, int nWidth)
{
	nValue -= 64;
	if (nValue < 0) return std::to_string(nValue) + " L";
	else if (nValue == 0) return " 0 Center";
	else return std::string("+") + std::to_string(nValue) + " R";
}

static std::string ToCenter64 (int nValue, int nWidth)
{
	nValue -= 64;
	if (nValue > 0) return std::string("+") + std::to_string(nValue);
	else if (nValue == 0) return " 0";
	else return std::to_string(nValue);
}

static std::string ToPrePost (int nValue, int nWidth)
{
	static const char *PrePost[] = {"Pre", "Post"};

	assert ((unsigned) nValue < sizeof PrePost / sizeof *PrePost);

	return PrePost[nValue];
}

FX::FXParameterType FX::s_FXParameter[FX::FXParameterUnknown] =
{
	{0,	FX::effects_num - 1,	0,	1,	"Slot1",	ToEffectName, FX::FXSaveAsString},
	{0,	FX::effects_num - 1,	0,	1,	"Slot2",	ToEffectName, FX::FXSaveAsString},
	{0,	FX::effects_num - 1,	0,	1,	"Slot3",	ToEffectName, FX::FXSaveAsString},
	{0,	zyn::Distortion::presets_num - 1,	0,	1,	"ZynDistortionPreset", zyn::Distortion::ToPresetName, FX::FXComposite | FX::FXSaveAsString},
	{0,	100,	0,	1,	"ZynDistortionMix",		ToDryWet},
	{0,	127,	0,	1,	"ZynDistortionPanning",		ToPan},
	{0,	127,	0,	1,	"ZynDistortionDrive"},
	{0,	127,	0,	1,	"ZynDistortionLevel"},
	{0,	16,	0,	1,	"ZynDistortionType",		zyn::Distortion::ToDistortionType},
	{0,	1,	0,	1,	"ZynDistortionNegate",		ToOnOff},
	{0,	1,	1,	1,	"ZynDistortionFiltering",	ToPrePost},
	{0,	60,	0,	1,	"ZynDistortionLowcut",		ToHz},
	{0,	60,	60,	1,	"ZynDistortionHighcut",		ToHz},
	{0,	127,	0,	1,	"ZynDistortionStereo",		ToOnOff},
	{0,	127,	0,	1,	"ZynDistortionLRCross"},
	{0,	127,	0,	1,	"ZynDistortionShape"},
	{0,	127,	0,	1,	"ZynDistortionOffset",		ToCenter64},
	{0,	1,	0,	1,	"ZynDistortionBypass",		ToOnOff},
	{0,	100,	0,	1,	"YKChorusMix",	ToDryWet},
	{0,	1,	1,	1,	"YKChorusEnable1",	ToOnOff},
	{0,	1,	1,	1,	"YKChorusEnable2",	ToOnOff},
	{0,	100,	50,	1,	"YKChorusLFORate1"},
	{0,	100,	83,	1,	"YKChorusLFORate2"},
	{0,	1,	0,	1,	"YKChorusBypass",	ToOnOff},
	{0,	zyn::Chorus::presets_num -1,	0,	1,	"ZynChorusPreset", zyn::Chorus::ToPresetName, FX::FXComposite | FX::FXSaveAsString},
	{0,	100,	0,	1,	"ZynChorusMix",		ToDryWet},
	{0,	127,	64,	1,	"ZynChorusPanning",	ToPan},
	{1,	600,	14,	1,	"ZynChorusLFOFreq"},
	{0,	127,	0,	1,	"ZynChorusLFORandomness"},
	{0,	1,	1,	1,	"ZynChorusLFOType",	zyn::EffectLFO::ToLFOType},
	{0,	127,	64,	1,	"ZynChorusLFOLRDelay",	ToLRDelay},
	{0,	127,	40,	1,	"ZynChorusDepth"},
	{0,	127,	85,	1,	"ZynChorusDelay"},
	{0,	127,	64,	1,	"ZynChorusFeedback",	ToCenter64},
	{0,	127,	0,	1,	"ZynChorusLRCross"},
	{0,	3,	0,	1,	"ZynChorusMode",	zyn::Chorus::ToChorusMode},
	{0,	1,	0,	1,	"ZynChorusSubtractive",	ToOnOff},
	{0,	1,	0,	1,	"ZynChorusBypass",	ToOnOff},
	{0,	zyn::Sympathetic::presets_num -1,	0,	1,	"ZynSympatheticPreset", zyn::Sympathetic::ToPresetName, FX::FXComposite | FX::FXSaveAsString},
	{0,	100,	0,	1,	"ZynSympatheticMix",		ToDryWet},
	{0,	127,	64,	1,	"ZynSympatheticPanning",	ToPan},
	{0,	1,	0,	1,	"ZynSympatheticUseSustain",	ToOnOff},
	{0,	127,	125,	1,	"ZynSympatheticQ"},
	{0,	127,	5,	1,	"ZynSympatheticDrive"},
	{0,	127,	80,	1,	"ZynSympatheticLevel"},
	{0,	zyn::Sympathetic::types_num - 1,	0,	1,	"ZynSympatheticType",	zyn::Sympathetic::ToTypeName},
	{1,	3,	1,	1,	"ZynSympatheticUnisonSize"},
	{0,	127,	10,	1,	"ZynSympatheticUnisonSpread"},
	{0,	76,	12,	1,	"ZynSympatheticStrings"},
	{1,	10,	1,	1,	"ZynSympatheticInterval",	ToSemitones},
	{0,	127,	57,	1,	"ZynSympatheticBaseNote",	ToMIDINote},
	{0,	60,	0,	1,	"ZynSympatheticLowcut",	ToHz},
	{0,	60,	60,	1,	"ZynSympatheticHighcut",	ToHz},
	{0,	1,	0,	1,	"ZynSympatheticNegate",	ToOnOff},
	{0,	1,	0,	1,	"ZynSympatheticBypass",	ToOnOff},
	{0,	zyn::APhaser::presets_num - 1,	0,	1,	"ZynAPhaserPreset", zyn::APhaser::ToPresetName, FX::FXComposite | FX::FXSaveAsString},
	{0,	100,	0,	1,	"ZynAPhaserMix",	ToDryWet},
	{0,	127,	64,	1,	"ZynAPhaserPanning",	ToPan},
	{1,	600,	14,	1,	"ZynAPhaserLFOFreq"},
	{0,	127,	0,	1,	"ZynAPhaserLFORandomness"},
	{0,	1,	1,	1,	"ZynAPhaserLFOType",	zyn::EffectLFO::ToLFOType},
	{0,	127,	64,	1,	"ZynAPhaserLFOLRDelay",	ToLRDelay},
	{0,	127,	64,	1,	"ZynAPhaserDepth",	ToCenter64},
	{0,	127,	40,	1,	"ZynAPhaserFeedback",	ToCenter64},
	{1,	12,	4,	1,	"ZynAPhaserStages"},
	{0,	127,	0,	1,	"ZynAPhaserLRCross"},
	{0,	1,	0,	1,	"ZynAPhaserSubtractive",ToOnOff},
	{0,	127,	110,	1,	"ZynAPhaserWidth"},
	{0,	100,	20,	1,	"ZynAPhaserDistortion"},
	{0,	127,	10,	1,	"ZynAPhaserMismatch"},
	{0,	1,	1,	1,	"ZynAPhaserHyper",	ToOnOff},
	{0,	1,	0,	1,	"ZynAPhaserBypass",	ToOnOff},
	{0,	zyn::Phaser::presets_num - 1,	0,	1,	"ZynPhaserPreset", zyn::Phaser::ToPresetName, FX::FXComposite | FX::FXSaveAsString},
	{0,	100,	0,	1,	"ZynPhaserMix",		ToDryWet},
	{0,	127,	64,	1,	"ZynPhaserPanning",	ToPan},
	{1,	600,	11,	1,	"ZynPhaserLFOFreq"},
	{0,	127,	0,	1,	"ZynPhaserLFORandomness"},
	{0,	1,	0,	1,	"ZynPhaserLFOType",	zyn::EffectLFO::ToLFOType},
	{0,	127,	64,	1,	"ZynPhaserLFOLRDelay",	ToLRDelay},
	{0,	127,	110,	1,	"ZynPhaserDepth"},
	{0,	127,	64,	1,	"ZynPhaserFeedback",	ToCenter64},
	{1,	12,	1,	1,	"ZynPhaserStages"},
	{0,	127,	0,	1,	"ZynPhaserLRCross"},
	{0,	1,	0,	1,	"ZynPhaserSubtractive",	ToOnOff},
	{0,	127,	20,	1,	"ZynPhaserPhase"},
	{0,	1,	0,	1,	"ZynPhaserBypass",	ToOnOff},
	{0,	100,	0,	1,	"DreamDelayMix",	ToDryWet},
	{0,	2,	0,	1,	"DreamDelayMode",	ToDelayMode},
	{0,	112,	36,	1,	"DreamDelayTime",	ToDelayTime, FX::FXComposite},
	{0,	112,	36,	1,	"DreamDelayTimeL",	ToDelayTime},
	{0,	112,	36,	1,	"DreamDelayTimeR",	ToDelayTime},
	{30,	240,	120,	1,	"DreamDelayTempo",	ToBPM},
	{0,	100,	60,	1,	"DreamDelayFeedback"},
	{0,	60,	50,	1,	"DreamDelayHighCut",	ToHz},
	{0,	1,	0,	1,	"DreamDelayBypass",	ToOnOff},
	{0,	100,	0,	1,	"PlateReverbMix",	ToDryWet},
	{0,	99,	50,	1,	"PlateReverbSize"},
	{0,	99,	25,	1,	"PlateReverbHighDamp"},
	{0,	99,	25,	1,	"PlateReverbLowDamp"},
	{0,	99,	85,	1,	"PlateReverbLowPass"},
	{0,	99,	65,	1,	"PlateReverbDiffusion"},
	{0,	1,	0,	1,	"PlateReverbBypass",	ToOnOff},
	{0,	AudioEffectCloudSeed2::presets_num - 1,	0,	1,	"CloudSeed2Preset",	AudioEffectCloudSeed2::getPresetName, FX::FXComposite | FX::FXSaveAsString},
	{0,	1,	0,	1,	"CloudSeed2Interpolation", ToOnOff},
	{0,	1,	0,	1,	"CloudSeed2LowCutEnabled", ToOnOff},
	{0,	1,	0,	1,	"CloudSeed2HighCutEnabled", ToOnOff},
	{0,	127,	0,	1,	"CloudSeed2InputMix"},
	{0,	127,	0,	1,	"CloudSeed2LowCut"},
	{0,	127,	127,	1,	"CloudSeed2HighCut"},
	{0,	127,	127,	1,	"CloudSeed2DryOut"},
	{0,	127,	0,	1,	"CloudSeed2EarlyOut"},
	{0,	127,	0,	1,	"CloudSeed2LateOut"},
	{0,	1,	0,	1,	"CloudSeed2TapEnabled", ToOnOff},
	{0,	127,	64,	1,	"CloudSeed2TapCount"},
	{0,	127,	127,	1,	"CloudSeed2TapDecay"},
	{0,	127,	0,	1,	"CloudSeed2TapPredelay"},
	{0,	127,	62,	1,	"CloudSeed2TapLength"},
	{0,	1,	0,	1,	"CloudSeed2EarlyDiffuseEnabled", ToOnOff},
	{1,	12,	4,	1,	"CloudSeed2EarlyDiffuseCount"},
	{0,	127,	18,	1,	"CloudSeed2EarlyDiffuseDelay"},
	{0,	127,	19,	1,	"CloudSeed2EarlyDiffuseModAmount"},
	{0,	127,	89,	1,	"CloudSeed2EarlyDiffuseFeedback"},
	{0,	127,	20,	1,	"CloudSeed2EarlyDiffuseModRate"},
	{0,	1,	1,	1,	"CloudSeed2LateMode", AudioEffectCloudSeed2::GetLateMode},
	{1,	12,	6,	1,	"CloudSeed2LateLineCount"},
	{0,	1,	0,	1,	"CloudSeed2LateDiffuseEnabled", ToOnOff},
	{1,	8,	2,	1,	"CloudSeed2LateDiffuseCount"},
	{0,	127,	64,	1,	"CloudSeed2LateLineSize"},
	{0,	127,	19,	1,	"CloudSeed2LateLineModAmount"},
	{0,	127,	64,	1,	"CloudSeed2LateDiffuseDelay"},
	{0,	127,	20,	1,	"CloudSeed2LateDiffuseModAmount"},
	{0,	127,	62,	1,	"CloudSeed2LateLineDecay"},
	{0,	127,	20,	1,	"CloudSeed2LateLineModRate"},
	{0,	127,	90,	1,	"CloudSeed2LateDiffuseFeedback"},
	{0,	127,	19,	1,	"CloudSeed2LateDiffuseModRate"},
	{0,	1,	0,	1,	"CloudSeed2EqLowShelfEnabled", ToOnOff},
	{0,	1,	0,	1,	"CloudSeed2EqHighShelfEnabled", ToOnOff},
	{0,	1,	0,	1,	"CloudSeed2EqLowpassEnabled", ToOnOff},
	{0,	127,	40,	1,	"CloudSeed2EqLowFreq"},
	{0,	127,	65,	1,	"CloudSeed2EqHighFreq"},
	{0,	127,	104,	1,	"CloudSeed2EqCutoff"},
	{0,	127,	107,	1,	"CloudSeed2EqLowGain"},
	{0,	127,	108,	1,	"CloudSeed2EqHighGain"},
	{0,	127,	0,	1,	"CloudSeed2EqCrossSeed"},
	{0,	127,	62,	1,	"CloudSeed2SeedTap"},
	{0,	127,	6,	1,	"CloudSeed2SeedDiffusion"},
	{0,	127,	12,	1,	"CloudSeed2SeedDelay"},
	{0,	127,	19,	1,	"CloudSeed2SeedPostDiffusion"},
	{0,	1,	0,	1,	"CloudSeed2Bypass",	ToOnOff},
	{-20,	20,	0,	1,	"CompressorPreGain",	TodB},
	{-60,	0,	-20,	1,	"CompressorThresh",	TodBFS},
	{1,	AudioEffectCompressor::CompressorRatioInf,	5,	1,	"CompressorRatio",	ToRatio},
	{0,	1000,	5,	5,	"CompressorAttack",	ToMillisec},
	{0,	2000,	200,	5,	"CompressorRelease",	ToMillisec},
	{-20,	20,	0,	1,	"CompressorMakeupGain",	TodB},
	{0,	1,	0,	1,	"CompressorHPFilterEnable",	ToOnOff},
	{0,	1,	0,	1,	"CompressorBypass",	ToOnOff},
	{-24,	24,	0,	1,	"EQLow",		TodB},
	{-24,	24,	0,	1,	"EQMid",		TodB},
	{-24,	24,	0,	1,	"EQHigh",		TodB},
	{-24,	24,	0,	1,	"EQGain",		TodB},
	{0,	46,	24,	1,	"EQLowMidFreq",		ToHz},
	{28,	59,	44,	1,	"EQMidHighFreq",	ToHz},
	{0,	60,	0,	1,	"EQPreLowCut",		ToHz},
	{0,	60,	60,	1,	"EQPreHighCut",		ToHz},
	{0,	1,	0,	1,	"EQBypass",		ToOnOff},
	{0,	99,	0,	1,	"ReturnLevel"},
	{0,	1,	0,	1,	"Bypass",		ToOnOff},
};

constexpr const FX::EffectType FX::s_effects[];

uint8_t getIDFromEffectName(const char* name)
{
	for(uint8_t i = 0; i < FX::effects_num; ++i)
		if (strcmp(FX::s_effects[i].Name, name) == 0)
			return i;

	return 0;
}

int FX::getIDFromName(TFXParameter param, const char* name)
{
	switch (param)
	{
		case FX::FXParameterSlot0:
		case FX::FXParameterSlot1:
		case FX::FXParameterSlot2:
			return getIDFromEffectName(name);
		case FX::FXParameterZynDistortionPreset:
			return zyn::Distortion::ToIDFromPreset(name);
		case FX::FXParameterZynChorusPreset:
			return zyn::Chorus::ToIDFromPreset(name);
		case FX::FXParameterZynSympatheticPreset:
			return zyn::Sympathetic::ToIDFromPreset(name);
		case FX::FXParameterZynAPhaserPreset:
			return zyn::APhaser::ToIDFromPreset(name);
		case FX::FXParameterZynPhaserPreset:
			return zyn::Phaser::ToIDFromPreset(name);
		case FX::FXParameterCloudSeed2Preset:
			return AudioEffectCloudSeed2::getIDFromPresetName(name);
		default:
			assert(false);
	}
	return 0;
}

const char *FX::getNameFromID(TFXParameter param, int nID)
{
	switch (param)
	{
		case FX::FXParameterSlot0:
		case FX::FXParameterSlot1:
		case FX::FXParameterSlot2:
			assert (nID < FX::effects_num);
			return FX::s_effects[nID].Name;
		case FX::FXParameterZynDistortionPreset:
			return zyn::Distortion::ToPresetNameChar(nID);
		case FX::FXParameterZynChorusPreset:
			return zyn::Chorus::ToPresetNameChar(nID);
		case FX::FXParameterZynSympatheticPreset:
			return zyn::Sympathetic::ToPresetNameChar(nID);
		case FX::FXParameterZynAPhaserPreset:
			return zyn::APhaser::ToPresetNameChar(nID);
		case FX::FXParameterZynPhaserPreset:
			return zyn::Phaser::ToPresetNameChar(nID);
		case FX::FXParameterCloudSeed2Preset:
			return AudioEffectCloudSeed2::getPresetNameChar(nID);
		default:
			assert(false);
	}
	return 0;
}
