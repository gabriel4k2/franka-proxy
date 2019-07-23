/**
 *************************************************************************
 *
 * @file image_resize.hpp
 *
 * Utilities for image resizing.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_RESIZE_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_RESIZE_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "image_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class image_resize_sampling_window
 * @ingroup image_module
 *
 * A one-dimensional sampling window for shrinking image_rgba%s.
 *
 * Implementation notes:
 * - See image_resize::shrink() and sampling_window_for_pixel().
 *
 ************************************************************************/
class image_resize_sampling_window
{
	friend class image_resize;

	private:

		image_resize_sampling_window
			(int64 old_sample_first, int64 old_sample_last,
			 float weight_first, float weight_last);

		static image_resize_sampling_window sampling_window_for_pixel
			(int64 index, float scale_new_to_old, int64 old_size);

		const int64 old_sample_first_;
		const int64 old_sample_last_;
		const float weight_first_;
		const float weight_last_;
};




/**
 *************************************************************************
 *
 * @class image_resize
 * @ingroup image_module
 *
 * Static utilities for resizing image_rgba%s.
 *
 * Usage notes:
 * - Clients should avoid resizing images to their current size,
 *		as this incurs a (somewhat expensive) copy of the image.
 *
 * Color spaces and artifacts:
 * - Resampling artifacts depend strongly on the color space used.
 *		Notably, edges between areas with vastly different colors
 *		exhibit a dark halo when interpolating in sRGB space
 *		and a bright halo when interpolating in linear space.
 * - Since there is no optimal solution to color space artifacts,
 *		resizing does not perform any color space transformations,
 *		but instead uses the color space of the source image.
 *
 * Implementation notes:
 * - Resizing uses cubic interpolation for magnification
 *		and a filter mask for minification.
 *
 ************************************************************************/
class image_resize :
	private non_constructable
{

	public:

		static image_rgba resize
			(const image_rgba& i, const vector2i& new_size);


	private:

		static image_rgba shrink
			(const image_rgba& i, const vector2i& new_size);
		static image_rgba enlarge
			(const image_rgba& i, const vector2i& new_size);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_RESIZE_HPP) */
