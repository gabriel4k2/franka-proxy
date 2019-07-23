/**
 *************************************************************************
 *
 * @file engine_config.cpp
 *
 * Core engine configuration, implementation.
 *
 ************************************************************************/


#include "engine_config.hpp"


#if defined(VIRAL__OS_TARGET_WINDOWS)
	#include <shlobj.h>
#endif


#include "log.hpp"
#include "util.hpp"
#include "auto_pointer.hpp"
#include "tree_map.hpp"
#include "command_line.hpp"
#include "file_util.hpp"
#include "xml.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// engine_config
//
//////////////////////////////////////////////////////////////////////////


engine_config::engine_config
	(const string& application_name,
	 network_context_type network_context,
	 window_context_type window_context,
	 input_context_type input_context,
	 audio_context_type audio_context,
	 render_context_type render_context)
	:
	application_name(application_name),
	network_context(network_context),
	window_context(window_context),
	input_context(input_context),
	audio_context(audio_context),
	render_context(render_context)
{ }


void engine_config::save()
{
	auto_pointer<xml_node> config_root
		(new xml_node("engine_config"));

	config_root->insert_attribute
		("network_context", to_string(network_context));
	config_root->insert_attribute
		("window_context", to_string(window_context));
	config_root->insert_attribute
		("input_context", to_string(input_context));
	config_root->insert_attribute
		("audio_context", to_string(audio_context));
	config_root->insert_attribute
		("render_context", to_string(render_context));

	disk_file config_file
		(config_file_path(application_name),
		 file::read_write_truncate);

	config_root->save(config_file);
}


engine_config engine_config::load
	(const string& application_name,
	 bool from_command_line,
	 bool from_config_file)
{
	// By default, use default contexts 
	// (e.g. empty command line, no file).
	options_map options;
	options.insert("network_context", "default_network_context");
	options.insert("window_context", "default_window_context");
	options.insert("input_context", "default_input_context");
	options.insert("audio_context", "default_audio_context");
	options.insert("render_context", "default_render_context");


	// Override defaults by option values
	// from config file and command line.
	if (from_config_file)
		load_config_file(application_name, options);

	if (from_command_line)
		parse_command_line(options);


	// Parse (possibly overridden) strings in options map.
	// Keep individual default contexts on parsing error.
	network_context_type network_context = default_network_context;
	window_context_type window_context = default_window_context;
	input_context_type input_context = default_input_context;
	audio_context_type audio_context = default_audio_context;
	render_context_type render_context = default_render_context;

	try
	{
		network_context =
			to_network_context
				(options.at("network_context").value);
	}
	catch (const invalid_parameter&) { }

	try
	{
		window_context =
			to_window_context
				(options.at("window_context").value);
	}
	catch (const invalid_parameter&) { }

	try
	{
		input_context =
			to_input_context
				(options.at("input_context").value);
	}
	catch (const invalid_parameter&) { }

	try
	{
		audio_context =
			to_audio_context
				(options.at("audio_context").value);
	}
	catch (const invalid_parameter&) { }

	try
	{
		render_context =
			to_render_context
				(options.at("render_context").value);
	}
	catch (const invalid_parameter&) { }


	// Success.
	// Return final configuration.
	return engine_config
				(application_name,
				 network_context,
				 window_context,
				 input_context,
				 audio_context,
				 render_context);
}


void engine_config::load_config_file
	(const string& application_name,
	 options_map& options)
{
	LOG_INFO("Fetching engine config from config file.");


	// Parse XML file.
	auto_pointer<xml_node> config_root;

	try
	{
		disk_file config_file
			(config_file_path(application_name),
			 file::read_only);

		config_root =
			xml_node::load(config_file);
	}
	catch (...)
	{
		LOG_WARN("Error while loading config file, using defaults.");
		return;
	}

	if (config_root->name() != "engine_config")
	{
		LOG_ERROR("Name of XML root is not engine_config, using defaults.");
		return;
	}


	// Fetch an XML attribute for each option.
	// Ignore any other XML attributes.
	for (options_map::iterator it(options.first()); it; ++it)
	{
		config_root->try_get_attribute
			(it->key, it->value);
	}
}


string engine_config::config_file_path
	(const string& application_name)
{
	/**
	 * Fetch full, OS-specific path to config file.
	 *
	 * Implementation notes:
	 * - We do not use any file_context utilities here,
	 *		as the @ref engine creates all its contexts
	 *		but after reading the configuration file.
	 * - The resulting path is only used internally,
	 *		so there is no need to fix conventions
	 *		(e.g. forward slashes of virtual_path).
	 */

	#if defined(VIRAL__OS_TARGET_WINDOWS)

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

		return string(appdata_path) +
					"\\" + application_name +
					"\\engine_config.xml";

	#else
		#error "Not implemented for current build target."
	#endif
}


void engine_config::parse_command_line
	(options_map& options)
{
	LOG_INFO("Fetching engine config from command line.");

	const list<string> parameters
		(command_line::parameters());

	for (list<string>::const_iterator it
			(parameters.first()); it; ++it)
	{
		string key;
		string value;

		if (!parse_key_value_pair(*it, key, value))
			continue;


		// Ignore unrelated command line options.
		if (!options.contains(key))
			continue;


		// Override existing options.
		options.at(key).value = value;
	}
}


bool engine_config::parse_key_value_pair
	(const string& parameter, string& key, string& value)
{
	/**
	 * Split a key-value pair in a command line parameter
	 * into @bref{key} and @bref{value} and return @true,
	 * or return @false if there is no key-value pair.
	 *
	 * Implementation notes:
	 * - Parsing does not support masking of characters,
	 *		as this is not required for current values.
	 * - Error handling is provisional, but sufficient
	 *		for usual command lines.
	 */

	string parameter_trimmed
		(parameter.trim());
	int64 first_equals =
		parameter_trimmed.seek('=');

	if (first_equals == string::invalid_index)
		return false;

	key = parameter_trimmed.substring(0, first_equals);
	value = parameter_trimmed.substring(first_equals + 1);
	return true;
}


string engine_config::to_string
	(network_context_type network_context)
{
	RETURN_STRING_ON_ENUM_MATCH(network_context, default_network_context);
	RETURN_STRING_ON_ENUM_MATCH(network_context, null_network_context);
	RETURN_STRING_ON_ENUM_MATCH(network_context, ms_network_context);
	RETURN_STRING_ON_ENUM_MATCH(network_context, posix_network_context);

	LOG_ERROR("Unknown network context constant: " + network_context);
	throw invalid_parameter();
}

engine_config::network_context_type
	engine_config::to_network_context(const string& s)
{
	RETURN_ENUM_ON_STRING_MATCH(s, default_network_context);
	RETURN_ENUM_ON_STRING_MATCH(s, null_network_context);
	RETURN_ENUM_ON_STRING_MATCH(s, ms_network_context);
	RETURN_ENUM_ON_STRING_MATCH(s, posix_network_context);

	LOG_ERROR("Unknown network context string: " + s);
	throw invalid_parameter();
}


string engine_config::to_string
	(window_context_type window_context)
{
	RETURN_STRING_ON_ENUM_MATCH(window_context, default_window_context);
	RETURN_STRING_ON_ENUM_MATCH(window_context, null_window_context);
	RETURN_STRING_ON_ENUM_MATCH(window_context, ms_window_context);
	RETURN_STRING_ON_ENUM_MATCH(window_context, x11_window_context);

	LOG_ERROR("Unknown window context constant: " + window_context);
	throw invalid_parameter();
}

engine_config::window_context_type
	engine_config::to_window_context(const string& s)
{
	RETURN_ENUM_ON_STRING_MATCH(s, default_window_context);
	RETURN_ENUM_ON_STRING_MATCH(s, null_window_context);
	RETURN_ENUM_ON_STRING_MATCH(s, ms_window_context);
	RETURN_ENUM_ON_STRING_MATCH(s, x11_window_context);

	LOG_ERROR("Unknown window context string: " + s);
	throw invalid_parameter();
}


string engine_config::to_string
	(input_context_type input_context)
{
	RETURN_STRING_ON_ENUM_MATCH(input_context, default_input_context);
	RETURN_STRING_ON_ENUM_MATCH(input_context, null_input_context);
	RETURN_STRING_ON_ENUM_MATCH(input_context, ms_input_context);
	RETURN_STRING_ON_ENUM_MATCH(input_context, x11_input_context);

	LOG_ERROR("Unknown input context constant: " + input_context);
	throw invalid_parameter();
}

engine_config::input_context_type
	engine_config::to_input_context(const string& s)
{
	RETURN_ENUM_ON_STRING_MATCH(s, default_input_context);
	RETURN_ENUM_ON_STRING_MATCH(s, null_input_context);
	RETURN_ENUM_ON_STRING_MATCH(s, ms_input_context);
	RETURN_ENUM_ON_STRING_MATCH(s, x11_input_context);

	LOG_ERROR("Unknown input context string: " + s);
	throw invalid_parameter();
}


string engine_config::to_string
	(audio_context_type audio_context)
{
	RETURN_STRING_ON_ENUM_MATCH(audio_context, default_audio_context);
	RETURN_STRING_ON_ENUM_MATCH(audio_context, null_audio_context);
	RETURN_STRING_ON_ENUM_MATCH(audio_context, ms_audio_context);

	LOG_ERROR("Unknown audio context constant: " + audio_context);
	throw invalid_parameter();
}

engine_config::audio_context_type
	engine_config::to_audio_context(const string& s)
{
	RETURN_ENUM_ON_STRING_MATCH(s, default_audio_context);
	RETURN_ENUM_ON_STRING_MATCH(s, null_audio_context);
	RETURN_ENUM_ON_STRING_MATCH(s, ms_audio_context);

	LOG_ERROR("Unknown audio context string: " + s);
	throw invalid_parameter();
}


string engine_config::to_string
	(render_context_type render_context)
{
	RETURN_STRING_ON_ENUM_MATCH(render_context, default_render_context);
	RETURN_STRING_ON_ENUM_MATCH(render_context, null_render_context);
	RETURN_STRING_ON_ENUM_MATCH(render_context, gl3_render_context);

	LOG_ERROR("Unknown render context constant: " + render_context);
	throw invalid_parameter();
}

engine_config::render_context_type
	engine_config::to_render_context(const string& s)
{
	RETURN_ENUM_ON_STRING_MATCH(s, default_render_context);
	RETURN_ENUM_ON_STRING_MATCH(s, null_render_context);
	RETURN_ENUM_ON_STRING_MATCH(s, gl3_render_context);

	LOG_ERROR("Unknown render context string: " + s);
	throw invalid_parameter();
}




} /* namespace viral_core */
