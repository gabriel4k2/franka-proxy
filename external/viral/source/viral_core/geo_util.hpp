/**
 *************************************************************************
 *
 * @file geo_util.hpp
 *
 * Geometry utilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__GEO_UTIL_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class geo_constants
 * @ingroup geo_module
 *
 * Contains several static constants, such as pi and angle unit
 * conversion factors.
 *
 ************************************************************************/
class geo_constants :
	private non_constructable
{

	public: 

		static const float degrees_to_radians;
		static const float radians_to_degrees;

		static const float pi;
		static const float double_pi;
		static const float half_pi;
};




/**
 *************************************************************************
 *
 * @class geo_util
 * @ingroup geo_module
 *
 * Geometric utilities.
 *
 ************************************************************************/
class geo_util :
	private non_constructable
{
	
	public:

		/**
		 * Arctangent as in STL std::atan2f(),
		 * but applies @bref{atan2_epsilon},
		 * and returns 0 on degenerate cases.
		 */
		//@{
		static const float atan2_epsilon;
		static float atan2(float y, float x) NOTHROW;
		//@}


		/**
		 * Calculate the volume of a unit ball
		 * with given @bref{dimensions}.
		 */
		//@{
		static const int64 unit_ball_volume_max_dimensions = 128;
		static float unit_ball_volume(int64 dimensions) NOTHROW;
		//@}
};




/**
 *************************************************************************
 *
 * @class grid_math
 * @ingroup geo_module
 *
 * Contains static math helpers for grid calculations, such as finding
 * the nearest on-grid point for some position.
 *
 ************************************************************************/
class grid_math :
	private non_constructable
{

	public:

		/**
		 * Round a floating point @bref{value} to the nearest
		 * integer multiple of some @bref{grid}.
		 *
		 * Grid rounding examples:
		 * - value 9, grid 8, output 8,
		 * - value 1, grid 8, output 0,
		 * - value -22, grid 9, output -18.
		 *
		 * If @bref{grid} is less than or equal to 0,
		 * the call directly returns @bref{value}.
		 */
		static float round_to_grid(float value, float grid) NOTHROW;


		/**
		 * Tile a given floating point @bref{value} to the interval
		 * within @bref{minimum} and @bref{maximum} values,
		 * so that a @bref{value} slightly larger than @bref{maximum}
		 * starts at @bref{minimum} again, and vice versa.
		 *
		 * Tiling examples:
		 * - value 11, minimum -2, maximum 8, output 1,
		 * - value 9, minimum -2, maximum 8, output -1,
		 * - value -26, minimum -2, maximum 8, output 4,
		 * - value 3, minimum -2, maximum 8, output 3.
		 *
		 * If the given interval is empty,
		 * tiling directly returns @bref{value}.
		 */
		static float tile_to_interval
			(float value, float minimum, float maximum) NOTHROW;


		/**	 
		 * Clamps a given floating point @bref{value} to the interval
		 * within @bref{minimum} and @bref{maximum} values.
		 * Returns @bref{maximum} for greater @bref{values},
		 * and @bref{minimum} for smaller @bref{values}.
		 *
		 * Clamping examples:
		 * - value 11, minimum -2, maximum 8, output 8,
		 * - value -11, minimum -2, maximum 8, output -2,
		 * - value 3, minimum -2, maximum 8, output 3.
		 * 
		 * If the given interval is empty,
		 * clamping directly returns @bref{value}.
		 */
		static float clamp_to_interval
			(float value, float minimum, float maximum) NOTHROW;


		/**
		 * Sign-correct integer modulo function whose result always
		 * has the sign of the @bref{dividend}, and which returns 0
		 * if the @bref{divisor} is 0. This ships around undefined
		 * behaviour in the C standard for integer modulo arithmetics
		 * with a negative operand. For performance reasons, only use
		 * this method if the divisor can be 0 or negative.
		 */
		//@{
		static int32 safe_int_mod
			(int32 dividend, int32 divisor) NOTHROW;
		static int64 safe_int_mod
			(int64 dividend, int64 divisor) NOTHROW;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_UTIL_HPP) */