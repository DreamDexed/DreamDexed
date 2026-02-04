#pragma once

#include <atomic>

#include "common.h"
#include "effect_lpf.h"

class AudioEffectDreamDelay
{
public:
	enum Mode
	{
		DUAL,
		CROSSOVER,
		PINGPONG,
		MODE_UNKNOWN
	};
	enum Sync
	{
		SYNC_NONE,
		T1_1,
		T1_1T,
		T1_2,
		T1_2T,
		T1_4,
		T1_4T,
		T1_8,
		T1_8T,
		T1_16,
		T1_16T,
		T1_32,
		T1_32T,
		SYNC_UNKNOWN
	};

	AudioEffectDreamDelay(float samplerate);
	~AudioEffectDreamDelay();

	void setMode(Mode m) { mode = m; }
	void setTimeL(float time);
	void setTimeLSync(Sync sync);
	void setTimeR(float time);
	void setTimeRSync(Sync sync);
	void setFeedback(float fb) { feedback = constrain(fb, 0.0f, 1.0f); }
	void setHighCut(float highcut) { lpf.setCutoff_Hz(highcut); }
	void setTempo(int t);
	void setMix(float mix);

	Mode getMode() { return mode; }
	float getTimeL() { return timeL; }
	float getTimeR() { return timeR; }
	float getFeedback() { return feedback; }
	float getHighCut() { return lpf.getCutoff_Hz(); }
	int getTempo() { return tempo; }
	float getMix() { return mix; }

	void process(float *blockL, float *blockR, int len);

	void resetState();

	std::atomic<bool> bypass;

private:
	float samplerate;

	Mode mode;

	int bufferSize;
	float *bufferL;
	float *bufferR;
	int index;
	int indexDL;
	int indexDR;

	float timeL; // Left delay time in seconds
	float timeR; // Right delay time in seconds
	Sync timeLSync;
	Sync timeRSync;

	float feedback; // 0.0 - 1.0

	AudioEffectLPF lpf;

	int tempo;

	float mix;
	float dry;
	float wet;
};
