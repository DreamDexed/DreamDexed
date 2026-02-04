/*
 * Stereo Low Pass Filter
 * Ported from (https://github.com/jnonis/MiniDexed)
 */

#pragma once

#include "common.h"

class AudioEffectLPF
{
public:
	static constexpr float MIN_CUTOFF = 0.00001f;
	static constexpr float MAX_CUTOFF = 20000.0f;
	static constexpr float MIN_RES = 0.0f;
	static constexpr float MAX_RES = 1.0f;

	struct LPFState
	{
		float y1;
		float y2;
		float y3;
		float y4;
		float oldx;
		float oldy1;
		float oldy2;
		float oldy3;
	};

	AudioEffectLPF(float samplerate, float cutoff_Hz, float resonance) :
	samplerate{samplerate},
	cutoff{constrain(cutoff_Hz, MIN_CUTOFF, MAX_CUTOFF)},
	resonance{constrain(resonance, MIN_RES, MAX_RES)},
	stateL{},
	stateR{}
	{
		recalculate();
	}

	float getCutoff_Hz() { return cutoff; }

	void setCutoff_Hz(float value)
	{
		cutoff = constrain(value, MIN_CUTOFF, MAX_CUTOFF);
		recalculate();
	}

	void setResonance(float value)
	{
		resonance = constrain(value, MIN_RES, MAX_RES);
		recalculate();
	}

	float processSampleL(float input)
	{
		return processSample(input, &stateL);
	}

	float processSampleR(float input)
	{
		return processSample(input, &stateR);
	}

	void process(float *blockL, float *blockR, int len)
	{
		for (int i = 0; i < len; i++)
		{
			blockL[i] = processSample(blockL[i], &stateL);
			blockR[i] = processSample(blockR[i], &stateR);
		}
	}

	void resetState()
	{
		stateL = {};
		stateR = {};
	}

private:
	void recalculate()
	{
		float f = (cutoff + cutoff) / samplerate;
		p = f * (1.8 - (0.8 * f));
		k = p + p - 1.0;

		float t = (1.0 - p) * 1.386249;
		float t2 = 12.0 + t * t;
		r = resonance * (t2 + 6.0 * t) / (t2 - 6.0 * t);
	}

	float processSample(float input, LPFState *state)
	{
		float y1 = state->y1;
		float y2 = state->y2;
		float y3 = state->y3;
		float y4 = state->y4;
		float oldx = state->oldx;
		float oldy1 = state->oldy1;
		float oldy2 = state->oldy2;
		float oldy3 = state->oldy3;

		// Process input
		float x = input - r * y4;

		// Four cascaded one pole filters (bilinear transform)
		y1 = x * p + oldx * p - k * y1;
		y2 = y1 * p + oldy1 * p - k * y2;
		y3 = y2 * p + oldy2 * p - k * y3;
		y4 = y3 * p + oldy3 * p - k * y4;

		// Clipper band limited sigmoid
		y4 -= (y4 * y4 * y4) / 6.0;

		state->y1 = y1;
		state->y2 = y2;
		state->y3 = y3;
		state->y4 = y4;
		state->oldx = x;
		state->oldy1 = y1;
		state->oldy2 = y2;
		state->oldy3 = y3;

		return y4;
	}

	float samplerate;
	float cutoff;
	float resonance;
	float r, p, k;
	LPFState stateL;
	LPFState stateR;
};
