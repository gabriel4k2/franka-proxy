/**
 *************************************************************************
 *
 * @file zip.cpp
 *
 * Zip archives, implementation.
 *
 ************************************************************************/


#include "zip.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "integer_operations.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	zip_entry
//
//////////////////////////////////////////////////////////////////////////


zip_entry::zip_entry
	(const string& file_path, const string& archive_path,
	 int64 header_position, int64 data_position,
	 uint16 flags, uint16 zip_type, uint32 crc_value,
	 uint32 compressed_size, uint32 uncompressed_size)
	:
	file_path(file_path), archive_path(archive_path),
	header_position(header_position), data_position(data_position),
	flags(flags), zip_type(zip_type), crc_value(crc_value),
	compressed_size(compressed_size), uncompressed_size(uncompressed_size)
{ }


zip_entry zip_entry::read
	(disk_file& archive, const string& archive_path)
{
	int64 header_position = archive.tell();


	// We don't check much data here, because zip_index
	// performs most checks on initial archive parsing.


	// Extract and check header signature.
	uint32 signature;
	archive.read(signature);

	if (signature != zip_header_signature_)
	{
		LOG_ERROR("Invalid zip header signature in archive: " + archive_path);
		throw corrupt_data();
	}


	// Extract various parameters.
	// Ignore version, modification date and time.
	uint16 flags;
	uint16 zip_type;
	uint32 crc_value;
	uint32 compressed_size;
	uint32 uncompressed_size;
	uint16 pathlen;
	uint16 extralen;

	archive.seek(sizeof(uint16), file::seek_current);
	archive.read(flags);
	archive.read(zip_type);
	archive.seek(sizeof(uint16) * 2, file::seek_current);
	archive.read(crc_value);
	archive.read(compressed_size);
	archive.read(uncompressed_size);
	archive.read(pathlen);
	archive.read(extralen);


	// Extract file path and correct path slashes
	// in accordance to virtual_path conventions.
	string file_path;
	file_path.resize(pathlen);
	archive.read_bytes(file_path.data(), pathlen);
	file_path.replace_all('\\', '/');


	// Calculate absolute offset to actual file data.
	// Ignore in-between extra data.
	int64 data_position =
		integer_operations::add_or_throw
			(archive.tell(), extralen);


	return
		zip_entry(file_path, archive_path,
				  header_position, data_position,
				  flags, zip_type, crc_value,
				  compressed_size, uncompressed_size);
}


bool zip_entry::operator !=(const zip_entry& other)
{
	return
		file_path != other.file_path ||
		archive_path != other.archive_path ||
		header_position != other.header_position ||
		data_position != other.data_position ||
		flags != other.flags ||
		zip_type != other.zip_type ||
		crc_value != other.crc_value ||
		compressed_size != other.compressed_size ||
		uncompressed_size != other.uncompressed_size;
}


const uint32 zip_entry::zip_header_signature_ = 0x04034b50;




//////////////////////////////////////////////////////////////////////////
//
//  zip_index
//
//////////////////////////////////////////////////////////////////////////


zip_index::zip_index()
{
	// Insert dummy root.
	folders.insert("", subordinate_path_set());
}


void zip_index::add_archive(const string& archive_path)
{
	disk_file archive(archive_path, file::read_only);


	// Note that this loading routine is not universally correct,
	// as it reads zip_entry objects from the front of the archive
	// instead of the central directory at the end of the archive.
	while (true)
	{

		// Check valid end-of-archive conditions:
		// Either we are at the EoF of the complete archive,
		// or we arrived at the central directory structure.
		if (!archive) return;

		uint32 central_directory_signature = 0;
		archive.read(central_directory_signature);
		archive.seek(-integer_operations::cast_or_throw<int64>
						(sizeof(central_directory_signature)),
					 file::seek_current);

		if (central_directory_signature == zip_central_directory_signature_)
			return;


		// Not at the end of the archive, so there is another entry
		// to read and process.
		//
		// Notes:
		// - We perform more checks than the general zip_entry::read().
		//		In particular, files with unsupported compression modes
		//		abort the entire parsing process, as these could break
		//		reading further files.
		// - Some zip entries refer to folders and should be ignored.
		//		As there is no flag that indicates a folder, we ignore
		//		everything that comes without a size.
		zip_entry entry
			(zip_entry::read(archive, archive_path));

		if ((entry.flags & zip_encryption_flag_) != 0)
		{
			LOG_ERROR("Encrypted archive not supported: " + archive_path);
			throw unsupported_feature();
		}

		if ((entry.flags & zip_size_postponed_flag_) != 0)
		{
			LOG_ERROR("Postpone size not supported in archive: "  + archive_path);
			throw unsupported_feature();
		}

		if (entry.uncompressed_size > max_uncompressed_file_size_)
		{
			LOG_ERROR
				("File exceeding maximum size of: " + max_uncompressed_file_size_ +
				 " bytes in archive: " + archive_path);
			throw bounds_exceeded();
		}

		if (entry.zip_type != zip_compression_stored_)
		{
			LOG_ERROR("Compressed files not supported in archive: " + archive_path);
			throw unsupported_feature();
		}

		if (entry.uncompressed_size == 0)
			continue;

		if (!virtual_path::is_valid(entry.file_path))
		{
			LOG_ERROR
				("File: " + entry.file_path + " in archive: " + archive_path +
				 " has invalid path and is ignored.");
			continue;
		}

		add_entry(entry);		

		int64 next_entry_position =
			integer_operations::add_or_throw
				(entry.data_position, entry.uncompressed_size);

		archive.seek
			(next_entry_position, file::seek_begin);
	}
}


void zip_index::add_entry(const zip_entry& entry)
{
	// Insert entry into cache, but do not allow
	// to replace an existing file.
	if (entries.contains(entry.file_path))
	{
		LOG_WARN("File with path: " + entry.file_path + 
				 " is contained in more than one zip archive.");
		return;
	}

	entries.try_insert(entry.file_path, entry);


	// Iteratively add the entry and all its parent folders
	// into the folder lookup structure. We can safely abort
	// once we arrive at the first, already inserted folder.
	string insert_path(entry.file_path);

	while (insert_path != "")
	{
		string parent_folder
			(virtual_path::extract_parent(insert_path));

		folder_map::iterator it(folders.find(parent_folder));
		if (it)
		{
			it->value.insert(insert_path);
			break;
		}

		it = folders.insert(parent_folder, subordinate_path_set());
		it->value.insert(insert_path);
		insert_path = parent_folder;
	}
}


const uint32 zip_index::zip_central_directory_signature_ = 0x02014b50;
const uint16 zip_index::zip_encryption_flag_ = (1 << 0);
const uint16 zip_index::zip_size_postponed_flag_ = (1 << 3);
const uint32 zip_index::zip_compression_stored_ = 0;


/** ~2GB, fits in int32 range as a safety precaution. */
const uint32 zip_index::max_uncompressed_file_size_ = 2147483647; 




//////////////////////////////////////////////////////////////////////////
//
//  zip_file
//
//////////////////////////////////////////////////////////////////////////


zip_file::zip_file(const zip_entry& entry)
	:
	archive_(entry.archive_path, read_only),
	entry_(entry),
	position_in_entry_(0)
{
	validate_entry_header();
}


zip_file::~zip_file() NOTHROW { }


void zip_file::seek(int64 offset, seek_mode source)
{
	// Determine new position
	// relative to entry data.
	int64 new_position_in_entry = -1;

	if (source == seek_begin)
	{
		new_position_in_entry = offset;
	}
	else if (source == seek_end)
	{
		new_position_in_entry =
			integer_operations::add_or_throw
				(entry_.uncompressed_size, offset);
	}
	else if (source == seek_current)
	{
		new_position_in_entry =
			integer_operations::add_or_throw
				(position_in_entry_, offset);
	}
	else
	{
		LOG_ERROR("Unknown seeking mode on zipped file: " + entry_.file_path);
		throw file_operation_failed();
	}


	// Check if the new position marker is still valid.
	// Note the position marker may end up at eof().
	if (new_position_in_entry < 0 ||
		new_position_in_entry > entry_.uncompressed_size)
	{
		LOG_ERROR("Cannot seek outside of zipped file: " + entry_.file_path);
		throw file_operation_failed();
	}


	// Success.
	// Exception safety: NOTHROW from here.
	position_in_entry_ =
		new_position_in_entry;
}


int64 zip_file::tell()
	{ return position_in_entry_; }


int64 zip_file::find_size()
	{ return entry_.uncompressed_size; }


bool zip_file::eof()
{
	// End of file is one past the last valid byte.
	return position_in_entry_ >= entry_.uncompressed_size;
}


void zip_file::read_bytes
	(void* data, int64 count)
{
	// Validate parameters.
	if (count < 0 || (count > 0 && !data))
	{
		LOG_ERROR("Invalid read parameters on zipped file: " + entry_.file_path);
		throw invalid_parameter();
	}


	// Fetch and validate reading positions.
	// Note the position marker may end up at eof().
	int64 read_start_in_archive =
		integer_operations::add_or_throw
			(entry_.data_position, position_in_entry_);
	int64 read_end_in_entry =
		integer_operations::add_or_throw
			(position_in_entry_, count);

	if (read_end_in_entry > entry_.uncompressed_size)
	{
		LOG_ERROR("Cannot read past end of ZIP entry");
		throw file_operation_failed();
	}


	// Synch with disk_file and read data.
	archive_.seek(read_start_in_archive, file::seek_begin);
	archive_.read_bytes(data, count);

	
	// Success.
	// NOTHROW from here.
	position_in_entry_ = read_end_in_entry;
}


void zip_file::write_bytes
	(const void* data, int64 count)
{
	LOG_ERROR("Write not supported on zipped file: " + entry_.file_path);
	throw file_operation_failed();
}


void zip_file::validate_entry_header()
{
	archive_.seek
		(entry_.header_position, seek_begin);
	zip_entry compare
		(zip_entry::read(archive_, entry_.archive_path));

	if (compare != entry_)
	{
		LOG_CRITICAL
			("Zip header for file: " + entry_.file_path +
			 " in archive: " + entry_.archive_path +
			 " was modified after header parsing.");
		throw file_operation_failed();
	}
}




} /* namespace viral_core */
