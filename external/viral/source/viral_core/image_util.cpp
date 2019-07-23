/**
 *************************************************************************
 *
 * @file image_util.cpp
 *
 * Utilities for images, implementation.
 *
 ************************************************************************/


#include "image_util.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "integer_operations.hpp"
#include "image_raw.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// image_converter
//
//////////////////////////////////////////////////////////////////////////


image_binary image_converter::to_image_binary
	(const image_greyscale& greyscale,
	 unsigned char threshold)
{
	image_binary result(greyscale.size());

	const image_greyscale::pixel_type* VIRAL__RESTRICT_PTR source =
		greyscale.data();
	image_binary::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;

	for (int64 i = 0; i < pixel_count; ++i)
		dest[i].b = source[i].v > threshold;
	
	return std::move(result);
}


image_binary image_converter::to_image_binary
	(const image_rgba& rgba,
	 bool test_red_channel_only,
	 unsigned char threshold)
{
	image_binary result(rgba.size());

	const image_rgba::pixel_type* VIRAL__RESTRICT_PTR source =
		rgba.data();
	image_binary::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;

	if (test_red_channel_only)
	{
		for (int64 i = 0; i < pixel_count; ++i)
			dest[i].b = source[i].r > threshold;
	}
	else
	{
		for (int64 i = 0; i < pixel_count; ++i)
		{
			int32 average =
				(static_cast<int32>(source[i].r) +
				 static_cast<int32>(source[i].g) +
				 static_cast<int32>(source[i].b)) / 3;

			dest[i].b = average > threshold;
		}
	}

	return std::move(result);
}


image_greyscale image_converter::to_image_greyscale
	(const image_binary& binary)
{
	image_greyscale result(binary.size());

	const image_binary::pixel_type* VIRAL__RESTRICT_PTR source =
		binary.data();
	image_greyscale::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;

	for (int64 i = 0; i < pixel_count; ++i)
		dest[i].v =
			static_cast<unsigned char>
				(source[i].b ? 255 : 0);
	
	return std::move(result);
}


image_greyscale image_converter::to_image_greyscale
	(const image_rgba& rgba)
{
	image_greyscale result(rgba.size());

	const image_rgba::pixel_type* VIRAL__RESTRICT_PTR source =
		rgba.data();
	image_greyscale::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;
	
	for (int64 i = 0; i < pixel_count; ++i)
	{
		int32 average =
			(static_cast<int32>(source[i].r) +
			 static_cast<int32>(source[i].g) +
			 static_cast<int32>(source[i].b)) / 3;

		dest[i].v =
			integer_operations::cast_or_clamp<unsigned char>(average);
	}
	
	return std::move(result);
}


image_rgba image_converter::to_image_rgba
	(const image_binary& binary)
{
	image_rgba result(binary.size());
	
	const image_binary::pixel_type* VIRAL__RESTRICT_PTR source =
		binary.data();
	image_rgba::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;
	
	for (int64 i = 0; i < pixel_count; ++i)
	{
		dest[i].r =
			static_cast<unsigned char>(source[i].b ? 255 : 0);
		dest[i].g =
			static_cast<unsigned char>(source[i].b ? 255 : 0);
		dest[i].b =
			static_cast<unsigned char>(source[i].b ? 255 : 0);
		dest[i].a =
			static_cast<unsigned char>(255);
	}
	
	return std::move(result);
}


image_rgba image_converter::to_image_rgba
	(const image_greyscale& greyscale)
{
	image_rgba result(greyscale.size());

	const image_greyscale::pixel_type* VIRAL__RESTRICT_PTR source =
		greyscale.data();
	image_rgba::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;
	
	for (int64 i = 0; i < pixel_count; ++i)
	{
		dest[i].r = source[i].v;
		dest[i].g = source[i].v;
		dest[i].b = source[i].v;
		dest[i].a = 255;
	}
	
	return std::move(result);
}


image_rgba image_converter::to_image_rgba
	(const image_hsva& hsva)
{
	return to_image_rgba
				(hsva,
				 default_hsv_to_rgb_black_epsilon,
				 default_hsv_to_rgb_grey_epsilon);
}


image_rgba image_converter::to_image_rgba
	(const image_hsva& hsva,
	 float hsv_to_rgb_black_epsilon,
	 float hsv_to_rgb_grey_epsilon)
{
	/** See https://en.wikipedia.org/wiki/HSL_and_HSV for algorithm. */

	image_rgba result(hsva.size());

	const image_hsva::pixel_type* VIRAL__RESTRICT_PTR source =
		hsva.data();
	image_rgba::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;

	for (int64 i = 0; i < pixel_count; ++i)
	{
		float h = max(0.f, min(6.f, source[i].h / 255.f * 6.f));
		float s = max(0.f, min(1.f, source[i].s / 255.f));
		float v = max(0.f, min(1.f, source[i].v / 255.f));

		float r, g, b;
		
		bool is_black = (v < hsv_to_rgb_black_epsilon);
		bool is_grey = (s < hsv_to_rgb_grey_epsilon);

		if (is_black)
			{ r = 0; g = 0; b = 0; }
		else if (is_grey)
			{ r = v; g = v; b = v; }
		else
		{
			unsigned char hi =
				integer_operations::cast_or_clamp<unsigned char>(h);

			float f = h - hi;

			float p = v * (1.f - s);
			float q = v * (1.f - s * f);
			float t = v * (1.f - s * (1 - f));

			switch (hi)
			{
				case 0: r = v; g = t; b = p; break;
				case 1: r = q; g = v; b = p; break;
				case 2: r = p; g = v; b = t; break;
				case 3: r = p; g = q; b = v; break;
				case 4: r = t; g = p; b = v; break;
				case 5: r = v; g = p; b = q; break;
				case 6: r = v; g = t; b = p; break; // Wrapping.

				default:
					LOG_ERROR("Unknown hue sector.");
				throw assert_failed();
			}
		}
		
		// Safety against round-off issues.
		dest[i].r =
			integer_operations::cast_or_clamp<unsigned char>(r * 255);
		dest[i].g =
			integer_operations::cast_or_clamp<unsigned char>(g * 255);
		dest[i].b =
			integer_operations::cast_or_clamp<unsigned char>(b * 255);

		dest[i].a = source[i].a;
	}

	return std::move(result);
}


image_hsva image_converter::to_image_hsva
	(const image_rgba& rgba)
{
	return to_image_hsva
				(rgba,
				 default_rgb_to_hsv_black_epsilon,
				 default_rgb_to_hsv_grey_epsilon);
}


image_hsva image_converter::to_image_hsva
	(const image_rgba& rgba,
	 float rgb_to_hsv_black_epsilon,
	 float rgb_to_hsv_grey_epsilon)
{
	/**
	 * Implementation notes:
	 * - We use a traditional hexcone transformation
	 *		that maps from RGB in range [0,1] to HSV
	 *		in range [0,1].
	 */

	image_hsva result(rgba.size());

	const image_rgba::pixel_type* VIRAL__RESTRICT_PTR source =
		rgba.data();
	image_hsva::pixel_type* VIRAL__RESTRICT_PTR dest =
		result.data();

	int64 pixel_count =
		result.size().x * result.size().y;
	
	for (int64 i = 0; i < pixel_count; ++i)
	{
		float r = max(0.f, min(1.f, source[i].r / 255.f));
		float g = max(0.f, min(1.f, source[i].g / 255.f));
		float b = max(0.f, min(1.f, source[i].b / 255.f));

		float rgbmax = max(max(r, g), b);
		float rgbmin = min(min(r, g), b);
		float rgbdif = rgbmax - rgbmin;

		bool has_chroma =
			rgbdif > rgb_to_hsv_grey_epsilon;

		float hue_denorm =
			(has_chroma && rgbmax == r) ? (0.f + (g - b) / rgbdif) :
			(has_chroma && rgbmax == g) ? (2.f + (b - r) / rgbdif) :
			(has_chroma && rgbmax == b) ? (4.f + (r - g) / rgbdif) : 0.f;

		// Denormalized hue has a range of [-1, 5].
		// We must wrap-around the negative blueish-red hues,
		// and we have to scale the result to [0, 1] range.

		float h =
			(hue_denorm / 6.f) +
			((hue_denorm < 0.f) ? 1.f : 0.f);

		float s =
			rgbmax > rgb_to_hsv_black_epsilon ?
			(rgbdif / rgbmax) : 0.f;

		float v = rgbmax;

		// Safety against round-off issues.
		dest[i].h =
			integer_operations::cast_or_clamp<unsigned char>(h * 255);
		dest[i].s =
			integer_operations::cast_or_clamp<unsigned char>(s * 255);
		dest[i].v =
			integer_operations::cast_or_clamp<unsigned char>(v * 255);

		dest[i].a = source[i].a;
	}
	
	return std::move(result);
}


const float image_converter::default_hsv_to_rgb_black_epsilon = 1e-5f;
const float image_converter::default_hsv_to_rgb_grey_epsilon = 1e-5f;

const float image_converter::default_rgb_to_hsv_black_epsilon = 1e-5f;
const float image_converter::default_rgb_to_hsv_grey_epsilon = 1e-5f;




} /* namespace viral_core */
