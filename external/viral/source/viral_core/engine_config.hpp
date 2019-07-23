/**
 *************************************************************************
 *
 * @file engine_config.hpp
 *
 * Core engine configuration.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ENGINE_CONFIG_HPP)
#define INCLUDED__VIRAL_CORE__ENGINE_CONFIG_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class engine_config
 *
 * Configuration for the core @ref engine class.
 *
 ************************************************************************/
class engine_config
{

	public:

		enum network_context_type
		{
			default_network_context,
			null_network_context,
			ms_network_context,
			posix_network_context
		};

		enum window_context_type
		{
			default_window_context,
			null_window_context,
			ms_window_context,
			x11_window_context
		};

		enum input_context_type
		{
			default_input_context,
			null_input_context,
			ms_input_context,
			x11_input_context
		};

		enum audio_context_type
		{
			default_audio_context,
			null_audio_context,
			ms_audio_context
		};

		enum render_context_type
		{
			default_render_context,
			null_render_context,
			gl3_render_context
		};


		engine_config
			(const string& application_name,
			 network_context_type network_context,
			 window_context_type window_context,
			 input_context_type input_context,
			 audio_context_type audio_context,
			 render_context_type render_context);


		/**
		 * Store current engine configuration
		 * to application-specific config file.
		 */
		void save();


		/**
		 * Fetch engine config from command_line singleton
		 * and from an application-specific config file.
		 *
		 * Usage notes:
		 * - Any options set on the command line override
		 *		concurrent ones in the configuration file.
		 * - Most failures (e.g. config file not existent,
		 *		unspecified option, unknown option value)
		 *		set defaults for individual or all options,
		 *		but do not throw.
		 * - Loading may only throw on rare, critical errors
		 *		(e.g. when out of memory in engine_config()).
		 *
		 * Options format:
		 * - Command line options are set as key-value pairs
		 *		@code
		 *			key=value
		 *		@endcode
		 *		without quotes, escaping, or leading dashes.
		 * - Config XML files set options as XML attributes
		 *		@code
		 *			key="value"
		 *		@endcode
		 *		on a root node with tag engine_config.
		 * - A specification of all options exceeds the scope
		 *		of this documentation. See load_config_file()
		 *		and parse_command_line() for further details.
		 */
		static engine_config load
			(const string& application_name,
			 bool from_command_line,
			 bool from_config_file);


		const string application_name;

		const network_context_type network_context;
		const window_context_type window_context;
		const input_context_type input_context;
		const audio_context_type audio_context;
		const render_context_type render_context;


	private:

		typedef tree_map<string, string> options_map;


		static void load_config_file
			(const string& application_name,
			 options_map& options);
		static string config_file_path
			(const string& application_name);


		static void parse_command_line
			(options_map& options);
		static bool parse_key_value_pair
			(const string& parameter, string& key, string& value);			


		static string to_string(network_context_type network_context);
		static network_context_type to_network_context(const string& s);

		static string to_string(window_context_type window_context);
		static window_context_type to_window_context(const string& s);

		static string to_string(input_context_type input_context);
		static input_context_type to_input_context(const string& s);

		static string to_string(audio_context_type audio_context);
		static audio_context_type to_audio_context(const string& s);

		static string to_string(render_context_type render_context);
		static render_context_type to_render_context(const string& s);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ENGINE_CONFIG_HPP) */
