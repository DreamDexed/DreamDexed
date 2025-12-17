/*
  ZynAddSubFX - a software synthesizer

  Chorus.h - Chorus and Flange effects
  Copyright (C) 2002-2005 Nasca Octavian Paul
  Author: Nasca Octavian Paul

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.
*/

#pragma once

#include <atomic>

#include "EffectLFO.h"

namespace zyn {

class Chorus
{
public:
	enum ChorusModes {
		ModeDefault,
		ModeFlange,	// flanger mode (very short delays)
		ModeDual,	// 180° dual phase chorus
		ModeTriple,	// 120° triple phase chorus
		ModeCount,
	};

	Chorus(float samplerate);
	void process(float *inputL, float *inputR, uint16_t period);
	void loadpreset(unsigned char npreset);
	void changepar(int npar, unsigned char value);
	unsigned char getpar(int npar) const;
	void cleanup();

	std::atomic<bool> bypass;

	static constexpr float max_delay_time = 250.0f; // ms
	static constexpr int delay_size = max_delay_time / 1000 * 192000;
	static constexpr unsigned presets_num = 13;

	enum Parameter {
		ParameterMix,
		ParameterPanning,
		ParameterLFOFreq,
		ParameterLFORandomness,
		ParameterLFOType,
		ParameterLFOLRDelay,
		ParameterDepth,
		ParameterDelay,
		ParameterFeedback,
		ParameterLRCross,
		ParameterMode,
		ParameterSubtractive,
		ParameterCount,
	};

	static std::string ToChorusMode(int nValue, int nWidth);
	static std::string ToPresetName(int nValue, int nWidth);
	static const char * ToPresetNameChar(int nValue);
	static unsigned ToIDFromPreset(const char *preset);

private:
	inline float getSample(float* delayline, float mdel, int dk);

	float samplerate;
	EffectLFO lfo;			//lfo-ul chorus

	//Chorus Parameters
	unsigned char Ppreset;
	unsigned char Pmix;
	unsigned char Ppanning;
	unsigned char Pdepth;		//the depth of the Chorus(ms)
	unsigned char Pdelay;		//the delay (ms)
	unsigned char Pfb;		//feedback
	unsigned char Plrcross;		//left/right cross
	unsigned char Pflangemode;	//mode as described above in CHORUS_MODES
	unsigned char Psubtractive;	//if I wish to subtract the output instead of the adding it

	//Parameter Controls
	void setmix(unsigned char _Pmix);
	void setpanning(unsigned char _Ppanning);
	void setdepth(unsigned char _Pdepth);
	void setdelay(unsigned char _Pdelay);
	void setfb(unsigned char _Pfb);
	void setlrcross(unsigned char _Plrcross);

	//Internal Values
	float dry, wet, panl, panr, depth, delay, fb, lrcross;
	float dlNew, drNew;
	float dlNew2, drNew2;
	float dlNew3, drNew3;
	int maxdelay;
	float delaySampleL[delay_size];
	float delaySampleR[delay_size];
	int dlk, drk;
	float getdelay(float xlfo);
};

}
