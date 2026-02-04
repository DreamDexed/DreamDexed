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
#include <string>

#include "EffectLFO.h"

namespace zyn
{

class APhaser
{
public:
	APhaser(float samplerate);
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
		ParameterWidth,
		ParameterDistortion,
		ParameterMismatch,
		ParameterHyper,
		ParameterCount
	};
	static std::string ToPresetName(int nValue, int nWidth);
	static const char *ToPresetNameChar(int nValue);
	static int ToIDFromPreset(const char *preset);

private:
	EffectLFO lfo; // Phaser modulator

	int Ppreset;

	signed char Pmix; // Used in Process.C to set wet/dry mix
	signed char Ppanning;
	signed char Pdepth;  // Depth of phaser sweep
	signed char Pfb;     // feedback
	signed char Pstages; // Number of first-order All-Pass stages
	signed char Plrcross;
	signed char Psubtractive; // if I wish to subtract the output instead of the adding it
	signed char Pwidth;	  // Phaser width (LFO amplitude)
	signed char Pdistortion;  // Model distortion added by FET element
	signed char Pmismatch;	  // Model mismatch between variable resistors
	signed char Phyper;	  // lfo^2 -- converts tri into hyper-sine

	// Control parameters
	void setmix(signed char Pmix);
	void setpanning(signed char Ppanning);
	void setdepth(signed char Pdepth);
	void setfb(signed char Pfb);
	void setstages(signed char Pstages);
	void setlrcross(signed char Plrcross);
	void setwidth(signed char Pwidth);
	void setdistortion(signed char Pdistortion);
	void setmismatch(signed char Pmismatch);

	// Internal Variables
	float dry, wet, panl, panr, depth, fb, lrcross, width, distortion, mismatchpct;
	float lxn1[max_stages];
	float lyn1[max_stages];
	float rxn1[max_stages];
	float ryn1[max_stages];
	float oldlgain, oldrgain, fbl, fbr;

	const float CFs;  // A constant derived from capacitor and resistor relationships
};

} // namespace zyn
