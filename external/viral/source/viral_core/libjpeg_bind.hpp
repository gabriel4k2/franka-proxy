/**
 *************************************************************************
 *
 * @file libjpeg_bind.hpp
 *
 * Header for the libjpeg wrapper.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__LIBJPEG_BIND_HPP)
#define INCLUDED__VIRAL_CORE__LIBJPEG_BIND_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_LIBJPEG)


#include "container_forward.hpp"
#include "image_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class libjpeg_bind
 * @ingroup image_module
 *
 * Provides static wrapper calls around libjpeg
 * with support for exceptions and dynamic containers.
 *
 * Compression notes:
 * - Compressed images are stored in the lossy JPG format.
 *		Respective data buffers hold the entire JPG stream,
 *		including JPG headers and JPG markers.
 * - Only 3 components and 24 bit RGB colorspaces are supported
 *		on JPEG-compressed images. Compression discards alpha,
 *		while decompression zero-initializes alpha.
 * - Compression quality follows the standard suggestions,
 *		where 1 means lowest and 100 means highest quality.
 *
 * General behaviour:
 * - The libjpeg_bind class is only included into compilation
 *		if @ref VIRAL__HAVE_LIBJPEG is set.
 * - There is an implementation-defined (although large) maximum
 *		supported image size to avoid memory overflows.
 * - As libjpeg states to be reentrant, libjpeg wrappers are threadsafe
 *		for an arbitrary number of threads at a time.
 *
 ************************************************************************/
class libjpeg_bind :
	private non_constructable
{

	public:

		typedef dynamic_buffer<unsigned char> data_buffer;


		/**
		 * Compress an in-memory @bref{input} image to JPG.
		 * The @bref{quality} setting controls the compression. 
		 */
		//@{
		static data_buffer compress
			(const image_rgba& input, int8 quality);

		static data_buffer compress
			(const image_greyscale& input, int8 quality);
		//@}


		/**
		 * Decompress an image from JPEG-compressed
		 * @bref{input} buffer.
		 */
		//@{
		static image_rgba decompress_rgba
			(const data_buffer& input);

		static image_greyscale decompress_greyscale
			(const data_buffer& input);
		//@}


	private:

		/**
		 * The @bref{max_image_size_} must be chosen as to allow
		 * indexing of all image pixels with int64 and JDIMENSION.
		 */
		static const int64 max_image_size_ = 8192;


		static const int64 data_block_grow_size_ = 262144;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_LIBJPEG) */


#endif /* !defined(INCLUDED__VIRAL_CORE__LIBJPEG_BIND_HPP) */
