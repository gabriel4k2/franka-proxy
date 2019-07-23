/**
 *************************************************************************
 *
 * @file libjpeg_bind.cpp
 *
 * Implementation of libjpeg wrapper.
 *
 ************************************************************************/


#include "libjpeg_bind.hpp"


#if defined(VIRAL__HAVE_LIBJPEG)


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "buffer.hpp"
#include "geo_2d.hpp"
#include "image_raw.hpp"


// stdio.h is a prerequisite for jpeglib.h,
// but seemingly is not included automatically.
#include <stdio.h>


// Nasty windows.h headers potentially set an INT32 define,
// which conflicts with a definition in jpeglib.h, so fix that.
#if defined(VIRAL__OS_TARGET_WINDOWS)
	#undef INT32
#endif


#include <jpeglib.h>
#include <jerror.h>


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  libjpeg_bind internals
//
//////////////////////////////////////////////////////////////////////////


/**
 *************************************************************************
 *
 * Detail namespace with libjpeg error handler methods.
 *
 * Notes:
 * - Fatal error and warning both mean corrupted image data.
 *		We propagate both of these to the client by exception
 *		rather than by originally suggested setjmp / longjmp.
 * - Fatal errors anyways must not return to libjpeg.
 * - As demonstrated in the libjpeg example code, warnings also
 *		are allowed to jump to a higher scope without leaking.
 * - Trace-typed warning messages (i.e. with msg_level == -1)
 *		do not indicate an error and must be ignored.
 *
 ************************************************************************/
namespace libjpeg_error_handler
{
	void error_exit_fatal(j_common_ptr cinfo)
	{
		LOG_ERROR("libjpeg reported fatal error.");
		throw compression_failed();
	}

	void error_exit_warning(j_common_ptr cinfo, int msg_level)
	{
		if (msg_level != -1) return;

		LOG_ERROR("libjpeg reported warning (corrupt image).");
		throw compression_failed();
	}

	void init_default_error(jpeg_error_mgr &errmgr)
	{
		jpeg_std_error(&errmgr);
		errmgr.error_exit = &error_exit_fatal;
		errmgr.emit_message = &error_exit_warning;
	}
}




/**
 *************************************************************************
 *
 * Detail namespace with libjpeg destination handlers
 * for data buffers, used for compression to JPG.
 *
 * These replace the default STDIO functionality in libjpeg.
 * See the libjpeg default implementation in the jdatadst.h
 * file for details.
 *
 ************************************************************************/
namespace libjpeg_destination_handler
{

	/** Transfers an @bref{output} buffer into JPG decoding. */
	struct jpeg_destination_buffer_mgr
	{
		jpeg_destination_mgr dstmgr;
		libjpeg_bind::data_buffer *output;
		int64 blocksize;
	};


	void init_destination(j_compress_ptr cinfo)
	{
		// Allocate first output block here,
		// the call may not leave free_in_buffer at 0.
		jpeg_destination_buffer_mgr *dstbufmgr =
			reinterpret_cast<jpeg_destination_buffer_mgr*>(cinfo->dest);

		dstbufmgr->output->resize(dstbufmgr->blocksize);

		dstbufmgr->dstmgr.next_output_byte =
			dstbufmgr->output->data();
		dstbufmgr->dstmgr.free_in_buffer =
			integer_operations::cast_or_throw<size_t>
				(dstbufmgr->blocksize);
	}


	boolean empty_output_buffer(j_compress_ptr cinfo)
	{
		// Allocate another output block.
		jpeg_destination_buffer_mgr *dstbufmgr =
			reinterpret_cast<jpeg_destination_buffer_mgr*>(cinfo->dest);

		int64 newblockstart = dstbufmgr->output->size();

		dstbufmgr->output->resize
			(integer_operations::add_or_throw
				(dstbufmgr->output->size(), dstbufmgr->blocksize));

		dstbufmgr->dstmgr.next_output_byte =
			dstbufmgr->output->data() + newblockstart;
		dstbufmgr->dstmgr.free_in_buffer =
			integer_operations::cast_or_throw<size_t>
				(dstbufmgr->blocksize);

		return TRUE;
	}


	void term_destination(j_compress_ptr cinfo)
	{
		// Finished with entire compression.
		// We now need to shrink the buffer back so it only contains
		// actually written data, and not any remaining free bytes.
		jpeg_destination_buffer_mgr *dstbufmgr =
			reinterpret_cast<jpeg_destination_buffer_mgr*>(cinfo->dest);

		int64 used_bytes = 
			dstbufmgr->output->size() -
			integer_operations::cast_or_throw<int64>
				(dstbufmgr->dstmgr.free_in_buffer); // < output->size()

		dstbufmgr->output->resize(used_bytes);
	}


	void init_default_destination
		(jpeg_destination_buffer_mgr &dstbufmgr,
		 libjpeg_bind::data_buffer &output,
		 int64 datablocksize)
	{
		dstbufmgr.dstmgr.init_destination = &init_destination;
		dstbufmgr.dstmgr.empty_output_buffer = &empty_output_buffer;
		dstbufmgr.dstmgr.term_destination = &term_destination;

		dstbufmgr.output = &output;
		dstbufmgr.blocksize = datablocksize;
	}
}




/**
 *************************************************************************
 *
 * Detail namespace with libjpeg source handlers
 * for data buffers, used for decompression.
 *
 * These replace the default STDIO functionality in libjpeg.
 * See the libjpeg default implementation in the jdatasrc.h
 * file for details.
 *
 ************************************************************************/
namespace libjpeg_source_handler
{

	/** Transfers an @bref{input} buffer into JPG encoding. */
	struct jpeg_source_buffer_mgr
	{
		jpeg_source_mgr srcmgr;
		const libjpeg_bind::data_buffer *input;
	};


	void init_source(j_decompress_ptr cinfo)
	{
		jpeg_source_buffer_mgr *srcbufmgr =
			reinterpret_cast<jpeg_source_buffer_mgr*>(cinfo->src);

		srcbufmgr->srcmgr.next_input_byte =
			srcbufmgr->input->data();
		srcbufmgr->srcmgr.bytes_in_buffer =
			integer_operations::cast_or_throw<size_t>
				(srcbufmgr->input->size());
	}


	boolean fill_input_buffer(j_decompress_ptr cinfo)
	{
		// We have an empty buffer - this shouldn't actually happen,
		// unless the image is corrupt.
		// Thus, jump out here, return shouldn't be reached.
		ERREXIT(cinfo, JERR_INPUT_EOF);
		return TRUE;
	}


	void skip_input_data(j_decompress_ptr cinfo, long num_bytes)
	{
		size_t num_bytes_checked =
			integer_operations::cast_or_throw<size_t>(num_bytes);

		if (num_bytes_checked > cinfo->src->bytes_in_buffer)
		{
			// Forced exit if we jump outside the buffer.
			ERREXIT(cinfo, JERR_INPUT_EOF);
		}

		// Jump within the buffer
		cinfo->src->next_input_byte += num_bytes_checked;
		cinfo->src->bytes_in_buffer -= num_bytes_checked;
	}


	void term_source(j_decompress_ptr cinfo)
	{
		// No action required to close down input data_buffer.
	}


	void init_default_source
		(jpeg_source_buffer_mgr &srcbufmgr,
		 const libjpeg_bind::data_buffer& input)
	{
		srcbufmgr.srcmgr.init_source = &init_source;
		srcbufmgr.srcmgr.fill_input_buffer = &fill_input_buffer;
		srcbufmgr.srcmgr.skip_input_data = &skip_input_data;
		srcbufmgr.srcmgr.resync_to_restart = &jpeg_resync_to_restart;
		srcbufmgr.srcmgr.term_source = &term_source;

		srcbufmgr.input = &input;
	}
}




//////////////////////////////////////////////////////////////////////////
//
//  libjpeg_bind
//
//////////////////////////////////////////////////////////////////////////


libjpeg_bind::data_buffer libjpeg_bind::compress
	(const image_rgba& input, int8 quality)
{
	// Check input consistency.
	if (input.size().min() < 0 ||
		input.size().max() > max_image_size_)
	{
		LOG_ERROR("Inconsistent or invalid input size for image compression.");
		throw invalid_parameter();
	}

	if (quality < 0 || quality > 100)
	{
		LOG_ERROR("Requested quality exceeds supported bounds [0, ..., 100]");
		throw bounds_exceeded();
	}


	// Fetch validated image size.
	int64 size_x = input.size().x;
	int64 size_y = input.size().y;


	// Setup: Error manager, compression master structure,
	// destination manager. This order is required to catch
	// errors on libjpeg construction!
	// Cleanup is automatically performed by a scope guard
	// once setup finishes.
	jpeg_error_mgr errmgr;
	libjpeg_error_handler::init_default_error(errmgr);

	jpeg_compress_struct cinfo;
	cinfo.err = &errmgr;

	jpeg_create_compress(&cinfo);
	SCOPE_GUARD
		(compression_guard, (&jpeg_destroy_compress, &cinfo));

	data_buffer output;	
	
	libjpeg_destination_handler::jpeg_destination_buffer_mgr dstbufmgr;
	libjpeg_destination_handler::init_default_destination
		(dstbufmgr, output, data_block_grow_size_);
	cinfo.dest = &dstbufmgr.dstmgr;


	// Apply compression parameters:
	// Target size, target RGB colorspace, quality settings.
	cinfo.image_width =
		integer_operations::cast_or_throw<JDIMENSION>(size_x);
	cinfo.image_height =
		integer_operations::cast_or_throw<JDIMENSION>(size_y);
	cinfo.input_components = 3;
	cinfo.in_color_space = JCS_RGB;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);


	// Perform compression into target buffer.
	// For each scanline, the alpha channel of the input buffer
	// must be removed along the way.
	jpeg_start_compress(&cinfo, TRUE);

	if (cinfo.jpeg_width != cinfo.image_width ||
		cinfo.jpeg_height != cinfo.image_height)
	{
		LOG_ERROR("Inconsistent size on compressed image.");
		throw compression_failed();
	}

	dynamic_buffer<unsigned char> rgb_row_buffer;
	rgb_row_buffer.resize(size_x * 3);

	for (int64 y = 0; y < size_y; ++y)
	{
		const image_rgba::pixel_type *rgba_input_row =
			input.data() + y * size_x;

		for (int64 x = 0; x < size_x; ++x)
		{
			rgb_row_buffer[x * 3] = rgba_input_row[x].r;
			rgb_row_buffer[x * 3 + 1] = rgba_input_row[x].g;
			rgb_row_buffer[x * 3 + 2] = rgba_input_row[x].b;
		}

		JSAMPROW scanline =
			const_cast<JSAMPROW>(rgb_row_buffer.data());
		jpeg_write_scanlines(&cinfo, &scanline, 1);
	}

	jpeg_finish_compress(&cinfo);


	// Success.
	return std::move(output);
}


libjpeg_bind::data_buffer libjpeg_bind::compress
	(const image_greyscale& input, int8 quality)
{
	// We directly provide untyped image data
	// to libjpeg later on, so ensure consistency.
	if (image_greyscale::bytes_per_pixel != 1)
	{
		LOG_ERROR("Greyscale image does not have one byte per pixel.");
		throw assert_failed();
	}


	// Check input consistency.
	if (input.size().min() < 0 ||
		input.size().max() > max_image_size_)
	{
		LOG_ERROR("Inconsistent or invalid input size for image compression.");
		throw invalid_parameter();
	}

	if (quality < 0 || quality > 100)
	{
		LOG_ERROR("Requested quality exceeds supported bounds [0, ..., 100]");
		throw bounds_exceeded();
	}


	// Fetch validated image size.
	int64 size_x = input.size().x;
	int64 size_y = input.size().y;


	// Setup: Error manager, compression master structure,
	// destination manager. This order is required to catch
	// errors on libjpeg construction!
	// Cleanup is automatically performed by a scope guard
	// once setup finishes.
	jpeg_error_mgr errmgr;
	libjpeg_error_handler::init_default_error(errmgr);

	jpeg_compress_struct cinfo;
	cinfo.err = &errmgr;

	jpeg_create_compress(&cinfo);
	SCOPE_GUARD
		(compression_guard, (&jpeg_destroy_compress, &cinfo));

	data_buffer output;

	libjpeg_destination_handler::jpeg_destination_buffer_mgr dstbufmgr;
	libjpeg_destination_handler::init_default_destination
		(dstbufmgr, output, data_block_grow_size_);
	cinfo.dest = &dstbufmgr.dstmgr;


	// Apply compression parameters:
	// Target size, greyscale colorspace, quality settings.
	cinfo.image_width =
		integer_operations::cast_or_throw<JDIMENSION>(size_x);
	cinfo.image_height =
		integer_operations::cast_or_throw<JDIMENSION>(size_y);
	cinfo.input_components = 1;
	cinfo.in_color_space = JCS_GRAYSCALE;

	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE);


	// Perform compression into target buffer.
	// No need for conversions, libjpeg accepts our format.
	jpeg_start_compress(&cinfo, TRUE);

	if (cinfo.jpeg_width != cinfo.image_width ||
		cinfo.jpeg_height != cinfo.image_height)
	{
		LOG_ERROR("Inconsistent size on compressed image.");
		throw compression_failed();
	}

	for (int64 y = 0; y < size_y; ++y)
	{
		JSAMPROW scanline =
			const_cast<JSAMPROW>
				(input.untyped_data() + y * size_x);

		jpeg_write_scanlines(&cinfo, &scanline, 1);
	}

	jpeg_finish_compress(&cinfo);


	// Success.
	return std::move(output);
}


image_rgba libjpeg_bind::decompress_rgba
	(const data_buffer& input)
{
	// Setup: Error manager, decompression master structure,
	// source manager. This order is required to catch errors
	// on libjpeg construction!
	// Cleanup is automatically performed by a scope guard
	// once setup finishes.
	jpeg_error_mgr errmgr;
	libjpeg_error_handler::init_default_error(errmgr);

	jpeg_decompress_struct cinfo;
	cinfo.err = &errmgr;

	jpeg_create_decompress(&cinfo);
	SCOPE_GUARD
		(decompression_guard, (&jpeg_destroy_decompress, &cinfo));

	libjpeg_source_handler::jpeg_source_buffer_mgr srcbufmgr;
	libjpeg_source_handler::init_default_source(srcbufmgr, input);
	cinfo.src = &srcbufmgr.srcmgr;


	// Extract header.
	// Internal consistency checks.
	jpeg_read_header(&cinfo, TRUE);

	if (cinfo.image_width >
			integer_operations::cast_or_throw<JDIMENSION>(max_image_size_) ||
		cinfo.image_height >
			integer_operations::cast_or_throw<JDIMENSION>(max_image_size_))
	{
		LOG_ERROR("Image exceeds maximum size for decompression.");
		throw bounds_exceeded();
	}


	// Retrieve uncompressed RGB data.
	// Each scanline is converted to RGBA along the way.
	// Output sizes and component count are filled automatically,
	// and thus checked for consistency.
	cinfo.out_color_space = JCS_RGB;

	jpeg_start_decompress(&cinfo);

	if (cinfo.output_width != cinfo.image_width ||
		cinfo.output_height != cinfo.image_height ||
		cinfo.output_components != 3)
	{
		LOG_ERROR("Inconsistent size or depth on decompressed image.");
		throw compression_failed();
	}

	int64 size_x = 
		integer_operations::cast_or_throw<int64>(cinfo.image_width);
	int64 size_y =
		integer_operations::cast_or_throw<int64>(cinfo.image_height);

	image_rgba output
		(vector2i(size_x, size_y));

	dynamic_buffer<unsigned char> rgb_row_buffer;
	rgb_row_buffer.resize(size_x * 3);

	for (int64 y = 0; y < size_y; ++y)
	{
		JSAMPROW scanline = rgb_row_buffer.data();
		jpeg_read_scanlines(&cinfo, &scanline, 1);

		image_rgba::pixel_type *rgba_output_row =
			output.data() + y * size_x;

		for (int64 x = 0; x < size_x; ++x)
		{
			rgba_output_row[x].r = rgb_row_buffer[x * 3];
			rgba_output_row[x].g = rgb_row_buffer[x * 3 + 1];
			rgba_output_row[x].b = rgb_row_buffer[x * 3 + 2];
			rgba_output_row[x].a = 0;
		}
	}

	jpeg_finish_decompress(&cinfo);


	// Success.
	return std::move(output);
}


image_greyscale libjpeg_bind::decompress_greyscale
	(const data_buffer& input)
{
	// We directly fetch untyped image data
	// from libjpeg later on, so ensure consistency.
	if (image_greyscale::bytes_per_pixel != 1)
	{
		LOG_ERROR("Greyscale image does not have one byte per pixel.");
		throw assert_failed();
	}


	// Setup: Error manager, decompression master structure,
	// source manager. This order is required to catch errors
	// on libjpeg construction!
	// Cleanup is automatically performed by a scope guard
	// once setup finishes.
	jpeg_error_mgr errmgr;
	libjpeg_error_handler::init_default_error(errmgr);

	jpeg_decompress_struct cinfo;
	cinfo.err = &errmgr;

	jpeg_create_decompress(&cinfo);
	SCOPE_GUARD
		(decompression_guard, (&jpeg_destroy_decompress, &cinfo));

	libjpeg_source_handler::jpeg_source_buffer_mgr srcbufmgr;
	libjpeg_source_handler::init_default_source(srcbufmgr, input);
	cinfo.src = &srcbufmgr.srcmgr;


	// Extract header.
	// Internal consistency checks.
	jpeg_read_header(&cinfo, TRUE);

	if (cinfo.image_width >
			integer_operations::cast_or_throw<JDIMENSION>(max_image_size_) ||
		cinfo.image_height >
			integer_operations::cast_or_throw<JDIMENSION>(max_image_size_))
	{
		LOG_ERROR("Image exceeds maximum size for decompression.");
		throw bounds_exceeded();
	}


	// Retrieve uncompressed greyscale data.
	// No need for conversions, libjpeg fills the required format.
	// Output sizes and component count are filled automatically,
	// and thus checked for consistency.
	cinfo.out_color_space = JCS_GRAYSCALE;

	jpeg_start_decompress(&cinfo);

	if (cinfo.output_width != cinfo.image_width ||
		cinfo.output_height != cinfo.image_height ||
		cinfo.output_components != 1)
	{
		LOG_ERROR("Inconsistent size or depth on decompressed image.");
		throw compression_failed();
	}

	int64 size_x = 
		integer_operations::cast_or_throw<int64>(cinfo.image_width);
	int64 size_y =
		integer_operations::cast_or_throw<int64>(cinfo.image_height);

	image_greyscale output
		(vector2i(size_x, size_y));

	for (int64 y = 0; y < size_y; ++y)
	{
		JSAMPROW scanline =
			const_cast<JSAMPROW>
				(output.untyped_data() + y * size_x);

		jpeg_read_scanlines(&cinfo, &scanline, 1);
	}

	jpeg_finish_decompress(&cinfo);


	// Success.
	return std::move(output);
}




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_LIBJPEG) */
