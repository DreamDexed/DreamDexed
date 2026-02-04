/*
  ZynAddSubFX - a software synthesizer

  Phaser.h - Phaser effect
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

#pragma once

#include <atomic>
#include <string>

#include "EffectLFO.h"

namespace zyn
{

class Phaser
{
public:
	Phaser(float samplerate);
	void process(float *smpsl, float *smpsr, int period);
	void loadpreset(int npreset);
	void changepar(int npar, int value);
	int getpar(int npar);
	void cleanup();

	std::atomic<bool> bypass;

	static constexpr int max_stages = 12;
	static constexpr int presets_num = 7;
	enum Parameter
	{
		ParameterMix,
		ParameterPanning,
		ParameterLFOFreq,
		ParameterLFORandomness,
		ParameterLFOType,
		ParameterLFOLRDelay,
		ParameterDepth,
		ParameterFeedback,
		ParameterStages,
		ParameterLRCross,
		ParameterSubtractive,
		ParameterPhase,
		ParameterCount
	};
	static std::string ToPresetName(int nValue, int nWidth);
	static const char *ToPresetNameChar(int nValue);
	static int ToIDFromPreset(const char *preset);

private:
	EffectLFO lfo; // Phaser modulator

	int Ppreset;

	// Parametrii Phaser
	signed char Pmix;
	signed char Ppanning;
	signed char Pdepth; // the depth of the Phaser
	signed char Pfb; // feedback
	signed char Plrcross; // feedback
	signed char Pstages;
	signed char Psubtractive; // if I wish to substract the output instead of the adding it
	signed char Pphase;

	void setmix(signed char Pmix);
	void setpanning(signed char Ppanning);
	void setdepth(signed char Pdepth);
	void setfb(signed char Pfb);
	void setlrcross(signed char Plrcross);
	void setstages(signed char Pstages);
	void setphase(signed char Pphase);

	// Valorile interne
	float dry, wet, panl, panr, fb, depth, lrcross, fbl, fbr, phase;
	float oldl[max_stages * 2];
	float oldr[max_stages * 2];
	float oldlgain, oldrgain;
};

} // namespace zyn
