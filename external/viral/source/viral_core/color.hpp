/**
 *************************************************************************
 *
 * @file color.hpp
 *
 * Floating-point colors.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COLOR_HPP)
#define INCLUDED__VIRAL_CORE__COLOR_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class color
 * @ingroup image_module
 *
 * Floating precision RGBA color.
 *
 * Notes:
 * - Color components are not necessarily normalized to [0, 1], but
 *		most applications (e.g. conversion to unsigned char colors)
 *		expect them to be.
 * - The alpha channel usually defines opacity, where an alpha of 0
 *		is fully translucent, and an alpha of 1 is fully opaque.
 *
 ************************************************************************/
class color
{

	public:

		/** Create default, black (0, 0, 0, 0) color. */
		color();

		/** Create fully opaque RGB color with alpha 1. */
		color(float r, float g, float b);

		/** Create color with explicit transparency. */
		color(float r, float g, float b, float a);


		bool operator ==(const color &other) const;
		bool operator !=(const color &other) const;

		color operator +(const color& other) const;
		color operator -(const color& other) const;
		color operator *(const color& other) const;
		color operator /(const color& other) const;

		color operator *(float val) const;
		color operator /(float val) const;


		/**
		 * Linearly interpolate color between caller at
		 * @bref{ratio} = 0 and @bref{target} at @bref{ratio} = 1.
		 */
		color lerp(const color& target, float ratio) const;


		/**
		 * Calculate (possibly negative) sum of color components.
		 * Useful for determining light falloff.
		 */
		float component_sum() const;


		/** Vector-like arithmetics. */
		//@{
		color abs() const;
		float min() const;
		float max() const;
		//@}


		union
		{
			float c[4];
			struct { float r, g, b, a; };
		};		
};




/**
 *************************************************************************
 *
 * @class color_util
 * @ingroup image_module
 *
 * Static utilities for @ref color%s.
 *
 ************************************************************************/
class color_util :
	private non_constructable
{

	public:

		/**
		 * Interpolate into @bref{color_count} @bref{colors}
		 * at a @bref{ratio} in range [0, 1].
		 *
		 * Usage notes:
		 * - Ratios outside [0, 1] range are clamped silently.
		 * - On invalid parameters (e.g. @NULL @bref{colors}),
		 *		interpolation returns a default, black color
		 *		to satisfy @ref NOTHROW behaviour.
		 */
		static color interpolate_colors_cubic
			(float ratio, const color* colors, int64 color_count) NOTHROW;


		/** Interpolate along high-visibility default gradients. */
		//@{
		static color interpolate_high_visibility_red_to_green(float ratio) NOTHROW;
		static color interpolate_high_visibility_yellow_to_red(float ratio) NOTHROW;
		//@}
	

	private:

		static const color high_visibility_red_to_green_[6];
		static const color high_visibility_yellow_to_red_[6];
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COLOR_HPP) */
