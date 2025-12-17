/*
Cubic Sine Approximation based upon a modified Taylor Series Expansion
Author: Ryan Billing (C) 2010

This is unlicensed.  Do whatever you want with but use at your own disgression.
The author makes no guarantee of its suitability for any purpose.
*/


#pragma once

#include <math.h>

#ifndef PI
#define PI 3.141592653589793f
#endif

#ifndef PI_2
#define PI_2 1.5707963267948966f
#endif

#ifndef D_PI
#define D_PI 6.283185307179586f
#endif

//globals
static const float fact3 = 0.148148148148148f; //can multiply by 1/fact3

static inline float f_sin(float x)
{
	float y;
	bool sign;

	if (x > D_PI || x < -D_PI) x = fmod(x, D_PI);
	if (x < 0.0f) x += D_PI;
	if (x > PI) {
		x = D_PI - x;
		sign = 1;
	} else {
		sign = 0;
	}

	if (x <= PI_2) {
		y = x - x * x * x * fact3;
	} else {
		float tmp = x - PI;
		y = -tmp + tmp * tmp * tmp * fact3;
	}

	if (sign) y = -y;

	return y;
}

static inline float f_cos(float x)
{
	return f_sin(PI_2 + x);
}
