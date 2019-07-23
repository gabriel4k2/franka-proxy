/**
 *************************************************************************
 *
 * @file file.hpp
 *
 * Base header for the virtual file system.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__FILE_HPP)
#define INCLUDED__VIRAL_CORE__FILE_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "string.hpp"
#include "file_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup file_module File Module
 *
 * Defines a virtual file system for OS- and storage independent
 * application file management, including file input, output,
 * and search operations.
 *
 * Usage notes:
 * - Relative POSIX paths, such as "folder/sub_folder/test.txt", form the
 *		base of the virtual file system. See the virtual_path class for an
 *		in-depth discussion and additional restrictions.
 * - A file_context resolves relative, virtual paths into abstract @ref file
 *		objects for read-write access. The corresponding OS files are hidden
 *		from the application, and may for instance be contained in a zip
 *		archive or stored in the user's home folder.
 * - A file_context may map one virtual path to multiple system locations,
 *		such as to the home folder for read-write access and to a resource
 *		folder for read-only access.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class file_context
 * @ingroup file_module
 *
 * Manages the virtual file system, including file input, output
 * and search capabilities.
 *
 * Notes:
 * - Concurrent read/write and write/write conflicts for on-disk files
 *		are not handled, behaviour is undefined in these cases.
 * - Apart from the above, a file context is threadsafe for use by an
 *		arbitrary number of threads.
 *
 ************************************************************************/
class file_context :
	private non_copyable
{

	public:


		file_context(const string& context_name);
		virtual ~file_context() NOTHROW;


		/**
		 * Test if an openable file exists for a @bref{path}
		 * on the virtual file system.
		 */
		virtual bool does_file_exist(const string& path) const = 0;


		/**
		 * Open a file corresponding to the given virtual file system
		 * @bref{path} with the provided file @bref{operation}.
		 * The call throws on any problems and never returns @NULL.
		 */
		virtual auto_pointer<file> open_file
			(const string& path, file::file_operation operation) const = 0;


		/**
		 * Add virtual_path%s for all existing files and folders that match
		 * the given virtual @bref{parent_path} and file @bref{extension}
		 * to the provided @bref{index}.
		 *
		 * Parameter usage:
		 * - An empty @bref{parent_path} lists root paths.
		 * - An empty @bref{extension} lists files with any extension.
		 * - An @bref{extension} must not include the separating fullstop.
		 * - Folders ignore any file @bref{extension} filtering.
		 * - If @bref{add_folders} is not set, folder output is suppressed.
		 * - If @bref{recursive} is set, folders are scanned recursively.
		 *
		 * If the call throws, the provided path index may already have
		 * been partially populated.
		 */
		virtual void build_path_index
			(path_index& index,
			 const string& parent_path, const string& extension,
			 bool add_folders, bool recursive) const = 0;


		/**
		 * Convert a virtual_path to a system-specific absolute path.
		 * @attent May return any of multiple backing locations.
		 */
		virtual string to_absolute_path(const string& path) const = 0;


		const string context_name;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__FILE_HPP) */
