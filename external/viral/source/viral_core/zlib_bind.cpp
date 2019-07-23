/**
 *************************************************************************
 *
 * @file zlib_bind.cpp
 *
 * ZLib binding implementation.
 *
 ************************************************************************/


#include "zlib_bind.hpp"


#if defined(VIRAL__HAVE_ZLIB)


#include <zlib.h>


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_checks.hpp"
#include "integer_operations.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// zlib_bind
//
//////////////////////////////////////////////////////////////////////////


zlib_bind::data_buffer zlib_bind::compress
	(const data_buffer &input)
{
	// Check input bounds.
	if (input.size() > max_input_size_)
	{
		LOG_ERROR
			("Input data exceeds maximum size for ZLib compression.");
		throw bounds_exceeded();
	}


	// Prepare input and output streams.
	// Internally, ZLib works with int sizes, usually 32 bit.
	// This means we must split int64 input and output sizes
	// over multiple deflate() calls.
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	int64 input_bytes_remaining = input.size();

	stream.avail_in =
		integer_operations::cast_or_throw<uInt>
			(min(input_bytes_remaining,
				 max_input_chunk_size_));
	stream.next_in =
		const_cast<Bytef*>(input.data());

	input_bytes_remaining -= stream.avail_in;

	data_buffer output;
	output.resize(output_chunk_size_);

	stream.avail_out =
		integer_operations::cast_or_throw<uInt>
			(output_chunk_size_);
	stream.next_out =
		const_cast<Bytef*>(output.data());


	// Initialization.
	// Scope guard ensures that the stream object
	// is correctly released on any method exit.
	if (deflateInit(&stream, Z_DEFAULT_COMPRESSION) != Z_OK)
	{
		LOG_ERROR("ZLib initialization failed.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(compression_guard, (deflateEnd, &stream));
	

	// Main compression loop.
	// May throw if the output buffer fails to reallocate.
	while(true)
	{

		// Compress some data.
		// Use Z_FINISH to indicate all input has been consumed,
		// which causes Z_STREAM_END after writing all output.
		int retcode =
			deflate(&stream,
					input_bytes_remaining == 0 ?
						Z_FINISH : Z_NO_FLUSH);


		// Success.
		// Remove overallocated space at the end
		// of the output buffer.
		if (retcode == Z_STREAM_END)
		{
			output.resize(output.size() - stream.avail_out);
			break;
		}


		// Handle input underflow and output overflow.
		// Also check for exceeding bounds here.
		if (retcode == Z_OK || retcode == Z_BUF_ERROR)
		{
			if (stream.avail_out != 0 && stream.avail_in != 0)
			{
				// This should never happen: ZLib made no progress,
				// but it still has input data and output space.
				LOG_ERROR("ZLib compression reported unexpected buffer error.")
				throw assert_failed();
			}

			if (stream.avail_in == 0 && input_bytes_remaining > 0)
			{
				stream.avail_in =
					integer_operations::cast_or_throw<uInt>
						(min(input_bytes_remaining,
							 max_input_chunk_size_));

				stream.next_in += max_input_chunk_size_;
				input_bytes_remaining -= stream.avail_in;
			}

			if (stream.avail_out == 0)
			{
				if (!integer_checks::is_add_safe(output.size(), output_chunk_size_) ||
					output.size() + output_chunk_size_ > max_output_size_)
				{
					LOG_ERROR
						("Output data exceeded maximum size for ZLib compression.");
					throw bounds_exceeded();
				}

				int64 next_offset = output.size();
				output.resize(output.size() + output_chunk_size_); // Reallocation!
					
				stream.avail_out =
					integer_operations::cast_or_throw<uInt>
						(output_chunk_size_);
				stream.next_out =
					const_cast<Bytef*>(output.data() + next_offset);
			}
		
			continue;
		}


		// Handle any of these fatal errors:
		// Z_NEED_DICT, Z_DATA_ERROR, Z_STREAM_ERROR, Z_MEM_ERROR, ...		
		LOG_ERROR("ZLib compression failed, usually due to an internal error.");
		throw compression_failed();
	}


	// Success.
	return std::move(output);
}


zlib_bind::data_buffer zlib_bind::decompress
	(const data_buffer &input)
{
	// Check input bounds.
	if (input.size() > max_input_size_)
	{
		LOG_ERROR
			("Input data exceeds maximum size for ZLib decompression.");
		throw bounds_exceeded();
	}


	// Prepare input and output streams.
	// Internally, ZLib works with int sizes, usually 32 bit.
	// This means we must split int64 input and output sizes
	// over multiple inflate() calls.
	z_stream stream;
	stream.zalloc = Z_NULL;
	stream.zfree = Z_NULL;
	stream.opaque = Z_NULL;

	int64 input_bytes_remaining = input.size();

	stream.avail_in =
		integer_operations::cast_or_throw<uInt>
			(min(input_bytes_remaining,
				 max_input_chunk_size_));
	stream.next_in =
		const_cast<Bytef*>(input.data());

	input_bytes_remaining -= stream.avail_in;

	data_buffer output;
	output.resize(output_chunk_size_);

	stream.avail_out =
		integer_operations::cast_or_throw<uInt>
			(output_chunk_size_);
	stream.next_out =
		const_cast<Bytef*>(output.data());


	// Initialization.
	// Scope guard ensures that the stream object
	// is correctly released on any method exit.
	if (inflateInit(&stream) != Z_OK)
	{
		LOG_ERROR("ZLib initialization failed.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(decompression_guard, (inflateEnd, &stream));		
	

	// Main decompression loop.
	// May throw if the output buffer fails to reallocate.
	while (true)
	{

		// Decompress some data.
		// No Z_FINISH, there is a marker in compressed data
		// that automatically causes Z_STREAM_END if reached.
		int retcode =
			inflate(&stream, Z_NO_FLUSH);


		// Success.
		// Remove overallocated space at the end
		// of the output buffer.
		if (retcode == Z_STREAM_END)
		{
			output.resize(output.size() - stream.avail_out);
			break;
		}


		// Handle input underflow and output overflow.
		// Also check for exceeding bounds here.
		if (retcode == Z_OK || retcode == Z_BUF_ERROR)
		{
			if (stream.avail_out != 0 && stream.avail_in != 0)
			{
				// This should never happen: ZLib made no progress,
				// but it still has input data and output space.
				LOG_ERROR("ZLib decompression reported unexpected buffer error.")
				throw assert_failed();
			}

			if (stream.avail_in == 0 && input_bytes_remaining > 0)
			{
				stream.avail_in =
					integer_operations::cast_or_throw<uInt>
						(min(input_bytes_remaining,
							 max_input_chunk_size_));

				stream.next_in += max_input_chunk_size_;
				input_bytes_remaining -= stream.avail_in;
			}

			if (stream.avail_out == 0)
			{
				if (!integer_checks::is_add_safe(output.size(), output_chunk_size_) ||
					output.size() + output_chunk_size_ > max_output_size_)
				{
					LOG_ERROR
						("Output data exceeded maximum size for ZLib decompression.");
					throw bounds_exceeded();
				}

				int64 next_offset = output.size();
				output.resize(output.size() + output_chunk_size_); // Reallocation!
					
				stream.avail_out =
					integer_operations::cast_or_throw<uInt>
						(output_chunk_size_);
				stream.next_out =
					const_cast<Bytef*>(output.data() + next_offset);
			}
		
			continue;
		}


		// Handle any of these fatal errors:
		// Z_NEED_DICT, Z_DATA_ERROR, Z_STREAM_ERROR, Z_MEM_ERROR, ...		
		LOG_ERROR("ZLib decompression failed, usually due to corrupt input data.");
		throw compression_failed();
	}	


	// Success.
	return std::move(output);
}


const int64 zlib_bind::max_input_size_ = 17179869184; // ~~ 16 GB
const int64 zlib_bind::max_output_size_ = 17179869184; // ~~ 16 GB

const int zlib_bind::max_input_chunk_size_ = 1048576; // ~~ 1 MB
const int zlib_bind::output_chunk_size_ = 65536; // ~~ 64 KB




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_ZLIB) */
