class FX
{
public:
	enum TFXParameter
	{
		FXParameterReverbEnable,
		FXParameterReverbSize,
		FXParameterReverbHighDamp,
		FXParameterReverbLowDamp,
		FXParameterReverbLowPass,
		FXParameterReverbDiffusion,
		FXParameterReverbLevel,
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
	};

	static FX::FXParameterType s_FXParameter[];
};
