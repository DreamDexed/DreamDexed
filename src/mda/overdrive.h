/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>
  Copyright 1999-2000 Paul Kellett (Maxim Digital Audio)

  This is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License,
  or (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this software. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <atomic>
#include <math.h>

namespace mda {

class Overdrive
{
public:
	enum Parameter {
		ParameterMix,
		ParameterDrive,
		ParameterMuffle,
		ParameterGain,
		ParameterBypass,
		ParameterCount
	};

	Overdrive(float samplerate):
	bypass{},
	samplerate{samplerate},
	drive{},
	filtL{},
	filtR{}
	{
		setMuffle(0.0f);
		setGain(0.5f);
		setMix(0.0f);
	}

	void setMuffle(float value)
	{
		muffle = value;

		// MDA EPiano uses 44100 Hz by default
		// modify the muffle based on the samplerate
		//float muffleSR = muffle * (44100.0f / samplerate);
		//filt = powf(10.0f, -1.6f * muffleSR);
		// Todo: need more testing

		filt = powf(10.0f, -1.6f * muffle);
	}

	void setGain(float value)
	{
		gain = value;
		// 0..1 -> -20dB .. 20dB
		outputGain = powf(10.0f, (gain - 0.5f) * 2.0f);
	}

	void setMix(float value)
	{
		mix = value;

		if (mix <= 0.5f)
		{
			dry = 1.0f;
			wet = mix * 2.0f;
		}
		else
		{
			dry = 1.0f - ((mix - 0.5f) * 2.0f);
			wet = 1.0f;
		}
	}

	void setParameter(int32_t index, float value)
	{
		switch(index) {
			case ParameterMix: setMix(value); break;
			case ParameterDrive: drive = value; break;
			case ParameterMuffle: setMuffle(value); break;
			case ParameterGain: setGain(value); break;
			case ParameterBypass: bypass = value; break;
		}
	}

	float getParameter(int32_t index)
	{
		switch(index) {
			case ParameterMix: return mix;
			case ParameterDrive: return drive;
			case ParameterMuffle: return muffle;
			case ParameterGain: return gain;
			case ParameterBypass: return bypass;
		}
	}

	void process(float *blockL, float *blockR, uint16_t len)
	{
		if (bypass) return;

		if (wet == 0.0f) return;
	
		float f=filt, fL=filtL, fR=filtR, d=drive, g=outputGain;

		while(len-- > 0)
		{
			float inL = *blockL;
			float inR = *blockR;

			// overdrive
			float odL = inL > 0.0f ? sqrtf(inL) : -sqrtf(-inL);
			float odR = inR > 0.0f ? sqrtf(inR) : -sqrtf(-inR);

			// filter
			fL = fL + f * (d * (odL - inL) + inL - fL);
			fR = fR + f * (d * (odR - inR) + inR - fR);

			*blockL = inL * dry + fL * g * wet;
			*blockR = inR * dry + fR * g * wet;

			blockL++;
			blockR++;
		}

		filtL = fL;
		filtR = fR;
	}

	void cleanup() { filtL = filtR = 0.0f; }

	std::atomic<bool> bypass;

private:
	float samplerate;

	float drive;
	float muffle;
	float gain;

	float filtL, filtR; //filter buffers
	float filt; //filter coeff
	float outputGain;

	float mix;
	float dry;
	float wet;
};

}
