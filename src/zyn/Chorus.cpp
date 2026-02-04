/*
  ZynAddSubFX - a software synthesizer

  Chorus.cpp - Chorus and Flange effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <string>

#include "Chorus.h"

#ifndef PI
#define PI 3.141592653589793f
#endif

namespace zyn {

constexpr float PHASE_120 = 0.33333333f;
constexpr float PHASE_180 = 0.5f;
constexpr float PHASE_240 = 0.66666666f;

Chorus::Chorus(float samplerate):
bypass{},
samplerate{samplerate},
lfo{samplerate},
Ppreset{},
dlNew{}, drNew{},
dlNew2{}, drNew2{},
dlNew3{}, drNew3{},
maxdelay{(int)(max_delay_time / 1000.0f * samplerate)},
dlk{}, drk{}
{
	loadpreset(Ppreset);
	cleanup();
}

//get the delay value in samples; xlfo is the current lfo value
float Chorus::getdelay(float xlfo)
{
	float result = (Pflangemode == ModeFlange) ? 0 : (delay + xlfo * depth) * samplerate;

	//check if delay is too big (caused by bad setdelay() and setdepth()
	if (result + 0.5f >= maxdelay) {
		result = maxdelay - 1.0f;
	}
	return result;
}

float cinterpolate(const float *data, size_t len, float pos)
{
	const unsigned int i_pos = (int)pos;
	const unsigned int l_pos = i_pos % len;
	const unsigned int r_pos = (l_pos + 1) < len ? l_pos + 1 : 0;
	const float rightness = pos - (float)i_pos;
	return data[l_pos] + (data[r_pos] - data[l_pos]) * rightness;
}

inline float Chorus::getSample(float* delayline, float mdel, int dk)
{
	float samplePos = dk - mdel + float(maxdelay * 2); //where should I get the sample from
	return cinterpolate(delayline, maxdelay, samplePos);
}

void Chorus::process(float *inputL, float *inputR, uint16_t period)
{
	if (bypass) return;

	if (wet == 0.0f) return;

	if (lfo.nPeriod != period) lfo.updateparams(period);

	float lfol, lfor;
	float output;
	// store old delay value for linear interpolation
	float dlHist = dlNew, drHist = drNew;
	float dlHist2 = dlNew2, drHist2 = drNew2;
	float dlHist3 = dlNew3, drHist3 = drNew3;

	// calculate new lfo values
	lfo.effectlfoout(&lfol, &lfor);
	// calculate new delay values
	dlNew = getdelay(lfol);
	drNew = getdelay(lfor);
	float fbComp = fb;
	
	switch (Pflangemode) {
	case ModeDual: // ensemble mode
		// same for second member for ensemble mode with 180° phase offset
		lfo.effectlfoout(&lfol, &lfor, PHASE_180);
		dlNew2 = getdelay(lfol);
		drNew2 = getdelay(lfor);
		fbComp /= 2.0f;
		break;
	case ModeTriple: // ensemble mode
		// same for second member for ensemble mode with 120° phase offset
		lfo.effectlfoout(&lfol, &lfor, PHASE_120);
		dlNew2 = getdelay(lfol);
		drNew2 = getdelay(lfor);

		// same for third member for ensemble mode with 240° phase offset
		lfo.effectlfoout(&lfol, &lfor, PHASE_240);
		dlNew3 = getdelay(lfol);
		drNew3 = getdelay(lfor);
		// reduce amplitude to match single phase modes
		// 0.85 * fbComp / 3
		fbComp /= 3.53f;
		break;
	default:
		break;
	}

	for(int i = 0; i < period; ++i) {
		float inL = inputL[i];
		float inR = inputR[i];

		//LRcross
		float tmpL = inL;
		float tmpR = inR;
		inL = tmpL * (1.0f - lrcross) + tmpR * lrcross;
		inR = tmpR * (1.0f - lrcross) + tmpL * lrcross;

		//Left channel

		// increase delay line writing position and handle turnaround
		if(++dlk >= maxdelay)
			dlk = 0;
		// linear interpolate from old to new value over length of the buffer
		float dl = (dlHist * (period - i) + dlNew * i) / period;
		// get sample with that delay from delay line and add to output accumulator
		output = getSample(delaySampleL, dl, dlk);

		switch (Pflangemode) {
		case ModeDual:
			// calculate and apply delay for second ensemble member
			dl = (dlHist2 * (period - i) + dlNew2 * i) / period;
			output += getSample(delaySampleL, dl, dlk);
			break;
		case ModeTriple:
			// calculate and apply delay for second ensemble member
			dl = (dlHist2 * (period - i) + dlNew2 * i) / period;
			output += getSample(delaySampleL, dl, dlk);
			// same for third ensemble member
			dl = (dlHist3 * (period - i) + dlNew3 * i) / period;
			output += getSample(delaySampleL, dl, dlk);
			// reduce amplitude to match single phase modes
			output *= 0.85f;
			break;
		default:
			// nothing to do for standard chorus
			break;
		}

		delaySampleL[dlk] = inL + output * fbComp;

		if (Psubtractive) output *= -1.0f;
		output *= panl;
		inputL[i] = inputL[i] * dry + output * wet;

		//Right channel

		// increase delay line writing position and handle turnaround
		if(++drk >= maxdelay)
			drk = 0;
		// linear interpolate from old to new value over length of the buffer
		float dr = (drHist * (period - i) + drNew * i) / period;
		output = getSample(delaySampleR, dr, drk);

		switch (Pflangemode) {
		case ModeDual:
			// calculate and apply delay for second ensemble member
			dr = (drHist2 * (period - i) + drNew2 * i) / period;
			output += getSample(delaySampleR, dr, drk);
			break;
		case ModeTriple:
			// calculate and apply delay for second ensemble member
			dr = (drHist2 * (period - i) + drNew2 * i) / period;
			output += getSample(delaySampleR, dr, drk);
			// same for third ensemble member
			dr = (drHist3 * (period - i) + drNew3 * i) / period;
			output += getSample(delaySampleR, dr, drk);
			// reduce amplitude to match single phase modes
			output *= 0.85f;
			break;
		default:
			// nothing to do for standard chorus
			break;
		}

		delaySampleR[drk] = inR + output * fbComp;

		if (Psubtractive) output *= -1.0f;
		output *= panr;
		inputR[i] = inputR[i] * dry + output * wet;
	}
}

void Chorus::cleanup()
{
	memset(delaySampleL, 0, sizeof delaySampleL);
	memset(delaySampleR, 0, sizeof delaySampleR);
}

void Chorus::setdepth(unsigned char _Pdepth)
{
	Pdepth = _Pdepth;
	depth  = (powf(8.0f, (Pdepth / 127.0f) * 2.0f) - 1.0f) / 1000.0f; //seconds
}

void Chorus::setdelay(unsigned char _Pdelay)
{
	Pdelay = _Pdelay;
	delay  = (powf(10.0f, (Pdelay / 127.0f) * 2.0f) - 1.0f) / 1000.0f; //seconds
}

void Chorus::setfb(unsigned char _Pfb)
{
	Pfb = _Pfb;
	fb  = (Pfb - 64.0f) / 64.1f;
}

void Chorus::setlrcross(unsigned char _Plrcross)
{
	Plrcross = _Plrcross;
	lrcross = (float)Plrcross / 127.0f;
};

void Chorus::setmix(unsigned char _Pmix)
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

void Chorus::setpanning(unsigned char _Ppanning)
{
	Ppanning = _Ppanning;
	float panning = ((float)Ppanning - .5f)/ 127.0f;
	panl = cosf(panning * PI / 2.0f);
	panr = cosf((1.0f - panning) * PI / 2.0f);
};

std::string Chorus::ToChorusMode(int nValue, int nWidth)
{
	switch(nValue) {
	case ModeDefault: return "Default";
	case ModeFlange: return "Flange";
	case ModeDual: return "Dual";
	case ModeTriple: return "Triple";
	default: return "Invalid";
	}
}

std::string Chorus::ToPresetName(int nValue, int nWidth)
{
	return ToPresetNameChar(nValue);
}

static const char *PresetNames[Chorus::presets_num] = {
	"Init",
	"Chorus1",
	"Chorus2",
	"Chorus3",
	"Celeste1",
	"Celeste2",
	"Flange1",
	"Flange2",
	"Flange3",
	"Flange4",
	"Flange5",
	"Ensemble1",
	"Ensemble2",
};

const char * Chorus::ToPresetNameChar(int nValue)
{
	assert (nValue >= 0 && (unsigned)nValue < presets_num);
	return PresetNames[nValue];
}

unsigned Chorus::ToIDFromPreset(const char *preset)
{
	for (unsigned i = 0; i < presets_num; ++i)
		if (strcmp(PresetNames[i], preset) == 0)
			return i;

	return 0;
}

void Chorus::loadpreset(unsigned char npreset)
{
	const unsigned char presets[presets_num][ParameterCount] = {
		{	//Init
			[ParameterMix] = 0,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 14,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 40,
			[ParameterDelay] = 85,
			[ParameterFeedback] = 64,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Chorus1
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 50,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 90,
			[ParameterDepth] = 40,
			[ParameterDelay] = 85,
			[ParameterFeedback] = 64,
			[ParameterLRCross] = 119,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Chorus2
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 45,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 98,
			[ParameterDepth] = 56,
			[ParameterDelay] = 90,
			[ParameterFeedback] = 64,
			[ParameterLRCross] = 19,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Chorus3
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 29,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 42,
			[ParameterDepth] = 97,
			[ParameterDelay] = 95,
			[ParameterFeedback] = 90,
			[ParameterLRCross] = 127,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Celeste1
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 26,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 42,
			[ParameterDepth] = 115,
			[ParameterDelay] = 18,
			[ParameterFeedback] = 90,
			[ParameterLRCross] = 127,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Celeste2
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 29,
			[ParameterLFORandomness] = 117,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 50,
			[ParameterDepth] = 115,
			[ParameterDelay] = 9,
			[ParameterFeedback] = 31,
			[ParameterLRCross] = 127,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 1,
		},
		{	//Flange1
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 57,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 60,
			[ParameterDepth] = 23,
			[ParameterDelay] = 3,
			[ParameterFeedback] = 62,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Flange2
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 33,
			[ParameterLFORandomness] = 34,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 40,
			[ParameterDepth] = 35,
			[ParameterDelay] = 3,
			[ParameterFeedback] = 109,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Flange3
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 53,
			[ParameterLFORandomness] = 34,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 94,
			[ParameterDepth] = 35,
			[ParameterDelay] = 3,
			[ParameterFeedback] = 54,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 1,
		},
		{	//Flange4
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 40,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 62,
			[ParameterDepth] = 12,
			[ParameterDelay] = 19,
			[ParameterFeedback] = 97,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 0,
		},
		{	//Flange5
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 55,
			[ParameterLFORandomness] = 105,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 24,
			[ParameterDepth] = 39,
			[ParameterDelay] = 19,
			[ParameterFeedback] = 17,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDefault,
			[ParameterSubtractive] = 1,
		},
		{	//Ensemble1
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 68,
			[ParameterLFORandomness] = 25,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 24,
			[ParameterDepth] = 35,
			[ParameterDelay] = 55,
			[ParameterFeedback] = 64,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeTriple,
			[ParameterSubtractive] = 0,
		},
		{	//Ensemble2
			[ParameterMix] = 100,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 55,
			[ParameterLFORandomness] = 25,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 24,
			[ParameterDepth] = 32,
			[ParameterDelay] = 55,
			[ParameterFeedback] = 80,
			[ParameterLRCross] = 0,
			[ParameterMode] = ModeDual,
			[ParameterSubtractive] = 0,
		},
	};

	if (npreset >= presets_num)
		npreset = presets_num - 1;

	for (int n = 0; n < ParameterCount; n++)
		changepar(n, presets[npreset][n]);

	Ppreset = npreset;
}

void Chorus::changepar(int npar, unsigned char value)
{
	switch(npar) {
	case ParameterMix: setmix(value); break;
	case ParameterPanning: setpanning(value); break;
	case ParameterLFOFreq:
		lfo.Pfreq = value;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterLFORandomness:
		lfo.Prandomness = value;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterLFOType:
		lfo.PLFOtype = value;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterLFOLRDelay:
		lfo.Pstereo = value;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterDepth: setdepth(value); break;
	case ParameterDelay: setdelay(value); break;
	case ParameterFeedback: setfb(value); break;
	case ParameterLRCross: setlrcross(value); break;
	case ParameterMode:
		lfo.updateparams(lfo.nPeriod);
		Pflangemode = (value >= ModeCount) ? ModeCount - 1 : value;
		break;
	case ParameterSubtractive: Psubtractive = (value > 1) ? 1 : value; break;
	}
}

unsigned char Chorus::getpar(int npar) const
{
	switch(npar) {
	case ParameterMix: return Pmix;
	case ParameterPanning: return Ppanning;
	case ParameterLFOFreq: return lfo.Pfreq;
	case ParameterLFORandomness: return lfo.Prandomness;
	case ParameterLFOType: return lfo.PLFOtype;
	case ParameterLFOLRDelay: return lfo.Pstereo;
	case ParameterDepth: return Pdepth;
	case ParameterDelay: return Pdelay;
	case ParameterFeedback: return Pfb;
	case ParameterLRCross: return Plrcross;
	case ParameterMode: return Pflangemode;
	case ParameterSubtractive: return Psubtractive;
	default: return 0;
	}
}

}
