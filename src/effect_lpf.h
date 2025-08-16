/* 
 * Stereo Low Pass Filter
 * Ported from (https://github.com/jnonis/MiniDexed)
 */

#pragma once

class AudioEffectLPF
{
public:
	static constexpr float32_t MIN_CUTOFF = 0.00001f;
	static constexpr float32_t MAX_CUTOFF = 20000.0f;
	static constexpr float32_t MIN_RES = 0.0f;
	static constexpr float32_t MAX_RES = 1.0f;

	struct LPFState
	{
		float32_t y1;
		float32_t y2;
		float32_t y3;
		float32_t y4;
		float32_t oldx;
		float32_t oldy1;
		float32_t oldy2;
		float32_t oldy3;
	};

	AudioEffectLPF(float32_t samplerate, float32_t cutoff_Hz, float32_t resonance):
	samplerate{samplerate},
	cutoff{constrain(cutoff_Hz, MIN_CUTOFF, MAX_CUTOFF)},
	resonance{constrain(resonance, MIN_RES, MAX_RES)},
	stateL{},
	stateR{}
	{
		recalculate();
	}

	float32_t getCutoff_Hz() { return cutoff; }

	void setCutoff_Hz(float32_t value)
	{
		cutoff = constrain(value, MIN_CUTOFF, MAX_CUTOFF); 
		recalculate();
	}

	void setResonance(float32_t value)
	{
		resonance = constrain(value, MIN_RES, MAX_RES);
		recalculate();
	}

	float32_t processSampleL(float32_t input)
	{
		return processSample(input, &stateL);
	}

	float32_t processSampleR(float32_t input)
	{
		return processSample(input, &stateR);
	}

    	void process(float32_t* blockL, float32_t* blockR, uint16_t len)
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
        	float32_t f = (cutoff + cutoff) / samplerate;
		p = f * (1.8 - (0.8 * f));
		k = p + p - 1.0;

		float32_t t = (1.0 - p) * 1.386249;
		float32_t t2 = 12.0 + t * t;
		r = resonance * (t2 + 6.0 * t) / (t2 - 6.0 * t);
	}

	float32_t processSample(float32_t input, LPFState* state)
	{
		float32_t y1 = state->y1;
		float32_t y2 = state->y2;
		float32_t y3 = state->y3;
		float32_t y4 = state->y4;
		float32_t oldx = state->oldx;
		float32_t oldy1 = state->oldy1;
		float32_t oldy2 = state->oldy2;
		float32_t oldy3 = state->oldy3;

		// Process input
		float32_t x = input - r * y4;
		
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

	float32_t samplerate;
	float32_t cutoff;
	float32_t resonance;
	float32_t r, p, k;
	LPFState stateL;
	LPFState stateR;
};
