#pragma once

#include <string>
#include <cstdint>

class FX
{
public:
	static constexpr int FXComposite = 1 << 0; // updates multiple controls, it shouldn't update the controls at startup and performance load
	static constexpr int FXSaveAsString = 1 << 1; // save this parameter as string in the performace file

	enum TFXParameter
	{
		FXParameterSlot0,
		FXParameterSlot1,
		FXParameterSlot2,
		FXParameterYKChorusMix,
		FXParameterYKChorusEnable1,
		FXParameterYKChorusEnable2,
		FXParameterYKChorusLFORate1,
		FXParameterYKChorusLFORate2,
		FXParameterDreamDelayMix,
		FXParameterDreamDelayMode,
		FXParameterDreamDelayTime,
		FXParameterDreamDelayTimeL,
		FXParameterDreamDelayTimeR,
		FXParameterDreamDelayTempo,
		FXParameterDreamDelayFeedback,
		FXParameterDreamDelayHighCut,
		FXParameterPlateReverbMix,
		FXParameterPlateReverbSize,
		FXParameterPlateReverbHighDamp,
		FXParameterPlateReverbLowDamp,
		FXParameterPlateReverbLowPass,
		FXParameterPlateReverbDiffusion,
		FXParameterCloudSeed2Preset,
		FXParameterCloudSeed2Interpolation,
		FXParameterCloudSeed2LowCutEnabled,
		FXParameterCloudSeed2HighCutEnabled,
		FXParameterCloudSeed2InputMix,
		FXParameterCloudSeed2LowCut,
		FXParameterCloudSeed2HighCut,
		FXParameterCloudSeed2DryOut,
		FXParameterCloudSeed2EarlyOut,
		FXParameterCloudSeed2LateOut,
		FXParameterCloudSeed2TapEnabled,
		FXParameterCloudSeed2TapCount,
		FXParameterCloudSeed2TapDecay,
		FXParameterCloudSeed2TapPredelay,
		FXParameterCloudSeed2TapLength,
		FXParameterCloudSeed2EarlyDiffuseEnabled,
		FXParameterCloudSeed2EarlyDiffuseCount,
		FXParameterCloudSeed2EarlyDiffuseDelay,
		FXParameterCloudSeed2EarlyDiffuseModAmount,
		FXParameterCloudSeed2EarlyDiffuseFeedback,
		FXParameterCloudSeed2EarlyDiffuseModRate,
		FXParameterCloudSeed2LateMode,
		FXParameterCloudSeed2LateLineCount,
		FXParameterCloudSeed2LateDiffuseEnabled,
		FXParameterCloudSeed2LateDiffuseCount,
		FXParameterCloudSeed2LateLineSize,
		FXParameterCloudSeed2LateLineModAmount,
		FXParameterCloudSeed2LateDiffuseDelay,
		FXParameterCloudSeed2LateDiffuseModAmount,
		FXParameterCloudSeed2LateLineDecay,
		FXParameterCloudSeed2LateLineModRate,
		FXParameterCloudSeed2LateDiffuseFeedback,
		FXParameterCloudSeed2LateDiffuseModRate,
		FXParameterCloudSeed2EqLowShelfEnabled,
		FXParameterCloudSeed2EqHighShelfEnabled,
		FXParameterCloudSeed2EqLowpassEnabled,
		FXParameterCloudSeed2EqLowFreq,
		FXParameterCloudSeed2EqHighFreq,
		FXParameterCloudSeed2EqCutoff,
		FXParameterCloudSeed2EqLowGain,
		FXParameterCloudSeed2EqHighGain,
		FXParameterCloudSeed2EqCrossSeed,
		FXParameterCloudSeed2SeedTap,
		FXParameterCloudSeed2SeedDiffusion,
		FXParameterCloudSeed2SeedDelay,
		FXParameterCloudSeed2SeedPostDiffusion,
		FXParameterCompressorPreGain,
		FXParameterCompressorThresh,
		FXParameterCompressorRatio,
		FXParameterCompressorAttack,
		FXParameterCompressorRelease,
		FXParameterCompressorMakeupGain,
		FXParameterCompressorHPFilterEnable,
		FXParameterCompressorBypass,
		FXParameterReturnLevel,
		FXParameterUnknown,
	};

	typedef std::string TToString (int nValue, int nWidth);

	struct FXParameterType
	{
		int Minimum;
		int Maximum;
		int Default;
		int Increment;
		const char *Name;
		TToString *ToString;
		int Flags;
	};

	static FX::FXParameterType s_FXParameter[];

	struct EffectType
	{
		const char *Name;
		const unsigned MinID;
		const unsigned MaxID;
	};

        static constexpr const EffectType s_effects[] = {
                {"None"},
                {"YKChorus", FXParameterYKChorusMix, FXParameterYKChorusLFORate2},
                {"DreamDelay", FXParameterDreamDelayMix, FXParameterDreamDelayHighCut},
                {"PlateReverb", FXParameterPlateReverbMix, FXParameterPlateReverbDiffusion},
                {"CloudSeed2", FXParameterCloudSeed2Preset, FXParameterCloudSeed2SeedPostDiffusion},
		{"Compressor", FXParameterCompressorPreGain, FXParameterCompressorBypass},
        };
        static constexpr uint8_t effects_num = sizeof s_effects / sizeof *s_effects;
        static constexpr uint8_t slots_num = 3;

	static const char *getNameFromID(TFXParameter param, int nID);
	static int getIDFromName(TFXParameter param, const char* name);
};
