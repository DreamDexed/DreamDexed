#include "CombFilterBank.h"

#include <algorithm>
#include <cassert>
#include <cmath>

namespace zyn
{

CombFilterBank::CombFilterBank(float samplerate, float initgain) :
delays{},
inputgain{1.0f},
outgain{1.0f},
gainbwd{initgain},
string_smps{},
basefreq{},
strings_nr{},
pos_writer{},
mem_size{},
samplerate{samplerate}
{
	gain_smoothing.cutoff(1.0f);
	gain_smoothing.sample_rate(samplerate / 16);
	gain_smoothing.thresh(0.02f);
	gain_smoothing.reset(gainbwd);
}

void CombFilterBank::setStrings(int strings_nr_n, float basefreq_n)
{
	strings_nr_n = std::min(max_strings, strings_nr_n);

	if (strings_nr_n == strings_nr && basefreq_n == basefreq)
		return;

	int mem_size_n = std::min(max_samples, static_cast<int>(ceilf((samplerate / basefreq_n * 1.03f /*+ buffersize*/ + 2) / 16) * 16));
	if (mem_size_n == mem_size)
	{
		if (strings_nr_n > strings_nr)
		{
			for (int i = strings_nr; i < strings_nr_n; ++i)
			{
				std::fill_n(string_smps[i], mem_size_n, 0.0f);
			}
		}
	}
	else
	{
		for (int i = 0; i < strings_nr_n; ++i)
		{
			std::fill_n(string_smps[i], mem_size_n, 0.0f);
		}

		mem_size = mem_size_n;
		basefreq = basefreq_n;
		pos_writer = 0;
	}

	strings_nr = strings_nr_n;
}

static float tanhX(float x)
{
	// Pade approximation of tanh(x) bound to [-1 .. +1]
	// https://mathr.co.uk/blog/2017-09-06_approximating_hyperbolic_tangent.html
	float x2 = x * x;
	return x * (105.0f + 10.0f * x2) / (105.0f + (45.0f + x2) * x2);
}

static float sampleLerp(float *smp, float pos, int mem_size)
{
	int poshi = pos;
	float poslo = pos - poshi;
	return smp[poshi] + poslo * (smp[(poshi + 1) % mem_size] - smp[poshi]);
}

void CombFilterBank::filterout(float *smp, int period)
{
	if (strings_nr == 0) return;

	// interpolate gainbuf values over buffer length using value smoothing filter (lp)
	// this should prevent popping noise when controlled binary with 0 / 127
	// new control rate = samplerate / 16
	int gainbufsize = period / 16;
	float gainbuf[gainbufsize];

	if (!gain_smoothing.apply(gainbuf, gainbufsize, gainbwd))
		std::fill_n(gainbuf, gainbufsize, gainbwd); // if nothing to interpolate (constant value)

	float temp[period];
	std::fill_n(temp, period, 0);

	int strings_act_nr = 0;

	for (int j = 0; j < strings_nr; ++j)
	{
		if (delays[j] == 0.0f) continue;

		strings_act_nr++;

		int pos_writer_ = pos_writer;
		float delay = delays[j];

		for (int i = 0; i < period; ++i)
		{
			float input_smp = smp[i] * inputgain;
			float gain = gainbuf[i / 16];

			float pos_reader = pos_writer_ + mem_size - delay;
			if (pos_reader > mem_size) pos_reader -= mem_size;

			float sample = sampleLerp(string_smps[j], pos_reader, mem_size);
			temp[i] += string_smps[j][pos_writer_] = input_smp + tanhX(sample * gain);

			pos_writer_ = (pos_writer_ + 1) % mem_size;
		}
	}

	pos_writer = (pos_writer + period) % mem_size;

	float gain = outgain / strings_act_nr;
	for (int i = 0; i < period; ++i)
	{
		smp[i] = temp[i] * gain;
	}
}

} // namespace zyn
