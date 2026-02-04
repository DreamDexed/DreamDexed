/*
  ZynAddSubFX - a software synthesizer

  Distortion.cpp - Distortion effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include "Distortion.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <string>

#include "AnalogFilter.h"
#include "WaveShapeSmps.h"

#ifndef PI
#define PI 3.141592653589793f
#endif

static const short MIDI_EQ_HZ[] = {
	20, 22, 25, 28, 32, 36, 40, 45, 50, 56,
	63, 70, 80, 90, 100, 110, 125, 140, 160, 180,
	200, 225, 250, 280, 315, 355, 400, 450, 500, 560,
	630, 700, 800, 900, 1000, 1100, 1200, 1400, 1600, 1800,
	2000, 2200, 2500, 2800, 3200, 3600, 4000, 4500, 5000, 5600,
	6300, 7000, 8000, 9000, 10000, 11000, 12000, 14000, 16000, 18000,
	20000};
static constexpr short MIDI_EQ_N = sizeof MIDI_EQ_HZ / sizeof *MIDI_EQ_HZ;

namespace zyn
{

Distortion::Distortion(float samplerate) :
samplerate{samplerate},
Ppreset{},
lpfl{2, 20000, 1, 0, samplerate},
lpfr{2, 20000, 1, 0, samplerate},
hpfl{3, 20, 1, 0, samplerate},
hpfr{3, 20, 1, 0, samplerate}
{
	loadpreset(Ppreset);
}

// Cleanup the effect
void Distortion::cleanup()
{
	lpfl.cleanup();
	hpfl.cleanup();
	lpfr.cleanup();
	hpfr.cleanup();
}

// Apply the filters
void Distortion::applyfilters(float *inputL, float *inputR, int period)
{
	if (Phighcut != MIDI_EQ_N - 1) lpfl.filterout(inputL, period);
	if (Plowcut != 0) hpfl.filterout(inputL, period);
	if (Pstereo != 0)
	{ // stereo
		if (Phighcut != MIDI_EQ_N - 1) lpfr.filterout(inputR, period);
		if (Plowcut != 0) hpfr.filterout(inputR, period);
	}
}

void Distortion::process(float *inputL, float *inputR, int period)
{
	if (bypass) return;

	if (wet == 0.0f) return;

	float inputvol = powf(5.0f, (Pdrive - 32.0f) / 127.0f);
	if (Pnegate)
		inputvol *= -1.0f;

	float tempL[period];
	float tempR[period];

	if (Pstereo)
		for (int i = 0; i < period; ++i)
		{
			tempL[i] = inputL[i] * inputvol * panl;
			tempR[i] = inputR[i] * inputvol * panr;
		}
	else
		for (int i = 0; i < period; ++i)
			tempL[i] = (inputL[i] * panl + inputR[i] * panr) * inputvol;

	if (Pfiltering == FilteringPre)
		applyfilters(tempL, tempR, period);

	waveShapeSmps(period, tempL, Ptype, Pdrive, Poffset, Pshape);
	if (Pstereo)
		waveShapeSmps(period, tempR, Ptype, Pdrive, Poffset, Pshape);

	if (Pfiltering == FilteringPost)
		applyfilters(tempL, tempR, period);

	if (!Pstereo)
		memcpy(tempR, tempL, static_cast<size_t>(period) * sizeof(float));

	for (int i = 0; i < period; ++i)
	{
		float lout = tempL[i];
		float rout = tempR[i];
		float l = lout * (1.0f - lrcross) + rout * lrcross;
		float r = rout * (1.0f - lrcross) + lout * lrcross;

		inputL[i] = inputL[i] * dry + l * 2.0f * level * wet;
		inputR[i] = inputR[i] * dry + r * 2.0f * level * wet;
	}
}

void Distortion::setmix(signed char _Pmix)
{
	Pmix = _Pmix;

	float mix = Pmix / 100.0f;
	if (mix < 0.5f)
	{
		dry = 1.0f;
		wet = mix * 2.0f;
	}
	else
	{
		dry = (1.0f - mix) * 2.0f;
		wet = 1.0f;
	}
}

void Distortion::setlowcut(signed char _Plowcut)
{
	assert(_Plowcut < MIDI_EQ_N);

	Plowcut = _Plowcut;
	float fr = MIDI_EQ_HZ[Plowcut];
	hpfl.setfreq(fr);
	hpfr.setfreq(fr);
}

void Distortion::sethighcut(signed char _Phighcut)
{
	assert(_Phighcut < MIDI_EQ_N);

	Phighcut = _Phighcut;
	float fr = MIDI_EQ_HZ[Phighcut];
	lpfl.setfreq(fr);
	lpfr.setfreq(fr);
}

void Distortion::setpanning(signed char Ppanning_)
{
	Ppanning = Ppanning_;
	float panning = (Ppanning - 0.5f) / 127.0f;
	panl = cosf(panning * PI / 2.0f);
	panr = cosf((1.0f - panning) * PI / 2.0f);
}

void Distortion::setlevel(signed char Plevel_)
{
	Plevel = Plevel_;
	level = dB2rap(60.0f * Plevel_ / 127.0f - 40.0f);
}

void Distortion::setlrcross(signed char Plrcross_)
{
	Plrcross = Plrcross_;
	lrcross = Plrcross / 127.0f;
}

static const char *DistortionTypes[Distortion::types_num] = {
	"Arctangent",
	"Asymmetric",
	"Pow",
	"Sine",
	"Quantisize",
	"Zigzag",
	"Limiter",
	"Upper Limiter",
	"Lower Limiter",
	"Inverse Limiter",
	"Clip",
	"Asymmetric2",
	"Pow2",
	"Sigmoid",
	"TanhSoft",
	"Cubic",
	"Square",
};

std::string Distortion::ToDistortionType(int nValue, int nWidth)
{
	assert(nValue >= 0 && nValue < types_num);
	return DistortionTypes[nValue];
}

static const char *PresetNames[Distortion::presets_num] = {
	"Init",
	"Overdrive 1",
	"Overdrive 2",
	"A. Exciter 1",
	"A. Exciter 2",
	"Guitar Amp",
	"Quantisize",
};

const char *Distortion::ToPresetNameChar(int nValue)
{
	assert(nValue >= 0 && nValue < presets_num);
	return PresetNames[nValue];
}

std::string Distortion::ToPresetName(int nValue, int nWidth)
{
	return ToPresetNameChar(nValue);
}

int Distortion::ToIDFromPreset(const char *preset)
{
	for (int i = 0; i < presets_num; ++i)
		if (strcmp(PresetNames[i], preset) == 0)
			return i;

	return 0;
}

void Distortion::loadpreset(int npreset)
{
	const signed char presets[presets_num][ParameterCount] = {
		{
			// Init
			[ParameterMix] = 0,
			[ParameterPanning] = 64,
			[ParameterDrive] = 56,
			[ParameterLevel] = 70,
			[ParameterType] = WaveShapeArctangent,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterStereo] = 1,
			[ParameterLRCross] = 0,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
		{
			// Overdrive 1
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterDrive] = 56,
			[ParameterLevel] = 70,
			[ParameterType] = WaveShapeArctangent,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 51,
			[ParameterStereo] = 0,
			[ParameterLRCross] = 35,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
		{
			// Overdrive 2
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterDrive] = 29,
			[ParameterLevel] = 75,
			[ParameterType] = WaveShapeAsymmetric,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterStereo] = 0,
			[ParameterLRCross] = 35,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
		{
			// A. Exciter 1
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterDrive] = 75,
			[ParameterLevel] = 80,
			[ParameterType] = WaveShapeZigzag,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 54,
			[ParameterHighcut] = 60,
			[ParameterStereo] = 1,
			[ParameterLRCross] = 35,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
		{
			// A. Exciter 2
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterDrive] = 85,
			[ParameterLevel] = 62,
			[ParameterType] = WaveShapeAsymmetric,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 59,
			[ParameterHighcut] = 60,
			[ParameterStereo] = 1,
			[ParameterLRCross] = 35,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
		{
			// Guitar Amp
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterDrive] = 63,
			[ParameterLevel] = 75,
			[ParameterType] = WaveShapePow,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 32,
			[ParameterStereo] = 0,
			[ParameterLRCross] = 35,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
		{
			// Quantisize
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterDrive] = 3,
			[ParameterLevel] = 75,
			[ParameterType] = WaveShapeQuantisize,
			[ParameterNegate] = 0,
			[ParameterFiltering] = FilteringPost,
			[ParameterLowcut] = 0,
			[ParameterHighcut] = 60,
			[ParameterStereo] = 1,
			[ParameterLRCross] = 35,
			[ParameterShape] = 32,
			[ParameterOffset] = 64,
		},
	};

	if (npreset >= presets_num)
		npreset = presets_num - 1;

	for (int n = 0; n < ParameterCount; n++)
		changepar(n, presets[npreset][n]);

	cleanup();
}

void Distortion::changepar(int npar, int value)
{
	signed char cValue = static_cast<signed char>(value);
	switch (npar)
	{
	case ParameterMix:
		setmix(cValue);
		break;
	case ParameterPanning:
		setpanning(cValue);
		break;
	case ParameterDrive:
		Pdrive = cValue;
		break;
	case ParameterLevel:
		setlevel(cValue);
		break;
	case ParameterType:
		Ptype = cValue > 16 ? 16 : cValue;
		break;
	case ParameterNegate:
		Pnegate = (cValue > 1) ? 1 : cValue;
		break;
	case ParameterFiltering:
		Pfiltering = cValue > FilteringPost ? FilteringPost : cValue;
		break;
	case ParameterLowcut:
		setlowcut(cValue);
		break;
	case ParameterHighcut:
		sethighcut(cValue);
		break;
	case ParameterStereo:
		Pstereo = (cValue > 1) ? 1 : cValue;
		break;
	case ParameterLRCross:
		setlrcross(cValue);
		break;
	case ParameterShape:
		Pshape = cValue;
		break;
	case ParameterOffset:
		Poffset = cValue;
		break;
	}
}

int Distortion::getpar(int npar) const
{
	switch (npar)
	{
	case ParameterMix:
		return Pmix;
	case ParameterPanning:
		return Ppanning;
	case ParameterDrive:
		return Pdrive;
	case ParameterLevel:
		return Plevel;
	case ParameterType:
		return Ptype;
	case ParameterNegate:
		return Pnegate;
	case ParameterFiltering:
		return Pfiltering;
	case ParameterLowcut:
		return Plowcut;
	case ParameterHighcut:
		return Phighcut;
	case ParameterStereo:
		return Pstereo;
	case ParameterLRCross:
		return Plrcross;
	case ParameterShape:
		return Pshape;
	case ParameterOffset:
		return Poffset;
	default:
		return 0;
	}
}

} // namespace zyn
