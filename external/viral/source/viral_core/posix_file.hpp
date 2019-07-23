/**
 *************************************************************************
 *
 * @file posix_file.hpp
 *
 * A file_context for POSIX files.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__POSIX_FILE_HPP)
#define INCLUDED__VIRAL_CORE__POSIX_FILE_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_POSIX_FILE)


#include "file.hpp"
#include "zip.hpp"
#include "thread_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class posix_file_context
 * @ingroup file_module
 *
 * Handles file access for POSIX systems.
 *
 * Implementation specialities:
 * - POSIX file interaction depends on two environment variables:
 *		- $VIRAL_WRITE_PATH$ points to a writeable, non-temporary
 *			base path, usually the user's home folder.
 *		- $VIRAL_INSTALL_PATH$ points to the base directory where
 *			the running application is installed.
 * - Files are held in three prioritized locations:
 *		-# In a save folder, $VIRAL_WRITE_PATH$/@bref{save_path}.
 *		-# In a resource folder, $VIRAL_INSTALL_PATH$/@bref{resource_path}.
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
 * - @attent POSIX opendir() as used by build_path_index() is not fully
 *		threadsafe. While posix_file_context protects build_path_index()
 *		even over multiple class instances, concurrent client-side calls
 *		to opendir() must explicitly ensure thread safety.
 *
 ************************************************************************/
class posix_file_context :
	public file_context
{
	public:


		/**
		 * Create context that manages files within folders
		 * $VIRAL_INSTALL_PATH$/@bref{resource_path} and
		 * $VIRAL_SAVE_PATH$/@bref{save_path}.
		 */
		posix_file_context
			(const string& context_name, const string& resource_path,
			 const string& save_path);


		~posix_file_context() NOTHROW override;


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

		void init_resource_folder(const string& resource_path);
		void init_save_folder(const string& save_path);
		void init_archive_lookup();


		static const char* const archive_file_extension_;

		static mutex opendir_mutex_;

		string save_folder_;
		string resource_folder_;
		zip_index archive_lookup_;
};




}  /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_POSIX_FILE) */


#endif /* !defined(INCLUDED__VIRAL_CORE__POSIX_FILE_HPP) */
