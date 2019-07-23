/**
 *************************************************************************
 *
 * @file command_line.hpp
 *
 * Static access to command-line parameters.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COMMAND_LINE_HPP)
#define INCLUDED__VIRAL_CORE__COMMAND_LINE_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "list.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class command_line
 * @ingroup utility_module
 *
 * Provides static access to command-line parameters.
 *
 * Usage notes:
 * - Clients must explicitly initialize the command_line utility
 *		with actual parameters on startup (e.g. in their main())
 *		by calling command_line::init().
 * - If the command_line utility has not been explicitly initialized,
 *		all queries still are safe, but return no parameters.
 *
 * Thread safety:
 * - @attent Initialization with command_line::init() is not threadsafe.
 *		Clients should call command_line::init() before creating threads.
 * - All other command_line queries are read-only and thus threadsafe.
 *
 * Implementation notes:
 * - Some OS targets allow for late queries of command line parameters
 *		(e.g. GetCommandLine() on Microsoft Windows). Other OS targets
 *		(e.g. Linux) only provide parameters to the main() entry point.
 *		The command_line utility abstracts over both parameter variants
 *		and eliminates the need to pass parameters to all points of use.
 *
 ************************************************************************/
class command_line :
	private non_constructable
{

	public:

		/**
		 * Store @bref{argc} and @bref{argv} for later use.
		 * Throws on (and ignores) duplicate initialization.
		 */
		static void init
			(int argc, const char* const* argv);

		static int argc() NOTHROW;
		static const char* const* argv() NOTHROW;

		static list<string> parameters();


		/**
		 * Maximum number of supported command parameters.
		 * A call to init() throws on excessive arguments.
		 */
		static const int max_argc = 2048;


	private:

		static bool& initialized() NOTHROW;
		static int& argc_store() NOTHROW;
		static const char* const*& argv_store() NOTHROW;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COMMAND_LINE_HPP) */

