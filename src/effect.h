class FX
{
public:
	static const int FXComposite = 1 << 0; // updates multiple controls, it shouldn't update the controls at startup and performance load

	enum TFXParameter
	{
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

	static constexpr const char *s_CS2PresetNames[] = {
		"Init",
		"FXDivineInspiration",
		"FXLawsOfPhysics",
		"FXSlowBraaam",
		"FXTheUpsideDown",
		"LBigSoundStage",
		"LDiffusionCyclone",
		"LScreamIntoTheVoid",
		"M90sDigitalReverb",
		"MAiryAmbience",
		"MDarkPlate",
		"MGhostly",
		"MTappedLines",
		"SFastAttack",
		"SSmallPlate",
		"SSnappyAttack",
	};
	static constexpr unsigned cs2_preset_num = sizeof s_CS2PresetNames / sizeof *s_CS2PresetNames;

	static std::string getCS2PresetName (int nValue, int nWidth)
	{
		assert (nValue >= 0 && (unsigned)nValue < cs2_preset_num);
		return s_CS2PresetNames[nValue];
	}

	static unsigned getIDFromCS2PresetName(const char *presetName)
	{
		for (unsigned i = 0; i < cs2_preset_num; ++i)
		if (strcmp(s_CS2PresetNames[i], presetName) == 0)
			return i;

		return 0;
	}
};
