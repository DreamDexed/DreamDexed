#pragma once

#include <cassert>
#include <string>

struct SDFilter
{
	enum class Type : uint8_t {
		None,
		TGLink,
		TG,
		MIDIChannel
	};
	Type type;
	uint8_t param;

	static constexpr unsigned get_maximum(unsigned nTGNum)
	{
		return 4 + nTGNum + 16;
	}

	static SDFilter to_filter(unsigned nValue, unsigned nTGNum)
	{
		assert (nValue <= get_maximum(nTGNum));

		if (nValue == 0) return {Type::None, 0};
		if (nValue >= 1 && nValue <= 4) return {Type::TGLink, (uint8_t)nValue};
		if (nValue >= 5 && nValue < 5 + nTGNum) return {Type::TG, (uint8_t)(nValue - 5)};
		return {Type::MIDIChannel, (uint8_t)(nValue - 5 - nTGNum)};
	}

	std::string to_string()
	{
		switch (type)
		{
		case Type::None: return "None";
		case Type::TGLink: return std::string("TG-Link ") + (char)('A' + param - 1);
		case Type::TG: return std::string("TG ") + std::to_string(param + 1);
		default: return std::string("MIDI Ch ") + std::to_string(param + 1);
		}
	}
};
