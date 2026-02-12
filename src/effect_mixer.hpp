// Taken from https://github.com/manicken/Audio/tree/templateMixer
// Adapted for MiniDexed by Holger Wirtz <dcoredump@googlemail.com>

#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>

#include <dsp/support_functions.h>

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
uint32_t blockSize)
{
	uint32_t blkCnt; /* Loop counter */
	float scale = *pScale;

	blkCnt = blockSize;

	while (blkCnt > 0U)
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
	AudioMixer(uint16_t len, float samplerate) :
	ramp{10.0f / samplerate} // 100ms
	{
		buffer_length = len;
		for (uint8_t i = 0; i < NN; i++)
			multiplier[i] = UNITY_GAIN;

		sumbufL = new float[buffer_length];
		arm_fill_f32(0.0f, sumbufL, len);
	}

	~AudioMixer()
	{
		delete[] sumbufL;
	}

	void doAddMix(uint8_t channel, float *in)
	{
		float tmp[buffer_length];

		assert(in);

		if (multiplier[channel] != UNITY_GAIN)
			arm_scale_f32(in, multiplier[channel], tmp, buffer_length);
		arm_add_f32(sumbufL, tmp, sumbufL, buffer_length);
	}

	void gain(uint8_t channel, float gain)
	{
		if (channel >= NN)
			return;

		if (gain > MAX_GAIN)
			gain = MAX_GAIN;
		else if (gain < MIN_GAIN)
			gain = MIN_GAIN;
		multiplier[channel] = powf(gain, 2);
	}

	void gain(float gain)
	{
		float gain4 = powf(gain, 2);

		for (uint8_t i = 0; i < NN; i++)
		{
			if (gain > MAX_GAIN)
				gain = MAX_GAIN;
			else if (gain < MIN_GAIN)
				gain = MIN_GAIN;
			multiplier[i] = gain4;
		}
	}

	void getMix(float *buffer)
	{
		assert(buffer);
		assert(sumbufL);
		arm_copy_f32(sumbufL, buffer, buffer_length);

		if (sumbufL)
			arm_fill_f32(0.0f, sumbufL, buffer_length);
	}

protected:
	float multiplier[NN];
	float *sumbufL;
	uint16_t buffer_length;
	const float ramp;
};

template <int NN>
class AudioStereoMixer : public AudioMixer<NN>
{
public:
	AudioStereoMixer(uint16_t len, float samplerate) : AudioMixer<NN>(len, samplerate)
	{
		for (uint8_t i = 0; i < NN; i++)
		{
			panorama[i][0] = UNITY_PANORAMA;
			panorama[i][1] = UNITY_PANORAMA;
			mp[i][0] = mp_w[i][0] = UNITY_GAIN * UNITY_PANORAMA;
			mp[i][1] = mp_w[i][1] = UNITY_GAIN * UNITY_PANORAMA;
		}

		sumbufR = new float[buffer_length];
		arm_fill_f32(0.0f, sumbufR, buffer_length);
	}

	~AudioStereoMixer()
	{
		delete[] sumbufR;
	}

	void gain(uint8_t channel, float gain)
	{
		if (channel >= NN)
			return;

		if (gain > MAX_GAIN)
			gain = MAX_GAIN;
		else if (gain < MIN_GAIN)
			gain = MIN_GAIN;
		multiplier[channel] = powf(gain, 2);

		mp_w[channel][0] = multiplier[channel] * panorama[channel][0];
		mp_w[channel][1] = multiplier[channel] * panorama[channel][1];
	}

	void gain(float gain)
	{
		float gain4 = powf(gain, 2);

		for (uint8_t i = 0; i < NN; i++)
		{
			if (gain > MAX_GAIN)
				gain = MAX_GAIN;
			else if (gain < MIN_GAIN)
				gain = MIN_GAIN;
			multiplier[i] = gain4;

			mp_w[i][0] = multiplier[i] * panorama[i][0];
			mp_w[i][1] = multiplier[i] * panorama[i][1];
		}
	}

	void pan(uint8_t channel, float pan)
	{
		if (channel >= NN)
			return;

		if (pan > MAX_PANORAMA)
			pan = MAX_PANORAMA;
		else if (pan < MIN_PANORAMA)
			pan = MIN_PANORAMA;

		// From: https://stackoverflow.com/questions/67062207/how-to-pan-audio-sample-data-naturally
		panorama[channel][0] = cosf(mapfloat(pan, MIN_PANORAMA, MAX_PANORAMA, 0.0, M_PI / 2.0));
		panorama[channel][1] = sinf(mapfloat(pan, MIN_PANORAMA, MAX_PANORAMA, 0.0, M_PI / 2.0));

		mp_w[channel][0] = multiplier[channel] * panorama[channel][0];
		mp_w[channel][1] = multiplier[channel] * panorama[channel][1];
	}

	void doAddMix(uint8_t channel, float *in)
	{
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
