/**
 *************************************************************************
 *
 * @file zip.hpp
 *
 * Header for handling zip archives.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ZIP_HPP)
#define INCLUDED__VIRAL_CORE__ZIP_HPP


#include "build_config.hpp"


#include "string.hpp"
#include "tree_set.hpp"
#include "tree_map.hpp"
#include "file_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class zip_entry
 * @ingroup file_module
 *
 * Stores information about a single file in a zip archive,
 * such as the size of the file or the byte offset to the file.
 *
 ************************************************************************/
class zip_entry
{

	public:

		/**
		 * Create entry from the provided zip header information.
		 * Here, @bref{file_path} is the relative path of the file in the archive,
		 * and @bref{archive_path} is the absolute, OS-specific path of the archive.
		 */
		zip_entry
			(const string& file_path, const string& archive_path,
			 int64 header_position, int64 data_position,
			 uint16 flags, uint16 zip_type, uint32 crc_value,
			 uint32 compressed_size, uint32 uncompressed_size);


		/**
		 * Read zip_entry from the current position in a source @bref{archive}
		 * that corresponds to the absolute, OS-specific @bref{archive_path}.
		 *
		 * Detailed explanations for the zip_entry header and the archive layout
		 * are found in PKWAREs zip application notes file.
		 */
		static zip_entry read
			(disk_file& archive, const string& archive_path);


		bool operator !=(const zip_entry& other);


		const string file_path;
		const string archive_path;

		const int64 header_position;
		const int64 data_position;

		const uint16 flags;
		const uint16 zip_type;
		const uint32 crc_value;

		const uint32 compressed_size;
		const uint32 uncompressed_size;


	private:

		static const uint32 zip_header_signature_;
};




/**
 *************************************************************************
 *
 * @class zip_index
 * @ingroup file_module
 * 
 * A fast lookup map for files and folders over several zip archives
 * which caches zip_entry headers for fast access to archived files.
 *
 * Usage notes:
 * - On startup, applications build a zip_index for all resource archives
 *		inside their application folder. The index is then used as a cache
 *		to speed up any file search or file opening operations.
 * - To access a single file in a zip archive, clients build a zip_file
 *		through a zip_entry from the zip_index.
 * - @attent
 *		Cached zip entries must remain synchronized to actual archives.
 *		Thus, archives must be run-time constant (i.e. read-only files). 
 *		As a precaution, file access (e.g. with a zip_file) should check
 *		in-memory zip_entry objects against actual file data.
 * - The folder structure of a zip_index may be inconsistent, for example
 *		due to a load-time exception. Most clients applications can live
 *		with this, though.
 *
 * Limitations:
 * - Only uncompressed zip archives are supported.
 * - All file paths in a zip archive must be relative virtual_path%s.
 *		OS quirks (e.g. backslashes on Windows) are fixed automatically.
 * - Certain options are unsupported (e.g. data before first zip entry,
 *		postponed size, zip64, ...) or ignored (e.g. empty entries).
 * - There is a maximum supported size to content files.
 *
 ************************************************************************/
class zip_index
{

	public:

		typedef tree_map<string, zip_entry> entry_map;

		typedef tree_set<string> subordinate_path_set;
		typedef tree_map<string, subordinate_path_set> folder_map;


		zip_index();


		/**
		 * Add the contents of a zip archive with the OS-specific
		 * absolute @bref{archive_path} to the index.
		 */
		void add_archive(const string& archive_path);


		/**
		 * Cache file @bref{entry} and create associated folder structure.
		 * If an entry with an ambiguous virtual_path exists in the index,
		 * ignore the new entry and log a warning message.
		 */
		void add_entry(const zip_entry& entry);


		/** Map of virtual_path%s for files to cached zip_entry objects. */
		entry_map entries;


		/**
		 * Map of virtual_path%s for folders to sets of virtual_path%s
		 * subordinate to each folder path.
		 *
		 * A subordinate path may at the same time refer to:
		 * - A file: there is a corresponding zip_entry in @bref{entries}.
		 * - A folder: there is a corresponding entry in @bref{folders}
		 *		that lists subordinate paths of that folder.
		 *
		 * Finally, there always is a dummy empty folder ""
		 * that groups together all root level paths.
		 */
		folder_map folders;


	private:

		static const uint32 zip_central_directory_signature_;
		static const uint16 zip_encryption_flag_;
		static const uint16 zip_size_postponed_flag_;
		static const uint32 zip_compression_stored_;

		static const uint32 max_uncompressed_file_size_;
};




/**
 *************************************************************************
 *
 * @class zip_file
 * @ingroup file_module
 *
 * Variant of @ref file that reads a single file from a zip archive.
 *
 * Usage notes:
 * - File operations (e.g. seek(), read()) fail and throw
 *		on exceeding the data of the target file in the archive.
 *
 * Limitations:
 * - Compressed zip archives currently are not supported.
 * - Writing to archives is not trivial and not supported by zip_file.
 *
 * Implementation notes:
 * - To avoid perpetual synchronization with the backing disk_file,
 *		we track a separate @bref{position_in_entry_} and only move
 *		the disk_file position on read_bytes().
 *
 ************************************************************************/
class zip_file : public file
{

	public:

		/**
		 * Open the zip archive that @bref{entry} is stored in,
		 * and restrict reading to the corresponding file data.
		 *
		 * If the @bref{entry} does not match its on-disk version,
		 * the constructor fails with an exception.
		 */
		zip_file(const zip_entry& entry);

		~zip_file() NOTHROW override;

		void seek(int64 offset, seek_mode source) override;
		int64 tell() override;
		int64 find_size() override;
		bool eof() override;

		void read_bytes(void* data, int64 count) override;
		void write_bytes(const void* data, int64 count) override;


	private:

		void validate_entry_header();


		disk_file archive_;

		zip_entry entry_;
		int64 position_in_entry_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ZIP_HPP) */
