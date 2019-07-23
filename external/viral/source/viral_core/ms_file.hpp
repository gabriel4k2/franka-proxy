/**
 *************************************************************************
 *
 * @file ms_file.hpp
 *
 * Virtual file system on the Windows OS.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_FILE_HPP)
#define INCLUDED__VIRAL_CORE__MS_FILE_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_FILE)


#include "file.hpp"
#include "zip.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_file_context
 * @ingroup file_module
 *
 * A file_context implementation for use on Microsoft Windows.
 *
 * Implementation specialities:
 * - Files are held in three prioritized locations:
 *		-# A customizeable save folder within the user's appdata folder.
 *		-# A resource folder defined relative to the program executable.
 *		-# Any zip archives within the resource folder.
 * - Due to access rights, all write or read/write operations only access
 *		files in the save folder. For read-only access, storage locations
 *		are checked in the above order. Thus a previously written file is
 *		restored correctly from the save folder.
 * - On-disk storage appends an incoming relative, virtual path to the
 *		save or resource folder. Archive storage looks for an incoming
 *		relative virtual path within any zip archive via a zip_index.
 * - Altough the above logic handles ambiguous virtual file paths,
 *		client applications should avoid such constellations.
 *
 ************************************************************************/
class ms_file_context :
	public file_context
{
	public:

		/**
		 * Create context that manages files within folders specified
		 * by virtual_path%s @bref{resource_path} and @bref{save_path}.
		 */
		ms_file_context
			(const string& context_name, const string& resource_path,
			 const string& save_path);


		~ms_file_context() NOTHROW override;


		bool does_file_exist
			(const string& path) const override;

		auto_pointer<file> open_file
			(const string& path, file::file_operation operation) const override;

		void build_path_index
			(path_index& index,
			 const string& parent_path, const string& extension,
			 bool add_folders, bool recursive) const override;

		string to_absolute_path
			(const string& path) const override;


		/**
		 * Fetch absolute file system paths
		 * for read-only and read-write access.
		 */
		//@{
		const string& save_folder() const NOTHROW;
		const string& resource_folder() const NOTHROW;
		//@}


	private:

		void create_relative_folders
			(const string& relative_path, const string& base_folder) const;

		void build_path_index_disk
			(const string& base_folder, path_index& index,
			 const string& parent_path, const string& extension,
			 bool add_folders, bool recursive, bool matchext) const;
		void build_path_index_archives
			(path_index& index,
			 const string& parent_path, const string& extension,
			 bool add_folders, bool recursive, bool matchext) const;

		string fix_win_path(const string& path) const;

		void init_resource_folder(const string& resource_path);
		void init_save_folder(const string& save_path);
		void init_archive_lookup();


		static const char* const archive_file_extension_;

		string save_folder_;
		string resource_folder_;
		zip_index archive_lookup_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_FILE) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_FILE_HPP) */
