#include "uitostring.h"

#include <algorithm>
#include <cassert>
#include <cstdio>
#include <string>

#include "bus.h"
#include "effect.h"
#include "effect_compressor.h"
#include "midi.h"
#include "mididevice.h"
#include "util.h"

std::string ToOnOff(int nValue, int nWidth)
{
	static const char *OnOff[] = {"Off", "On"};

	assert(nValue < ARRAY_LENGTH(OnOff));

	return OnOff[nValue];
}

std::string ToDelayMode(int nValue, int nWidth)
{
	static const char *Mode[] = {"Dual", "Crossover", "PingPong"};

	assert(nValue < ARRAY_LENGTH(Mode));

	return Mode[nValue];
}

std::string ToDelayTime(int nValue, int nWidth)
{
	static const char *Sync[] = {"1/1", "1/1T", "1/2", "1/2T", "1/4", "1/4T", "1/8", "1/8T", "1/16", "1/16T", "1/32", "1/32T"};

	assert(nValue >= 0 && nValue <= 112);

	if (nValue <= 100)
		return std::to_string(nValue * 10) + " ms";

	return Sync[nValue - 100 - 1];
}

std::string ToBPM(int nValue, int nWidth)
{
	return std::to_string(nValue) + " BPM";
}

static const char MIDINoteName[128][9] =
{
	"0   C-2", "1   C#-2", "2   D-2", "3   D#-2", "4   E-2", "5   F-2", "6   F#-2", "7   G-2", "8   G#-2", "9   A-2", "10  A#-2", "11  B-2",
	"12  C-1", "13  C#-1", "14  D-1", "15  D#-1", "16  E-1", "17  F-1", "18  F#-1", "19  G-1", "20  G#-1", "21  A-1", "22  A#-1", "23  B-1",
	"24  C0", "25  C#0", "26  D0", "27  D#0", "28  E0", "29  F0", "30  F#0", "31  G0", "32  G#0", "33  A0", "34  A#0", "35  B0",
	"36  C1", "37  C#1", "38  D1", "39  D#1", "40  E1", "41  F1", "42  F#1", "43  G1", "44  G#1", "45  A1", "46  A#1", "47  B1",
	"48  C2", "49  C#2", "50  D2", "51  D#2", "52  E2", "53  F2", "54  F#2", "55  G2", "56  G#2", "57  A2", "58  A#2", "59  B2",
	"60  C3", "61  C#3", "62  D3", "63  D#3", "64  E3", "65  F3", "66  F#3", "67  G3", "68  G#3", "69  A3", "70  A#3", "71  B3",
	"72  C4", "73  C#4", "74  D4", "75  D#4", "76  E4", "77  F4", "78  F#4", "79  G4", "80  G#4", "81  A4", "82  A#4", "83  B4",
	"84  C5", "85  C#5", "86  D5", "87  D#5", "88  E5", "89  F5", "90  F#5", "91  G5", "92  G#5", "93  A5", "94  A#5", "95  B5",
	"96  C6", "97  C#6", "98  D6", "99  D#6", "100 E6", "101 F6", "102 F#6", "103 G6", "104 G#6", "105 A6", "106 A#6", "107 B6",
	"108 C7", "109 C#7", "110 D7", "111 D#7", "112 E7", "113 F7", "114 F#7", "115 G7", "116 G#7", "117 A7", "118 A#7", "119 B7",
	"120 C8", "121 C#8", "122 D8", "123 D#8", "124 E8", "125 F8", "126 F#8", "127 G8"};

std::string ToMIDINote(int nValue, int nWidth)
{
	assert(nValue < ARRAY_LENGTH(MIDINoteName));

	return MIDINoteName[nValue];
}

std::string ToHz(int nValue, int nWidth)
{
	assert(nValue < ARRAY_LENGTH(MIDI_EQ_HZ));

	short hz = MIDI_EQ_HZ[nValue];
	char buf[20] = {};

	if (hz < 1000)
		return std::to_string(hz) + " Hz";

	std::snprintf(buf, sizeof(buf), "%.1f kHz", hz / 1000.0);
	return buf;
}

std::string ToSemitones(int nValue, int nWidth)
{
	return std::to_string(nValue) + " semitone" + (nValue > 1 ? "s" : "");
}

std::string ToDryWet(int nValue, int nWidth)
{
	int dry, wet;
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

	return std::to_string(dry) + ":" + std::to_string(wet) + (wet == 0 ? " Off" : "");
}

std::string ToEffectName(int nValue, int nWidth)
{
	assert(nValue >= 0 && nValue < FX::effects_num);
	return FX::s_effects[nValue].Name;
}

std::string TodB(int nValue, int nWidth)
{
	return std::to_string(nValue) + " dB";
}

std::string TodBFS(int nValue, int nWidth)
{
	return std::to_string(nValue) + " dBFS";
}

std::string ToMillisec(int nValue, int nWidth)
{
	return std::to_string(nValue) + " ms";
}

std::string ToRatio(int nValue, int nWidth)
{
	if (nValue == AudioEffectCompressor::CompressorRatioInf)
		return "INF:1";

	return std::to_string(nValue) + ":1";
}

std::string ToPan(int nValue, int nWidth)
{
	char buf[nWidth + 1];
	int nBarWidth = nWidth - 3;
	int nIndex = std::min(nValue * nBarWidth / 127, nBarWidth - 1);
	std::fill_n(buf, nBarWidth, '.');
	buf[nBarWidth / 2] = ':';
	buf[nIndex] = 0xFF; // 0xFF is the block character
	snprintf(buf + nBarWidth, 4, "%3d", nValue);
	return buf;
}

std::string ToVolume(int nValue, int nWidth)
{
	char buf[nWidth + 1];
	int nBarWidth = nWidth - 3;
	int nFillWidth = (nValue * nBarWidth + 63) / 127;
	int nDotWidth = nBarWidth - nFillWidth;
	std::fill_n(buf, nFillWidth, 0xFF);
	std::fill_n(buf + nFillWidth, nDotWidth, '.');
	snprintf(buf + nFillWidth + nDotWidth, 4, "%3d", nValue);
	return buf;
}

std::string ToLRDelay(int nValue, int nWidth)
{
	nValue -= 64;
	if (nValue < 0)
		return std::to_string(nValue) + " L";
	else if (nValue == 0)
		return " 0 Center";
	else
		return std::string("+") + std::to_string(nValue) + " R";
}

std::string ToCenter64(int nValue, int nWidth)
{
	nValue -= 64;
	if (nValue > 0)
		return std::string("+") + std::to_string(nValue);
	else if (nValue == 0)
		return " 0";
	else
		return std::to_string(nValue);
}

std::string ToPrePost(int nValue, int nWidth)
{
	static const char *PrePost[] = {"Pre", "Post"};

	assert(nValue < ARRAY_LENGTH(PrePost));

	return PrePost[nValue];
}

std::string ToAlgorithm(int nValue, int nWidth)
{
	return std::to_string(nValue + 1);
}

std::string ToLFOWaveform(int nValue, int nWidth)
{
	static const char *Waveform[] = {"Triangle", "Saw down", "Saw up",
					 "Square", "Sine", "Sample/Hold"};

	assert(nValue < ARRAY_LENGTH(Waveform));

	return Waveform[nValue];
}

std::string ToMIDIChannel(int nValue, int nWidth)
{
	switch (nValue)
	{
	case CMIDIDevice::OmniMode:
		return "Omni";
	case CMIDIDevice::Disabled:
		return "Off";
	default:
		return std::to_string(nValue + 1);
	}
}

static const char NoteName[100][5] =
{
	"A-1", "A#-1", "B-1", "C0", "C#0", "D0", "D#0", "E0", "F0", "F#0", "G0", "G#0",
	"A0", "A#0", "B0", "C1", "C#1", "D1", "D#1", "E1", "F1", "F#1", "G1", "G#1",
	"A1", "A#1", "B1", "C2", "C#2", "D2", "D#2", "E2", "F2", "F#2", "G2", "G#2",
	"A2", "A#2", "B2", "C3", "C#3", "D3", "D#3", "E3", "F3", "F#3", "G3", "G#3",
	"A3", "A#3", "B3", "C4", "C#4", "D4", "D#4", "E4", "F4", "F#4", "G4", "G#4",
	"A4", "A#4", "B4", "C5", "C#5", "D5", "D#5", "E5", "F5", "F#5", "G5", "G#5",
	"A5", "A#5", "B5", "C6", "C#6", "D6", "D#6", "E6", "F6", "F#6", "G6", "G#6",
	"A6", "A#6", "B6", "C7", "C#7", "D7", "D#7", "E7", "F7", "F#7", "G7", "G#7",
	"A7", "A#7", "B7", "C8"};

static const int NoteC3 = 39;

std::string ToTransposeNote(int nValue, int nWidth)
{
	nValue += NoteC3 - 24;

	assert(nValue < ARRAY_LENGTH(NoteName));

	return NoteName[nValue];
}

std::string ToBreakpointNote(int nValue, int nWidth)
{
	assert(nValue < ARRAY_LENGTH(NoteName));

	return NoteName[nValue];
}

static const char MIDINoteShift[49][8] =
{
	"-24 C1", "-23 C#1", "-22 D1", "-21 D#1", "-20 E1", "-19 F1", "-18 F#1", "-17 G1", "-16 G#1", "-15 A1", "-14 A#1", "-13 B1",
	"-12 C2", "-11 C#2", "-10 D2", "-9  D#2", "-8  E2", "-7  F2", "-6  F#2", "-5  G2", "-4  G#2", "-3  A2", "-2  A#2", "-1  B2",
	"0   C3", "+1  C#3", "+2  D3", "+3  D#3", "+4  E3", "+5  F3", "+6  F#3", "+7  G3", "+8  G#3", "+9  A3", "+10 A#3", "+11 B3",
	"+12 C4", "+13 C#4", "+14 D4", "+15 D#4", "+16 E4", "+17 F4", "+18 F#4", "+19 G4", "+20 G#4", "+21 A4", "+22 A#4", "+23 B4",
	"+24 C5"};

std::string ToMIDINoteShift(int nValue, int nWidth)
{
	assert(nValue + 24 < ARRAY_LENGTH(MIDINoteShift));

	return MIDINoteShift[nValue + 24];
}

std::string ToKeyboardCurve(int nValue, int nWidth)
{
	static const char *Curve[] = {"-Lin", "-Exp", "+Exp", "+Lin"};

	assert(nValue < ARRAY_LENGTH(Curve));

	return Curve[nValue];
}

std::string ToOscillatorMode(int nValue, int nWidth)
{
	static const char *Mode[] = {"Ratio", "Fixed"};

	assert(nValue < ARRAY_LENGTH(Mode));

	return Mode[nValue];
}

std::string ToOscillatorDetune(int nValue, int nWidth)
{
	std::string Result;

	nValue -= 7;

	if (nValue > 0)
	{
		Result = "+" + std::to_string(nValue);
	}
	else
	{
		Result = std::to_string(nValue);
	}

	return Result;
}

std::string ToPortaMode(int nValue, int nWidth)
{
	switch (nValue)
	{
	case 0:
		return "Fingered";
	case 1:
		return "Full time";
	default:
		return std::to_string(nValue);
	}
};

std::string ToPortaGlissando(int nValue, int nWidth)
{
	switch (nValue)
	{
	case 0:
		return "Off";
	case 1:
		return "On";
	default:
		return std::to_string(nValue);
	}
};

std::string ToPolyMono(int nValue, int nWidth)
{
	switch (nValue)
	{
	case 0:
		return "Poly";
	case 1:
		return "Mono";
	default:
		return std::to_string(nValue);
	}
}

std::string ToTGLinkName(int nValue, int nWidth)
{
	if (nValue == 0) return "-";
	return std::string{static_cast<char>(nValue + 'A' - 1)};
}

std::string ToLoadType(int nValue, int nWidth)
{
	switch (nValue)
	{
	case Bus::LoadType::TGsSendFXs:
		return "TGs + SendFXs";
	case Bus::LoadType::TGs:
		return "TGs";
	case Bus::LoadType::SendFXs:
		return "SendFXs";
	case Bus::LoadType::SendFX1:
		return "SendFX1";
	case Bus::LoadType::SendFX2:
		return "SendFX2";
	case Bus::LoadType::SendFX1ToFX2:
		return "SendFX1 to FX2";
	case Bus::LoadType::SendFX2ToFX1:
		return "SendFX2 to FX1";
	case Bus::LoadType::MasterFX:
		return "Master FX";
	case Bus::LoadType::BusAndMasterFX:
		return "Bus + MasterFX";
	default:
		return std::to_string(nValue);
	}
}
