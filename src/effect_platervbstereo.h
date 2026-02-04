/*  Stereo plate reverb for Teensy 4
 *
 *  Adapted for use in MiniDexed (Holger Wirtz <wirtz@parasitstudio.de>)
 *
 *  Author: Piotr Zapart
 *          www.hexefx.com
 *
 * Copyright (c) 2020 by Piotr Zapart
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/***
 * Algorithm based on plate reverbs developed for SpinSemi FV-1 DSP chip
 *
 * Allpass + modulated delay line based lush plate reverb
 *
 * Input parameters are float in range 0.0 to 1.0:
 *
 * size - reverb time
 * hidamp - hi frequency loss in the reverb tail
 * lodamp - low frequency loss in the reverb tail
 * lowpass - output/master lowpass filter, useful for darkening the reverb sound
 * diffusion - lower settings will make the reverb tail more "echoey", optimal value 0.65
 *
 */

#pragma once

#include <atomic>
#include <cstdint>

#include "common.h"

/***
 * Loop delay modulation: comment/uncomment to switch sin/cos
 * modulation for the 1st or 2nd tap, 3rd tap is always modulated
 * more modulation means more chorus type sounding reverb tail
 */
// #define TAP1_MODULATED
#define TAP2_MODULATED

class AudioEffectPlateReverb
{
public:
	AudioEffectPlateReverb(float samplerate);
	void process(const float *inblockL, const float *inblockR, float *outblockL, float *outblockR, int len);

	void size(float n)
	{
		n = constrain(n, 0.0f, 1.0f);
		n = mapfloat(n, 0.0f, 1.0f, 0.2f, rv_time_k_max);
		float attn = mapfloat(n, 0.0f, rv_time_k_max, 0.5f, 0.25f);
		rv_time_k = n;
		input_attn = attn;
	}

	void hidamp(float n)
	{
		n = constrain(n, 0.0f, 1.0f);
		lp_hidamp_k = 1.0f - n;
	}

	void lodamp(float n)
	{
		n = constrain(n, 0.0f, 1.0f);
		lp_lodamp_k = -n;
		rv_time_scaler = 1.0f - n * 0.12f; // limit the max reverb time, otherwise it will clip
	}

	void lowpass(float n)
	{
		n = constrain(n, 0.0f, 1.0f);
		n = mapfloat(n * n * n, 0.0f, 1.0f, 0.05f, 1.0f);
		master_lowpass_f = n;
	}

	void diffusion(float n)
	{
		n = constrain(n, 0.0f, 1.0f);
		n = mapfloat(n, 0.0f, 1.0f, 0.005f, 0.65f);
		in_allp_k = n;
		loop_allp_k = n;
	}

	float get_size() { return rv_time_k; }

	void set_mix(float value)
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

	void reset();

	std::atomic<bool> bypass;

private:
	float mix, dry, wet;
	float input_attn;

	float in_allp_k; // input allpass coeff
	float in_allp1_bufL[224]; // input allpass buffers
	float in_allp2_bufL[420];
	float in_allp3_bufL[856];
	float in_allp4_bufL[1089];
	uint16_t in_allp1_idxL;
	uint16_t in_allp2_idxL;
	uint16_t in_allp3_idxL;
	uint16_t in_allp4_idxL;
	float in_allp_out_L; // L allpass chain output
	float in_allp1_bufR[156]; // input allpass buffers
	float in_allp2_bufR[520];
	float in_allp3_bufR[956];
	float in_allp4_bufR[1289];
	uint16_t in_allp1_idxR;
	uint16_t in_allp2_idxR;
	uint16_t in_allp3_idxR;
	uint16_t in_allp4_idxR;
	float in_allp_out_R; // R allpass chain output
	float lp_allp1_buf[2303]; // loop allpass buffers
	float lp_allp2_buf[2905];
	float lp_allp3_buf[3175];
	float lp_allp4_buf[2398];
	uint16_t lp_allp1_idx;
	uint16_t lp_allp2_idx;
	uint16_t lp_allp3_idx;
	uint16_t lp_allp4_idx;
	float loop_allp_k; // loop allpass coeff
	float lp_allp_out;
	float lp_dly1_buf[3423];
	float lp_dly2_buf[4589];
	float lp_dly3_buf[4365];
	float lp_dly4_buf[3698];
	uint16_t lp_dly1_idx;
	uint16_t lp_dly2_idx;
	uint16_t lp_dly3_idx;
	uint16_t lp_dly4_idx;

	static constexpr uint16_t lp_dly1_offset_L = 201; // delay line tap offets
	static constexpr uint16_t lp_dly2_offset_L = 145;
	static constexpr uint16_t lp_dly3_offset_L = 1897;
	static constexpr uint16_t lp_dly4_offset_L = 280;

	static constexpr uint16_t lp_dly1_offset_R = 1897;
	static constexpr uint16_t lp_dly2_offset_R = 1245;
	static constexpr uint16_t lp_dly3_offset_R = 487;
	static constexpr uint16_t lp_dly4_offset_R = 780;

	float lp_hidamp_k; // loop high band damping coeff
	float lp_lodamp_k; // loop low baand damping coeff

	float lpf1; // lowpass filters
	float lpf2;
	float lpf3;
	float lpf4;

	float hpf1; // highpass filters
	float hpf2;
	float hpf3;
	float hpf4;

	float lp_lowpass_f; // loop lowpass scaled frequency
	float lp_hipass_f; // loop highpass scaled frequency

	float master_lowpass_f;
	float master_lowpass_l;
	float master_lowpass_r;

	static constexpr float rv_time_k_max = 0.95f;
	float rv_time_k; // reverb time coeff
	float rv_time_scaler; // with high lodamp settings lower the max reverb time to avoid clipping

	uint32_t lfo1_phase_acc; // LFO 1
	uint32_t lfo1_adder;

	uint32_t lfo2_phase_acc; // LFO 2
	uint32_t lfo2_adder;
};
