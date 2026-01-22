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

#include <cassert>
#include <cstring>
#include <math.h>

#include "Sympathetic.h"

#ifndef PI
#define PI 3.141592653589793f
#endif

static const uint16_t MIDI_EQ_HZ[] = {
	20, 22, 25, 28, 32, 36, 40, 45, 50, 56,
	63, 70, 80, 90, 100, 110, 125, 140, 160, 180,
	200, 225, 250, 280, 315, 355, 400, 450, 500, 560,
	630, 700, 800, 900, 1000, 1100, 1200, 1400, 1600, 1800,
	2000, 2200, 2500, 2800, 3200, 3600, 4000, 4500, 5000, 5600,
	6300, 7000, 8000, 9000, 10000, 11000, 12000, 14000, 16000, 18000,
	20000
};
static constexpr uint8_t MIDI_EQ_N = sizeof MIDI_EQ_HZ / sizeof *MIDI_EQ_HZ;

// coefficients for calculating gainbwd from Pq
// gainbwd = gainbwd_offset + Pq * gainbwd_factor
// designed for gainbwd range up to 1.0 at Pq==127
static const float gainbwd_offset = 0.873f;
static const float gainbwd_factor = 0.001f;
// precalc gainbwd_init = gainbwd_offset + gainbwd_factor * Pq
// 0.873f + 0.001f * 65 = 0.873f + 0.065f = 0.938f
static const float gainbwd_init = 0.938f;

namespace zyn {

Sympathetic::Sympathetic(float samplerate):
samplerate{samplerate},
lpf{2, 20000, 1, 0, samplerate},
hpf{3, 20, 1, 0, samplerate},
filterBank{samplerate, gainbwd_offset}
{
	loadpreset(0);
}

void Sympathetic::cleanup()
{
	lpf.cleanup();
	hpf.cleanup();
}

void Sympathetic::process(float *inputL, float *inputR, uint16_t period)
{
	if (bypass) return;

	if (wet == 0.0f) return;

	float inputvol = drive;
	if (Pnegate)
		inputvol *= -1.0f;

	float temp[period];

	for(int i = 0; i < period; ++i)
		temp[i] = (inputL[i] * panl + inputR[i] * panr) * inputvol;

	filterBank.filterout(temp, period);

	if (Plowcut != 0) hpf.filterout(temp, period);
	if (Phighcut != MIDI_EQ_N - 1) lpf.filterout(temp, period);

	float outscale = 2.0f * level * wet;
	for (int i = 0; i < period; ++i) {
		float t = temp[i] * outscale;
	
		inputL[i] = inputL[i] * dry + t;
		inputR[i] = inputR[i] * dry + t;
	}
}

void Sympathetic::setmix(unsigned char _Pmix)
{
	Pmix = _Pmix;

	float mix = (float)Pmix / 100.0f;
	if (mix < 0.5f) {
		dry = 1.0f;
		wet = mix * 2.0f;
	} else {
		dry = (1.0f - mix) * 2.0f;
		wet = 1.0f;
	}
}

void Sympathetic::setpanning(unsigned char _Ppanning)
{
	Ppanning = _Ppanning;
	float panning = ((float)Ppanning - 0.5f) / 127.0f;
	panl = cosf(panning * PI / 2.0f);
	panr = cosf((1.0f - panning) * PI / 2.0f);
}

void Sympathetic::setdrive(unsigned char _Pdrive)
{
	Pdrive = _Pdrive;
	drive = powf(2.0f, (Pdrive - 65.0f) / 128.0f) / 2.0f;
}

void Sympathetic::setlevel(unsigned char _Plevel)
{
	Plevel = _Plevel;
	level = dB2rap(60.0f * _Plevel / 127.0f - 40.0f);
}

void Sympathetic::setlowcut(unsigned char _Plowcut)
{
	assert (_Plowcut < MIDI_EQ_N);

	Plowcut = _Plowcut;
	float fr = MIDI_EQ_HZ[Plowcut];
	hpf.setfreq(fr);
}

void Sympathetic::sethighcut(unsigned char _Phighcut)
{
	assert (_Phighcut < MIDI_EQ_N);

	Phighcut = _Phighcut;
	float fr = MIDI_EQ_HZ[Phighcut];
	lpf.setfreq(fr);
}

void Sympathetic::calcFreqs()
{
	switch (Ptype) {
	case TypeGeneric: calcFreqsGeneric(); break;
	case TypePiano: calcFreqsPiano(); break;
	case TypeGuitar: calcFreqsGuitar(); break;
	default: assert(false);
	}
}

void Sympathetic::calcFreqsGeneric()
{
	float unison_spread_semicent = powf(Punison_spread / 63.5f, 2.0f) * 25.0f;
	float unison_real_spread_up = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);
	float unison_real_spread_down = 1.0f / unison_real_spread_up;

	for (unsigned i = 0; i < Pstrings; ++i)
	{
		float centerFreq = powf(2.0f, i * Pinterval / 12.0f) * baseFreq;

		int n = i * Punison_size;
		filterBank.delays[n] = samplerate / centerFreq;

		if (Punison_size > 1)
			filterBank.delays[n + 1] = samplerate / (centerFreq * unison_real_spread_up);

		if (Punison_size > 2)
			filterBank.delays[n + 2] = samplerate / (centerFreq * unison_real_spread_down);
	}
	filterBank.setStrings(Pstrings * Punison_size, baseFreq);
}

void Sympathetic::calcFreqsPiano()
{
	float unison_spread_semicent = powf(Punison_spread / 63.5f, 2.0f) * 25.0f;
	float unison_real_spread_up = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);
	float unison_real_spread_down = 1.0f / unison_real_spread_up;

	for (unsigned int i = 0; i < Pstrings; ++i)
	{
		float centerFreq = powf(2.0f, i * Pinterval / 12.0f) * baseFreq;
		unsigned int stringchoir_size;

		if (centerFreq < 52.0f) // 1 string for Low bass section keys 1 - 12 (51.91 Hz)
			stringchoir_size = 1;
		else if (centerFreq < 93.0f) // 2 strings for Bass break section keys 13 - 22 (92.50 Hz)
			stringchoir_size = 2;
		else // 3 strings for Mid Range & High Treble section keys 23 - 88
			stringchoir_size = 3;
		
		int n = i * Punison_size;
		filterBank.delays[n] = samplerate / centerFreq;

		if (Punison_size > 1)
			if (stringchoir_size > 1)
				filterBank.delays[n + 1] = samplerate / (centerFreq * unison_real_spread_up);
			else
				filterBank.delays[n + 1] = 0;

		if (Punison_size > 2)
			if (stringchoir_size > 2)
				filterBank.delays[n + 2] = samplerate / (centerFreq * unison_real_spread_down);
			else
				filterBank.delays[n + 2] = 0;
	}
	filterBank.setStrings(Pstrings * Punison_size, baseFreq);
}

void Sympathetic::calcFreqsGuitar()
{
	// frequencies steps of a guitar in standard e tuning
	//static constexpr float guitar_freqs[6] = {82.4f, 110.0f, 146.8f, 196.0f, 246.9f, 329.6f};
	static constexpr uint8_t strings = 6;
	static constexpr uint8_t steps[strings] = {0, 5, 10, 15, 19, 24};
	

	float unison_spread_semicent = powf(Punison_spread / 63.5f, 2.0f) * 25.0f;
	float unison_real_spread_up = powf(2.0f, (unison_spread_semicent * 0.5f) / 1200.0f);
	float unison_real_spread_down = 1.0f / unison_real_spread_up;

	for (unsigned int i = 0; i < strings; ++i)
	{
		float centerFreq = powf(2.0f, steps[i] / 12.0f) * baseFreq;

		int n = i * Punison_size;
		filterBank.delays[n] = samplerate / centerFreq;

		if (Punison_size > 1)
			filterBank.delays[n + 1] = samplerate / (centerFreq * unison_real_spread_up);

		if (Punison_size > 2)
			filterBank.delays[n + 2] = samplerate / (centerFreq * unison_real_spread_down);
	}
	filterBank.setStrings(strings * Punison_size, baseFreq);
}

static const char *SympTypes[Sympathetic::types_num] = {
	"Generic",
	"Piano",
	"Guitar",
};

std::string Sympathetic::ToTypeName(int nValue, int nWidth)
{
	assert (nValue >= 0 && (unsigned)nValue < types_num);
	return SympTypes[nValue];
}

static const char *PresetNames[Sympathetic::presets_num] = {
	"Init",
	"Generic",
	"Piano 12-String",
	"Piano 60-String",
	"Guitar 6-String",
	"Guitar 12-String",
	"Violin",
	"Double Bass",
};

const char * Sympathetic::ToPresetNameChar(int nValue)
{
	assert (nValue >= 0 && (unsigned)nValue < presets_num);
	return PresetNames[nValue];
}

std::string Sympathetic::ToPresetName(int nValue, int nWidth)
{
	return ToPresetNameChar(nValue);
}

unsigned Sympathetic::ToIDFromPreset(const char *preset)
{
	for (unsigned i = 0; i < presets_num; ++i)
		if (strcmp(PresetNames[i], preset) == 0)
			return i;

	return 0;
}

void Sympathetic::loadpreset(unsigned char npreset)
{
	const int presets[presets_num][ParameterCount] = {
		{ // Init
			[ParameterMix] = 0,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 0,
			[ParameterQ] = 125,
			[ParameterDrive] = 5,
			[ParameterLevel] = 80,
			[ParameterType] = TypeGeneric,
			[ParameterUnisonSize] = 1,
			[ParameterUnisonSpread] = 10,
			[ParameterStrings] = 12,
			[ParameterInterval] = 1,
			[ParameterBaseNote] = 57,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Generic
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 0,
			[ParameterQ] = 125,
			[ParameterDrive] = 5,
			[ParameterLevel] = 80,
			[ParameterType] = TypeGeneric,
			[ParameterUnisonSize] = 3,
			[ParameterUnisonSpread] = 10,
			[ParameterStrings] = 12,
			[ParameterInterval] = 1,
			[ParameterBaseNote] = 57,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Piano 12-String
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 1,
			[ParameterQ] = 125,
			[ParameterDrive] = 5,
			[ParameterLevel] = 80,
			[ParameterType] = TypePiano,
			[ParameterUnisonSize] = 3,
			[ParameterUnisonSpread] = 10,
			[ParameterStrings] = 12,
			[ParameterInterval] = 1,
			[ParameterBaseNote] = 57,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Piano 60-String
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 1,
			[ParameterQ] = 125,
			[ParameterDrive] = 5,
			[ParameterLevel] = 90,
			[ParameterType] = TypePiano,
			[ParameterUnisonSize] = 1,
			[ParameterUnisonSpread] = 5,
			[ParameterStrings] = 60,
			[ParameterInterval] = 1,
			[ParameterBaseNote] = 33,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Guitar 6-String
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 0,
			[ParameterQ] = 110,
			[ParameterDrive] = 20,
			[ParameterLevel] = 65,
			[ParameterType] = TypeGuitar,
			[ParameterUnisonSize] = 1,
			[ParameterUnisonSpread] = 0,
			[ParameterStrings] = 6,
			[ParameterInterval] = 1,
			[ParameterBaseNote] = 40,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Guitar 12-String
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 0,
			[ParameterQ] = 110,
			[ParameterDrive] = 20,
			[ParameterLevel] = 77,
			[ParameterType] = TypeGuitar,
			[ParameterUnisonSize] = 2,
			[ParameterUnisonSpread] = 10,
			[ParameterStrings] = 6,
			[ParameterInterval] = 1,
			[ParameterBaseNote] = 40,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Violin
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 0,
			[ParameterQ] = 110,
			[ParameterDrive] = 20,
			[ParameterLevel] = 77,
			[ParameterType] = TypeGeneric,
			[ParameterUnisonSize] = 1,
			[ParameterUnisonSpread] = 10,
			[ParameterStrings] = 4,
			[ParameterInterval] = 7,
			[ParameterBaseNote] = 55,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
		{ // Double Bass
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterUseSustain] = 0,
			[ParameterQ] = 110,
			[ParameterDrive] = 20,
			[ParameterLevel] = 77,
			[ParameterType] = TypeGeneric,
			[ParameterUnisonSize] = 1,
			[ParameterUnisonSpread] = 10,
			[ParameterStrings] = 4,
			[ParameterInterval] = 5,
			[ParameterBaseNote] = 28,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterNegate] = 0,
		},
	};

	if (npreset >= presets_num)
		npreset = presets_num;

	for (int n = 0; n < ParameterCount; n++)
		changepar(n, presets[npreset][n]);

	cleanup();
}

void Sympathetic::changepar(int npar, unsigned char value)
{
	switch(npar) {
	case ParameterMix: setmix(value); break;
	case ParameterPanning: setpanning(value); break;
	case ParameterUseSustain:
		Puse_sustain = value;
		filterBank.gainbwd = Puse_sustain ? gainbwd_offset : gainbwd_offset + Pq * gainbwd_factor;
	break;
	case ParameterQ:
		Pq = value;
		if (!Puse_sustain)
			filterBank.gainbwd = gainbwd_offset + Pq * gainbwd_factor;
	break;
	case ParameterDrive:
		setdrive(value);
		filterBank.inputgain = (float)Pdrive / 65.0f;
	break;
	case ParameterLevel:
		setlevel(value);
		filterBank.outgain = (float)Plevel / 65.0f;
	break;
	case ParameterType:
		value = value < types_num ? value : types_num - 1;
		if (Ptype != value)
		{
			Ptype = value;
			calcFreqs();
		}
	break;
	case ParameterUnisonSize:
	{
		value = value > 3 ? 3 : value;
		value = value < 1 ? 1 : value;
		if(Punison_size != value)
		{
			Punison_size = value;
			calcFreqs();
		}
	}
	break;
	case ParameterUnisonSpread:
		if(Punison_spread != value)
		{
			Punison_spread = value;
			calcFreqs();
		}
	break;
	case ParameterStrings:
	{
		value = value > 76 ? 76 : value;
		if (Ptype == TypeGuitar) value = 6;
		if (Pstrings != value)
		{
			Pstrings = value;
			calcFreqs();
		}
	}
	break;
	case ParameterInterval:
	{
		value = value < 1 ? 1 : value;
		value = value > 10 ? 10 : value;
		if (Pinterval != value)
		{
			Pinterval = value;
			calcFreqs();
		}
	}
	break;
	case ParameterBaseNote:
		if (Pbasenote != value)
		{
			Pbasenote = value;
			baseFreq = powf(2.0f, ((float)Pbasenote - 69.0f) / 12.0f) * 440.0f;
			calcFreqs();
		}
	break;
	case ParameterLowcut: setlowcut(value); break;
	case ParameterHighcut: sethighcut(value); break;
	case ParameterNegate: Pnegate = value > 1 ? 1 : value; break;
	default: assert(false);
	}
}

unsigned char Sympathetic::getpar(int npar) const
{
	switch(npar) {
	case ParameterMix: return Pmix;
	case ParameterPanning: return Ppanning;
	case ParameterUseSustain: return Puse_sustain;
	case ParameterQ: return Pq;
	case ParameterDrive: return Pdrive;
	case ParameterLevel: return Plevel;
	case ParameterType: return Ptype;
	case ParameterUnisonSize: return Punison_size;
	case ParameterUnisonSpread: return Punison_spread;
	case ParameterStrings: return Pstrings;
	case ParameterInterval: return Pinterval;
	case ParameterBaseNote: return Pbasenote;
	case ParameterLowcut: return Plowcut;
	case ParameterHighcut: return Phighcut;
	case ParameterNegate: return Pnegate;
	default: assert(false);
	}
}

void Sympathetic::sustain(bool sustain)
{
	if (Puse_sustain)
		filterBank.gainbwd = sustain ? gainbwd_offset + Pq * gainbwd_factor : gainbwd_offset;
}

}
