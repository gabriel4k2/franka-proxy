/**
 *************************************************************************
 *
 * @file ms_file.cpp
 *
 * Windows implementation of the virtual file system.
 *
 ************************************************************************/


#include "ms_file.hpp"


#if defined(VIRAL__HAVE_MS_FILE)


#include <windows.h>
#include <shlobj.h>
#include <io.h>


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "list.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  ms_file_context
//
//////////////////////////////////////////////////////////////////////////


ms_file_context::ms_file_context
	(const string& context_name, const string& resource_path,
	 const string& save_path)
	:
	file_context(context_name)
{
	LOG_INFO("Using ms_file_context implementation.");

	init_resource_folder(resource_path);
	init_save_folder(save_path);
	init_archive_lookup();
}


ms_file_context::~ms_file_context() NOTHROW
	{ LOG_INFO("Destroying."); }


bool ms_file_context::does_file_exist
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
		DWORD attribs =
			GetFileAttributes(save_path.data());

		if (attribs != INVALID_FILE_ATTRIBUTES &&
			(attribs & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			return true;
		}
	}
	{
		string resource_path(resource_folder_ + "/" + path);
		DWORD attribs =
			GetFileAttributes(resource_path.data());

		if (attribs != INVALID_FILE_ATTRIBUTES &&
			(attribs & FILE_ATTRIBUTE_DIRECTORY) == 0)
		{
			return true;
		}
	}
	{
		if (archive_lookup_.entries.contains(path))
			return true;
	}

	return false;
}


auto_pointer<file> ms_file_context::open_file
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
			DWORD attribs =
				GetFileAttributes(save_path.data());

			if (attribs != INVALID_FILE_ATTRIBUTES &&
				(attribs & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				return auto_pointer<file>
							(new disk_file(save_path, operation));
			}
		}
		{
			string resource_path(resource_folder_ + "/" + path);
			DWORD attribs =
				GetFileAttributes(resource_path.data());

			if (attribs != INVALID_FILE_ATTRIBUTES &&
				(attribs & FILE_ATTRIBUTE_DIRECTORY) == 0)
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


void ms_file_context::build_path_index
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


string ms_file_context::to_absolute_path(const string& path) const
	{ return resource_folder_ + "/" + path; }


const string& ms_file_context::save_folder() const NOTHROW
	{ return save_folder_; }
const string& ms_file_context::resource_folder() const NOTHROW
	{ return resource_folder_; }


void ms_file_context::create_relative_folders
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

		if (!CreateDirectory(create_path.data(), NULL))
		{
			if (GetLastError() != ERROR_ALREADY_EXISTS)
			{
				LOG_ERROR("Failed to create folder: " + create_path);
				throw folder_creation_failed();
			}
		}

		++it;
	}
}


void ms_file_context::build_path_index_disk
	(const string& base_path, path_index& index,
	 const string& parent_path, const string& extension,
	 bool add_folders, bool recursive, bool matchext) const
{
	// Seach for files and folders to add within the
	// relative parent_path inside base_path.
	string seekstr
		((parent_path == "") ?
						(base_path + "\\*") :
						(base_path + "\\" + parent_path + "\\*"));

	_finddata_t finddata;
	intptr_t handle = _findfirst(seekstr.data(), &finddata);
	if (handle == -1) return;

	SCOPE_GUARD
		(find_close_guard, (&_findclose, handle));

	do
	{
		// Build complete virtual path, from file name
		// in parent folder and parent folder path.
		string path(finddata.name);

		if (path == "" || path == "." || path == "..")
			continue;

		if (parent_path != "")
			path = parent_path + "/" + path;

		if (!virtual_path::is_valid(path))
			continue;


		// Perform filtering of files and folders,
		// add results to index, and recurse.
		if ((finddata.attrib & _A_SUBDIR) != 0)
		{
			if (add_folders)
				index.folder_paths.insert(path);

			if (recursive)
				build_path_index_disk
					(base_path, index, path, extension,
					 add_folders, recursive, matchext);
		}
		else
		{
			if (matchext &&
				virtual_path::extract_extension(path) != extension)
					continue;

			index.file_paths.insert(path);
		}
	}
	while (_findnext(handle, &finddata) == 0);
}


void ms_file_context::build_path_index_archives
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


string ms_file_context::fix_win_path(const string& path) const
{
	/**
	 * Fix windows @bref{path} to match with POSIX slashes,
	 * and detach any obsolete trailing slashes for folders.
	 * This allows to reuse virtual_path methods on these.
	 */

	string ret(path);
	ret.replace_all('\\', '/');

	while (ret.size() != 0 && ret[ret.size() - 1] == '/')
		ret.remove(ret.size() - 1);

	return ret;
}


void ms_file_context::init_resource_folder(const string& resource_path)
{
	/**
	 * Build the resource folder from the parent path of the
	 * application executable and the incoming relative path.
	 *
	 * Notes:
	 * - Unlike the save folder, we need not create the resource folder,
	 *		as it contains no files that are written to later on.
	 * - We require resource_path to be a virtual_path to avoid shenanigans
	 *		related to path interpretation.
	 */

	if (!virtual_path::is_valid(resource_path))
	{
		LOG_ERROR("Resource path is not a valid virtual path.");
		throw invalid_parameter();
	}


	// Fetch path to program executable.
	// GetModuleFileName() does not always \0-terminate its results,
	// thus we allocate an additional character and zero out memory.
	char module_path[_MAX_PATH + 1];
	memory::fill_with_zero_typed
		(module_path, _MAX_PATH + 1);
	DWORD retcount =
		GetModuleFileName(NULL, module_path, _MAX_PATH);

	if (retcount == _MAX_PATH || retcount == 0)
	{
		LOG_CRITICAL("Failed to determine program executable path.");
		throw system_call_failed();
	}


	// Locate resource folder next to executable.
	string program_folder
		(virtual_path::extract_parent(fix_win_path(module_path)));

	if (program_folder == "")
	{
		LOG_CRITICAL("Program executable folder seems corrupt.");
		throw system_call_failed();
	}

	resource_folder_ = program_folder + "/" + resource_path;

	LOG_INFO("Resource folder: " + resource_folder_);
}


void ms_file_context::init_save_folder(const string& save_path)
{
	/**
	 * Retrieve appdata path of the current user from windows,
	 * then combine it with the save path to build the absolute
	 * save folder.
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

	char appdata_path[_MAX_PATH];
	HRESULT result =
		SHGetFolderPath(NULL, CSIDL_APPDATA, NULL,
						SHGFP_TYPE_CURRENT, appdata_path);

	if (result != S_OK)
	{
		LOG_CRITICAL("Failed to retrieve appdata folder.");
		throw system_call_failed();
	}

	if (appdata_path[0] == '\0')
	{
		LOG_CRITICAL("Appdata folder seems corrupt.");
		throw system_call_failed();
	}

	string fixed_appdata_path
		(fix_win_path(appdata_path));

	create_relative_folders
		(save_path, fixed_appdata_path);

	save_folder_ = fixed_appdata_path + "/" + save_path;

	LOG_INFO("Save folder: " + save_folder_);
}


void ms_file_context::init_archive_lookup()
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


const char* const ms_file_context::archive_file_extension_ = "zip";




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_FILE) */
