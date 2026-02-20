#pragma once

inline int mapint(int x, int in_min, int in_max, int out_min, int out_max)
{
	return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline float mapfloat(float val, float in_min, float in_max, float out_min, float out_max)
{
	return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

inline float mapfloat(int val, int in_min, int in_max, float out_min, float out_max)
{
	return (val - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

#define constrain(amt, low, high) ({ \
	__typeof__(amt) _amt = (amt); \
	__typeof__(low) _low = (low); \
	__typeof__(high) _high = (high); \
	(_amt < _low) ? _low : ((_amt > _high) ? _high : _amt); \
})
