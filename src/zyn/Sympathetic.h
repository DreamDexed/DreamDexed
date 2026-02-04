/*
  ZynAddSubFX - a software synthesizer

  Sympathetic.cpp - Distorsion effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once

#include <atomic>
#include <string>

#include "AnalogFilter.h"
#include "CombFilterBank.h"

namespace zyn
{

class Sympathetic
{
public:
	Sympathetic(float samplerate);
	void process(float *inputL, float *inputR, int period);
	void loadpreset(int npreset);
	void changepar(int npar, int value, bool updateFreqs);
	int getpar(int npar) const;
	void cleanup();

	void sustain(bool sustain);

	std::atomic<bool> bypass;

	static constexpr int presets_num = 8;

	enum Parameter
	{
		ParameterMix,
		ParameterPanning,
		ParameterQ,
		ParameterQSustain,
		ParameterDrive,
		ParameterLevel,
		ParameterType,
		ParameterUnisonSize,
		ParameterUnisonSpread,
		ParameterStrings,
		ParameterInterval,
		ParameterBaseNote,
		ParameterLowcut,
		ParameterHighcut,
		ParameterNegate,
		ParameterCount,
	};

	enum Type
	{
		TypeGeneric,
		TypePiano,
		TypeGuitar,
		types_num,
	};

	static std::string ToTypeName(int nValue, int nWidth);
	static std::string ToPresetName(int nValue, int nWidth);
	static const char *ToPresetNameChar(int nValue);
	static int ToIDFromPreset(const char *preset);

private:
	float samplerate;

	// Parameters
	signed char Pmix;
	signed char Ppanning;
	signed char Pq; // 0=0.95 ... 127=1.05
	signed char Pq_sustain; // 0=0.95 ... 127=1.05
	signed char Pdrive; // the input amplification
	signed char Plevel; // the output amplification
	signed char Ptype; // type (generic/piano/guitar)
	signed char Punison_size; // number of unison strings
	signed char Punison_spread;
	signed char Pstrings; // number of strings
	signed char Pinterval; // number of semitones between strings
	signed char Pbasenote; // midi note of lowest string
	signed char Plowcut;
	signed char Phighcut;
	signed char Pnegate; // if the input is negated

	float baseFreq;

	void setmix(signed char _Pmix);
	void setpanning(signed char _Ppanning);
	void setdrive(signed char _Pdrive);
	void setlevel(signed char _Plevel);
	void setlowcut(signed char _Plowcut);
	void sethighcut(signed char _Phighcut);
	void calcFreqs();
	void calcFreqsGeneric();
	void calcFreqsPiano();
	void calcFreqsGuitar();

	// Real Parameters
	AnalogFilter lpf, hpf;

	CombFilterBank filterBank;

	float dry, wet, panl, panr, drive, level;
};

} // namespace zyn
