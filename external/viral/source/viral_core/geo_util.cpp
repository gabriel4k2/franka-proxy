/**
 *************************************************************************
 *
 * @file geo_util.cpp
 *
 * Geometry utilities, implementation.
 *
 ************************************************************************/


#include "geo_util.hpp"


#include <math.h>


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// geo_constants
//
//////////////////////////////////////////////////////////////////////////


const float geo_constants::degrees_to_radians = 0.0174532925199432957692369076848861f;
const float geo_constants::radians_to_degrees = 57.2957795130823208767981548141052f;

const float geo_constants::pi = 3.1415926535897932384626433832795f;
const float geo_constants::double_pi = 6.28318530717958647692528676655901f;
const float geo_constants::half_pi = 1.57079632679489661923132169163975f;




//////////////////////////////////////////////////////////////////////////
//
// geo_util
//
//////////////////////////////////////////////////////////////////////////


const float geo_util::atan2_epsilon = 1.e-5f;
float geo_util::atan2(float y, float x) NOTHROW
{
	if (fabsf(x) < atan2_epsilon) x = 0.f;
	if (fabsf(y) < atan2_epsilon) y = 0.f;

	if (x > 0) return atanf(y / x);
	if (x < 0)
		return ((y > 0) ? 1.f : -1.f) *
					(geo_constants::pi - atanf(fabsf(y / x)));

	if (y > 0) return geo_constants::half_pi;
	if (y < 0) return -geo_constants::half_pi;
	return 0.f;
}


float geo_util::unit_ball_volume(int64 dimensions) NOTHROW
{
	/**
	 * Implementation by Nico Höllerich.
	 *
	 * See https://en.wikipedia.org/wiki/Volume_of_an_n-ball
	 * for formulas.
	 */

	if (dimensions <= 0) return 0;
	if (dimensions > unit_ball_volume_max_dimensions) return 0;


	// Even dimensions, usual factorial.
	if (dimensions % 2 == 0)
	{
		float factorial = 1;
		for (int64 i = 2; i <= dimensions / 2; ++i)
			factorial *= i;

		float pi_power =
			powf(geo_constants::pi,
				 static_cast<float>(dimensions / 2));

		return pi_power / factorial;
	}


	// Odd dimensions, double factorial.
	float factorial = 1;
	for (int64 i = 3; i <= dimensions; i += 2)
		factorial *= i;

	float pi_power =
		powf(geo_constants::pi,
			 static_cast<float>(dimensions / 2));
	float two_power =
		powf(2.f, (dimensions + 1.f) / 2.f);

	return
		pi_power * two_power / factorial;
}




//////////////////////////////////////////////////////////////////////////
//
// grid_math
//
//////////////////////////////////////////////////////////////////////////


float grid_math::round_to_grid
	(float value, float grid) NOTHROW
{
	if (grid <= 0) return value;

	// Adjust the incoming value so that removal
	// of the grid modulo will round correctly.
	// In this context, note that fmodf keeps
	// the sign of the incoming value.

	value = (value > 0) ?
				(value + 0.5f * grid) :
				(value - 0.5f * grid);

	return value - fmodf(value, grid);
}


float grid_math::tile_to_interval
	(float value, float minimum, float maximum) NOTHROW
{
	if (minimum >= maximum) return value;

	// Idea:
	// - Tile to a grid through origin with interval length.
	//		This requires translation to remove minimum.
	// - For tiling, determine distance to nearest grid line
	//		via rounding, then adjust in case of round-up.
	// - Translate back to reestablish minimum.

	float grid = maximum - minimum;
	float translated = value - minimum;
 	float ongrid = round_to_grid(translated, grid);
	if (translated < ongrid) ongrid -= grid;
	return minimum + translated - ongrid;
}


float grid_math::clamp_to_interval
	(float value, float minimum, float maximum) NOTHROW
{
	if (minimum > maximum) return value;
	if (value < minimum) return minimum;
	if (value > maximum) return maximum;
	return value;
}


int32 grid_math::safe_int_mod
	(int32 dividend, int32 divisor) NOTHROW
{
	if (divisor == 0) return 0;

	if (dividend >= 0)
		return dividend % abs(divisor);

	return -((-dividend) % abs(divisor));
}


int64 grid_math::safe_int_mod
	(int64 dividend, int64 divisor) NOTHROW
{
	if (divisor == 0) return 0;

	if (dividend >= 0)
		return dividend % abs(divisor);

	return -((-dividend) % abs(divisor));
}




} /* namespace viral_core */
