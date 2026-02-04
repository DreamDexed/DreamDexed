#pragma once

#include "ValueSmoothingFilter.h"

namespace zyn
{

/**Comb Filter Bank for sympathetic Resonance*/
class CombFilterBank
{
public:
	CombFilterBank(float samplerate, float initgain);
	void filterout(float *smp, int period);

	static constexpr int max_strings = 76 * 3;
	static constexpr int max_samples = 6048;

	float delays[max_strings];
	float inputgain;
	float outgain;
	float gainbwd;

	void setStrings(int nr, float basefreq);

private:
	float string_smps[max_strings][max_samples];
	float basefreq;
	int strings_nr;
	int pos_writer;

	/* for smoothing gain jump when using binary valued sustain pedal */
	ValueSmoothingFilter gain_smoothing;

	int mem_size;
	float samplerate;
};

} // namespace zyn
