#pragma once

#include <cassert>
#include <string>

struct SDFilter
{
	enum class Type
	{
		None,
		TGLink,
		TG,
		MIDIChannel
	};
	Type type;
	int param;

	static constexpr int get_maximum(int nTGNum)
	{
		return 4 + nTGNum + 16;
	}

	static SDFilter to_filter(int nValue, int nTGNum)
	{
		assert(nValue >= 0 && nValue <= get_maximum(nTGNum));

		if (nValue == 0) return {Type::None, 0};
		if (nValue >= 1 && nValue <= 4) return {Type::TGLink, nValue};
		if (nValue >= 5 && nValue < 5 + nTGNum) return {Type::TG, nValue - 5};
		return {Type::MIDIChannel, nValue - 5 - nTGNum};
	}

	std::string to_string()
	{
		switch (type)
		{
		case Type::None:
			return "None";
		case Type::TGLink:
			return std::string("TG-Link ") + static_cast<char>('A' + param - 1);
		case Type::TG:
			return std::string("TG ") + std::to_string(param + 1);
		default:
			return std::string("MIDI Ch ") + std::to_string(param + 1);
		}
	}
};
