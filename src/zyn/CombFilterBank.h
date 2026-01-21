#include "ValueSmoothingFilter.h"

#pragma once

namespace zyn {

/**Comb Filter Bank for sympathetic Resonance*/
class CombFilterBank
{
public:
	CombFilterBank(float samplerate, float initgain);
	void filterout(float *smp, uint16_t period);

	static constexpr unsigned max_strings = 76 * 3;
	static constexpr unsigned max_samples = 6048;

	float delays[max_strings];
	float inputgain;
	float outgain;
	float gainbwd;

	void setStrings(unsigned nr, float basefreq);

private:
	float string_smps[max_strings][max_samples];
	float basefreq;
	unsigned strings_nr;
	unsigned pos_writer;

	/* for smoothing gain jump when using binary valued sustain pedal */
	ValueSmoothingFilter gain_smoothing;

	unsigned mem_size;
	float samplerate;
};

}
