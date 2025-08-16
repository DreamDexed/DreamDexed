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
		FXParameterPlateReverbEnable,
		FXParameterPlateReverbSize,
		FXParameterPlateReverbHighDamp,
		FXParameterPlateReverbLowDamp,
		FXParameterPlateReverbLowPass,
		FXParameterPlateReverbDiffusion,
		FXParameterOutputLevel,
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
		int flags;
	};

	static FX::FXParameterType s_FXParameter[];
};
