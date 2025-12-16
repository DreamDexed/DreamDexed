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

#include "APhaser.h"

#define PHASER_LFO_SHAPE 2
#define ONE_  0.99999f        // To prevent LFO ever reaching 1.0 for filter stability purposes
#define ZERO_ 0.00001f        // Same idea as above.

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

		float lxn = smpsl[i];
		float rxn = smpsr[i];

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
			lyn1[j] += DENORMAL_GUARD;
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
			ryn1[j] += DENORMAL_GUARD;
			hpfr = ryn1[j] + (1.0f-rgain)*rxn1[j];  //high pass filter

			rxn1[j] = rxn;
			rxn = ryn1[j];
			if (j==1) rxn += fbr;  //Insert feedback after first phase stage
		};

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

void APhaser::setwidth(int Pwidth)
{
	this->Pwidth = Pwidth;
	width = ((float)Pwidth / 127.0f);
};

void APhaser::setfb(int Pfb)
{
	this->Pfb = Pfb;
	fb = ((float)Pfb - 64.0f) / 64.2f;
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

void APhaser::setstages(int Pstages)
{
	if (Pstages >= MAX_PHASER_STAGES)
		Pstages = MAX_PHASER_STAGES ;
	this->Pstages = Pstages;

	cleanup ();
};

void APhaser::setdepth(int Pdepth)
{
	this->Pdepth = Pdepth;
	depth = (float)(Pdepth - 64) / 127.0f;  //Pdepth input should be 0-127.  depth shall range 0-0.5 since we don't need to shift the full spectrum.
};

void APhaser::loadpreset(unsigned npreset)
{
	const int PRESET_SIZE = 13;
	int presets[presets_num][PRESET_SIZE] = {
		{0, 20, 14, 0, 1, 64, 110, 40, 4, 10, 0, 64, 1},
		{50, 20, 14, 0, 1, 64, 110, 40, 4, 10, 0, 64, 1},
		{50, 20, 14, 5, 1, 64, 110, 40, 6, 10, 0, 70, 1},
		{50, 20, 9, 0, 0, 64, 40, 40, 8, 10, 0, 60, 0},
		{50, 20, 14, 10, 0, 64, 110, 80, 7, 10, 1, 45, 1},
		{20, 20, 240, 10, 0, 64, 25, 16, 8, 100, 0, 25, 0},
		{50, 20, 1, 10, 1, 64, 110, 40, 12, 10, 0, 70, 1}
	};

	if (npreset >= presets_num)
		npreset = presets_num;

	for (int n = 0; n < PRESET_SIZE; n++)
		changepar(n, presets[npreset][n]);

	Ppreset = npreset;
};

void APhaser::changepar(unsigned npar, int value)
{
	switch (npar) {
	case ParameterMix:
		setmix(value);
		break;
	case ParameterDistortion:
		setdistortion(value);
		break;
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
	case ParameterWidth:
		setwidth(value);
		break;
	case ParameterFeedback:
		setfb(value);
		break;
	case ParameterStages:
		setstages(value);
		break;
	case ParameterMismatch:
		setmismatch(value);
		break;
	case ParameterSubtractive:
		if (value > 1)
			value = 1;
		Psubtractive = value;
		break;
	case ParameterDepth:
		setdepth(value);
		break;
	case ParameterHyper:
		if (value > 1)
			value = 1;
		Phyper = value;
		break;
	};
};

int APhaser::getpar(unsigned npar)
{
	switch (npar) {
	case ParameterMix: return Pmix;
	case ParameterDistortion: return Pdistortion;
	case ParameterLFOFreq: return lfo.Pfreq;
	case ParameterLFORandomness: return lfo.Prandomness;
	case ParameterLFOType: return lfo.PLFOtype;
	case ParameterLFOLRDelay: return lfo.Pstereo;
	case ParameterWidth: return Pwidth;
	case ParameterFeedback: return Pfb;
	case ParameterStages: return Pstages;
	case ParameterMismatch: return Pmismatch;
	case ParameterSubtractive: return Psubtractive;
	case ParameterDepth: return Pdepth;
	case ParameterHyper: return Phyper;
	default: return 0;
	};
};
