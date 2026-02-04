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

namespace zyn
{

class EffectLFO
{
public:
	EffectLFO(float samplerate);
	void effectlfoout(float *outl, float *outr, float phaseOffset = 0.0f);
	void updateparams(int period);
	int nPeriod;

	signed char Pfreq; //!< Frequency parameter (0-127)
	signed char Prandomness; //!< Randomness parameter (0-127)
	signed char PLFOtype; //!< LFO type parameter
	signed char Pstereo; //!< Stereo parameter (64 = 0)

	static std::string ToLFOType(int nValue, int nWidth);

private:
	float getlfoshape(float x);

	float xl, xr;
	float incx;
	float ampl1, ampl2, ampr1, ampr2; // necesar pentru "randomness"
	float lfornd;
	int lfotype;

	float samplerate;
};

} // namespace zyn
