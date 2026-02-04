/*
  ZynAddSubFX - a software synthesizer

  Distortion.h - Distortion Effect
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once

#include <atomic>
#include <string>

#include "AnalogFilter.h"

namespace zyn
{

class Distortion
{
public:
	Distortion(float samplerate);
	void process(float *inputL, float *inputR, int period);
	void loadpreset(int npreset);
	void changepar(int npar, int value);
	int getpar(int npar) const;
	void cleanup();

	std::atomic<bool> bypass;

	static constexpr int presets_num = 7;
	static constexpr int types_num = 17;

	enum Parameter
	{
		ParameterMix,
		ParameterPanning,
		ParameterDrive,
		ParameterLevel,
		ParameterType,
		ParameterNegate,
		ParameterFiltering,
		ParameterLowcut,
		ParameterHighcut,
		ParameterStereo,
		ParameterLRCross,
		ParameterShape,
		ParameterOffset,
		ParameterCount,
	};

	enum Filtering
	{
		FilteringPre,
		FilteringPost,
	};

	static std::string ToDistortionType(int nValue, int nWidth);
	static std::string ToPresetName(int nValue, int nWidth);
	static const char *ToPresetNameChar(int nValue);
	static int ToIDFromPreset(const char *preset);

private:
	void applyfilters(float *inputL, float *inputR, int period);

	float samplerate;

	int Ppreset;

	// Parameters
	signed char Pmix;
	signed char Ppanning;
	signed char Pdrive; // the input amplification
	signed char Plevel; // the output amplification
	signed char Ptype; // Distortion type
	signed char Pnegate; // if the input is negated
	signed char Pfiltering; // if you want to do the filtering before or after the distortion
	signed char Plowcut; // lowcut
	signed char Phighcut; // higcut
	signed char Pstereo; // 0=mono, 1=stereo
	signed char Plrcross; // L/R mix
	signed char Pshape; // for waveshaper shape
	signed char Poffset; // the input offset

	void setmix(signed char _Pmix);
	void setlowcut(signed char _Plowcut);
	void sethighcut(signed char _Phighcut);
	void setpanning(signed char _Ppanning);
	void setlevel(signed char _Plevel);
	void setlrcross(signed char _Plrcross);

	// Real Parameters
	AnalogFilter lpfl, lpfr, hpfl, hpfr;

	float dry, wet, panl, panr, level, lrcross;
};

} // namespace zyn
