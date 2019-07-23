/**
 *************************************************************************
 *
 * @file riff_util.cpp
 *
 * Utilities for RIFF files, implementation.
 *
 ************************************************************************/


#include "riff_util.hpp"


#include "log.hpp"
#include "scope_guard.hpp"
#include "tree_map.hpp"
#include "string.hpp"
#include "file.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// riff_util
//
//////////////////////////////////////////////////////////////////////////


void riff_util::check_file_and_map_chunks
	(file& f,
	 const string& required_file_type_id,
	 chunk_id_to_offset_map& chunk_id_to_offset)
{
	SCOPE_GUARD
		(chunk_id_to_offset_guard,
			(chunk_id_to_offset, &chunk_id_to_offset_map::clear));


	// Restart reading the file,
	// just in case of prior file operations.
	f.seek(0, file::seek_begin);


	// Read and validate RIFF chunk ID.
	string riff_chunk_id("xxxx");
	f.read_bytes(riff_chunk_id.data(), sizeof(char) * 4);

	if (riff_chunk_id.to_upper() != "RIFF")
	{
		LOG_ERROR("RIFF file has no RIFF chunk ID.");
		throw corrupt_data();
	}


	// Ignore RIFF chunk length.
	// This need not match the file size,
	// but we do not allow tailing content.
	f.seek(4, file::seek_current);


	// Read and validate file type ID.
	string actual_file_type_id("xxxx");
	f.read_bytes(actual_file_type_id.data(), sizeof(char) * 4);

	if (actual_file_type_id.to_upper() !=
		required_file_type_id.to_upper())
	{
		LOG_ERROR("RIFF file should have type ID: " + required_file_type_id +
				  " but has type ID: " + actual_file_type_id);
		throw corrupt_data();
	}


	// Create map with byte offsets
	// to all subordinate chunks.
	chunk_id_to_offset.clear();

	while (!f.eof())
	{
		if (chunk_id_to_offset.size() >= max_chunk_count)
		{
			LOG_ERROR("RIFF file exceeds maximum supported chunk count.");
			throw unsupported_feature();
		}

		string chunk_id("xxxx");
		f.read_bytes(chunk_id.data(), sizeof(char) * 4);

		if (!chunk_id_to_offset.try_insert
				(chunk_id.to_upper(), f.tell()))
		{
			LOG_ERROR("Duplicate chunk ID in RIFF file.");
			throw unsupported_feature();
		}

		int32 chunk_length = 0;
		f.read(chunk_length);

		if (chunk_length < 0 || chunk_length >= max_chunk_length)
		{
			LOG_ERROR("Invalid chunk length in RIFF file.");
			throw corrupt_data();
		}
			
	
		// Add one byte (chunk-less) padding
		// after all odd-sized chunks.
		int32 skip_word_aligned = 
			(chunk_length % 2 == 0) ?
				chunk_length : chunk_length + 1;


		// Skip over all contents of chunk.
		// Upcast int32 chunk length to int64 offset,
		// which never causes any overflow.
		f.seek
			(skip_word_aligned, file::seek_current);
	}


	// Success.
	chunk_id_to_offset_guard.dismiss();
}


const int64 riff_util::max_chunk_count = 64;
const int32 riff_util::max_chunk_length = 536870912; // ~512 MB




} /* namespace viral_core */
