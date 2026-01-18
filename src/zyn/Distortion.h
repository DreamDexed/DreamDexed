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
#include <cstdint>
#include <string>

#include "AnalogFilter.h"

namespace zyn {

class Distortion
{
public:
	Distortion(float samplerate);
	void process(float *inputL, float *inputR, uint16_t period);
	void loadpreset(unsigned char npreset);
	void changepar(int npar, unsigned char value);
	unsigned char getpar(int npar) const;
	void cleanup();

	std::atomic<bool> bypass;

	static constexpr unsigned presets_num = 7;
	static constexpr unsigned types_num = 17;

	enum Parameter {
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
		ParameterFuncPar,
		ParameterOffset,
		ParameterCount,
	};

	enum Filtering {
		FilteringPre,
		FilteringPost,
	};

	static std::string ToDistortionType(int nValue, int nWidth);
	static std::string ToPresetName(int nValue, int nWidth);
	static const char * ToPresetNameChar(int nValue);
	static unsigned ToIDFromPreset(const char *preset);

private:
	void applyfilters(float *inputL, float *inputR, uint16_t period);


	float samplerate;

	unsigned char Ppreset;

	//Parameters
	unsigned char Pmix;
	unsigned char Ppanning;
	unsigned char Pdrive;        //the input amplification
	unsigned char Plevel;        //the output amplification
	unsigned char Ptype;         //Distortion type
	unsigned char Pnegate;       //if the input is negated
	unsigned char Pfiltering;    //if you want to do the filtering before or after the distortion
	unsigned char Plowcut;       //lowcut
	unsigned char Phighcut;      //higcut
	unsigned char Pstereo;       //0=mono, 1=stereo
	unsigned char Plrcross;      //L/R mix
	unsigned char Pfuncpar;      //for parametric functions
	unsigned char Poffset;       //the input offset

	void setmix(unsigned char _Pmix);
	void setlowcut(unsigned char _Plowcut);
	void sethighcut(unsigned char _Phighcut);
	void setpanning(unsigned char _Ppanning);
	void setlevel(unsigned char _Plevel);
	void setlrcross(unsigned char _Plrcross);

	//Real Parameters
	AnalogFilter lpfl, lpfr, hpfl, hpfr;

	float dry, wet, panl, panr, level, lrcross;
};

}
