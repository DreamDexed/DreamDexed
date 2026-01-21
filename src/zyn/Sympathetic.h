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

// coefficients for calculating gainbwd from Pq
// gainbwd = gainbwd_offset + Pq * gainbwd_factor
// designed for gainbwd range up to 1.0 at Pq==127
const float gainbwd_offset = 0.873f;
const float gainbwd_factor = 0.001f;
// precalc gainbwd_init = gainbwd_offset + gainbwd_factor * Pq
// 0.873f + 0.001f * 65 = 0.873f + 0.065f = 0.938f
const float gainbwd_init = 0.938f;

// number of piano keys with single string
const unsigned int num_single_strings = 12;
// number of piano keys with triple strings
const unsigned int num_triple_strings = 48;

// frequencies of a guitar in standard e tuning
const float guitar_freqs[6] = {82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f};

class Sympathetic
{
public:
	Sympathetic(float samplerate);
	void process(float* inputL, float *inputR, uint16_t period);
	void loadpreset(unsigned char npreset);
	void changepar(int npar, unsigned char value);
	unsigned char getpar(int npar) const;
	void cleanup();

	std::atomic<bool> bypass;

	static constexpr unsigned presets_num = 6;

	enum Parameter {
		ParameterMix,
		ParameterPanning,
		ParameterQ,
		ParameterDrive,
		ParameterLevel,
		ParameterType,
		ParameterUnisonSize,
		ParameterUnisonSpread,
		ParameterStrings,
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
	unsigned char Pdrive; //the input amplification
	unsigned char Plevel; //the output amplification
	unsigned char Ptype; //type (generic/piano/guitar)
	unsigned char Punison_size; //number of unison strings
	unsigned char Punison_spread;
	unsigned char Pstrings; //number of strings
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
