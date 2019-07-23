/**
 *************************************************************************
 *
 * @file libpng_bind.hpp
 *
 * Header for the libpng wrapper.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__LIBPNG_BIND_HPP)
#define INCLUDED__VIRAL_CORE__LIBPNG_BIND_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_LIBPNG)


#include "container_forward.hpp"
#include "image_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class libpng_bind
 * @ingroup image_module
 *
 * Provides static wrapper calls around libpng
 * with support for exceptions and dynamic containers.
 *
 * Compression notes:
 * - Only 8-bit per-component PNG images are supported.
 * - Compressed images are stored in the lossless PNG format.
 *		Respective data buffers hold the entire PNG stream,
 *		including the PNG header.
 * - Compression always yields RGBA PNGs.
 * - On decompression, all greyscale PNGs duplicate grey values
 *		to output R, G, and B channels, and all alpha-less PNGs
 *		zero-initialize the output alpha channel.
 *
 * General behaviour:
 * - The libpng_bind class is only included into compilation
 *		if @ref VIRAL__HAVE_LIBPNG is set.
 * - There is an implementation-defined (although large) maximum
 *		supported image size to avoid memory overflows.
 * - As libpng states to be reentrant, libpng wrappers are threadsafe
 *		for an arbitrary number of threads at a time.
 *
 ************************************************************************/
class libpng_bind :
	private non_constructable
{

	public:

		typedef dynamic_buffer<unsigned char> data_buffer;


		/** Compress an in-memory @bref{input} image to PNG. */
		//@{
		static data_buffer compress
			(const image_rgba& input);

		static data_buffer compress
			(const image_greyscale& input);
		//@}


		/** Decompress an image from PNG-compressed @bref{input}. */
		//@{
		static image_rgba decompress_rgba
			(const data_buffer& input);

		static image_greyscale decompress_greyscale
			(const data_buffer& input);
		//@}


	private:

		/**
		 * The @bref{max_image_size_} must be chosen as to allow
		 * indexing of all image pixels with int64 and png_uint_32.
		 */
		static const int64 max_image_size_ = 8192;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_LIBPNG) */


#endif /* !defined(INCLUDED__VIRAL_CORE__LIBPNG_BIND_HPP) */
