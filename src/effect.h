#pragma once

#include <string>

class FX
{
public:
	enum Flag
	{
		Composite = 1 << 0, // updates multiple controls, it shouldn't update the controls at startup and performance load
		SaveAsString = 1 << 1, // save this parameter as string in the performace file
	};

	enum Parameter
	{
		Slot0,
		Slot1,
		Slot2,
		ZynDistortionPreset,
		ZynDistortionMix,
		ZynDistortionPanning,
		ZynDistortionDrive,
		ZynDistortionLevel,
		ZynDistortionType,
		ZynDistortionNegate,
		ZynDistortionFiltering,
		ZynDistortionLowcut,
		ZynDistortionHighcut,
		ZynDistortionStereo,
		ZynDistortionLRCross,
		ZynDistortionShape,
		ZynDistortionOffset,
		ZynDistortionBypass,
		YKChorusMix,
		YKChorusEnable1,
		YKChorusEnable2,
		YKChorusLFORate1,
		YKChorusLFORate2,
		YKChorusBypass,
		ZynChorusPreset,
		ZynChorusMix,
		ZynChorusPanning,
		ZynChorusLFOFreq,
		ZynChorusLFORandomness,
		ZynChorusLFOType,
		ZynChorusLFOLRDelay,
		ZynChorusDepth,
		ZynChorusDelay,
		ZynChorusFeedback,
		ZynChorusLRCross,
		ZynChorusMode,
		ZynChorusSubtractive,
		ZynChorusBypass,
		ZynSympatheticPreset,
		ZynSympatheticMix,
		ZynSympatheticPanning,
		ZynSympatheticQ,
		ZynSympatheticQSustain,
		ZynSympatheticDrive,
		ZynSympatheticLevel,
		ZynSympatheticType,
		ZynSympatheticUnisonSize,
		ZynSympatheticUnisonSpread,
		ZynSympatheticStrings,
		ZynSympatheticInterval,
		ZynSympatheticBaseNote,
		ZynSympatheticLowcut,
		ZynSympatheticHighcut,
		ZynSympatheticNegate,
		ZynSympatheticBypass,
		ZynAPhaserPreset,
		ZynAPhaserMix,
		ZynAPhaserPanning,
		ZynAPhaserLFOFreq,
		ZynAPhaserLFORandomness,
		ZynAPhaserLFOType,
		ZynAPhaserLFOLRDelay,
		ZynAPhaserDepth,
		ZynAPhaserFeedback,
		ZynAPhaserStages,
		ZynAPhaserLRCross,
		ZynAPhaserSubtractive,
		ZynAPhaserWidth,
		ZynAPhaserDistortion,
		ZynAPhaserMismatch,
		ZynAPhaserHyper,
		ZynAPhaserBypass,
		ZynPhaserPreset,
		ZynPhaserMix,
		ZynPhaserPanning,
		ZynPhaserLFOFreq,
		ZynPhaserLFORandomness,
		ZynPhaserLFOType,
		ZynPhaserLFOLRDelay,
		ZynPhaserDepth,
		ZynPhaserFeedback,
		ZynPhaserStages,
		ZynPhaserLRCross,
		ZynPhaserSubtractive,
		ZynPhaserPhase,
		ZynPhaserBypass,
		DreamDelayMix,
		DreamDelayMode,
		DreamDelayTime,
		DreamDelayTimeL,
		DreamDelayTimeR,
		DreamDelayTempo,
		DreamDelayFeedback,
		DreamDelayHighCut,
		DreamDelayBypass,
		PlateReverbMix,
		PlateReverbSize,
		PlateReverbHighDamp,
		PlateReverbLowDamp,
		PlateReverbLowPass,
		PlateReverbDiffusion,
		PlateReverbBypass,
		CloudSeed2Preset,
		CloudSeed2Interpolation,
		CloudSeed2LowCutEnabled,
		CloudSeed2HighCutEnabled,
		CloudSeed2InputMix,
		CloudSeed2LowCut,
		CloudSeed2HighCut,
		CloudSeed2DryOut,
		CloudSeed2EarlyOut,
		CloudSeed2LateOut,
		CloudSeed2TapEnabled,
		CloudSeed2TapCount,
		CloudSeed2TapDecay,
		CloudSeed2TapPredelay,
		CloudSeed2TapLength,
		CloudSeed2EarlyDiffuseEnabled,
		CloudSeed2EarlyDiffuseCount,
		CloudSeed2EarlyDiffuseDelay,
		CloudSeed2EarlyDiffuseModAmount,
		CloudSeed2EarlyDiffuseFeedback,
		CloudSeed2EarlyDiffuseModRate,
		CloudSeed2LateMode,
		CloudSeed2LateLineCount,
		CloudSeed2LateDiffuseEnabled,
		CloudSeed2LateDiffuseCount,
		CloudSeed2LateLineSize,
		CloudSeed2LateLineModAmount,
		CloudSeed2LateDiffuseDelay,
		CloudSeed2LateDiffuseModAmount,
		CloudSeed2LateLineDecay,
		CloudSeed2LateLineModRate,
		CloudSeed2LateDiffuseFeedback,
		CloudSeed2LateDiffuseModRate,
		CloudSeed2EqLowShelfEnabled,
		CloudSeed2EqHighShelfEnabled,
		CloudSeed2EqLowpassEnabled,
		CloudSeed2EqLowFreq,
		CloudSeed2EqHighFreq,
		CloudSeed2EqCutoff,
		CloudSeed2EqLowGain,
		CloudSeed2EqHighGain,
		CloudSeed2EqCrossSeed,
		CloudSeed2SeedTap,
		CloudSeed2SeedDiffusion,
		CloudSeed2SeedDelay,
		CloudSeed2SeedPostDiffusion,
		CloudSeed2Bypass,
		CompressorPreGain,
		CompressorThresh,
		CompressorRatio,
		CompressorAttack,
		CompressorRelease,
		CompressorMakeupGain,
		CompressorHPFilterEnable,
		CompressorBypass,
		EQLow,
		EQMid,
		EQHigh,
		EQGain,
		EQLowMidFreq,
		EQMidHighFreq,
		EQPreLowCut,
		EQPreHighCut,
		EQBypass,
		ReturnLevel,
		Bypass,
		Unknown,
	};

	typedef std::string TToString(int nValue, int nWidth);

	struct ParameterType
	{
		int Minimum;
		int Maximum;
		int Default;
		int Increment;
		const char *Name;
		TToString *ToString;
		int Flags;
	};

	static FX::ParameterType s_Parameter[];

	struct EffectType
	{
		const char *Name;
		const int MinID;
		const int MaxID;
	};

	static constexpr const EffectType s_effects[] = {
		{"None"},
		{"ZynDistortion", Parameter::ZynDistortionPreset, Parameter::ZynDistortionBypass},
		{"YKChorus", Parameter::YKChorusMix, Parameter::YKChorusBypass},
		{"ZynChorus", Parameter::ZynChorusPreset, Parameter::ZynChorusBypass},
		{"ZynSympathetic", Parameter::ZynSympatheticPreset, Parameter::ZynSympatheticBypass},
		{"ZynAPhaser", Parameter::ZynAPhaserPreset, Parameter::ZynAPhaserBypass},
		{"ZynPhaser", Parameter::ZynPhaserPreset, Parameter::ZynPhaserBypass},
		{"DreamDelay", Parameter::DreamDelayMix, Parameter::DreamDelayBypass},
		{"PlateReverb", Parameter::PlateReverbMix, Parameter::PlateReverbBypass},
		{"CloudSeed2", Parameter::CloudSeed2Preset, Parameter::CloudSeed2Bypass},
		{"Compressor", Parameter::CompressorPreGain, Parameter::CompressorBypass},
		{"EQ", Parameter::EQLow, Parameter::EQBypass},
	};
	static constexpr int effects_num = sizeof s_effects / sizeof *s_effects;
	static constexpr int slots_num = 3;

	static const char *getNameFromID(Parameter param, int nID);
	static int getIDFromName(Parameter param, const char *name);
};
