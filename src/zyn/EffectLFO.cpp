/*
  ZynAddSubFX - a software synthesizer

  EffectLFO.C - Stereo LFO used by some effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  Modified for rakarrack by Josep Andreu 6 Ryan Billing


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

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <string>

#include "EffectLFO.h"
#include "f_sin.h"

namespace zyn {

#define RND (rand()/(RAND_MAX+1.0))

EffectLFO::EffectLFO(float samplerate):
nPeriod{256}, //this is our best guess at what it will be, later we'll correct it when we actually know fPERIOD
Pfreq{40},
Prandomness{0},
Pstereo{64},
xl{},
xr{},
ampl1{(float)RND},
ampl2{(float)RND},
ampr1{(float)RND},
ampr2{(float)RND},
lfornd{},
samplerate{samplerate}
{
	updateparams(nPeriod);
}

std::string EffectLFO::ToLFOType(int nValue, int nWidth)
{
	switch(nValue)
	{
		case 0: return "Sine";
		case 1: return "Triangle";
		default: return "Invalid";
	}
}

void EffectLFO::updateparams(uint16_t period)
{
	float lfofreq = (powf(2.0f, Pfreq / 127.0f * 10.0f) - 1.0f) * 0.03f;
	nPeriod = period;
	incx = fabsf(lfofreq) * (float)period / samplerate;
	if (incx > 0.49999999f)
		incx = 0.499999999f;		//Limit the Frequency

	lfornd = (float)Prandomness / 127.0f;
	lfornd = (lfornd > 1.0f) ? 1.0f : lfornd;

	if (PLFOtype > 1)   //this has to be updated if more lfo's are added
		PLFOtype = 1;		
	lfotype = PLFOtype;
	xr = xl + (Pstereo - 64.0f) / 127.0f + 1.0f;
	xr -= floorf(xr);
}

float EffectLFO::getlfoshape(float x)
{
	float out;

	if (x > 1.0f) x -= 1.0f;

	switch (lfotype) {
	case 0: //EffectLFO_SINE
		out = f_cos(x * D_PI);
		break;
	case 1: //EffectLFO_TRIANGLE
		if (x > 0.0f && x < 0.25f)
			out = 4.0f * x;
		else if (x > 0.25f && x < 0.75f)
			out = 2.0f - 4.0f * x;
		else
			out = 4.0f * x - 4.0f;
		break;
	default:
		assert(false);
		break;
	};
	return out;
}

void EffectLFO::effectlfoout(float *outl, float *outr, float phaseOffset)
{
	float out;

	// left stereo signal
	out = getlfoshape(xl + phaseOffset);
	out *= ampl1 + xl * (ampl2 - ampl1);
	*outl = (out + 1.0f) * 0.5f;

	// update left phase for master lfo
	if (phaseOffset == 0.0f) {
		xl += incx;
		if(xl > 1.0f) {
			xl -= 1.0f;
			ampl1 = ampl2;
			ampl2 = (1.0f - lfornd) + lfornd * (float)RND;
		}
	}

	// right stereo signal
	else out = getlfoshape(xr + phaseOffset);
	out *= ampr1 + xr * (ampr2 - ampr1);
	*outr = (out + 1.0f) * 0.5f;

	// update right phase for master lfo
	if(phaseOffset == 0.0f) {
		xr += incx;
		if(xr > 1.0f) {
			xr -= 1.0f;
			ampr1 = ampr2;
			ampr2 = (1.0f - lfornd) + lfornd * (float)RND;
		}
	}
}

}
