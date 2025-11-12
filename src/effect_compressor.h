/*
 * Chip Audette's Compressor
 * https://github.com/chipaudette/OpenAudio_ArduinoLibrary/blob/master/AudioEffectCompressor_F32.h
 */

#include <atomic>

#include "compressor.h"

class AudioEffectCompressor
{
public:
	static const unsigned CompressorRatioInf = 31;

	AudioEffectCompressor(float samplerate):
	bypass{},
	samplerate{samplerate},
	compL{samplerate},
	compR{samplerate}
	{}

	void setPreGain_dB(float gain)
	{
		compL.setPreGain_dB(gain);
		compR.setPreGain_dB(gain);
	}

	void setThresh_dBFS(float thresh)
	{
		compL.setThresh_dBFS(thresh);
		compR.setThresh_dBFS(thresh);
	}

	void setCompressionRatio(float ratio)
	{
		ratio = ratio == CompressorRatioInf ? INFINITY : ratio;

		compL.setCompressionRatio(ratio);
		compR.setCompressionRatio(ratio);
	}

	void setAttack_sec(float sec)
	{
		compL.setAttack_sec(sec, samplerate);
		compR.setAttack_sec(sec, samplerate);
	}

	void setRelease_sec(float sec)
	{
		compL.setRelease_sec(sec, samplerate);
		compR.setRelease_sec(sec, samplerate);
	}

	void setMakeupGain_dB(float gain)
	{
		compL.setMakeupGain_dB(gain);
		compR.setMakeupGain_dB(gain);
	}

	void enableHPFilter(bool hpfilter)
	{
		compL.enableHPFilter(hpfilter);
		compR.enableHPFilter(hpfilter);
	}

	void resetState()
	{
		compL.resetStates();
		compR.resetStates();
	}

	void process(float32_t* blockL, float32_t* blockR, uint16_t len)
	{
		if (bypass) return;

		compL.doCompression (blockL, len);
		compR.doCompression (blockR, len);
    	}

	std::atomic<bool> bypass;

private:
	const float samplerate;

	Compressor compL;
	Compressor compR;
};
