/*
  ZynAddSubFX - a software synthesizer

  APhaser.h - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu and Ryan Billing

  Further modified for rakarrack by Ryan Billing (Transmogrifox) to model Analog Phaser behavior 2009

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

class APhaser
{
public:
	APhaser(float samplerate);
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
		ParameterWidth,
		ParameterDistortion,
		ParameterMismatch,
		ParameterHyper,
		ParameterCount
	};
	static std::string ToPresetName(int nValue, int nWidth);

private:
	EffectLFO lfo;		//Phaser modulator

	unsigned Ppreset;

	//Phaser parameters
	int Pmix;		//Used in Process.C to set wet/dry mix
	int Ppanning;
	int Pdepth;		//Depth of phaser sweep
	int Pfb;		//feedback
	int Pstages;		//Number of first-order All-Pass stages
	int Plrcross;
	int Psubtractive;		//if I wish to subtract the output instead of the adding it
	int Pwidth;		//Phaser width (LFO amplitude)
	int Pdistortion;	//Model distortion added by FET element
	int Pmismatch;		//Model mismatch between variable resistors
	int Phyper;		//lfo^2 -- converts tri into hyper-sine

	//Control parameters
	void setmix(int Pmix);
	void setpanning(int Ppanning);
	void setdepth(int Pdepth);
	void setfb(int Pfb);
	void setstages(int Pstages);
	void setlrcross(int Plrcross);
	void setwidth(int Pwidth);
	void setdistortion(int Pdistortion);
	void setmismatch(int Pmismatch);

	//Internal Variables
	bool barber;			//Barber pole phasing flag
	float dry, wet, panl, panr, depth, fb, lrcross, width, distortion, mismatchpct;
	float lxn1[max_stages];
	float lyn1[max_stages];
	float rxn1[max_stages];
	float ryn1[max_stages];
	const float offset[max_stages];	//model mismatch between JFET devices
	float oldlgain, oldrgain, fbl, fbr;

	const float Rmin;	// 2N5457 typical on resistance at Vgs = 0
	const float Rmx;	// Rmin/Rmax to avoid division in loop
	const float CFs;	// A constant derived from capacitor and resistor relationships
};

}
