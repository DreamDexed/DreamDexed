/*
  ZynAddSubFX - a software synthesizer

  Phaser.C - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2 of the GNU General Public License
  as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License (version 2) for more details.

  You should have received a copy of the GNU General Public License (version 2)
  along with this program; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA

*/

#include "Phaser.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <string>

namespace zyn
{

#define PHASER_LFO_SHAPE 2

#ifndef PI
#define PI 3.141592653589793f
#endif

Phaser::Phaser(float samplerate) :
bypass{},
lfo{samplerate},
Ppreset{},
dry{1.0f},
wet{}
{
	loadpreset(Ppreset);
	cleanup();
};

void Phaser::process(float *smpsl, float *smpsr, int period)
{
	if (bypass) return;

	if (wet == 0.0f) return;

	if (lfo.nPeriod != period) lfo.updateparams(period);

	float lgain, rgain;
	lfo.effectlfoout(&lgain, &rgain);

	lgain = (expf(lgain * PHASER_LFO_SHAPE) - 1.0f) / (expf(PHASER_LFO_SHAPE) - 1.0f);
	rgain = (expf(rgain * PHASER_LFO_SHAPE) - 1.0f) / (expf(PHASER_LFO_SHAPE) - 1.0f);

	lgain = 1.0f - phase * (1.0f - depth) - (1.0f - phase) * lgain * depth;
	rgain = 1.0f - phase * (1.0f - depth) - (1.0f - phase) * rgain * depth;

	if (lgain > 1.0f)
		lgain = 1.0f;
	else if (lgain < 0.0f)
		lgain = 0.0f;

	if (rgain > 1.0f)
		rgain = 1.0f;
	else if (rgain < 0.0f)
		rgain = 0.0f;

	for (int i = 0; i < period; i++)
	{
		float x = float(i) / period;
		float x1 = 1.0f - x;
		float gl = lgain * x + oldlgain * x1;
		float gr = rgain * x + oldrgain * x1;
		float inl = smpsl[i] * panl + fbl;
		float inr = smpsr[i] * panr + fbr;

		// Left channel
		for (int j = 0; j < Pstages * 2; j++)
		{
			// Phasing routine
			float tmp = oldl[j];
			oldl[j] = gl * tmp + inl;
			inl = tmp - gl * oldl[j];
		};
		// Right channel
		for (int j = 0; j < Pstages * 2; j++)
		{
			// Phasing routine
			float tmp = oldr[j];
			oldr[j] = (gr * tmp) + inr;
			inr = tmp - (gr * oldr[j]);
		};
		// Left/Right crossing
		float l = inl;
		float r = inr;
		inl = l * (1.0f - lrcross) + r * lrcross;
		inr = r * (1.0f - lrcross) + l * lrcross;

		fbl = inl * fb;
		fbr = inr * fb;

		if (Psubtractive != 0)
		{
			inl *= -1.0f;
			inr *= -1.0f;
		}

		smpsl[i] = smpsl[i] * dry + inl * wet;
		smpsr[i] = smpsr[i] * dry + inr * wet;
	};

	oldlgain = lgain;
	oldrgain = rgain;
};

void Phaser::cleanup()
{
	fbl = fbr = 0.0f;
	oldlgain = oldrgain = 0.0f;
	memset(oldl, 0, sizeof oldl);
	memset(oldr, 0, sizeof oldr);
};

void Phaser::setdepth(signed char Pdepth)
{
	this->Pdepth = Pdepth;
	depth = Pdepth / 127.0f;
};

void Phaser::setfb(signed char Pfb)
{
	this->Pfb = Pfb;
	fb = (Pfb - 64.0f) / 64.1f;
};

void Phaser::setmix(signed char Pmix)
{
	this->Pmix = Pmix;

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
};

void Phaser::setpanning(signed char Ppanning)
{
	this->Ppanning = Ppanning;
	float panning = (Ppanning - .5f) / 127.0f;
	panl = cosf(panning * PI / 2.0f);
	panr = cosf((1.0f - panning) * PI / 2.0f);
};

void Phaser::setlrcross(signed char Plrcross)
{
	this->Plrcross = Plrcross;
	lrcross = Plrcross / 127.0f;
};

void Phaser::setstages(signed char Pstages)
{
	if (Pstages > max_stages)
		Pstages = max_stages;
	this->Pstages = Pstages;
	cleanup();
};

void Phaser::setphase(signed char Pphase)
{
	this->Pphase = Pphase;
	phase = Pphase / 127.0f;
};

static const char *PresetNames[Phaser::presets_num] = {
	"Init",
	"Phaser1",
	"Phaser2",
	"Phaser3",
	"Phaser4",
	"Phaser5",
	"Phaser6",
};

const char *Phaser::ToPresetNameChar(int nValue)
{
	assert(nValue >= 0 && nValue < presets_num);
	return PresetNames[nValue];
}

std::string Phaser::ToPresetName(int nValue, int nWidth)
{
	return ToPresetNameChar(nValue);
}

int Phaser::ToIDFromPreset(const char *preset)
{
	for (int i = 0; i < presets_num; ++i)
		if (strcmp(PresetNames[i], preset) == 0)
			return i;

	return 0;
}

void Phaser::loadpreset(int npreset)
{
	const signed char presets[presets_num][ParameterCount] = {
		{
			[ParameterMix] = 0,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 36,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 110,
			[ParameterFeedback] = 64,
			[ParameterStages] = 1,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterPhase] = 20,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 36,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 110,
			[ParameterFeedback] = 64,
			[ParameterStages] = 1,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterPhase] = 20,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 35,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 88,
			[ParameterDepth] = 40,
			[ParameterFeedback] = 64,
			[ParameterStages] = 3,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterPhase] = 20,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 31,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 66,
			[ParameterDepth] = 68,
			[ParameterFeedback] = 107,
			[ParameterStages] = 2,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterPhase] = 20,
		},
		{
			[ParameterMix] = 31,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 22,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 66,
			[ParameterDepth] = 67,
			[ParameterFeedback] = 10,
			[ParameterStages] = 5,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 1,
			[ParameterPhase] = 20,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 20,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 110,
			[ParameterDepth] = 67,
			[ParameterFeedback] = 78,
			[ParameterStages] = 10,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterPhase] = 20,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 53,
			[ParameterLFORandomness] = 100,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 58,
			[ParameterDepth] = 37,
			[ParameterFeedback] = 78,
			[ParameterStages] = 3,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterPhase] = 20,
		},
	};

	if (npreset >= presets_num)
		npreset = presets_num - 1;

	for (int n = 0; n < ParameterCount; n++)
		changepar(n, presets[npreset][n]);

	Ppreset = npreset;
};

void Phaser::changepar(int npar, int value)
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
	case ParameterLFOFreq:
		lfo.Pfreq = cValue;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterLFORandomness:
		lfo.Prandomness = cValue;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterLFOType:
		lfo.PLFOtype = cValue;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterLFOLRDelay:
		lfo.Pstereo = cValue;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterDepth:
		setdepth(cValue);
		break;
	case ParameterFeedback:
		setfb(cValue);
		break;
	case ParameterStages:
		setstages(cValue);
		break;
	case ParameterLRCross:
		setlrcross(cValue);
		break;
	case ParameterSubtractive:
		Psubtractive = cValue > 1 ? 1 : cValue;
		break;
	case ParameterPhase:
		setphase(cValue);
		break;
	}
};

int Phaser::getpar(int npar)
{
	switch (npar)
	{
	case ParameterMix:
		return Pmix;
	case ParameterPanning:
		return Ppanning;
	case ParameterLFOFreq:
		return lfo.Pfreq;
	case ParameterLFORandomness:
		return lfo.Prandomness;
	case ParameterLFOType:
		return lfo.PLFOtype;
	case ParameterLFOLRDelay:
		return lfo.Pstereo;
	case ParameterDepth:
		return Pdepth;
	case ParameterFeedback:
		return Pfb;
	case ParameterStages:
		return Pstages;
	case ParameterLRCross:
		return Plrcross;
	case ParameterSubtractive:
		return Psubtractive;
	case ParameterPhase:
		return Pphase;
	default:
		return 0;
	}
};

} // namespace zyn
