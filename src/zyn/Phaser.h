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

#include "EffectLFO.h"

namespace zyn {

class Phaser
{
public:
	Phaser(float samplerate);
	void process(float *smpsl, float *smpsr, uint16_t period);
	void loadpreset(unsigned npreset);
	void changepar(unsigned npar, int value);
	int getpar(unsigned npar);
	void cleanup();

	std::atomic<bool> bypass;

	static constexpr unsigned max_stages = 12;
	static constexpr unsigned presets_num = 7;
	enum Parameter {
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
	static const char * ToPresetNameChar(int nValue);
	static unsigned ToIDFromPreset(const char *preset);

private:
	EffectLFO lfo; 		//Phaser modulator

	unsigned Ppreset;

	//Parametrii Phaser
	int Pmix;
	int Ppanning;
	int Pdepth;		//the depth of the Phaser
	int Pfb;		//feedback
	int Plrcross;		//feedback
	int Pstages;
	int Psubtractive;		//if I wish to substract the output instead of the adding it
	int Pphase;

	void setmix(int Pmix);
	void setpanning(int Ppanning);
	void setdepth(int Pdepth);
	void setfb(int Pfb);
	void setlrcross(int Plrcross);
	void setstages(int Pstages);
	void setphase(int Pphase);

	//Valorile interne
	float dry, wet, panl, panr, fb, depth, lrcross, fbl, fbr, phase;
	float oldl[max_stages * 2];
	float oldr[max_stages * 2];
	float oldlgain, oldrgain;
};

}
