#pragma once

#include <string>

class Bus
{
public:
	enum Parameter
	{
		MixerDryLevel,
		ReturnLevel,
		FXBypass,
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

	static ParameterType s_Parameter[Unknown];
};
