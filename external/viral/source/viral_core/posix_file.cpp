/**
 *************************************************************************
 *
 * @file posix_file.cpp
 *
 * A file_context for POSIX files, implementation.
 *
 ************************************************************************/


#include "posix_file.hpp"


#if defined(VIRAL__HAVE_POSIX_FILE)


#include <sys/stat.h>
#include <dirent.h>
#include <stdlib.h>


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "string.hpp"
#include "list.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// posix_file_context
//
//////////////////////////////////////////////////////////////////////////


posix_file_context::posix_file_context
	(const string& context_name, const string& resource_path,
	 const string& save_path)
	:
	file_context(context_name)
{
	LOG_INFO("Using posix_file_context implementation.");

	init_resource_folder(resource_path);
	init_save_folder(save_path);
	init_archive_lookup();
}


posix_file_context::~posix_file_context() NOTHROW
	{ LOG_INFO("Destroying."); }


bool posix_file_context::does_file_exist
	(const string& path) const
{
	if (!virtual_path::is_valid(path))
	{
		LOG_ERROR("Invalid virtual path provided: " + path);
		throw invalid_parameter();
	}


	// Check storage locations in order of priorities.
	// For existing on-disk pathes, ensure that these are
	// not a folder.
	{
		string save_path(save_folder_ + "/" + path);

		struct stat s;
		if (stat(save_path.data(), &s) != -1 &&
			S_ISREG(s.st_mode))
				return true;
	}
	{
		string resource_path(resource_folder_ + "/" + path);

		struct stat s;
		if (stat(resource_path.data(), &s) != -1 &&
			S_ISREG(s.st_mode))
				return true;
	}
	{
		if (archive_lookup_.entries.contains(path))
			return true;
	}

	return false;
}


auto_pointer<file> posix_file_context::open_file
	(const string& path, file::file_operation operation) const
{
	if (!virtual_path::is_valid(path))
	{
		LOG_ERROR("Invalid virtual path provided: " + path);
		throw invalid_parameter();
	}


	// Read-only modes need to consider all three storages.
	if (operation == file::read_only)
	{
		{
			string save_path(save_folder_ + "/" + path);

			struct stat s;
			if (stat(save_path.data(), &s) != -1 &&
				S_ISREG(s.st_mode))
			{
				return auto_pointer<file>
							(new disk_file(save_path, operation));
			}
		}
		{
			string resource_path(resource_folder_ + "/" + path);

			struct stat s;
			if (stat(resource_path.data(), &s) != -1 &&
				S_ISREG(s.st_mode))
			{
				return auto_pointer<file>
							(new disk_file(resource_path, operation));
			}
		}
		{
			zip_index::entry_map::const_iterator entryit
				(archive_lookup_.entries.find(path));

			if (entryit)
				return auto_pointer<file>(new zip_file(entryit->value));
		}

		LOG_ERROR("File: " + path + " not found for read-only access.");
		throw file_operation_failed();
	}

	// Read-write modes have only one possible storage location.
	else if (operation == file::read_write_truncate ||
			 operation == file::read_write_update)
	{
		create_relative_folders
			(virtual_path::extract_parent(path), save_folder_);

		string save_path(save_folder_ + "/" + path);
		return auto_pointer<file>(new disk_file(save_path, operation));
	}

	LOG_ERROR("Invalid operation provided for opening: " + path);
	throw invalid_parameter();
}


void posix_file_context::build_path_index
	(path_index& index,
	 const string& parent_path, const string& extension,
	 bool add_folders, bool recursive) const
{
	if (parent_path != "" &&
		!virtual_path::is_valid(parent_path))
	{
		LOG_ERROR
			("Invalid parent path for creating path_index: " + parent_path);
		throw invalid_parameter();
	}

	bool matchextension = (extension != "");

	build_path_index_disk
		(save_folder_, index, parent_path, extension,
		 add_folders, recursive, matchextension);
	build_path_index_disk
		(resource_folder_, index, parent_path, extension,
		 add_folders, recursive, matchextension);
	build_path_index_archives
		(index, parent_path, extension,
		 add_folders, recursive, matchextension);
}


string posix_file_context::to_absolute_path(const string& path) const
	{ return resource_folder_ + "/" + path; }


void posix_file_context::create_relative_folders
	(const string& relative_path, const string& base_folder) const
{
	/**
	 * Create folders, within the indicated base folder, for each
	 * path element in a given relative, virtual path. The relative
	 * path can be empty. In this case no folders are created.
	 */

	if (relative_path == "") return;

	list<string> pathlist;
	relative_path.split('/', pathlist);

	string create_path(base_folder);
	list<string>::iterator it(pathlist.first());
	while (it)
	{
		if (create_path != "") create_path = create_path + "/" + *it;
			else create_path = *it;

		struct stat s;
		if (stat(create_path.data(), &s) != -1)
		{
			if (!S_ISDIR(s.st_mode))
			{
				LOG_ERROR("Path: " + create_path + " exists already, but is no folder.");
				throw folder_creation_failed();
			}
		}
		else
		{
			if (mkdir(create_path.data(), S_IRUSR | S_IWUSR) == -1)
			{
				LOG_ERROR("Failed to create folder: " + create_path);
				throw folder_creation_failed();
			}
		}

		++it;
	}
}


void posix_file_context::build_path_index_disk
	(const string& base_path, path_index& index,
	 const string& parent_path, const string& extension,
	 bool add_folders, bool recursive, bool matchext) const
{
	/**
	 * Implementation note:
	 * POSIX opendir() is not threadsafe.
	 * We use a static mutex to protect access over all
	 * posix_file_context instances. This won't help for
	 * statically allocated file contexts or client-side
	 * calls to opendir(), but there is nothing better
	 * that we can do.
	 */
	MUTEX_SCOPE(opendir_mutex_);


	// Seach for files and folders to add within the
	// relative parent_path inside base_path.
	string seek_path
		((parent_path == "") ?
			 base_path : (base_path + "/" + parent_path));

	DIR* dir = opendir(seek_path.data());
	if (!dir) return;

	SCOPE_GUARD(dir_close, (&closedir, dir));

	while (true)
	{
		struct dirent* entry = readdir(dir);
		if (entry == NULL) break;


		// Transform POSIX relative (virtual) path into
		// file-system absolute and virtual absolute path.
		string relative_path(entry->d_name);

		if (relative_path == "" ||
			relative_path == "." ||
			relative_path == "..")
				continue;

		string full_path_fs
			(seek_path + "/" + relative_path);
		string full_path_virtual
			((parent_path == "") ?
				relative_path : (parent_path + "/" + parent_path));

		if (!virtual_path::is_valid(full_path_virtual))
			continue;


		// Perform filtering of files and folders,
		// add results to index, and recurse.	
		struct stat s;
		if (stat(full_path_fs.data(), &s) == -1)
			continue;

		if (S_ISDIR(s.st_mode))
		{
			if (add_folders)
				index.folder_paths.insert(full_path_virtual);

			if (recursive)
				build_path_index_disk
					(base_path, index, full_path_virtual, extension,
					 add_folders, recursive, matchext);
		}
		else if (S_ISREG(s.st_mode))
		{
			if (matchext &&
				virtual_path::extract_extension(relative_path) != extension)
					continue;

			index.file_paths.insert(full_path_virtual);
		}
	}
}


void posix_file_context::build_path_index_archives
	(path_index& index,
	 const string& parent_path, const string& extension,
	 bool add_folders, bool recursive, bool matchext) const
{
	/**
	 * Recursively walk the archive_lookup_ folder structure, starting
	 * with parent_path, and extract file and folder names.
	 *
	 * See zip_index for data structure explanations - in particular,
	 * we need to consider paths that are both files and folders.
	 * Note we here assume that our zip_index got built correctly.
	 * If it didn't, we just miss a few files, nothing too drastical.
	 */

	zip_index::folder_map::const_iterator parent
		(archive_lookup_.folders.find(parent_path));

	if (!parent) return;

	for (zip_index::subordinate_path_set::const_iterator child(parent->value.first());
		 child; ++child)
	{
		if (!virtual_path::is_valid(*child))
			continue;

		if (archive_lookup_.entries.contains(*child))
		{
			if (matchext &&
				virtual_path::extract_extension(*child) != extension)
					continue;

			index.file_paths.insert(*child);
		}

		if (archive_lookup_.folders.contains(*child))
		{
			if (add_folders)
				index.folder_paths.insert(*child);

			if (recursive)
				build_path_index_archives
					(index, *child, extension,
					 add_folders, recursive, matchext);
		}
	}
}


void posix_file_context::init_resource_folder(const string& resource_path)
{
	/**
	 * Build the resource folder from environment variable
	 * VIRAL_INSTALL_PATH and relative resource path.
	 *
	 * Notes:
	 * - Unlike the save folder, we needn't create the resource folder,
	 *		as it contains no files that are written to later on.
	 * - We require resource_path to be a virtual path to avoid
	 *		shenanigans related to path interpretation.
	 */

	if (!virtual_path::is_valid(resource_path))
	{
		LOG_ERROR("Resource path is not a valid virtual path.");
		throw invalid_parameter();
	}

	char* install_path = getenv("VIRAL_INSTALL_PATH");

	if (!install_path)
	{
		LOG_CRITICAL("Environment variable VIRAL_INSTALL_PATH not set.");
		throw invalid_state();
	}

	resource_folder_ = string(install_path) + "/" + resource_path;

	struct stat s;
	if (stat(resource_folder_.data(), &s) == -1 ||
		!S_ISDIR(s.st_mode))
	{
		LOG_CRITICAL("Resource folder: " + resource_folder_ +
					 " does not exist or is no directory.");
		throw invalid_state();
	}

	LOG_INFO("Resource folder: " + resource_folder_);
}


void posix_file_context::init_save_folder(const string& save_path)
{
	/**
	 * Build the save folder from environment variable
	 * VIRAL_WRITE_PATH and relative save path.
	 *
	 * Notes:
	 * - We must create the save folder if it doesn't exist yet,
	 *		so that later file saving need not create a complete
	 *		folder hierarchy.
	 * - We require save_path to be a virtual path to avoid
	 *		shenanigans related to path interpretation.
	 */

	if (!virtual_path::is_valid(save_path))
	{
		LOG_ERROR("Save path is not a valid virtual path.");
		throw invalid_parameter();
	}

	char* write_path = getenv("VIRAL_WRITE_PATH");

	if (!write_path)
	{
		LOG_CRITICAL("Environment variable VIRAL_WRITE_PATH not set.");
		throw invalid_state();
	}

	struct stat s;
	if (stat(write_path, &s) == -1 || !S_ISDIR(s.st_mode))
	{
		LOG_CRITICAL("Write path: " + write_path +
					 " does not exist or is no directory.");
		throw invalid_state();
	}

	save_folder_ = string(write_path) + "/" + save_path;

	if (stat(write_path, &s) == -1)
	{
		create_relative_folders(save_path, write_path);
	}
	else if (!S_ISDIR(s.st_mode))
	{
		LOG_CRITICAL("Save folder: " + save_folder_ + " is no directory.");
		throw invalid_state();
	}

	LOG_INFO("Save folder: " + save_folder_);
}


void posix_file_context::init_archive_lookup()
{
	/**
	 * Scan for all root level files with the archive extension
	 * inside the resource folder, and add them to our lookup table.
	 */

	path_index archives;
	build_path_index_disk
		(resource_folder_, archives, "", archive_file_extension_, false, false, true);

	for (path_index::file_path_set::const_iterator it = archives.file_paths.first();
		 it; ++it)
	{
		LOG_INFO("Parsing zip archive: " + (*it));

		string archivepath(resource_folder_ + "/" + *it);
		archive_lookup_.add_archive(archivepath);
	}
}


const char* const posix_file_context::archive_file_extension_ = "zip";

mutex posix_file_context::opendir_mutex_;




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_POSIX_FILE) */
