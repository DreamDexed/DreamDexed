// Taken from https://github.com/manicken/Audio/tree/templateMixer
// Adapted for MiniDexed by Holger Wirtz <dcoredump@googlemail.com>

#pragma once

#include <cassert>
#include <cmath>

#include <dsp/support_functions.h>
#include <dsp/basic_math_functions.h>

#include "common.h"

#define UNITY_GAIN 1.0f
#define MAX_GAIN 1.0f
#define MIN_GAIN 0.0f
#define UNITY_PANORAMA 1.0f
#define MAX_PANORAMA 1.0f
#define MIN_PANORAMA 0.0f
#ifndef M_PI
#define M_PI 3.14159265358979323846264338327950288
#endif

void inline scale_ramp_f32(
const float *pSrc,
float *pScale,
float dScale,
float ramp,
float *pDst,
int blockSize)
{
	int blkCnt; /* Loop counter */
	float scale = *pScale;

	blkCnt = blockSize;

	while (blkCnt > 0)
	{
		if (scale != dScale)
		{
			scale = dScale > scale ? fmin(dScale, scale + ramp) : fmax(dScale, scale - ramp);
		}

		*pDst++ = *pSrc++ * scale;

		blkCnt--;
	}

	*pScale = scale;
}

template <int NN>
class AudioMixer
{
public:
	AudioMixer(int len, float samplerate) :
	buffer_length{len},
	sumbufL{new float[buffer_length]{}},
	ramp{10.0f / samplerate} // 100ms
	{
		for (int i = 0; i < NN; i++)
			multiplier[i] = UNITY_GAIN;
	}

	~AudioMixer()
	{
		delete[] sumbufL;
	}

	void doAddMix(int channel, float *in)
	{
		assert(channel >= 0 && channel < NN);
		assert(in);

		if (multiplier[channel] != UNITY_GAIN)
		{
			float tmp[buffer_length];
			arm_scale_f32(in, multiplier[channel], tmp, buffer_length);
			arm_add_f32(sumbufL, tmp, sumbufL, buffer_length);
		}
		else
		{
			arm_add_f32(sumbufL, in, sumbufL, buffer_length);
		}
	}

	void gain(int channel, float gain)
	{
		assert(channel >= 0 && channel < NN);

		gain = constrain(gain, MIN_GAIN, MAX_GAIN);
		multiplier[channel] = powf(gain, 2);
	}

	void gain(float gain)
	{
		gain = constrain(gain, MIN_GAIN, MAX_GAIN);
		gain = powf(gain, 2);

		for (int i = 0; i < NN; i++)
		{
			multiplier[i] = gain;
		}
	}

	void getMix(float *buffer)
	{
		assert(buffer);
		assert(sumbufL);
		arm_copy_f32(sumbufL, buffer, buffer_length);

		arm_fill_f32(0.0f, sumbufL, buffer_length);
	}

protected:
	float multiplier[NN];
	int buffer_length;
	float *sumbufL;
	const float ramp;
};

template <int NN>
class AudioStereoMixer : public AudioMixer<NN>
{
public:
	AudioStereoMixer(int len, float samplerate) :
	AudioMixer<NN>{len, samplerate},
	sumbufR{new float[buffer_length]{}}
	{
		for (int i = 0; i < NN; i++)
		{
			panorama[i][0] = UNITY_PANORAMA;
			panorama[i][1] = UNITY_PANORAMA;
			mp[i][0] = mp_w[i][0] = UNITY_GAIN * UNITY_PANORAMA;
			mp[i][1] = mp_w[i][1] = UNITY_GAIN * UNITY_PANORAMA;
		}
	}

	~AudioStereoMixer()
	{
		delete[] sumbufR;
	}

	void gain(int channel, float gain)
	{
		assert(channel >= 0 && channel < NN);

		gain = constrain(gain, MIN_GAIN, MAX_GAIN);
		multiplier[channel] = powf(gain, 2);

		mp_w[channel][0] = multiplier[channel] * panorama[channel][0];
		mp_w[channel][1] = multiplier[channel] * panorama[channel][1];
	}

	void gain(float gain)
	{
		gain = constrain(gain, MIN_GAIN, MAX_GAIN);
		gain = powf(gain, 2);

		for (int i = 0; i < NN; i++)
		{
			multiplier[i] = gain;

			mp_w[i][0] = multiplier[i] * panorama[i][0];
			mp_w[i][1] = multiplier[i] * panorama[i][1];
		}
	}

	void pan(int channel, float pan)
	{
		assert(channel >= 0 && channel < NN);

		pan = constrain(pan, MIN_PANORAMA, MAX_PANORAMA);

		// From: https://stackoverflow.com/questions/67062207/how-to-pan-audio-sample-data-naturally
		panorama[channel][0] = cosf(mapfloat(pan, MIN_PANORAMA, MAX_PANORAMA, 0.0, M_PI / 2.0));
		panorama[channel][1] = sinf(mapfloat(pan, MIN_PANORAMA, MAX_PANORAMA, 0.0, M_PI / 2.0));

		mp_w[channel][0] = multiplier[channel] * panorama[channel][0];
		mp_w[channel][1] = multiplier[channel] * panorama[channel][1];
	}

	void doAddMix(int channel, float *in)
	{
		assert(channel >= 0 && channel < NN);

		float tmp[buffer_length];

		assert(in);

		if (mp[channel][0] != 0.0f || mp_w[channel][0] != 0.0f)
		{
			if (mp[channel][0] == mp_w[channel][0])
				arm_scale_f32(in, mp[channel][0], tmp, buffer_length);
			else
				scale_ramp_f32(in, &mp[channel][0], mp_w[channel][0], ramp, tmp, buffer_length);

			arm_add_f32(sumbufL, tmp, sumbufL, buffer_length);
		}

		if (mp[channel][1] != 0.0f || mp_w[channel][1] != 0.0f)
		{
			if (mp[channel][1] == mp_w[channel][1])
				arm_scale_f32(in, mp[channel][1], tmp, buffer_length);
			else
				scale_ramp_f32(in, &mp[channel][1], mp_w[channel][1], ramp, tmp, buffer_length);

			arm_add_f32(sumbufR, tmp, sumbufR, buffer_length);
		}
	}

	void getMix(float *bufferL, float *bufferR)
	{
		assert(bufferR);
		assert(bufferL);
		assert(sumbufL);
		assert(sumbufR);

		arm_copy_f32(sumbufL, bufferL, buffer_length);
		arm_copy_f32(sumbufR, bufferR, buffer_length);

		if (sumbufL)
			arm_fill_f32(0.0f, sumbufL, buffer_length);
		if (sumbufR)
			arm_fill_f32(0.0f, sumbufR, buffer_length);
	}

	void getBuffers(float(*buffers[2]))
	{
		buffers[0] = sumbufL;
		buffers[1] = sumbufR;
	}

	void zeroFill()
	{
		if (sumbufL)
			arm_fill_f32(0.0f, sumbufL, buffer_length);
		if (sumbufR)
			arm_fill_f32(0.0f, sumbufR, buffer_length);
	}

protected:
	using AudioMixer<NN>::sumbufL;
	using AudioMixer<NN>::multiplier;
	using AudioMixer<NN>::buffer_length;
	using AudioMixer<NN>::ramp;
	float panorama[NN][2];
	float mp[NN][2];
	float mp_w[NN][2];
	float *sumbufR;
};
