#include <cassert>

#include "effect.h"
#include "effect_cloudseed2.h"
#include "midi.h"

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

static std::string ToHz (int nValue, int nWidth)
{
	uint16_t hz = MIDI_EQ_HZ[nValue];
	char buf[20] = {};

	if (hz < 1000)
		return std::to_string (hz) + " Hz";

	std::snprintf (buf, sizeof(buf), "%.1f kHz", hz/1000.0);
	return buf;
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

FX::FXParameterType FX::s_FXParameter[FX::FXParameterUnknown] =
{
	{0,	FX::effects_num - 1,	0,	1,	"Slot1",	ToEffectName, FX::FXSaveAsString},
	{0,	FX::effects_num - 1,	0,	1,	"Slot2",	ToEffectName, FX::FXSaveAsString},
	{0,	FX::effects_num - 1,	0,	1,	"Slot3",	ToEffectName, FX::FXSaveAsString},
	{0,	100,	0,	1,	"YKChorusMix",	ToDryWet},
	{0,	1,	1,	1,	"YKChorusEnable1",	ToOnOff},
	{0,	1,	1,	1,	"YKChorusEnable2",	ToOnOff},
	{0,	100,	50,	1,	"YKChorusLFORate1"},
	{0,	100,	83,	1,	"YKChorusLFORate2"},
	{0,	100,	0,	1,	"DreamDelayMix",	ToDryWet},
	{0,	2,	0,	1,	"DreamDelayMode",	ToDelayMode},
	{0,	112,	36,	1,	"DreamDelayTime",	ToDelayTime, FX::FXComposite},
	{0,	112,	36,	1,	"DreamDelayTimeL",	ToDelayTime},
	{0,	112,	36,	1,	"DreamDelayTimeR",	ToDelayTime},
	{30,	240,	120,	1,	"DreamDelayTempo",	ToBPM},
	{0,	100,	60,	1,	"DreamDelayFeedback"},
	{0,	60,	50,	1,	"DreamDelayHighCut",	ToHz},
	{0,	100,	0,	1,	"PlateReverbMix",	ToDryWet},
	{0,	99,	50,	1,	"PlateReverbSize"},
	{0,	99,	25,	1,	"PlateReverbHighDamp"},
	{0,	99,	25,	1,	"PlateReverbLowDamp"},
	{0,	99,	85,	1,	"PlateReverbLowPass"},
	{0,	99,	65,	1,	"PlateReverbDiffusion"},
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
	{0,	99,	0,	1,	"ReturnLevel"},
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
		case FX::FXParameterCloudSeed2Preset:
			return AudioEffectCloudSeed2::getPresetNameChar(nID);
		default:
			assert(false);
	}
	return 0;
}
