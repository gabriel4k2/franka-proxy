/**
 *************************************************************************
 *
 * @file file_util.hpp
 *
 * Header for file utility classes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__FILE_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__FILE_UTIL_HPP


#include "build_config.hpp"


#include <cstdio>


#include "util.hpp"
#include "string.hpp"
#include "tree_set.hpp"


namespace viral_core
{

 
/**
 *************************************************************************
 *
 * @class virtual_path
 * @ingroup file_module
 *
 * Contains static helper methods for manipulating POSIX-like paths
 * from the virtual file system.
 *
 * Usage notes:
 * - Virtual paths are relative POSIX paths of the form
 *		"folder/sub_folder/file.tmp.ext".
 * - Each slash-separated substring "folder", "sub_folder", "file.tmp.ext"
 *		is referred to as a path element. "file.tmp.ext" is the target element
 *		of the entire path, while "folder/sub_folder" is the parent path, and
 *		"folder" is the root element. "file.tmp" is the title of the target,
 *		and "ext" (after the last fullstop) is the extension of the target.
 * - Paths are case sensitive. 
 * - For portability, only ASCII chars 'a' to 'z', 'A' to 'Z', '0'-'9',
 *		'_' and '.' must be used in a path. A fullstop must not be the
 *		first or the last character in any path element. Empty elements
 *		are not allowed in a path.
 * - An empty path "" is not valid, but is used as a marker for certain
 *		application scenarios - such as to express no file, no parent path,
 *		or no file extension.
 * - For performance reasons, path constraints are not rigidly enforced
 *		throughout code, but file_context%s still check constraints
 *		on saving and on loading.
 *
 ************************************************************************/
class virtual_path :
	private non_constructable
{

	public:

		/**
		 * Strip the parent path from the given path, and return
		 * but the target element, including title and extension.
		 * If no path separator exists in the input string,
		 * the entire input string is returned.
		 */
		static string extract_target(const string& path);


		/**
		 * Returns the entire path without the last extension
		 * on the last path element. The separating full stop
		 * character is removed as well. If no extension exists,
		 * the full input string is returned.
		 */
		static string remove_extension(const string& path);


		/**
		 * Returns the extension of a path target,
		 * excluding the separating, last fullstop character.
		 * Returns an empty string if there is no extension.
		 */
		static string extract_extension(const string& path);


		/**
		 * Get the parent path for a given virtual path.
		 * Returns an empty string if the given path contains
		 * just a single path element.
		 */
		static string extract_parent(const string& path);


		/**
		 * Get the root element for a given virtual path.
		 * If there is just a single path element within the path,
		 * an empty string is returned.
		 */
		static string extract_root(const string& path);


		/**
		 * Test whether a given string is a valid virtual path.
		 */
		static bool is_valid(const string& path);
};




/**
 *************************************************************************
 *
 * @class path_index
 * @ingroup file_module
 *
 * Stores separate path sets for folders and for actual files.
 *
 * Note that index entries need not be consistent.
 * In particular, a path index may for example contain:
 * - folders without files.
 * - files or folders without their relative parent folders.
 * - files or folders that do not exist on disk.
 * - files and folders with ambiguous paths.
 * - files or folders with invalid paths (see virtual_path).
 *
 ************************************************************************/
class path_index
{

	public:

		typedef tree_set<string> file_path_set;
		typedef tree_set<string> folder_path_set;

		file_path_set file_paths;
		folder_path_set folder_paths;
};




/**
 *************************************************************************
 *
 * @class file
 * @ingroup file_module
 *
 * Basic interface for reading and writing files, either directly
 * on disk (disk_file) or encapsulated within archives (zip_file).
 *
 * Notes:
 * - All file operations throw on failure. In this case, output variables
 *		(when reading) or the output file (when writing) may remain in an
 *		inconsistent, partially modified state. Likewise, the file object
 *		remains in an invalid state and must be destroyed.
 * - Operations throw if they are not supported by the implementation.
 *		For instance, zip_file does not support write operations.
 *
 ************************************************************************/
class file :
	private non_copyable,
	private bool_convert<file>
{

	public:

		enum file_operation
			{ read_only, read_write_truncate, read_write_update };
		enum seek_mode
			{ seek_begin, seek_current, seek_end};


		virtual ~file() NOTHROW { };


		/**
		 * Set current read/write position to @bref{source} + @bref{offset}.
		 * Implementations may, but need not, throw on invalid positions.
		 */
		virtual void seek(int64 offset, seek_mode source) = 0;


		/**
		 * Get current read/write position, in bytes.
		 * Throws, but never returns a negative value.
		 */
		virtual int64 tell() = 0;


		/**
		 * Retrieve file size, in bytes, never negative.
		 * On success, do not change the read/write position.		 
		 */
		virtual int64 find_size() = 0;


		/** Test if read/write position points past the last byte. */
		//@{
		virtual bool eof() = 0;
		operator bool_conversion_type()
			{ return eof() ? NULL : &file::bool_conversion_is_true; }
		//@}


		/** Read or write @bref{count} bytes from or to @bref{data} array. */
		//@{
		virtual void read_bytes(void* data, int64 count) = 0;
		virtual void write_bytes(const void* data, int64 count) = 0;
		//@}


		/**
		 * Read or write fixed-width @bref{value} as binary data
		 * at current position in file.
		 *
		 * @attent Clients should not rely on sizes for intrinsic ints,
		 * even if there are auto-conversions to fixed-width integers.
		 */
		//@{
		void read(char &value);
		void write(char value);
		void read(unsigned char &value);
		void write(unsigned char value);

		void read(int16 &value);
		void write(int16 value);
		void read(uint16 &value);
		void write(uint16 value);
		void read(int32 &value);
		void write(int32 value);
		void read(uint32 &value);
		void write(uint32 value);

		void read(float &value);
		void write(float value);
		void read(double &value);
		void write(double value);
		//@}


		/**
		 * Read or write \\0-terminated string, starting at current position.
		 * Reading throws if there is no terminating \\0 up to EoF.
		 */
		//@{
		string read_nullterminated();
		void write_nullterminated(const string& str);
		//@}


		/**
		 * Read or write text without terminating \\0.
		 *
		 * Reading returns the entire remaining file as a single string.
		 * The resulting string also contains any intermediate \\0 characters,
		 * and has its actual (i.e. final) terminating \\0 in place of EoF.
		 */
		//@{	
		string read_text();
		void write_text(const string& str);
		//@}
};




/**
 *************************************************************************
 *
 * @class disk_file
 * @ingroup file_module
 *
 * Wrapper for standard C on-disk file IO operations.
 *
 ************************************************************************/
class disk_file : public file
{

	public:

		/**
		 * Open on-disk file with given absolute, OS-specific @bref{path}
		 * for performing the indicated file @bref{operation}s.
		 */
		disk_file(const string& path, file_operation operation);

		~disk_file() NOTHROW override;

		void seek(int64 offset, seek_mode source) override;
		int64 tell() override;
		int64 find_size() override;
		bool eof() override;

		void read_bytes(void* data, int64 count) override;
		void write_bytes(const void* data, int64 count) override;


	private:

		string path_;
		file_operation operation_;
		FILE *handle_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__FILE_UTIL_HPP) */
