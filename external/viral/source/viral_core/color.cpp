/**
 *************************************************************************
 *
 * @file color.cpp
 *
 * Floating-point colors, implementation.
 *
 ************************************************************************/


#include "color.hpp"


#include <math.h>


#include "integer_operations.hpp"
#include "geo_interpolate.hpp"


namespace viral_core
{

	
//////////////////////////////////////////////////////////////////////////
//
// color
//
//////////////////////////////////////////////////////////////////////////


color::color()
	: r(0), g(0), b(0), a(0) { }
color::color(float r, float g, float b)
	: r(r), g(g), b(b), a(1) { }
color::color(float r, float g, float b, float a)
	: r(r), g(g), b(b), a(a) { }


bool color::operator ==(const color &other) const
	{ return r == other.r && g == other.g && b == other.b && a == other.a; }
bool color::operator !=(const color &other) const
	{ return r != other.r || g != other.g || b != other.b || a != other.a; }

color color::operator +(const color& other) const
	{ return color(r + other.r, g + other.g, b + other.b, a + other.a); }
color color::operator -(const color& other) const
	{ return color(r - other.r, g - other.g, b - other.b, a - other.a); }
color color::operator *(const color& other) const
	{ return color(r * other.r, g * other.g, b * other.b, a * other.a); }
color color::operator /(const color& other) const
	{ return color(r / other.r, g / other.g, b / other.b, a / other.a); }

color color::operator *(float val) const
	{ return color(r * val, g * val, b * val, a * val); }
color color::operator /(float val) const
	{ return color(r / val, g / val, b / val, a / val); }


color color::lerp(const color& target, float ratio) const
{
	float invratio = 1 - ratio;

	return
		color(r * invratio + target.r * ratio,
			  g * invratio + target.g * ratio,
			  b * invratio + target.b * ratio,
			  a * invratio + target.a * ratio);
}


float color::component_sum() const { return r + g + b + a; }


color color::abs() const
	{ return color(fabsf(r), fabsf(g), fabsf(b), fabsf(a)); }
float color::min() const
	{ return viral_core::min(r, viral_core::min(g, viral_core::min(b, a))); }
float color::max() const
	{ return viral_core::max(r, viral_core::max(g, viral_core::max(b, a))); }



	
//////////////////////////////////////////////////////////////////////////
//
// color_util
//
//////////////////////////////////////////////////////////////////////////


color color_util::interpolate_colors_cubic
	(float ratio, const color* colors, int64 color_count) NOTHROW
{
	// Check parameters.
	if (!colors) return color();
	if (color_count < 1) return color();


	 // Avoid later division by zero.
	if (color_count == 1) return colors[0];


	// Find ratio and indices for cubic interpolation.
	// Indices may be somewhat off (but still valid)
	// due to float precision for many input colors.
	ratio = max(0.f, min(1.f, ratio));

	int64 p1_index =
		max(0, min(color_count - 1,
			integer_operations::cast_or_clamp<int64>
				(ratio * (color_count - 1))));

	int64 p0_index =
		max(0, min(color_count - 1, p1_index - 1));
	int64 p2_index =
		max(0, min(color_count - 1, p1_index + 1));
	int64 p3_index =
		max(0, min(color_count - 1, p1_index + 2));

	float t_norm =
		max(0.f, min(1.f,
			(ratio * (color_count - 1) - p1_index) /
				(color_count - 1)));


	// Interpolate over surrounding colors.
	const color& p0 = colors[p0_index];
	const color& p1 = colors[p1_index];
	const color& p2 = colors[p2_index];
	const color& p3 = colors[p3_index];

	return color(max(0.f, min(1.f,
					 geo_interpolate::interpolate_cubic
							(t_norm, p0.c[0], p1.c[0], p2.c[0], p3.c[0]))),
				 max(0.f, min(1.f,
					 geo_interpolate::interpolate_cubic
							(t_norm, p0.c[1], p1.c[1], p2.c[1], p3.c[1]))),
				 max(0.f, min(1.f,
					 geo_interpolate::interpolate_cubic
							(t_norm, p0.c[2], p1.c[2], p2.c[2], p3.c[2]))),
				 max(0.f, min(1.f,
					 geo_interpolate::interpolate_cubic
							(t_norm, p0.c[3], p1.c[3], p2.c[3], p3.c[3]))));
}


color color_util::interpolate_high_visibility_red_to_green(float ratio) NOTHROW
{
	return interpolate_colors_cubic
				(ratio, high_visibility_red_to_green_,
				 sizeof(high_visibility_red_to_green_) /
					sizeof(high_visibility_red_to_green_[0]));
}

color color_util::interpolate_high_visibility_yellow_to_red(float ratio) NOTHROW
{
	return interpolate_colors_cubic
				(ratio, high_visibility_yellow_to_red_,
				 sizeof(high_visibility_yellow_to_red_) /
					sizeof(high_visibility_yellow_to_red_[0]));
}


const color color_util::high_visibility_red_to_green_[6] =
	{ color(0.843f, 0.188f, 0.152f, 1.f),
	  color(0.988f, 0.552f, 0.349f, 1.f),
	  color(0.996f, 0.878f, 0.545f, 1.f),
	  color(0.850f, 0.937f, 0.545f, 1.f),
	  color(0.568f, 0.811f, 0.376f, 1.f),
	  color(0.101f, 0.596f, 0.313f, 1.f) };

const color color_util::high_visibility_yellow_to_red_[6] =
	{ color(1.000f, 1.000f, 0.698f, 1.f),
	  color(0.996f, 0.850f, 0.462f, 1.f),
	  color(0.996f, 0.698f, 0.298f, 1.f),
	  color(0.992f, 0.552f, 0.235f, 1.f),
	  color(0.941f, 0.231f, 0.125f, 1.f),
	  color(0.741f, 0.000f, 0.149f, 1.f) };




} /* namespace viral_core */
