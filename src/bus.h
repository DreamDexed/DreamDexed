#pragma once

#include <string>

class Bus
{
public:
	enum Flag
	{
		UIOnly = 1 << 0, // it shouldn't update the controls at startup and performance load
	};

	enum LoadType
	{
		FullBus,
		BusTGOnly,
		BusFXOnly,
		Full,
	};

	enum Parameter
	{
		PerformanceBank,
		Performance,
		LoadType,
		MIDIChannel,
		MixerDryLevel,
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
