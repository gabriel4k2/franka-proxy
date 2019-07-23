/**
 *************************************************************************
 *
 * @file log.hpp
 *
 * Logging header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__LOG_HPP)
#define INCLUDED__VIRAL_CORE__LOG_HPP


#include "build_config.hpp"


#include <stdio.h>


#include "util.hpp"
#include "string.hpp"
#include "thread_synch.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup log_module Log Module
 * @ingroup string_module
 *
 * Offers general utilities for saving progress and error output
 * to multiple output targets.
 *
 * Global log usage:
 * - Logging macros (e.g. @ref LOG_INFO, @ref LOG_WARN, @ref LOG_ERROR)
 *		are the preferred approach to logging. These macros target
 *		the global log as represented by the log_singleton class.
 * - Clients must explicitly initialize and destroy the log_singleton
 *		to enable advanced logging features:
 *			@code
 *			// Logging only to stdout, not initialized.
 *			log_singleton::init(".my_app/log_file.html", flags);
 *			// Log in here.
 *			log_singleton::destroy();
 *			// Logging to stdout again.
 *			@endcode
 *		The singleton ignores redundant initialization or destruction.
 * - Clients can use the global log outside the log_singleton lifetime.
 *		In this case, incoming messages appear on standard output only,
 *		and log queries return default values. Notably, this also holds
 *		during pre-main() static initialization.
 * - Logging via the logging macros can be disabled altogether with the
 *		@ref VIRAL__DISABLE_LOGGING_MACROS define in build_config.hpp.
 * - Logging macros eat up any exceptions and thus do not throw.
 *		This allows to log from @ref NOTHROW methods without
 *		special precautions.
 *
 * General notes:
 * - Any exception or error within logging may result in missed
 *		log messages within any output target.
 * - Client programs may create additional log_context instances
 *		for alternative logging purposes. @attent In this case,
 *		the safety net of the logging macros is missing, though!
 *
 * Thread safety:
 * - In general, any log_context is thread-safe for use
 *		by an arbitrary number of client threads at a time.
 * - @attent The very first call to the global log_singleton
 *		is not threadsafe because it allocates the necessary mutex.
 *		Hence, clients must ensure a single-threaded call to either
 *		log_singleton::init_thread_safety() or log_singleton::init() 
 *		in order to establish later thread safety.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class log_entry
 * @ingroup log_module
 *
 * Stores a single logged message in a log_context, its error severity,
 * and additional debugging information.
 *
 ************************************************************************/
class log_entry
{

	public:

		enum log_entry_type
		{
			/**
			 * Severe error that probably terminates the program.
			 * Usually caused by failing OS calls or violated invariants.
			 */
			critical,

			/**
			 * Any non-fatal error condition, program flow might continue.
			 * Typically caused by higher-level programming errors.
			 */
			error,

			/**
			 * Non-critical error or inconsistency, program flow can continue.
			 * Used for slight programming errors or inconsistencies.
			 */
			warning,


			/** Purely informational message, such as a progress report. */
			info,

			/** Heading for graphically structuring the log into sections. */
			caption,

			/** Message directly generated as a reaction to user input. */
			user_notification,

			/** Debug file reference for clicking on within the C++ IDE. */
			file_reference,


			entry_type_count
		};


		log_entry()
			: source_line(0), type(info) { }

		log_entry(const string& source_file, int source_line,
				 const string& source_call, log_entry_type type,
				 const string& message)
			:
			source_file(source_file), source_line(source_line),
			source_call(source_call), type(type),
			message(message)
		{ }

		void swap_with(log_entry &other) NOTHROW
		{
			swap(source_file, other.source_file);
			swap(source_line, other.source_line);
			swap(source_call, other.source_call);
			swap_assign(type, other.type);
			swap(message, other.message);
		}


		string source_file;
		int source_line;
		string source_call;		
		log_entry_type type;
		string message;
};




/**
 *************************************************************************
 *
 * @class log_context
 * @ingroup log_module
 *
 * Accepts debug output messages for diagnostic purposes, stores these
 * internally in log_entry objects for later GUI display, and writes them
 * to various output targets.
 *
 * Notes:
 * - See the @ref log_module "Log Module" for usage guidelines.
 *
 ************************************************************************/
class log_context :
	private non_copyable
{
	friend class log_singleton;


	public:

		enum log_flag
		{
			/** Print all logging output to the HTML logging file. */
			output_to_file = 1,

			/** Print all logging output to standard out. */
			output_to_stdout = 2,

			/** Rereoute all error messages to error out. */
			output_to_stderr = 4,

			/** Provide error output on the IDE debugging pane. */
			debug_output_on_problem = 8,

			/** Display a modal message box for all errors. */
			notify_on_problem = 16,

			/** Beep on the PC speaker on all errors. */
			beep_on_problem = 32
		};


		/**
		 * Use given @bref{flags} from the log_flag enum
		 * and an implementation-specific @bref{target_path}
		 * to the html output file.
		 */
		log_context
			(const string& context_name,
			 uint32 flags, const string& target_path);


		~log_context() NOTHROW;
	 

		/**
		 * Add a @bref{message} with severity @bref{type} to the log,
		 * and store @bref{source_file}, @bref{source_call} and @bref{source_line}
		 * as associated debugging information.
		 */
		void log
			(const string& source_file, int source_line,
			 const string& source_call, log_entry::log_entry_type type,
			 const string& message);


		/** Retrieve most recently added log entry or its message. */
		//@{
		log_entry current_entry() const;
		string current_message() const;
		//@}


		/** Retrieve total number of messages logged up to now. */
		int64 message_count() const;


		const string context_name;


	private:

		static string escape_html(const string& text);

		void write_to_target_file(const log_entry& entry, int64 index);
		static void write_to_stream(const log_entry& entry, FILE *streamid);


		string create_absolute_target_path(const string& target_path);
		void write_to_debug_pane(const log_entry& entry);
		void show_in_notify_box(const log_entry& entry);
		void beep();


		static const int64 max_entry_count_ = 1000;

		static const char* const log_entry_signs_[log_entry::entry_type_count];
		static const char* const html_header_;
		static const char* const html_footer_; 
		static const char* const html_class_for_entry_type_even_[log_entry::entry_type_count];
		static const char* const html_class_for_entry_type_odd_[log_entry::entry_type_count];


		mutable mutex log_lock_;

		uint32 flags_;
		FILE *target_file_handle_;

		log_entry entries_[max_entry_count_];
		int64 current_entry_;
		int64 message_count_;
		int64 error_count_;
};




/**
 *************************************************************************
 *
 * @class log_singleton
 * @ingroup log_module
 *
 * Manages the global log_context that is used by logging macros
 * such as @ref LOG_INFO, @ref LOG_WARN, and @ref LOG_CRITICAL.
 *
 * Notes:
 * - See the @ref log_module "Log Module" for usage guidelines.
 *
 ************************************************************************/
class log_singleton :
	private non_constructable
{

	public:


		/**
		 * On first invocation, preallocate the mutex
		 * that guards the singleton instance.
		 * Ignores later invocations.
		 * @attent Not threadsafe on first invocation!
		 */
		static void init_thread_safety();


		/**
		 * Calls match those of a log_context,
		 * but work on the global log instead.
		 * All calls implicitly init_thread_safety().
		 * See @ref log_module "Log Module" for usage.
		 */
		//@{
		static void init
			(const string& context_name, uint32 flags,
			 const string& target_file);

		static void destroy() NOTHROW;

		static void log
			(const string& source_file, int source_line,
			 const string& source_call, log_entry::log_entry_type type,
			 const string& message);

		static log_entry current_entry();
		static string current_message();
		static int64 message_count();
		//@}


	private:

		static mutex& static_mutex();
		static log_context*& static_instance();
};




#if !defined(VIRAL__DISABLE_LOGGING_MACROS)


	/**
	 * Macros with @ref NOTHROW behaviour for logging
	 * via the (explicitly initialized!) global log_singleton.
	 *
	 * See log_entry::log_entry_type for a description
	 * of all available message types.
	 *
	 * @ingroup log_module
	 */
	//@{
	#define LOG_CRITICAL(message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(__FILE__, __LINE__, __FUNCTION__, \
				::viral_core::log_entry::critical, \
				::viral_core::string() + message); \
		} \
		catch (...) {}

	#define LOG_ERROR(message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(__FILE__, __LINE__, __FUNCTION__, \
				::viral_core::log_entry::error, \
				::viral_core::string() + message); \
		} \
		catch (...) {}

	#define LOG_WARN(message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(__FILE__, __LINE__, __FUNCTION__, \
				::viral_core::log_entry::warning, \
				::viral_core::string() + message); \
		} \
		catch (...) {}

	#define LOG_INFO(message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(__FILE__, __LINE__, __FUNCTION__, \
				::viral_core::log_entry::info, \
				::viral_core::string() + message); \
		} \
		catch (...) {}

	#define LOG_CAPTION(message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(__FILE__, __LINE__, __FUNCTION__, \
				::viral_core::log_entry::caption, \
				::viral_core::string() + message); \
		} \
		catch (...) {}

	#define LOG_USER(message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(__FILE__, __LINE__, __FUNCTION__, \
				::viral_core::log_entry::user_notification, \
				::viral_core::string() + message); \
		} \
		catch (...) {}

	#define LOG_FILE_REFERENCE(source_file, source_line, message) \
		try \
		{ \
		::viral_core::log_singleton::log \
			(source_file, source_line, "", \
				::viral_core::log_entry::file_reference, \
				::viral_core::string() + message); \
		} \
		catch (...) {}
	//@}

#else

	// Disabled logging macros still need try-catch blocks
	// for macro safety (e.g. in brace-less if statements).
	#define LOG_CRITICAL(message) \
		try { } catch (...) { }
	#define LOG_ERROR(message) \
		try { } catch (...) { }
	#define LOG_WARN(message) \
		try { } catch (...) { }
	#define LOG_INFO(message) \
		try { } catch (...) { }
	#define LOG_CAPTION(message) \
		try { } catch (...) { }
	#define LOG_USER(message) \
		try { } catch (...) { }
	#define LOG_FILE_REFERENCE(source_file, source_line, message) \
		try { } catch (...) { }

#endif




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__LOG_HPP) */
