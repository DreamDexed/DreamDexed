#pragma once

#include "butter.h"

class AudioEffectBWFMono
{
public:
	enum FilterType {
		LP,
		HP,
	};

	AudioEffectBWFMono(FilterType type, float32_t samplerate, float32_t cutoff_Hz, uint8_t order):
	type{type},
	samplerate{samplerate},
	cutoff{cutoff_Hz},
	order{order}
	{
		recalculate();
		arm_biquad_cascade_df1_init_f32(&hp_filt_struct, hp_nstages, hp_coeff, hp_state);
	}

	float32_t getCutoff_Hz() { return cutoff; }

	void setCutoff_Hz(float32_t value)
	{
		cutoff = value;
		recalculate();
	}
	
	uint8_t getOrder() { return order; }

	void setOrder(uint8_t value)
	{
		order = value > 2 ? 2 : value;
		recalculate();
	}

	void process(float32_t* block, uint16_t len)
	{
		arm_biquad_cascade_df1_f32(&hp_filt_struct, block, block, len);
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
	float32_t samplerate;
	float32_t cutoff;
	uint8_t order;

	arm_biquad_casd_df1_inst_f32 hp_filt_struct;

	static constexpr uint8_t hp_nstages = 1;
	float hp_coeff[5];
	float hp_state[4];
};
