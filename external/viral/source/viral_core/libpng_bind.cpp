/**
 *************************************************************************
 *
 * @file libpng_bind.cpp
 *
 * Implementation for libpng wrapper.
 *
 ************************************************************************/


#include "libpng_bind.hpp"


#if defined(VIRAL__HAVE_LIBPNG)


#include <png.h>


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "buffer.hpp"
#include "geo_2d.hpp"
#include "image_raw.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  libpng_bind internal helpers
//
//////////////////////////////////////////////////////////////////////////


/** Detail namespace with utilities for libpng_bind. */
namespace libpng_bind_detail
{

	void throw_on_error
		(png_structp png_ptr, png_const_charp error_msg)
	{
		LOG_ERROR("libpng reported error: " + error_msg);
		throw compression_failed();
	}

	void throw_on_warning
		(png_structp png_ptr, png_const_charp warning_msg)
	{
		LOG_ERROR("libpng reported warning: " + warning_msg);
		throw compression_failed();
	}




	/**
	 * An in-memory input buffer with compressed PNG data
	 * for use by libpng_bind::decompress().
 	 */
	class read_buffer
	{
		friend class ::viral_core::libpng_bind;


		private:

			read_buffer(const libpng_bind::data_buffer& source)
				: source(source), offset(0) { }


			static void read
				(png_structp png_ptr, png_bytep data, png_size_t length)
			{
				read_buffer* buf =
					reinterpret_cast<read_buffer*>(png_get_io_ptr(png_ptr));

				int64 length_checked =
					integer_operations::cast_or_throw<int64>(length);
				int64 next_offset =
					integer_operations::add_or_throw
						(buf->offset, length_checked);

				if (next_offset < 0 ||
					next_offset > buf->source.size())
				{
					LOG_ERROR("PNG reading exceeds file size, file damaged.");
					throw corrupt_data();
				}

				memory::copy_block_typed
					(data, buf->source.data() + buf->offset, length_checked);
				buf->offset = next_offset;
			}


			const libpng_bind::data_buffer& source;
			int64 offset;
	};




	/**
	 * An in-memory output buffer with uncompressed RGBA data
	 * for use by libpng_bind::compress().
	 */
	class write_buffer
	{
		friend class ::viral_core::libpng_bind;


		private:

			write_buffer(libpng_bind::data_buffer& target)
				: target(target) { }


			static void write
				(png_structp png_ptr, png_bytep data, png_size_t length)
			{
				write_buffer* buf =
					reinterpret_cast<write_buffer*>(png_get_io_ptr(png_ptr));

				int64 length_checked =
					integer_operations::cast_or_throw<int64>(length);
				int64 next_size =
					integer_operations::add_or_throw
						(buf->target.size(), length_checked);
					
				buf->target.resize(next_size);
				memory::copy_block_typed
					(buf->target.data() + next_size - length_checked,
					 data, length_checked);
			}


			static void flush(png_structp png_ptr) { }


			libpng_bind::data_buffer& target;
	};
}




//////////////////////////////////////////////////////////////////////////
//
//  libpng_bind
//
//////////////////////////////////////////////////////////////////////////


libpng_bind::data_buffer libpng_bind::compress
	(const image_rgba& input)
{
	// We directly provide untyped image data
	// to libpng later on, so ensure consistency.
	if (image_rgba::bytes_per_pixel != 4)
	{
		LOG_ERROR("RGBA image does not have four bytes per pixel.");
		throw assert_failed();
	}


	// Input validation.
	if (input.size().min() < 0 ||
		input.size().max() > max_image_size_)
	{
		LOG_ERROR("Input for PNG compression exceeds maximum supported size.");
		throw invalid_parameter();
	}


	// Fetch validated image size.
	int64 size_x = input.size().x;
	int64 size_y = input.size().y;


	// libpng setup.
	png_structp png_write = png_create_write_struct
		(PNG_LIBPNG_VER_STRING, NULL,
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_error),
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_warning));

	if (!png_write)
	{
		LOG_ERROR("Failed to initialize PNG write struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_write,
			(&png_destroy_write_struct, &png_write,
			 static_cast<png_infopp>(NULL)));


	png_infop png_info =
		png_create_info_struct(png_write);

	if (!png_info)
	{
		LOG_ERROR("Failed to create PNG info struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_info,
			(&png_destroy_info_struct, png_write, &png_info));


	// Direct libpng to our output buffer.
	data_buffer output;
	libpng_bind_detail::write_buffer png_output(output);
	png_set_write_fn
		(png_write, &png_output,
		 static_cast<png_rw_ptr>(&libpng_bind_detail::write_buffer::write),
		 static_cast<png_flush_ptr>(&libpng_bind_detail::write_buffer::flush));

	
	// Perform actual compression.
	png_set_IHDR(png_write, png_info, 
				 integer_operations::cast_or_throw<png_uint_32>(size_x),
				 integer_operations::cast_or_throw<png_uint_32>(size_y),
				 8, PNG_COLOR_TYPE_RGB_ALPHA,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	dynamic_buffer<png_bytep> input_rows;
	input_rows.resize(size_y);
	for (int64 y = 0; y < size_y; ++y)
		input_rows[y] =
			const_cast<png_bytep>
				(input.untyped_data() + y * size_x * 4);

	png_write_info(png_write, png_info);
	png_write_image(png_write, input_rows.data());
	png_write_end(png_write, NULL);


	// Success.
	return std::move(output);
}


libpng_bind::data_buffer libpng_bind::compress
	(const image_greyscale& input)
{
	// We directly provide untyped image data
	// to libpng later on, so ensure consistency.
	if (image_greyscale::bytes_per_pixel != 1)
	{
		LOG_ERROR("Greyscale image does not have one byte per pixel.");
		throw assert_failed();
	}


	// Input validation.
	if (input.size().min() < 0 ||
		input.size().max() > max_image_size_)
	{
		LOG_ERROR("Input for PNG compression exceeds maximum supported size.");
		throw invalid_parameter();
	}


	// Fetch validated image size.
	int64 size_x = input.size().x;
	int64 size_y = input.size().y;


	// libpng setup.
	png_structp png_write = png_create_write_struct
		(PNG_LIBPNG_VER_STRING, NULL,
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_error),
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_warning));

	if (!png_write)
	{
		LOG_ERROR("Failed to initialize PNG write struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_write,
			(&png_destroy_write_struct, &png_write,
			 static_cast<png_infopp>(NULL)));


	png_infop png_info =
		png_create_info_struct(png_write);

	if (!png_info)
	{
		LOG_ERROR("Failed to create PNG info struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_info,
			(&png_destroy_info_struct, png_write, &png_info));


	// Direct libpng to our output buffer.
	data_buffer output;
	libpng_bind_detail::write_buffer png_output(output);
	png_set_write_fn
		(png_write, &png_output,
		 static_cast<png_rw_ptr>(&libpng_bind_detail::write_buffer::write),
		 static_cast<png_flush_ptr>(&libpng_bind_detail::write_buffer::flush));

	
	// Perform actual compression.
	png_set_IHDR(png_write, png_info,
				 integer_operations::cast_or_throw<png_uint_32>(size_x),
				 integer_operations::cast_or_throw<png_uint_32>(size_y),
				 8, PNG_COLOR_TYPE_GRAY,
				 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT,
				 PNG_FILTER_TYPE_DEFAULT);

	dynamic_buffer<png_bytep> input_rows;
	input_rows.resize(size_y);
	for (int64 y = 0; y < size_y; ++y)
		input_rows[y] =
			const_cast<png_bytep>
				(input.untyped_data() + y * size_x);

	png_write_info(png_write, png_info);
	png_write_image(png_write, input_rows.data());
	png_write_end(png_write, NULL);


	// Success.
	return std::move(output);
}


image_rgba libpng_bind::decompress_rgba
	(const data_buffer& input)
{
	// We directly fetch untyped image data
	// from libjpeg later on, so ensure consistency.
	if (image_rgba::bytes_per_pixel != 4)
	{
		LOG_ERROR("RGBA image does not have four bytes per pixel.");
		throw assert_failed();
	}


	// Initial validation.
	if (input.size() < 8 ||
		png_sig_cmp(const_cast<unsigned char*>(input.data()), 0, 8) != 0)
	{
		LOG_ERROR("PNG image data is missing PNG header.");
		throw corrupt_data();
	}


	// libpng setup.
	png_structp png_read = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, NULL,
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_error),
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_warning));

	if (!png_read)
	{
		LOG_ERROR("Failed to initialize PNG read struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_read,
			(&png_destroy_read_struct, &png_read,
			 static_cast<png_infopp>(NULL),
			 static_cast<png_infopp>(NULL)));


	png_infop png_info =
		png_create_info_struct(png_read);

	if (!png_info)
	{
		LOG_ERROR("Failed to create PNG info struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_info,
			(&png_destroy_info_struct, png_read, &png_info));


	// Direct libpng to our input buffer.
	libpng_bind_detail::read_buffer png_input(input);
	png_set_read_fn
		(png_read, &png_input,
		 static_cast<png_rw_ptr>(&libpng_bind_detail::read_buffer::read));


	// Read and check PNG info header.
	png_read_info(png_read, png_info);

	png_uint_32 png_width = 0;
	png_uint_32 png_height = 0;
	int png_bit_depth = 0;
	int png_color_type = 0;
	png_get_IHDR(png_read, png_info, &png_width, &png_height,
				 &png_bit_depth, &png_color_type, NULL, NULL, NULL);

	if (png_width >
			integer_operations::cast_or_throw<png_uint_32>(max_image_size_) ||
		png_height >
			integer_operations::cast_or_throw<png_uint_32>(max_image_size_))
	{
		LOG_ERROR("PNG image exceeds maximum supported dimensions.");
		throw bounds_exceeded();
	}

	if (png_bit_depth != 8)
	{
		LOG_ERROR("PNG bit depth not supported.");
		throw unsupported_feature();
	}

	if (png_color_type != PNG_COLOR_TYPE_GRAY &&
		png_color_type != PNG_COLOR_TYPE_GRAY_ALPHA &&
		png_color_type != PNG_COLOR_TYPE_RGB &&
		png_color_type != PNG_COLOR_TYPE_RGBA)
	{
		LOG_ERROR("PNG color type not supported.");
		throw unsupported_feature();
	}

	if (png_color_type == PNG_COLOR_TYPE_RGB)
		png_set_filler(png_read, 0, PNG_FILLER_AFTER);


	// Fetch converted image size.
	int64 size_x =
		integer_operations::cast_or_throw<int64>(png_width);
	int64 size_y =
		integer_operations::cast_or_throw<int64>(png_height);
		

	// Read actual image data:
	// - libpng directly allows us to read RGB(A) into RGBA images.
	// - For greyscale images, we first must read to temp storage,
	//		then explicitly duplicate channels to fill an RGBA image.
	image_rgba output
		(vector2i(size_x, size_y));

	if (png_color_type == PNG_COLOR_TYPE_RGB ||
		png_color_type == PNG_COLOR_TYPE_RGBA)
	{
		dynamic_buffer<png_bytep> output_rows;
		output_rows.resize(size_y);
		for (int64 y = 0; y < size_y; ++y)
			output_rows[y] =
				output.untyped_data() + y * size_x * 4;

		png_read_image(png_read, output_rows.data());
		png_read_end(png_read, NULL);
	}

	else if (png_color_type == PNG_COLOR_TYPE_GRAY)
	{
		dynamic_buffer<unsigned char> tempbuf;
		tempbuf.resize(size_x * size_y);

		dynamic_buffer<png_bytep> tempbuf_rows;
		tempbuf_rows.resize(size_y);
		for (int64 y = 0; y < size_y; ++y)
			tempbuf_rows[y] = tempbuf.data() + y * size_x;

		png_read_image(png_read, tempbuf_rows.data());
		png_read_end(png_read, NULL);

		for (int64 i = 0; i < tempbuf.size(); ++i)
		{
			output.data()[i].r = tempbuf[i];
			output.data()[i].g = tempbuf[i];
			output.data()[i].b = tempbuf[i];
			output.data()[i].a = 0;
		}			
	}

	else if (png_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		dynamic_buffer<unsigned char> tempbuf;
		tempbuf.resize(size_x * size_y * 2);

		dynamic_buffer<png_bytep> tempbuf_rows;
		tempbuf_rows.resize(size_y);
		for (int64 y = 0; y < size_y; ++y)
			tempbuf_rows[y] = tempbuf.data() + y * size_x * 2;

		png_read_image(png_read, tempbuf_rows.data());
		png_read_end(png_read, NULL);

		for (int64 i = 0; i < tempbuf.size() / 2; ++i)
		{
			output.data()[i].r = tempbuf[i * 2];
			output.data()[i].g = tempbuf[i * 2];
			output.data()[i].b = tempbuf[i * 2];
			output.data()[i].a = tempbuf[i * 2 + 1];
		}
	}

	else
	{
		LOG_ERROR("Loading not implemented for supported PNG color type.");
		throw assert_failed();
	}


	// Success.
	return std::move(output);
}


image_greyscale libpng_bind::decompress_greyscale
	(const data_buffer& input)
{
	// We directly fetch untyped image data
	// from libjpeg later on, so ensure consistency.
	if (image_greyscale::bytes_per_pixel != 1)
	{
		LOG_ERROR("Greyscale image does not have one byte per pixel.");
		throw assert_failed();
	}


	// Initial validation.
	if (input.size() < 8 ||
		png_sig_cmp(const_cast<unsigned char*>(input.data()), 0, 8) != 0)
	{
		LOG_ERROR("PNG image data is missing PNG header.");
		throw corrupt_data();
	}


	// libpng setup.
	png_structp png_read = png_create_read_struct
		(PNG_LIBPNG_VER_STRING, NULL,
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_error),
		 static_cast<png_error_ptr>(&libpng_bind_detail::throw_on_warning));

	if (!png_read)
	{
		LOG_ERROR("Failed to initialize PNG read struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_read,
			(&png_destroy_read_struct, &png_read,
			 static_cast<png_infopp>(NULL),
			 static_cast<png_infopp>(NULL)));


	png_infop png_info =
		png_create_info_struct(png_read);

	if (!png_info)
	{
		LOG_ERROR("Failed to create PNG info struct.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(delete_png_info,
			(&png_destroy_info_struct, png_read, &png_info));


	// Direct libpng to our input buffer.
	libpng_bind_detail::read_buffer png_input(input);
	png_set_read_fn
		(png_read, &png_input,
		 static_cast<png_rw_ptr>(&libpng_bind_detail::read_buffer::read));


	// Read and check PNG info header.
	png_read_info(png_read, png_info);

	png_uint_32 png_width = 0;
	png_uint_32 png_height = 0;
	int png_bit_depth = 0;
	int png_color_type = 0;
	png_get_IHDR(png_read, png_info, &png_width, &png_height,
				 &png_bit_depth, &png_color_type, NULL, NULL, NULL);

	if (png_width >
			integer_operations::cast_or_throw<png_uint_32>(max_image_size_) ||
		png_height >
			integer_operations::cast_or_throw<png_uint_32>(max_image_size_))
	{
		LOG_ERROR("PNG image exceeds maximum supported dimensions.");
		throw bounds_exceeded();
	}

	if (png_bit_depth != 8)
	{
		LOG_ERROR("PNG bit depth not supported.");
		throw unsupported_feature();
	}

	if (png_color_type != PNG_COLOR_TYPE_GRAY &&
		png_color_type != PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		LOG_ERROR("PNG color type not supported.");
		throw unsupported_feature();
	}

	if (png_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_strip_alpha(png_read);
		

	// Fetch converted image size.
	int64 size_x =
		integer_operations::cast_or_throw<int64>(png_width);
	int64 size_y =
		integer_operations::cast_or_throw<int64>(png_height);
		

	// Directly read actual image data.
	image_greyscale output
		(vector2i(size_x, size_y));

	if (png_color_type == PNG_COLOR_TYPE_GRAY ||
		png_color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
	{
		dynamic_buffer<png_bytep> output_rows;
		output_rows.resize(size_y);
		for (int64 y = 0; y < size_y; ++y)
			output_rows[y] = output.untyped_data() + y * size_x;

		png_read_image(png_read, output_rows.data());
		png_read_end(png_read, NULL);
	}

	else
	{
		LOG_ERROR("Loading not implemented for supported PNG color type.");
		throw assert_failed();
	}


	// Success.
	return std::move(output);
}




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_LIBPNG) */
