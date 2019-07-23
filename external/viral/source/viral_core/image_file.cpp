/**
 *************************************************************************
 *
 * @file image_file.cpp
 *
 * File operations for images, implementation.
 *
 ************************************************************************/


#include "image_file.hpp"


#include "log.hpp"
#include "integer_operations.hpp"
#include "buffer.hpp"
#include "file.hpp"
#include "file_util.hpp"
#include "image_raw.hpp"
#include "libjpeg_bind.hpp"
#include "libpng_bind.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// image_file
//
//////////////////////////////////////////////////////////////////////////


void image_file::save
	(const image_rgba& i,
	 const string& path, file_context& files)
{
	string ext
		(virtual_path::extract_extension(path).to_lower());

	auto_pointer<file> output
		(files.open_file(path, file::read_write_truncate));

	if (ext == "tga")
		return save_tga(i, *output);
	else if (ext == "jpg")
		return save_jpg(i, *output, default_jpg_quality);
	else if (ext == "png")
		return save_png(i, *output);
	else
	{
		LOG_ERROR("Image extension unknown: " + path);
		throw invalid_parameter();
	}
}


void image_file::save_tga
	(const image_rgba& i, file& output)
{
	// Check for consistency of image semantics
	// against TGA format as a safety precaution.
	if (image_rgba::bytes_per_pixel != 4)
	{
		LOG_ERROR("RGBA image does not have four bytes per pixel.");
		throw assert_failed();
	}


	// Check whether image can even be stored as TGA.
	if (i.size().max() > tga_max_image_size_)
	{
		LOG_ERROR("Image exceeds maximum supported size for TGA output.");
		throw bounds_exceeded();
	}


	// TGA header: Fixed part and image properties.
	// Casts should not throw after above pretests.
	output.write_bytes(tga_header_, 12);

	uint16 out_size_x =
		integer_operations::cast_or_throw<uint16>(i.size().x);
	output.write(out_size_x);

	uint16 out_size_y =
		integer_operations::cast_or_throw<uint16>(i.size().y);
	output.write(out_size_y);

	unsigned char bit_depth = 32;
	output.write(bit_depth);

	unsigned char origin_flag = 0;
	output.write(origin_flag);


	// Write rows of TGA image in bottom-up order.
	// Transform from RGBA to BGRA order.
	image_rgba tga_order(i);
	tga_order.swap_components(0, 2);
	tga_order.flip_vertical();

	output.write_bytes
		(tga_order.data(),
		 size_of<image_rgba::pixel_type>() *
			tga_order.size().x * tga_order.size().y);
}


void image_file::save_jpg
	(const image_rgba& i, file& output, int8 quality)
{

#if defined(VIRAL__HAVE_LIBJPEG)

	libjpeg_bind::data_buffer jpeg_buffer
		(libjpeg_bind::compress(i, quality));

	output.write_bytes
		(jpeg_buffer.data(), jpeg_buffer.size());

#else

	LOG_ERROR("Not built with support for JPG files.");
	throw unsupported_feature();

#endif

}


void image_file::save_png
	(const image_rgba& i, file& output)
{

#if defined(VIRAL__HAVE_LIBPNG)

	libpng_bind::data_buffer png_buffer
		(libpng_bind::compress(i));

	output.write_bytes
		(png_buffer.data(), png_buffer.size());

#else

	LOG_ERROR("Not built with support for PNG files.");
	throw unsupported_feature();

#endif

}


image_rgba image_file::load
	(const string& path, file_context& files)
{
	string ext
		(virtual_path::extract_extension(path).to_lower());

	auto_pointer<file> input
		(files.open_file(path, file::read_only));

	if (ext == "tga")
		return load_tga(*input);
	else if (ext == "jpg")
		return load_jpg(*input);
	else if (ext == "png")
		return load_png(*input);
	else
	{
		LOG_ERROR("Image extension unknown: " + path);
		throw invalid_parameter();
	}
}


image_rgba image_file::load_tga(file& input)
{
	// Check for consistency of image semantics
	// against TGA format as a safety precaution.
	if (image_rgba::bytes_per_pixel != 4)
	{
		LOG_ERROR("RGBA image does not have four bytes per pixel.");
		throw assert_failed();
	}


	// Extract header, resolution, bit depth,
	// thereafter check data against bounds.
	// Widening of uint16 to int64 size does
	// not cause any overflow.
	unsigned char header[12];
	input.read_bytes(header, 12);

	if (!memory::are_blocks_equal_typed(header, tga_header_, 12))
	{
		LOG_ERROR("Corrupt or unsupported targa header.");
		throw corrupt_data();
	}

	uint16 size_x_narrow;
	input.read(size_x_narrow);
	uint16 size_y_narrow;
	input.read(size_y_narrow);

	int64 size_x = size_x_narrow;
	int64 size_y = size_y_narrow;

	if (size_x < 0 || size_x > image_rgba::max_size ||
		size_y < 0 || size_y > image_rgba::max_size)
	{
		LOG_ERROR("Targa resolution not supported.");
		throw bounds_exceeded();
	}

	unsigned char bit_depth;
	input.read(bit_depth);
	int64 bytes_per_pixel = bit_depth / 8;

	if (bytes_per_pixel != 3 && bytes_per_pixel != 4)
	{
		LOG_ERROR("Targa bitdepth not supported.");
		throw unsupported_feature();
	}

	unsigned char origin_flag;
	input.read(origin_flag);

	if ((origin_flag & (1 << 4)) != 0 ||
		(origin_flag & (1 << 5)) != 0)
	{
		LOG_ERROR("Upside-down targas not supported.");
		throw unsupported_feature();
	}


	// Allocate image storage and read in image data.
	//
	// We need to fix a few things along the way:
	// - Targas are stored bottom-up, we store top-down.
	// - Targa colors are BGRA, and hence need to be switched.
	// - Targa 24 bit images need to be inflated to 32 bit.
	//
	// Idea: Handle entire rows at a time for performance.

	image_rgba result
		(vector2i(size_x, size_y));

	dynamic_buffer<unsigned char> row_buffer;
	row_buffer.resize(size_x * bytes_per_pixel);

	image_rgba::pixel_type* target_row =
		result.data() + size_x * (size_y - 1);

	for (int64 y = 0; y < size_y; ++y)
	{
		input.read_bytes
			(row_buffer.data(), size_x * bytes_per_pixel);

		if (bytes_per_pixel == 3)
		{
			for (int64 i = 0; i < size_x; ++i)
			{
				target_row[i].r = row_buffer[i * 3 + 2];
				target_row[i].g = row_buffer[i * 3 + 1];
				target_row[i].b = row_buffer[i * 3];
				target_row[i].a = 0;
			}
		}
		else if (bytes_per_pixel == 4)
		{
			for (int64 i = 0; i < size_x; ++i)
			{
				target_row[i].r = row_buffer[i * 4 + 2];
				target_row[i].g = row_buffer[i * 4 + 1];
				target_row[i].b = row_buffer[i * 4];
				target_row[i].a = row_buffer[i * 4 + 3];
			}
		}
		else
		{
			LOG_CRITICAL("Image loading accepted TGA file with invalid bitdepth.");
			throw assert_failed();
		}

		target_row -= size_x;
	}

	return std::move(result);
}


image_rgba image_file::load_jpg(file& input)
{

#if defined(VIRAL__HAVE_LIBJPEG)

	libjpeg_bind::data_buffer jpeg_buffer;
	jpeg_buffer.resize(input.find_size());
	input.read_bytes(jpeg_buffer.data(), jpeg_buffer.size());

	return libjpeg_bind::decompress_rgba(jpeg_buffer);

#else

	LOG_ERROR("Not built with support for JPG files.");
	throw unsupported_feature();

#endif

}


image_rgba image_file::load_png(file& input)
{

#if defined(VIRAL__HAVE_LIBPNG)

	libpng_bind::data_buffer png_buffer;
	png_buffer.resize(input.find_size());
	input.read_bytes(png_buffer.data(), png_buffer.size());

	return libpng_bind::decompress_rgba(png_buffer);

#else

	LOG_ERROR("Not built with support for PNG files.");
	throw unsupported_feature();

#endif

}


const unsigned char image_file::tga_header_[12] =
	{ 0, 0, 2, 0,  0, 0, 0, 0,  0, 0, 0, 0 };


/** This limit more than avoids an overflow of TGA uint16 size. */
const int64 image_file::tga_max_image_size_ = 32767; 




} /* namespace viral_core */
