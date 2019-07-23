/**
 *************************************************************************
 *
 * @file geo_interpolate.hpp
 * 
 * Utilities for interpolation.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_INTERPOLATE_HPP)
#define INCLUDED__VIRAL_CORE__GEO_INTERPOLATE_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class geo_interpolate
 * @ingroup geo_module
 *
 * Utilities for interpolation over supporting points.
 *
 ************************************************************************/
class geo_interpolate :
	private non_constructable
{
	public:

		/**
		 * Cubic interpolation through supporting @bref{p}oints,
		 * optionally specified as a four-entry @bref{array}.
		 *
		 * A parameter @bref{t} of zero maps to @bref{p1},
		 * and a parameter @bref{t} of one gives @bref{p2}.
		 *
		 * All supporting points must be equidistant in @bref{t}.
		 */
		//@{
		static float interpolate_cubic
			(float t,
			 float p0, float p1, float p2, float p3) NOTHROW;

		static float interpolate_cubic
			(float t, const float *array) NOTHROW;
		//@}


		/**
		 * Bi-cubic version of interpolate_cubic().
		 *
		 * All supporting points are in row-first format,
		 * e.g. if @bref{y} is zero and @bref{x} is one,
		 * then interpolate_bicubic() returns @bref{p12}
		 * or @bref{array}[6].
		 * 
		 * The @bref{array} variant requires 16 entries.
		 */
		//@{
		static float interpolate_bicubic
			(float x, float y,
			 float p00, float p01, float p02, float p03,
			 float p10, float p11, float p12, float p13,
			 float p20, float p21, float p22, float p23,
			 float p30, float p31, float p32, float p33) NOTHROW;

		static float interpolate_bicubic
			(float x, float y, const float *array) NOTHROW;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_INTERPOLATE_HPP) */
