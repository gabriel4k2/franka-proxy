/**
 *************************************************************************
 *
 * @file riff_util.hpp
 *
 * Utilities for RIFF files.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RIFF_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__RIFF_UTIL_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "string_forward.hpp"
#include "file_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class riff_util
 * @ingroup file_module
 *
 * Static utilities for parsing files in RIFF format.
 *
 * Usage notes:
 * - The RIFF utilities internally apply uppercase for all FOURCC IDs
 *		to increase robustness against non-conformant files.
 * - There are implementation-specific limits to the number of chunks
 *		and to the chunk size as a safeguard against broken files.
 *
 ************************************************************************/
class riff_util :
	private non_constructable
{

	public:

		/**
		 * Map from chunk ID to byte offset in RIFF file.
		 * Note offsets refer to the position after the chunk ID.
		 * Offsets are int64 as used in the @ref file class.
		 */
		typedef tree_map<string, int64> chunk_id_to_offset_map;


		/**
		 * Check the header of a RIFF @bref{f}ile,
		 * verify the @bref{required_file_type_id},
		 * and map chunk IDs to byte offsets.
		 *
		 * The call throws on any error (e.g. too large chunks).
		 * In this case, the call clears @bref{chunk_id_to_offset}.
		 *
		 * Both on success and on failure, chunk mapping leaves
		 * the @bref{f}ile at an arbitrary read/write position.
		 */
		static void check_file_and_map_chunks
			(file& f,
			 const string& required_file_type_id,
			 chunk_id_to_offset_map& chunk_id_to_offset);


		/**
		 * Implementation-specific limits for RIFF files.
		 * Chunk length is 32 bits to match with RIFF precision.
		 */
		//@{
		static const int64 max_chunk_count;
		static const int32 max_chunk_length;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RIFF_UTIL_HPP) */
