/**
 *************************************************************************
 *
 * @file image_resize.cpp
 *
 * Utilities for image resizing, implementation.
 *
 ************************************************************************/


#include "image_resize.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "integer_operations.hpp"
#include "array.hpp"
#include "image_raw.hpp"
#include "geo_interpolate.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// image_resize_shrink_window
//
//////////////////////////////////////////////////////////////////////////


image_resize_sampling_window::image_resize_sampling_window
	(int64 old_sample_first, int64 old_sample_last,
	 float weight_first, float weight_last)
	 :
	old_sample_first_(old_sample_first),
	old_sample_last_(old_sample_last),
	weight_first_(weight_first),
	weight_last_(weight_last)
{ } 


image_resize_sampling_window
	image_resize_sampling_window::sampling_window_for_pixel
		(int64 index, float scale_new_to_old, int64 old_size)
{
	// Map a single pixel index on the new image
	// to a sequence of indices on the old image.
	// The integer precision of floats should suffice
	// for image_rgba::max_size, otherwise we might
	// duplicate or drop a few pixels of the old image.
	float float_first = index * scale_new_to_old;
	float float_last = (index + 1) * scale_new_to_old;

	int64 old_sample_first =
		integer_operations::cast_or_throw<int64>(float_first);
	int64 old_sample_last =
		integer_operations::cast_or_throw<int64>(float_last);


	// Determine the occupied pixel fractions
	// at the first and the last sequence pixel.
	// Clamping increases numerical robustness.
	float weight_first =
		(old_sample_first == old_sample_last) ?
		1.f : max(0.f, min(1.f, old_sample_first + 1.f - float_first));
	float weight_last =
		(old_sample_first == old_sample_last) ?
		1.f : max(0.f, min(1.f, float_last - old_sample_last));


	// Clamp samples and weights to valid old pixels
	// to avoid issues with limited float precision.
	if (old_size <= 0)
	{
		LOG_ERROR("Cannot clamp samples to empty source image.");
		throw assert_failed();
	}

	if (old_sample_first < 0)
		{ old_sample_first = 0; weight_first = 1.f; }
	if (old_sample_first >= old_size)
		{ old_sample_first = old_size - 1; weight_first = 1.f; }
	if (old_sample_last < 0)
		{ old_sample_last = 0; weight_last = 1.f; }
	if (old_sample_last >= old_size)
		{ old_sample_last = old_size - 1; weight_last = 1.f; }
				

	return image_resize_sampling_window
		(old_sample_first, old_sample_last,
		 weight_first, weight_last);
}




//////////////////////////////////////////////////////////////////////////
//
// image_resize
//
//////////////////////////////////////////////////////////////////////////


image_rgba image_resize::resize
	(const image_rgba& i, const vector2i& new_size)
{
	if (new_size.min() < 0 || new_size.max() > image_rgba::max_size)
	{
		LOG_ERROR("Resizing exceeds minimal or maximal image dimensions.");
		throw bounds_exceeded();
	}


	// Shrink first, then potentially enlarge.
	if (new_size.x < i.size().x || new_size.y < i.size().y)
	{
		image_rgba result_shrink
			(shrink(i, new_size.min(i.size())));

		if (new_size.x > result_shrink.size().x ||
			new_size.y > result_shrink.size().y)
		{
			image_rgba result_enlarge
				(enlarge(result_shrink,
						 new_size.max(result_shrink.size())));
	
			return std::move(result_enlarge);
		}

		return std::move(result_shrink);
	}


	// Enlarge only.
	if (new_size.x > i.size().x ||
		new_size.y > i.size().y)
	{
		image_rgba result_enlarge
			(enlarge(i, new_size.max(i.size())));

		return std::move(result_enlarge);
	}


	// Neither shrinking nor enlarging is necessary.
	return i;
}


image_rgba image_resize::shrink
	(const image_rgba& i, const vector2i& new_size)
{
	/**
	 * Shrink @bref{i}mage to not-@NULL image_rgba
	 * with @bref{new_size} through filtering kernel.
	 */


	// Pretests: Check new size.
	if (new_size.min() < 0 || new_size.max() > image_rgba::max_size)
	{
		LOG_ERROR("Image cannot exceed minimal or maximal dimensions on shrinking.");
		throw bounds_exceeded();
	}

	vector2i old_size(i.size());
	if (new_size.x > old_size.x || new_size.y > old_size.y)
	{
		LOG_ERROR("Cannot shrink image to larger size.");
		throw assert_failed();
	}


	// Allocate new data buffer.
	image_rgba::data_buffer new_buffer;
	new_buffer.resize(new_size.x * new_size.y);


	// Early out on an empty source image.
	// This allows to clamp sampling windows to valid pixels,
	// which in turn avoids expensive per-pixel boundary tests.
	if (old_size.x <= 0 || old_size.y <= 0)
	{
		memory::fill_with_zero_typed
			(new_buffer.data(),
			 new_size.x * new_size.y);

		return image_rgba(new_size, std::move(new_buffer));
	}


	// Determine which pixels of the old image
	// need to be sampled for each new pixel.
	vector2f scale_new_to_old
		(old_size.to_vector2f() / new_size.to_vector2f());

	array<image_resize_sampling_window> sampling_windows_x;
	sampling_windows_x.reserve(new_size.x);
	array<image_resize_sampling_window> sampling_windows_y;
	sampling_windows_y.reserve(new_size.y);

	for (int64 x = 0; x < new_size.x; ++x)
		sampling_windows_x.push_back
			(image_resize_sampling_window::sampling_window_for_pixel
				(x, scale_new_to_old.x, old_size.x));

	for (int64 y = 0; y < new_size.y; ++y)
		sampling_windows_y.push_back
			(image_resize_sampling_window::sampling_window_for_pixel
				(y, scale_new_to_old.y, old_size.y));


	// Perform actual downsampling:
	// For each pixel of the new image, run through a sampling window,
	// collect weighted samples of old pixels, and store averaged color.
	//
	// Implementation notes:
	// - We average each row separately to maintain integer precision
	//		in floats (e.g. xy_weights) for large sampling windows.
	//		This is possible since our filtering kernel is separable.
	// - Runtime is O(old_size.y * old_size.x) since most old pixels
	//		are part of only a single sampling window.
	const image_rgba::pixel_type* VIRAL__RESTRICT_PTR old_data = i.data();
	image_rgba::pixel_type* VIRAL__RESTRICT_PTR new_data = new_buffer.data();

	for (int64 new_y = 0; new_y < new_size.y; ++new_y)
	for (int64 new_x = 0; new_x < new_size.x; ++new_x)
	for (int64 c = 0; c < 4; ++c)
	{
		const image_resize_sampling_window& win_y = sampling_windows_y[new_y];
		int64 old_y_first = win_y.old_sample_first_;
		int64 old_y_last = win_y.old_sample_last_;

		float xy_sum = 0;
		float xy_weights = 0;

		for (int64 old_y = old_y_first; old_y <= old_y_last; ++old_y)
		{
			const image_resize_sampling_window& win_x = sampling_windows_x[new_x];
			int64 old_x_first = win_x.old_sample_first_;
			int64 old_x_last = win_x.old_sample_last_;

			float x_sum = 0;
			float x_weights = 0;

			for (int64 old_x = old_x_first; old_x <= old_x_last; ++old_x)
			{
				float x_weight =
					(old_x == old_x_first) ? win_x.weight_first_ :
					(old_x == old_x_last) ? win_x.weight_last_ : 1.f;

				x_sum += x_weight * old_data[old_y * old_size.x + old_x].c[c];
				x_weights += x_weight;
			}

			float y_weight =
				(old_y == old_y_first) ? win_y.weight_first_ :
				(old_y == old_y_last) ? win_y.weight_last_ : 1.f;

			xy_sum += y_weight * (x_sum / x_weights);
			xy_weights += y_weight;
		}


		// Apply result, round instead of truncating.
		new_data[new_y * new_size.x + new_x].c[c] =
			integer_operations::cast_or_clamp<unsigned char>
				(max(min((xy_sum / xy_weights) + 0.5f, 255.f), 0.f));
	}

	return image_rgba(new_size, std::move(new_buffer));
}
	

image_rgba image_resize::enlarge
	(const image_rgba& i, const vector2i& new_size)
{
	/**
	 * Enlarge @bref{i}mage to not-@NULL image_rgba
	 * with @bref{new_size} through bicubic interpolation.
	 */


	// Pretests: Check new size.
	if (new_size.min() < 0 || new_size.max() > image_rgba::max_size)
	{
		LOG_ERROR("Image cannot exceed minimal or maximal dimensions on enlarging.");
		throw bounds_exceeded();
	}

	vector2i old_size(i.size());
	if (new_size.x < old_size.x || new_size.y < old_size.y)
	{
		LOG_ERROR("Cannot enlarge image to smaller size.");
		throw assert_failed();
	}


	// Allocate new data buffer.
	image_rgba::data_buffer new_buffer;
	new_buffer.resize(new_size.x * new_size.y);


	// Early out on an empty source image.
	// Otherwise, we would need an expensive per-pixel test
	// for a valid data pointer in the source image.
	if (old_size.x <= 0 || old_size.y <= 0)
	{
		memory::fill_with_zero_typed
			(new_buffer.data(),
			 new_size.x * new_size.y);

		return image_rgba(new_size, std::move(new_buffer));
	}


	// Utility constants for later interpolation.
	// Floating point precision might cause minimal noise
	// in supporting points and interpolant fractions.
	vector2f scale_new_to_old
		(old_size.to_vector2f() / new_size.to_vector2f());
	vector2i max_offs
		(old_size.x - 1, (old_size.y - 1) * old_size.x);


	// Compensation avoids a bias to upper-left pixel edges
	// in the projection from new to old pixel coordinates
	// by shifting back and forth from half-pixel centers.
	// Rationale: Optimize constants to a single term in
	// old_y = ((new_y + 0.5f) * scale_new_to_old.y - 0.5f.
	vector2f compensation
		(vector2f(-0.5f, -0.5f) + scale_new_to_old * 0.5f);


	// Project each new pixel to old pixel data,
	// then find the new pixel color by interpolating
	// over colors in a neighborhood of old pixels.
	// This is minimal effort approach to ensure
	// that each new pixel is set to a new color.
	const image_rgba::pixel_type* VIRAL__RESTRICT_PTR old_data = i.data();
	image_rgba::pixel_type* VIRAL__RESTRICT_PTR new_data = new_buffer.data();

	for (int64 new_y = 0; new_y < new_size.y; ++new_y) 
	{
		float old_y_exact = (new_y * scale_new_to_old.y) + compensation.y;
		int64 old_y_index = integer_operations::cast_or_clamp<int64>(old_y_exact);
		float old_y_fraction = old_y_exact - old_y_index;
	
		// Bounded y offsets to supporting points for interpolation.
		int64 y_offs_base = old_y_index * old_size.x;
		int64 y0_offs = min(max(y_offs_base - old_size.x, 0), max_offs.y);		
		int64 y1_offs = min(max(y_offs_base, 0), max_offs.y);
		int64 y2_offs = min(max(y_offs_base + old_size.x, 0), max_offs.y);
		int64 y3_offs = min(max(y_offs_base + old_size.x * 2, 0), max_offs.y);

		for (int64 new_x = 0; new_x < new_size.x; ++new_x)
		{
			float old_x_exact = (new_x * scale_new_to_old.x) + compensation.x;
			int64 old_x_index = integer_operations::cast_or_clamp<int64>(old_x_exact);
			float old_x_fraction = old_x_exact - old_x_index;

			// Bounded x offsets to supporting points for interpolation.
			int64 x_offs_base = old_x_index;
			int64 x0_offs = min(max(x_offs_base - 1, 0), max_offs.x);		
			int64 x1_offs = min(max(x_offs_base, 0), max_offs.x);
			int64 x2_offs = min(max(x_offs_base + 1, 0), max_offs.x);
			int64 x3_offs = min(max(x_offs_base + 2, 0), max_offs.x);
			
			for (int64 c = 0; c < 4; ++c)
			{
				float interpolated =
					geo_interpolate::interpolate_bicubic
						(old_x_fraction, old_y_fraction, 
						 old_data[y0_offs + x0_offs].c[c], old_data[y0_offs + x1_offs].c[c],
						 old_data[y0_offs + x2_offs].c[c], old_data[y0_offs + x3_offs].c[c],
						 old_data[y1_offs + x0_offs].c[c], old_data[y1_offs + x1_offs].c[c],
						 old_data[y1_offs + x2_offs].c[c], old_data[y1_offs + x3_offs].c[c],
						 old_data[y2_offs + x0_offs].c[c], old_data[y2_offs + x1_offs].c[c],
						 old_data[y2_offs + x2_offs].c[c], old_data[y2_offs + x3_offs].c[c],
						 old_data[y3_offs + x0_offs].c[c], old_data[y3_offs + x1_offs].c[c],
						 old_data[y3_offs + x2_offs].c[c], old_data[y3_offs + x3_offs].c[c]);


				// Apply result, round instead of truncating.
				new_data[new_y * new_size.x + new_x].c[c] =
					integer_operations::cast_or_clamp<unsigned char>
						(max(min(interpolated + 0.5f, 255.f), 0.f));
			}
		}
	}

	return image_rgba(new_size, std::move(new_buffer));
}




} /* namespace viral_core */
