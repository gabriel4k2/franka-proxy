/**
 *************************************************************************
 *
 * @file image_util.hpp
 *
 * Utilities for images.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__IMAGE_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__IMAGE_UTIL_HPP


#include "build_config.hpp"


#include "image_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class image_converter
 * @ingroup image_module
 *
 * Static utilities for conversion between different image types.
 *
 ************************************************************************/
class image_converter :
	private non_constructable
{

	public:

		static image_binary to_image_binary
			(const image_greyscale& greyscale,
			 unsigned char threshold);
		static image_binary to_image_binary
			(const image_rgba& rgba,
			 bool test_red_channel_only,
			 unsigned char threshold);

		static image_greyscale to_image_greyscale
			(const image_binary& binary);
		static image_greyscale to_image_greyscale
			(const image_rgba& rgba);
	
		static image_rgba to_image_rgba
			(const image_binary& binary);
		static image_rgba to_image_rgba
			(const image_greyscale& greyscale);
		static image_rgba to_image_rgba
			(const image_hsva& hsva);
		static image_rgba to_image_rgba
			(const image_hsva& hsva,
			 float hsv_to_rgb_black_epsilon,
			 float hsv_to_rgb_grey_epsilon);

		static image_hsva to_image_hsva
			(const image_rgba& rgba);
		static image_hsva to_image_hsva
			(const image_rgba& rgba,
			 float rgb_to_hsv_black_epsilon,
			 float rgb_to_hsv_grey_epsilon);	

		
		/**
		 * Normalized tolerances below which conversion
		 * between RGB and HSV spaces considers colors
		 * as pure grey or as pure black.
		 */
		//@{
		static const float default_hsv_to_rgb_black_epsilon;
		static const float default_hsv_to_rgb_grey_epsilon;

		static const float default_rgb_to_hsv_black_epsilon;
		static const float default_rgb_to_hsv_grey_epsilon;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__IMAGE_UTIL_HPP) */
