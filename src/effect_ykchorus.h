/*
 * YK Chorus Port
 * Ported from https://github.com/SpotlightKid/ykchorus
 * Ported from https://github.com/jnonis/MiniDexed
 */

#pragma once

#include <atomic>

#include "common.h"
#include "ykchorus/ChorusEngine.h"

class AudioEffectYKChorus
{
public:
	AudioEffectYKChorus(float samplerate) :
	bypass{},
	engine{samplerate}
	{
		setChorus1(true);
		setChorus2(true);
		setChorus1LFORate(0.5f);
		setChorus2LFORate(0.83f);
		setMix(0.0f);
	}

	bool getChorus1() { return engine.isChorus1Enabled; }
	bool getChorus2() { return engine.isChorus2Enabled; }

	void setChorus1(bool enable) { engine.setEnablesChorus(enable, engine.isChorus2Enabled); }
	void setChorus2(bool enable) { engine.setEnablesChorus(engine.isChorus1Enabled, enable); }

	float getChorus1Rate() { return engine.chorus1L.rate; }
	float getChorus2Rate() { return engine.chorus2L.rate; }

	void setChorus1LFORate(float rate) { engine.setChorus1LfoRate(rate); }
	void setChorus2LFORate(float rate) { engine.setChorus2LfoRate(rate); }

	float getMix() { return mix; }

	void setMix(float value)
	{
		mix = constrain(value, 0.0f, 1.0f);

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

	void process(float *inblockL, float *inblockR, int len)
	{
		if (bypass) return;

		if (wet == 0.0f) return;

		if (!engine.isChorus1Enabled && !engine.isChorus2Enabled) return;

		for (int i = 0; i < len; i++)
		{
			engine.process(dry, wet, inblockL++, inblockR++);
		}
	}

	std::atomic<bool> bypass;

private:
	ChorusEngine engine;

	float mix, dry, wet;
};
