/*

  APhaser.C  - Approximate digital model of an analog JFET phaser.
  Analog modeling implemented by Ryan Billing aka Transmogrifox.
  November, 2009

  Credit to:
  ///////////////////
  ZynAddSubFX - a software synthesizer

  Phaser.C - Phaser effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu

  DSP analog modeling theory & practice largely influenced by various CCRMA publications, particularly works by Julius O. Smith.
  ////////////////////


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

#include <cstring>
#include <math.h>

#include "APhaser.h"

namespace zyn {

#define PHASER_LFO_SHAPE 2
#define ONE_  0.99999f        // To prevent LFO ever reaching 1.0 for filter stability purposes
#define ZERO_ 0.00001f        // Same idea as above.

#ifndef PI
#define PI 3.141592653589793f
#endif

APhaser::APhaser(float samplerate):
bypass{},
lfo{samplerate},
Ppreset{},
barber{},
dry{1.0f},
wet{},
offset{-0.2509303f, 0.9408924f, 0.998f, -0.3486182f, -0.2762545f, -0.5215785f, 0.2509303f, -0.9408924f, -0.998f, 0.3486182f,  0.2762545f, 0.5215785f},
Rmin{625.0f}, // 2N5457 typical on resistance at Vgs = 0
Rmx{Rmin / 22000.0f /* Rmax Resistor parallel to FET */},
CFs{2.0f * samplerate * 0.00000005f /* C 50 nF*/}
{
	loadpreset(Ppreset);//this will get done before out is run
	cleanup();
};

std::string APhaser::ToPresetName(int nValue, int nWidth)
{
	return nValue == 0 ? "INIT" : std::to_string(nValue);
}

void APhaser::process(float *smpsl, float *smpsr, uint16_t period)
{
	if (bypass) return;

	if (wet == 0.0f) return;

	if (lfo.nPeriod != period) lfo.updateparams(period);

	//initialize hpf
	float hpfl = 0.0;
	float hpfr = 0.0;

	float lfol, lfor;
	lfo.effectlfoout (&lfol, &lfor);
	float lmod = lfol*width + depth;
	float rmod = lfor*width + depth;

	if (lmod > ONE_) lmod = ONE_;
	else if (lmod < ZERO_) lmod = ZERO_;

	if (rmod > ONE_) rmod = ONE_;
	else if (rmod < ZERO_) rmod = ZERO_;

	if (Phyper != 0) {
		lmod *= lmod;  //Triangle wave squared is approximately sin on bottom, tri on top
		rmod *= rmod;  //Result is exponential sweep more akin to filter in synth with exponential generator circuitry.
	};

	lmod = sqrtf(1.0f - lmod);  //gl,gr is Vp - Vgs. Typical FET drain-source resistance follows constant/[1-sqrt(Vp - Vgs)]
	rmod = sqrtf(1.0f - rmod);

	float invperiod = 1.0f / period;
	float rdiff = (rmod - oldrgain) * invperiod;
	float ldiff = (lmod - oldlgain) * invperiod;

	float gl = oldlgain;
	float gr = oldrgain;

	oldlgain = lmod;
	oldrgain = rmod;

	for (unsigned int i = 0; i < period; i++) {

		gl += ldiff;	// Linear interpolation between LFO samples
		gr += rdiff;

		float lxn = smpsl[i] * panl;
		float rxn = smpsr[i] * panr;

		if (barber) {
			gl = fmodf((gl + 0.25f) , ONE_);
			gr = fmodf((gr + 0.25f) , ONE_);
		};

		//Left channel
		for (int j = 0; j < Pstages; j++) {
			//Phasing routine
			float mis = 1.0f + mismatchpct*offset[j];
			float d = (1.0f + 2.0f*(0.25f + gl)*hpfl*hpfl*distortion) * mis;  //This is symmetrical. FET is not, so this deviates slightly, however sym dist. is better sounding than a real FET.
			float Rconst =  1.0f + mis*Rmx;
			float bl = (Rconst - gl) / (d*Rmin);  // This is 1/R. R is being modulated to control filter fc.
			float lgain = (CFs - bl)/(CFs + bl);

			lyn1[j] = lgain * (lxn + lyn1[j]) - lxn1[j];
			hpfl = lyn1[j] + (1.0f-lgain)*lxn1[j];  //high pass filter -- Distortion depends on the high-pass part of the AP stage.

			lxn1[j] = lxn;
			lxn = lyn1[j];
			if (j==1) lxn += fbl;  //Insert feedback after first phase stage
		};

		//Right channel
		for (int j = 0; j < Pstages; j++) {
			//Phasing routine
			float mis = 1.0f + mismatchpct*offset[j];
			float d = (1.0f + 2.0f*(0.25f + gr)*hpfr*hpfr*distortion) * mis;   // distortion
			float Rconst =  1.0f + mis*Rmx;
			float br = (Rconst - gr )/ (d*Rmin);
			float rgain = (CFs - br)/(CFs + br);

			ryn1[j] = rgain * (rxn + ryn1[j]) - rxn1[j];
			hpfr = ryn1[j] + (1.0f-rgain)*rxn1[j];  //high pass filter

			rxn1[j] = rxn;
			rxn = ryn1[j];
			if (j==1) rxn += fbr;  //Insert feedback after first phase stage
		};

		// LR cross
		float l = lxn;
		float r = rxn;
		lxn = l * (1.0f - lrcross) + r * lrcross;
		rxn = r * (1.0f - lrcross) + l * lrcross;

		fbl = lxn * fb;
		fbr = rxn * fb;

		if (Psubtractive != 0) {
			lxn *= -1.0f;
			rxn *= -1.0f;
		}

		smpsl[i] = smpsl[i] * dry + lxn * wet;
		smpsr[i] = smpsr[i] * dry + rxn * wet;
	};
};

void APhaser::cleanup()
{
	fbl = fbr = 0.0f;
	oldlgain = oldrgain = 0.0f;
	memset(lxn1, 0, sizeof lxn1);
	memset(lyn1, 0, sizeof lyn1);
	memset(rxn1, 0, sizeof rxn1);
	memset(ryn1, 0, sizeof ryn1);
};

void APhaser::setmix(int Pmix)
{
	this->Pmix = Pmix;

	float mix = (float)Pmix / 100.0f;
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

void APhaser::setpanning(int Ppanning)
{
	this->Ppanning = Ppanning;
	float panning = ((float)Ppanning - .5f)/ 127.0f;
	panl = cosf(panning * PI / 2.0f);
	panr = cosf((1.0f - panning) * PI / 2.0f);
};

void APhaser::setdepth(int Pdepth)
{
	this->Pdepth = Pdepth;
	depth = (float)(Pdepth - 64) / 127.0f;  //Pdepth input should be 0-127.  depth shall range 0-0.5 since we don't need to shift the full spectrum.
};

void APhaser::setfb(int Pfb)
{
	this->Pfb = Pfb;
	fb = ((float)Pfb - 64.0f) / 64.2f;
};

void APhaser::setstages(int Pstages)
{
	if ((unsigned int)Pstages > max_stages)
		Pstages = max_stages;
	this->Pstages = Pstages;

	cleanup ();
};

void APhaser::setlrcross(int Plrcross)
{
	this->Plrcross = Plrcross;
	lrcross = (float)Plrcross / 127.0f;
};

void APhaser::setwidth(int Pwidth)
{
	this->Pwidth = Pwidth;
	width = ((float)Pwidth / 127.0f);
};

void APhaser::setdistortion(int Pdistortion)
{
	this->Pdistortion = Pdistortion;
	distortion = (float)Pdistortion / 127.0f;
};

void APhaser::setmismatch(int Pmismatch)
{
	this->Pmismatch = Pmismatch;
	mismatchpct = (float)Pmismatch / 127.0f;
};

void APhaser::loadpreset(unsigned npreset)
{
	const int presets[presets_num][ParameterCount] = {
		{
			[ParameterMix] = 0,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 14,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 64,
			[ParameterFeedback] = 40,
			[ParameterStages] = 4,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterWidth] = 110,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 10,
			[ParameterHyper] = 1,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 14,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 64,
			[ParameterFeedback] = 40,
			[ParameterStages] = 4,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterWidth] = 110,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 10,
			[ParameterHyper] = 1,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 14,
			[ParameterLFORandomness] = 5,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 70,
			[ParameterFeedback] = 40,
			[ParameterStages] = 6,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterWidth] = 110,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 10,
			[ParameterHyper] = 1,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 9,
			[ParameterLFORandomness] = 0,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 60,
			[ParameterFeedback] = 40,
			[ParameterStages] = 8,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterWidth] = 40,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 10,
			[ParameterHyper] = 0,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 14,
			[ParameterLFORandomness] = 10,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 45,
			[ParameterFeedback] = 80,
			[ParameterStages] = 7,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 1,
			[ParameterWidth] = 110,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 10,
			[ParameterHyper] = 1,
		},
		{
			[ParameterMix] = 20,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 240,
			[ParameterLFORandomness] = 10,
			[ParameterLFOType] = 0,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 25,
			[ParameterFeedback] = 16,
			[ParameterStages] = 8,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterWidth] = 15,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 100,
			[ParameterHyper] = 0,
		},
		{
			[ParameterMix] = 50,
			[ParameterPanning] = 64,
			[ParameterLFOFreq] = 1,
			[ParameterLFORandomness] = 10,
			[ParameterLFOType] = 1,
			[ParameterLFOLRDelay] = 64,
			[ParameterDepth] = 70,
			[ParameterFeedback] = 40,
			[ParameterStages] = 12,
			[ParameterLRCross] = 0,
			[ParameterSubtractive] = 0,
			[ParameterWidth] = 110,
			[ParameterDistortion] = 20,
			[ParameterMismatch] = 10,
			[ParameterHyper] = 1,
		},
	};

	if (npreset >= presets_num)
		npreset = presets_num;

	for (int n = 0; n < ParameterCount; n++)
		changepar(n, presets[npreset][n]);

	Ppreset = npreset;
};

void APhaser::changepar(unsigned npar, int value)
{
	switch (npar) {
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
		barber = 0;
		if (value == 2) barber = 1;
		break;
	case ParameterLFOLRDelay:
		lfo.Pstereo = value;
		lfo.updateparams(lfo.nPeriod);
		break;
	case ParameterDepth: setdepth(value); break;
	case ParameterFeedback: setfb(value); break;
	case ParameterStages: setstages(value); break;
	case ParameterLRCross: setlrcross(value); break;
	case ParameterSubtractive: Psubtractive = value > 1 ? 1 : value; break;
	case ParameterWidth: setwidth(value); break;
	case ParameterDistortion: setdistortion(value); break;
	case ParameterMismatch: setmismatch(value); break;
	case ParameterHyper: Phyper = value > 1 ? 1 : value; break;
	};
};

int APhaser::getpar(unsigned npar)
{
	switch (npar) {
	case ParameterMix: return Pmix;
	case ParameterPanning: return Ppanning;
	case ParameterLFOFreq: return lfo.Pfreq;
	case ParameterLFORandomness: return lfo.Prandomness;
	case ParameterLFOType: return lfo.PLFOtype;
	case ParameterLFOLRDelay: return lfo.Pstereo;
	case ParameterDepth: return Pdepth;
	case ParameterFeedback: return Pfb;
	case ParameterStages: return Pstages;
	case ParameterLRCross: return Plrcross;
	case ParameterSubtractive: return Psubtractive;
	case ParameterWidth: return Pwidth;
	case ParameterDistortion: return Pdistortion;
	case ParameterMismatch: return Pmismatch;
	case ParameterHyper: return Phyper;
	default: return 0;
	};
};

}
