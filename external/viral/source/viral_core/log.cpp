/**
 *************************************************************************
 *
 * @file log.cpp
 *
 * Logging implementation.
 *
 ************************************************************************/


#include "log.hpp"


#if defined(VIRAL__OS_TARGET_WINDOWS)
	#include <windows.h>
	#include <ShlObj.h>
#endif


#if defined(VIRAL__OS_TARGET_UNIX)
	#include <sys/stat.h>
	#include <dirent.h>
	#include <stdlib.h>
#endif


#include "list.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	log_context
//
//////////////////////////////////////////////////////////////////////////


/**
 * Log an internal, nonthrowing message within the calling log instance.
 */
#define LOG_INTERNAL(message) \
	try \
	{ \
		log(__FILE__, __LINE__, __FUNCTION__, \
			log_entry::info, string() + message); \
	} \
	catch (...) { }


log_context::log_context
	(const string& context_name,
	 uint32 flags, const string& target_path)
	:
	context_name(context_name),
	flags_(flags),
	target_file_handle_(NULL),
	current_entry_(max_entry_count_ - 1),
	message_count_(0),
	error_count_(0)
{
	// Initialize the output file.
	// We don't use a file_context or a file wrapper in here
	// to avoid circular dependencies.
	if ((flags_ & output_to_file) != 0)
	{
		string absolutepath
			(create_absolute_target_path(target_path));

		bool success =
			(fopen_s(&target_file_handle_, absolutepath.data(), "w") == 0) &&
			target_file_handle_ != NULL;

		if (success)
			fprintf(target_file_handle_, "%s", html_header_);
		else
			fprintf(stderr, "%s %s",
					"Failed to open log file:",
					absolutepath.data());
	}


	// Report log startup
	LOG_INTERNAL("Creating: " + context_name);
	LOG_INTERNAL("Maximum number of messages: " + max_entry_count_);
	
	if ((flags_ & output_to_file) != 0)
	{
		LOG_INTERNAL("File output enabled for: " + target_path);	

		if (!target_file_handle_)
			LOG_INTERNAL("Target file could not be opened.");
	}

	if ((flags_ & output_to_stdout) != 0)
		LOG_INTERNAL("Standard output enabled.");

	if ((flags_ & output_to_stderr) != 0)
		LOG_INTERNAL("Error output enabled.");

	if ((flags_ & debug_output_on_problem) != 0)
		LOG_INTERNAL("Debug pane output enabled for problem messages.");

	if ((flags_ & notify_on_problem) != 0)
		LOG_INTERNAL("Message box output enabled for problem messages.");

	if ((flags_ & beep_on_problem) != 0)
		LOG_INTERNAL("Beeping enabled for problem messages.");
}


log_context::~log_context() NOTHROW
{
	LOG_INTERNAL("Logged " + message_count_ + " messages.");
	LOG_INTERNAL("Among these " + error_count_ + " errors.");
	LOG_INTERNAL("Destroyed: " + context_name);


	// Append target file footer and close.
	// The above is NOTHROW, so the file always is closed.
	if (target_file_handle_)
	{
		fprintf(target_file_handle_, "%s", html_footer_);
		fclose(target_file_handle_);
	}
}


void log_context::log
	(const string& source_file, int source_line,
	 const string& source_call, log_entry::log_entry_type type,
	 const string& message)
{
	MUTEX_SCOPE(log_lock_);


	// Replace / insert incoming message at its proper place.
	// Exception safety: Avoid partial message by offhand construction.
	current_entry_ = current_entry_ + 1;

	if (current_entry_ >= max_entry_count_)
		current_entry_ = 0;

	log_entry newentry
		(source_file, source_line, source_call, type, message);
	entries_[current_entry_].swap_with(newentry);

	++message_count_;


	// Store message in file if file logging is enabled.
	if ((flags_ & output_to_file) != 0)
		write_to_target_file(entries_[current_entry_], current_entry_);


	// Handle normal message output.
	// Note warnings only appear on stdout if they
	// aren't sent on to stderr
	if ((flags_ & output_to_stdout) != 0)
	{
		if ((flags_ & output_to_stderr) == 0 ||
				(type != log_entry::critical &&
				 type != log_entry::error &&
				 type != log_entry::warning))
		{
			write_to_stream(entries_[current_entry_], stdout);
		}
	}


	// Provide special handling for any error messages
	if (type == log_entry::critical ||
		type == log_entry::error ||
		type == log_entry::warning)
	{
		++error_count_;

		if ((flags_ & output_to_stderr) != 0)
			write_to_stream(entries_[current_entry_], stderr);

		if ((flags_ & notify_on_problem) != 0)
			show_in_notify_box(entries_[current_entry_]);

		if ((flags_ & beep_on_problem) != 0)
			beep();
	}


	// Handle debugger output.
	// Note file refs also appear on debug output.
	if ((flags_ & debug_output_on_problem) != 0)
	{
		if (type == log_entry::critical ||
			type == log_entry::error ||
			type == log_entry::warning ||
			type == log_entry::file_reference)
		{
			write_to_debug_pane(entries_[current_entry_]);
		}
	}
}


log_entry log_context::current_entry() const
{
	MUTEX_SCOPE(log_lock_);

	// Return copy by value for thread safety.
	return entries_[current_entry_];
}


string log_context::current_message() const
{
	MUTEX_SCOPE(log_lock_);

	// Return copy by value for thread safety.
	return entries_[current_entry_].message;
}


int64 log_context::message_count() const
{
	MUTEX_SCOPE(log_lock_);
	return message_count_;
}


string log_context::escape_html(const string& text)
{
	/**
	 * Escape HTML entities and other dangerous characters
	 * within some input html @bref{text}.
	 * This call does not use xml_util to avoid a dependency
	 * on a higher-level, potentially log-using module.
	 */

	int64 size = text.size();

	string result;
	result.reserve(size);

	for (int64 i = 0; i < size; ++i)
	{
		char chr = text[i];


		// HTML supports all basic ASCII chars and numbers,
		// so do not escape these.
		if (chr >= 'a' && chr <= 'z' ||
			chr >= 'A' && chr <= 'Z' ||
			chr >= '0' && chr <= '9')
		{
			result.append(chr);
		}


		// HTML accepts whitespace as is. Users must explicitly
		// escape to preserve exact whitespace (e.g. "&nbsp;").
		else if (chr == ' ' || chr == '\t' ||
				 chr == '\r' || chr == '\n')
		{
			result.append(chr);
		}


		// Remaining non-printing characters.
		// HTML can't properly escape or display these,
		// so replace by error marker "?dd".
		else if (chr >= 0 && chr < 32)
		{
			result += "&#x3F;";
			result.append('0' + chr / 10);
			result.append('0' + chr % 10);
		}


		// Hex-escape all other characters, in particular
		// control characters, extended ASCII, and punctation.
		else
		{
			result += "&#x";

			char high_nib =
				static_cast<char>(static_cast<unsigned char>(chr) >> 4);
			result.append
				((high_nib < 10) ? ('0' + high_nib) : ('A' + high_nib - 10));

			char low_nib =
				static_cast<char>(static_cast<unsigned char>(chr) & 0x0F);
			result.append
				((low_nib < 10) ? ('0' + low_nib) : ('A' + low_nib - 10));

			result.append(';');
		}
	}

	return result;
}


void log_context::write_to_target_file
	(const log_entry& entry, int64 index)
{
	if (!target_file_handle_) return;


	// Retrieve entry data. All string data is escaped,
	// as to avoid potential HTML code exploits.
	string message(escape_html(entry.message));
	log_entry::log_entry_type type = entry.type;	
	string call(escape_html(entry.source_call));
	string file(escape_html(entry.source_file));
	int line = entry.source_line;


	// Entry html class: Row index determines alternating
	// background colour for each row.
	fprintf(target_file_handle_, "%s", "<tr class=\"");

	if (index % 2 == 0)
		fprintf(target_file_handle_, "%s", html_class_for_entry_type_even_[type]);
	else
		fprintf(target_file_handle_, "%s", html_class_for_entry_type_odd_[type]);

	fprintf(target_file_handle_, "%s", "\">");


	// Store actual entry. Captions occupy an entire row.
	if (type == log_entry::caption)
	{	
		fprintf(target_file_handle_, "%s", "<td colspan=\"5\">");
		fprintf(target_file_handle_, "%s", "<h2>");
		fprintf(target_file_handle_, "%s", message.data());
		fprintf(target_file_handle_, "%s", "</h2>");
		fprintf(target_file_handle_, "%s", "</td>");
	}
	else
	{
		fprintf(target_file_handle_, "%s", "<td><center>");
		fprintf(target_file_handle_, "%s", log_entry_signs_[type]);
		fprintf(target_file_handle_, "%s", "</center></td><td>");
		fprintf(target_file_handle_, "%s", message.data());
		fprintf(target_file_handle_, "%s", "</td><td>");
		fprintf(target_file_handle_, "%s", call.data());
		fprintf(target_file_handle_, "%s", "</td><td>");
		fprintf(target_file_handle_, "%s", file.data());
		fprintf(target_file_handle_, "%s", "</td><td><center>");
		fprintf(target_file_handle_, "%i", line);
		fprintf(target_file_handle_, "%s", "</center></td>");
	}

	fprintf(target_file_handle_, "%s", "</tr>\r\n");

	fflush(target_file_handle_);
}


void log_context::write_to_stream
	(const log_entry& entry, FILE *streamid)
{
	if (!streamid) return;

	fprintf(streamid, "%s %s: %s\r\n",
			log_entry_signs_[entry.type],
			entry.source_call.data(),
			entry.message.data());

	fflush(streamid);
}


const char* const log_context::log_entry_signs_[log_entry::entry_type_count] =
	{ "X", "!", "W", "I", " ", "U", "~" };


const char* const log_context::html_header_ =
		"<html>\r\n"
		"  <head>\r\n"
		"    <title>Application log</title>\r\n"
		"    <style type=\"text/css\">\r\n"
	
		"table { boder-width: 0px; border-spacing: 0px; border-style: none;"
				 " border-collapse: collapse; }\r\n"

		"h2 { margin-bottom: 4px; margin-top: 20px; }\r\n"

		"tr.header { color: #FFFFFF; background-color: #000000; }\r\n"
		"tr.caption { color: #000000; background-color: #FFFFFF; }\r\n"
		"tr.critical_even { color: #FFFFFF; background-color: #7D26CD; }\r\n"
		"tr.critical_odd { color: #FFFFFF; background-color: #651FA5; }\r\n"
		"tr.error_even { color: #FFFFFF; background-color: #A40000; }\r\n"
		"tr.error_odd { color: #FFFFFF; background-color: #990000; }\r\n"
		"tr.warning_even { color: #FFFFFF; background-color: #A06262; }\r\n"
		"tr.warning_odd { color: #FFFFFF; background-color: #995555; }\r\n"
		"tr.info_even { background-color: #EAEEEE; }\r\n"
		"tr.info_odd { background-color: #DFE2E2; }\r\n"
		"tr.user_even { background-color: #FFEFDB; }\r\n"
		"tr.user_odd { background-color: #EEDFCC; }\r\n"
		"tr.file_ref_even { background-color: #D9EED9; }\r\n"
		"tr.file_ref_odd { background-color: #D2E4CC; }\r\n"
		
		"td { border-width: 0px; border-style: none; padding: 5px 12px; }\r\n"

		"    </style>\r\n"
		"  </head>\r\n"
		"  <body>\r\n"
		"    <h1>Application log</h1>\r\n"
		"    <table>\r\n"
		"      <thead><tr class=\"Header\">\r\n"
		"        <td><center>Type</center></td>"
		"        <td>Message</td>"
		"        <td>Call</td>"
		"        <td>File</td>"
		"        <td><center>Line</center></td>\r\n"
		"      </tr></thead>\r\n"
		"      <tbody>\r\n";


const char* const log_context::html_footer_ =
		"      </tbody>\r\n"
		"    </table>\r\n"
		"  </body>\r\n"
		"</html>\r\n";


const char* const
	log_context::html_class_for_entry_type_even_[log_entry::entry_type_count] =
	{
		"critical_even", "error_even", "warning_even", "info_even",
		"caption", "user_even", "file_ref_even"
	};


const char* const
	log_context::html_class_for_entry_type_odd_[log_entry::entry_type_count] =
	{
		"critical_odd", "error_odd", "warning_odd", "info_odd",
		"caption", "user_odd", "file_ref_odd"
	};




//////////////////////////////////////////////////////////////////////////
//
//	log_context, Windows-specific implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__OS_TARGET_WINDOWS)


string log_context::create_absolute_target_path
	(const string& target_path)
{
	/**
	 * On windows, store logging files relative to the application
	 * data folder, see ms_file_context. If no target path is given,
	 * or if the query for the appdata path fails, we instead use
	 * the current working folder as a fallback.
	 */

	char appdata_path[_MAX_PATH];
	HRESULT result =
		SHGetFolderPath(NULL, CSIDL_APPDATA, NULL,
						SHGFP_TYPE_CURRENT, appdata_path);

	string base_path;
	if (result == S_OK)
	{
		base_path = string(appdata_path);
		base_path.replace_all('\\', '/');
	}


	// Try to create all paths up to the target file from either
	// the appdata folder or the current working folder. We simply
	// ignore folder creation failure, as later file creation
	// errors out approriately.
	string fixed_path(target_path);
	fixed_path.replace_all('\\', '/');
	list<string> pathlist;
	fixed_path.split('/', pathlist);

	string create_path(base_path);
	list<string>::iterator it(pathlist.first());
	while (it)
	{
		if (it == pathlist.last()) break; // Actual file name.
		
		create_path = 
			(create_path != "") ?
				(create_path + "/" + *it) : *it;

		CreateDirectory(create_path.data(), NULL);

		++it;
	}


	// Return absolute path to target file.
	return (base_path == "") ?
				fixed_path : (base_path + "/" + fixed_path);
}


void log_context::write_to_debug_pane
	(const log_entry& entry)
{
	/** Point & click functionality from within Visual Studio. */

	string result
		(entry.source_file +
		 "(" + entry.source_line + ") : " +
		 entry.message + "\r\n");

	OutputDebugString(result.data());
}


void log_context::show_in_notify_box
	(const log_entry& entry)
{
	/** Display a common Windows API message box with log entry. */

	const char *caption = "";
	UINT mbflags = MB_OK | MB_TOPMOST | MB_SETFOREGROUND;

	if (entry.type == log_entry::critical)
	{
		caption = "Critical error:";
		mbflags |= MB_ICONSTOP;
	}
	else if (entry.type == log_entry::warning)
	{
		caption = "Warning:";
		mbflags |= MB_ICONEXCLAMATION;
	}
	else
	{
		caption = "Info:";
		mbflags |= MB_ICONINFORMATION;
	}

	string message
		(entry.message + "\r\n" +
				"in file: " + entry.source_file + "\r\n" +
				"on line: " + entry.source_line + "\r\n" +
				"in call: " + entry.source_call);

	MessageBox
		(NULL, message.data(), caption, mbflags);
}


void log_context::beep() { Beep(350, 200); }


#endif




//////////////////////////////////////////////////////////////////////////
//
//	log_context, Unix-specific implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined (VIRAL__OS_TARGET_UNIX)


string log_context::create_absolute_target_path
	(const string& target_path)
{
	/**
	 * On Unix, store logging files relative to VIRAL_WRITE_FOLDER,
	 * see posix_file_context.
	 */

	char* write_path = getenv("VIRAL_WRITE_PATH");
	if (!write_path)
	{
		fprintf(stderr, "%s", "VIRAL_WRITE_PATH has not been set.");
		throw invalid_state();
	}

	struct stat s;
	if (stat(write_path, &s) == -1 || !S_ISDIR(s.st_mode))
	{
		fprintf(stderr, "%s", "VIRAL_WRITE_PATH does not exist or is no directory.");
		throw invalid_state();
	}


	// Try to create all paths up to the target file starting
	// from the (existing) write path. We simply ignore folder
	// creation failure, as later file creation errors out then.
	list<string> pathlist;
	target_path.split('/', pathlist);

	string create_path(write_path);
	list<string>::iterator it(pathlist.first());
	while (it)
	{
		if (it == pathlist.last()) break;  // Actual file name.

		create_path =
			(create_path != "") ?
				(create_path + "/" + *it) : *it;

		struct stat s;
		if (stat(create_path.data(), &s) == -1)
		{
			if (mkdir(create_path.data(), S_IRUSR | S_IWUSR) == -1)
				break;
		}

		++it;
	}


	// Return absolute path to target file.
	return string(write_path) + "/" + target_path;
}


void log_context::write_to_debug_pane
	(const log_entry& entry)
		{ /** Logging to debug pane is not supported under Unix. */ }

void log_context::show_in_notify_box
	(const log_entry& entry)
		{ /** Logging to message box is not supported under Unix. */ }


void log_context::beep()
	{ /** Beep on error is not supported under Unix. */ }


#endif




//////////////////////////////////////////////////////////////////////////
//
//  log_singleton
//
//////////////////////////////////////////////////////////////////////////


void log_singleton::init_thread_safety()
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);
}


void log_singleton::init
	(const string& context_name, uint32 flags,
	 const string& target_file)
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);

	log_context*& instance = static_instance();

	if (instance) return;

	instance =
		new log_context(context_name, flags, target_file);
}


void log_singleton::destroy() NOTHROW
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);

	log_context*& instance = static_instance();
	delete instance;
	instance = NULL;
}


void log_singleton::log
	(const string& source_file, int source_line,
	 const string& source_call, log_entry::log_entry_type type,
	 const string& message)
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);

	log_context*& instance = static_instance();

	if (instance)
	{
		instance->log
			(source_file, source_line, source_call, type, message);
	}
	else
	{
		// Log singleton not yet init()ialized.
		// Send messages to stdout so these do not get lost.
		log_context::write_to_stream
			(log_entry(source_file, source_line, source_call,
					   type, message), stdout);
	}
}


log_entry log_singleton::current_entry()
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);

	log_context*& instance = static_instance();
	if (!instance) return log_entry();
	return instance->current_entry();
}


string log_singleton::current_message()
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);

	log_context*& instance = static_instance();
	if (!instance) return string();
	return instance->current_message();
}


int64 log_singleton::message_count()
{
	mutex& m = static_mutex();
	MUTEX_SCOPE(m);

	log_context*& instance = static_instance();
	if (!instance) return 0;
	return instance->message_count();
}


mutex& log_singleton::static_mutex()
{
	/**
	 * Initialize and return static, local mutex
	 * that guards the static_instance().
	 *
	 * Implementation notes:
	 * - Static local variables initialize on first use.
	 *		This works correctly even during the (unordered)
	 *		static initialization phase.
	 * - On pre-C++11 compilers, static local initialization
	 *		is not thread-safe, hence the requirement for
	 *		a single-threaded init() or init_thread_safety().
	 */

	static mutex m;
	return m;
}


log_context*& log_singleton::static_instance()
{
	/**
	 * Initialize and return static, local instance pointer.
	 * See static_mutex() for rationale.
	 */

	static log_context* instance = NULL;
	return instance;
}




} /* namespace viral_core */
