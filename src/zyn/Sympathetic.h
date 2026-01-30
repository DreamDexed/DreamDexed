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

namespace zyn {

class Sympathetic
{
public:
	Sympathetic(float samplerate);
	void process(float* inputL, float *inputR, uint16_t period);
	void loadpreset(unsigned char npreset);
	void changepar(int npar, unsigned char value, bool updateFreqs);
	unsigned char getpar(int npar) const;
	void cleanup();

	void sustain(bool sustain);

	std::atomic<bool> bypass;

	static constexpr unsigned presets_num = 8;

	enum Parameter {
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

	enum Type {
		TypeGeneric,
		TypePiano,
		TypeGuitar,
		types_num,
	};

	static std::string ToTypeName(int nValue, int nWidth);
	static std::string ToPresetName(int nValue, int nWidth);
	static const char * ToPresetNameChar(int nValue);
	static unsigned ToIDFromPreset(const char *preset);

private:
	float samplerate;

	//Parameters
	unsigned char Pmix;
	unsigned char Ppanning;
	unsigned char Pq; //0=0.95 ... 127=1.05
	unsigned char Pq_sustain; //0=0.95 ... 127=1.05
	unsigned char Pdrive; //the input amplification
	unsigned char Plevel; //the output amplification
	unsigned char Ptype; //type (generic/piano/guitar)
	unsigned char Punison_size; //number of unison strings
	unsigned char Punison_spread;
	unsigned char Pstrings; //number of strings
	unsigned char Pinterval; //number of semitones between strings
	unsigned char Pbasenote; //midi note of lowest string
	unsigned char Plowcut;
	unsigned char Phighcut;
	unsigned char Pnegate; //if the input is negated

	float baseFreq;

	void setmix(unsigned char _Pmix);
	void setpanning(unsigned char _Ppanning);
	void setdrive(unsigned char _Pdrive);
	void setlevel(unsigned char _Plevel);
	void setlowcut(unsigned char _Plowcut);
	void sethighcut(unsigned char _Phighcut);
	void calcFreqs();
	void calcFreqsGeneric();
	void calcFreqsPiano();
	void calcFreqsGuitar();

	//Real Parameters
	AnalogFilter lpf, hpf;

	CombFilterBank filterBank;

	float dry, wet, panl, panr, drive, level;
};

}
