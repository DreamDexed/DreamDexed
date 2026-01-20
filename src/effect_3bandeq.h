/* 
 * DISTHRO 3 Band EQ
 * Ported from https://github.com/DISTRHO/Mini-Series/blob/master/plugins/3BandEQ
 * Ported from https://github.com/jnonis/MiniDexed
 */

#pragma once

#include <atomic>

#include "effect_3bandeqmono.h"

class AudioEffect3BandEQ
{
public:
	AudioEffect3BandEQ(float samplerate):
	bypass{},
	eqL{samplerate},
	eqR{samplerate}
	{
	}

	void setLow_dB(float value)
	{
		eqL.setLow_dB(value);
		eqR.setLow_dB(value);
	}

	void setMid_dB(float value)
	{
		eqL.setMid_dB(value);
		eqR.setMid_dB(value);
	}

	void setHigh_dB(float value)
	{
		eqL.setHigh_dB(value);
		eqR.setHigh_dB(value);
	}

	void setGain_dB(float value)
	{
		eqL.setGain_dB(value);
		eqR.setGain_dB(value);
	}

	float setLowMidFreq(float value)
	{
		eqL.setLowMidFreq(value);
		return eqR.setLowMidFreq(value);
	}

	float setMidHighFreq(float value)
	{
		eqL.setMidHighFreq(value);
		return eqR.setMidHighFreq(value);
	}

	unsigned setLowMidFreq_n(unsigned value)
	{
		eqL.setLowMidFreq_n(value);
		return eqR.setLowMidFreq_n(value);
	}

	unsigned setMidHighFreq_n(unsigned value)
	{
		eqL.setMidHighFreq_n(value);
		return eqR.setMidHighFreq_n(value);
	}

	void setPreLowCut(float value)
	{
		eqL.setPreLowCut(value);
		eqR.setPreLowCut(value);
	}

	void setPreHighCut(float value)
	{
		eqL.setPreHighCut(value);
		eqR.setPreHighCut(value);
	}

	float getLow_dB() const { return eqR.getLow_dB(); }
	float getMid_dB() const { return eqR.getMid_dB(); }
	float getHigh_dB() const { return eqR.getHigh_dB(); }
	float getGain_dB() const { return eqR.getGain_dB(); }
	float getLowMidFreq() const { return eqR.getLowMidFreq(); }
	float getMidHighFreq() const { return eqR.getMidHighFreq(); }
	unsigned getLowMidFreq_n() const { return eqR.getLowMidFreq_n(); }
	unsigned getMidHighFreq_n() const { return eqR.getMidHighFreq_n(); }

	float getPreLowCut() { return eqR.getPreLowCut(); }
	float getPreHighCut() { return eqR.getPreHighCut(); }

	void resetState() { eqL.resetState(); eqR.resetState(); }

	void process(float32_t* blockL, float32_t* blockR, uint16_t len)
	{
		if (bypass) return;

		eqL.process(blockL, len);
		eqR.process(blockR, len);
	}

	std::atomic<bool> bypass;

private:
	AudioEffect3BandEQMono eqL;
	AudioEffect3BandEQMono eqR;
};
