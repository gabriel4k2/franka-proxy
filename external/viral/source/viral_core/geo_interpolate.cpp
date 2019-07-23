/**
 *************************************************************************
 *
 * @file geo_interpolate.cpp
 * 
 * Utilities for interpolation, implementation.
 *
 ************************************************************************/


#include "geo_interpolate.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// geo_interpolate
//
//////////////////////////////////////////////////////////////////////////


float geo_interpolate::interpolate_cubic
	(float t,
	 float p0, float p1, float p2, float p3) NOTHROW
{
	// See https://en.wikipedia.org/wiki/Cubic_Hermite_spline
	// for cubic spline equations.

	float m1 = (p2 - p0) / 2;
	float m2 = (p3 - p1) / 2;

	float tp2 = t * t;
	float tp3 = tp2 * t;

	return
		(2 * tp3 - 3 * tp2 + 1) * p1 +
		(-2 * tp3 + 3 * tp2) * p2 +
		(tp3 - 2 * tp2 + t) * m1 +
		(tp3 - tp2) * m2;
}

float geo_interpolate::interpolate_cubic
	(float t, const float *array) NOTHROW
{
	return interpolate_cubic
			(t, array[0], array[1], array[2], array[3]);
}


float geo_interpolate::interpolate_bicubic
	(float x, float y,
	 float p00, float p01, float p02, float p03,
	 float p10, float p11, float p12, float p13,
	 float p20, float p21, float p22, float p23,
	 float p30, float p31, float p32, float p33) NOTHROW
{
	return interpolate_cubic
				(x,
				 interpolate_cubic(y, p00, p10, p20, p30),
				 interpolate_cubic(y, p01, p11, p21, p31),
				 interpolate_cubic(y, p02, p12, p22, p32),
				 interpolate_cubic(y, p03, p13, p23, p33));
}

float geo_interpolate::interpolate_bicubic
	(float x, float y, const float *array) NOTHROW
{
	return interpolate_bicubic
			(x, y,
			 array[0], array[1], array[2], array[3],
			 array[4], array[5], array[6], array[7],
			 array[8], array[9], array[10], array[11],
			 array[12], array[13], array[14], array[15]);
}




} /* namespace viral_core */
