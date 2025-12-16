/*
  ZynAddSubFX - a software synthesizer

  EffectLFO.h - Stereo LFO used by some effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu & Ryan Billing


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

#include <string>

#include "global.h"

class EffectLFO
{
public:
	EffectLFO(float sample_rate);
	void effectlfoout(float * outl, float * outr);
	void updateparams(uint16_t period);
	uint16_t nPeriod;
	int Pfreq;
	int Prandomness;
	int PLFOtype;
	int Pstereo;	//"64"=0

	static std::string ToLFOType(int nValue, int nWidth);
private:
	float getlfoshape(float x);

	float xl, xr;
	float incx;
	float ampl1, ampl2, ampr1, ampr2;	//necesar pentru "randomness"
	float lfointensity;
	float lfornd;
	int lfotype;

	//Lorenz Fractal parameters
	float x0,y0,z0,x1,y1,z1,radius;
	float h;
	float a;
	float b;
	float c;
	float scale;
	float iperiod;
	float ratediv;

	//Sample/Hold
	int holdflag;  //toggle left/right channel changes
	float tca, tcb, maxrate;
	float rreg, lreg, xlreg,xrreg, oldrreg, oldlreg;

	float fSAMPLE_RATE;
};
