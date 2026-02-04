#pragma once

#include <cstdint>
#include <cstring>

#include "butter.h"
#include "dsp/filtering_functions.h"

class AudioEffectBWFMono
{
public:
	enum FilterType
	{
		LP,
		HP,
	};

	AudioEffectBWFMono(FilterType type, float samplerate, float cutoff_Hz, int order) :
	type{type},
	samplerate{samplerate},
	cutoff{cutoff_Hz},
	order{order}
	{
		recalculate();
		arm_biquad_cascade_df1_init_f32(&hp_filt_struct, hp_nstages, hp_coeff, hp_state);
	}

	float getCutoff_Hz() { return cutoff; }

	void setCutoff_Hz(float value)
	{
		cutoff = value;
		recalculate();
	}

	int getOrder() { return order; }

	void setOrder(int value)
	{
		order = value < 0 ? 0 : value > 2 ? 2 : value;
		recalculate();
	}

	void process(float *block, int len)
	{
		arm_biquad_cascade_df1_f32(&hp_filt_struct, block, block, static_cast<uint32_t>(len));
	}

	void resetState()
	{
		memset(hp_state, 0, sizeof(hp_state));
	}

private:
	void recalculate()
	{
		switch (type)
		{
		case LP:
			butter_lp(order, cutoff / (samplerate / 2.0f), hp_coeff);
			break;
		case HP:
			butter_hp(order, cutoff / (samplerate / 2.0f), hp_coeff);
			break;
		}

		butter_stage_arrange_arm(order, hp_nstages, hp_coeff);
	}

	FilterType type;
	float samplerate;
	float cutoff;
	int order;

	arm_biquad_casd_df1_inst_f32 hp_filt_struct;

	static constexpr int hp_nstages = 1;
	float hp_coeff[5];
	float hp_state[4];
};
