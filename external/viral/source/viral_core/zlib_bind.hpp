/**
 *************************************************************************
 *
 * @file zlib_bind.hpp
 *
 * ZLib binding header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ZLIB_BIND_HPP)
#define INCLUDED__VIRAL_CORE__ZLIB_BIND_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_ZLIB)


#include "util.hpp"
#include "buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class zlib_bind
 * @ingroup file_module
 *
 * Static ZLib-based compression and decompression wrapper
 * with container and exception support.
 *
 * Notes:
 * - The zlib_bind class is only included into compilation
 *		if @ref VIRAL__HAVE_ZLIB is set.
 * - Compression and decompression throw an exception on failure
 *		(e.g. on corrupt compressed data).
 * - Compression and decompression fail if the input or output data
 *		exceeds a large, implementation-specific maximum size.
 * - ZLib wrappers are reentrant and thus threadsafe.
 *
 ************************************************************************/
class zlib_bind :
	private non_constructable
{

	public:

		typedef dynamic_buffer<unsigned char> data_buffer;


		/** Compress the provided raw @bref{input}. */
		static data_buffer compress
			(const data_buffer &input);


		/** Decompress a deflate-compressed @bref{input}. */
		static data_buffer decompress
			(const data_buffer &input);


	private:

		static const int64 max_input_size_;
		static const int64 max_output_size_;

		static const int max_input_chunk_size_;
		static const int output_chunk_size_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_ZLIB) */


#endif /* !defined(INCLUDED__VIRAL_CORE__ZLIB_BIND_HPP) */
