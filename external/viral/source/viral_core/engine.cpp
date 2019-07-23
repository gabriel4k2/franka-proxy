/**
 *************************************************************************
 *
 * @file engine.cpp
 *
 * Engine implementation.
 *
 ************************************************************************/


#include "engine.hpp"


#if defined(VIRAL__OS_TARGET_UNIX)
	#include <iostream>
#endif


#include "log.hpp"
#include "memory.hpp"
#include "scope_guard.hpp"

#include "file.hpp"
#include "ms_file.hpp"
#include "posix_file.hpp"

#include "network.hpp"
#include "network_task.hpp"
#include "ms_network.hpp"
#include "posix_network.hpp"
#include "null_network.hpp"

#include "window.hpp"
#include "ms_window.hpp"
#include "x11_window.hpp"
#include "null_window.hpp"

#include "input.hpp"
#include "ms_input.hpp"
#include "x11_input.hpp"
#include "null_input.hpp"

#include "audio.hpp"
#include "audio_task.hpp"
#include "ms_audio.hpp"
#include "port_audio.hpp"
#include "null_audio.hpp"

#include "render.hpp"
#include "render_task.hpp"
#include "gl3_render.hpp"
#include "null_render.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  engine
//
//////////////////////////////////////////////////////////////////////////


engine::engine
	(const string& application_name)
	:
	application_name(application_name)
{
	// Properly shut down all engine objects
	// on any exception during construction.
	SCOPE_GUARD
		(engine_guard, (*this, &engine::shutdown));


	// Initialize in correct order.
	try
	{
		init_memory();
		init_log();
		startup_info();
		create_contexts();
	}


	// Error handling:
	// Report and propagate any unhandled exceptions
	// that occurred during engine initialization.
	catch (const exception& ex)
	{
		critical_error(ex);
		throw;
	}
	catch (const std::exception& ex)
	{
		critical_error(ex.what());
		throw;
	}
	catch (...)
	{
		critical_error("Unknown critical error");
		throw;
	}


	// Success.
	engine_guard.dismiss();
}


engine::~engine() NOTHROW
	{ shutdown(); }


file_context& engine::files() const { return *files_; }
network_context& engine::network() const { return *network_; }
network_task& engine::ntask() const { return *ntask_; }
window_context& engine::window() const { return *window_; }
input_context& engine::input() const { return *input_; }
audio_context& engine::audio() const { return *audio_; }
audio_task& engine::atask() const { return *atask_; }
render_context& engine::renderer() const { return *renderer_; }
render_task& engine::rtask() const { return *rtask_; }


void engine::critical_error(const char* error_text)
{
	/**
	 * Report some critical error (e.g. an exception)
	 * to the user (i.e. not through the log_singleton)
	 * in an OS-specific way.
	 *
	 * @attent This may happen during engine construction,
	 * so no access to members (e.g. @bref{files_}) in here.
	 */	

	#if defined(VIRAL__OS_TARGET_WINDOWS)
		MessageBox
			(NULL, error_text, "Critical error:",
			 MB_OK | MB_ICONSTOP | MB_SETFOREGROUND | MB_TOPMOST| MB_TASKMODAL);
	#endif

	#if defined(VIRAL__OS_TARGET_UNIX)
		std::cerr << "Critical error: " << error_text << std::endl;
	#endif

	#if !defined(VIRAL__OS_TARGET_WINDOWS) && \
		!defined(VIRAL__OS_TARGET_UNIX)
			#error "Not implemented for current build target."
	#endif
}

void engine::critical_error(const string& error_text)
	{ critical_error(error_text.data()); }

void engine::critical_error(const exception& ex)
	{ critical_error(ex.what()); }


void engine::init_memory()
	{ memory::enable_debug_output(); }


void engine::init_log()
{
	/** Initialize global log singleton. */

	log_singleton::init
		(application_name + "_log_context",
		
		#if defined(VIRAL__BUILD_MODE_DEBUG)
			log_context::output_to_file | log_context::output_to_stdout |
			log_context::debug_output_on_problem,
		#else
			log_context::output_to_stdout | log_context::output_to_file,
		#endif

		application_name + "/" + application_name + "_log.html");
}


void engine::startup_info()
{
	/** Print generic startup information to log file. */

	LOG_CAPTION("Welcome to Viral!");

	LOG_CAPTION("Build information");
	LOG_INFO("Core version: " VIRAL__VERSION_STRING);
	LOG_INFO("Last compiled on: " __TIME__ " " __DATE__);
	LOG_INFO("Target OS: " VIRAL__OS_TARGET_STRING);
	LOG_INFO("Target architecture: " VIRAL__MACHINE_TARGET_STRING);
	LOG_INFO("Compiled with: " VIRAL__COMPILER_STRING);
	LOG_INFO("Built with mode: " VIRAL__BUILD_MODE_STRING);

	LOG_CAPTION("Licensing information");
	LOG_INFO("Viral Framework Copyright (C) 2000-2018 Tobias Werner.");
	LOG_INFO("Released under the GNU General Public License, Version 3, 29 June 2007.");
	LOG_INFO("See accompanying documentation for details.");
	LOG_INFO("Contact: wernaeh@gmail.com");
}


void engine::create_contexts()
{
	/** Create main engine contexts and utility objects. */

	LOG_CAPTION("Engine startup");
	LOG_INFO("Running application: " + application_name);
	LOG_INFO("Creating application contexts.");

	engine_config_.reset
		(new engine_config
			(engine_config::load(application_name, true, true)));

	#if defined(VIRAL__OS_TARGET_WINDOWS)
		ms_sched_fix_.reset(new ms_sched_fix());
		ms_com_init_.reset(new ms_com_init());
	#endif

	create_file_context();
	create_network_context();
	create_window_context();
	create_input_context();
	create_audio_context();
	create_render_context();

	LOG_INFO("Completed startup.");
}


void engine::create_file_context()
{
	/** Create file context for current configuration. */

	#if defined(VIRAL__HAVE_MS_FILE)
		files_.reset
			(new ms_file_context
				(application_name + "_file_context",
				 application_name, application_name));
	#endif

	#if defined(VIRAL__HAVE_POSIX_FILE)
		files_.reset
			(new posix_file_context
				(application_name + "_file_context",
				 application_name, application_name));
	#endif

	#if !defined(VIRAL__HAVE_MS_FILE) && \
		!defined(VIRAL__HAVE_POSIX_FILE)
			#error "Not implemented for current file_context back-end."
	#endif
}


void engine::create_network_context()
{
	/** Create network context for current configuration. */


	// Resolve default network context
	// to any actual implementation.
	engine_config::network_context_type network_context =
		engine_config_->network_context;

	if (network_context == engine_config::default_network_context)
	{
		#if defined(VIRAL__OS_TARGET_WINDOWS)
			network_context = engine_config::ms_network_context;
		#elif defined(VIRAL__OS_TARGET_UNIX)
			network_context = engine_config::posix_network_context;
		#else
			#error "Not implemented for current OS target."
		#endif
	}


	// Create selected network context.
	#if defined(VIRAL__HAVE_MS_NETWORK)
		if (network_context == engine_config::ms_network_context)
			network_.reset
				(new ms_network_context
					(application_name + "_network_context"));
	#endif

	#if defined(VIRAL__HAVE_POSIX_NETWORK)
		if (network_context == engine_config::posix_network_context)
			network_.reset
				(new posix_network_context
					(application_name + "_network_context"));
	#endif

	if (network_context == engine_config::null_network_context)
		network_.reset
			(new null_network_context
				(application_name + "_network_context"));

	if (!network_)
	{
		LOG_ERROR("Invalid network context specified.");
		throw invalid_parameter();
	}


	// Create network task utility.
	ntask_.reset
		(new network_task
			(application_name + "_network_task"));
}


void engine::create_window_context()
{
	/** Create window context for current configuration. */


	// Resolve default window context
	// to any actual implementation.
	engine_config::window_context_type window_context =
		engine_config_->window_context;

	if (window_context == engine_config::default_window_context)
	{
		#if defined(VIRAL__OS_TARGET_WINDOWS)
			window_context = engine_config::ms_window_context;
		#elif defined(VIRAL__OS_TARGET_UNIX)
			window_context = engine_config::x11_window_context;
		#else
			#error "Not implemented for current OS target."
		#endif
	}


	// Create selected window context.
	#if defined(VIRAL__HAVE_MS_WINDOW)
		if (window_context == engine_config::ms_window_context)
			window_.reset
				(new ms_window_context
					(application_name + "_window_context",
					 application_name, application_name, false));
	#endif

	#if defined(VIRAL__HAVE_X11_WINDOW)
		if (window_context == engine_config::x11_window_context)
			window_.reset
				(new x11_window_context
					(application_name + "_window_context"));
	#endif

	if (window_context == engine_config::null_window_context)
		window_.reset
			(new null_window_context
				(application_name + "_window_context"));

	if (!window_)
	{
		LOG_ERROR("Invalid window context specified.");
		throw invalid_parameter();
	}
}


void engine::create_input_context()
{
	/** Create input context for current configuration. */


	// Resolve default input context
	// to any actual implementation.
	engine_config::input_context_type input_context =
		engine_config_->input_context;

	if (input_context == engine_config::default_input_context)
	{
		#if defined(VIRAL__OS_TARGET_WINDOWS)
			input_context = engine_config::ms_input_context;
		#elif defined(VIRAL__OS_TARGET_UNIX)
			input_context = engine_config::x11_input_context;
		#else
			#error "Not implemented for current OS target."
		#endif
	}


	// Create selected input context.
	#if defined(VIRAL__HAVE_MS_INPUT)
		if (input_context == engine_config::ms_input_context)
			input_.reset
				(new ms_input_context
					(application_name + "_input_context",
					 static_cast<ms_window_context&>(*window_)));
	#endif

	#if defined(VIRAL__HAVE_X11_INPUT)
		if (input_context == engine_config::x11_input_context)
			input_.reset
				(new x11_input_context
					(application_name + "_input_context"));
	#endif

	if (input_context == engine_config::null_input_context)
		input_.reset
			(new null_input_context
				(application_name + "_input_context"));

	if (!input_)
	{
		LOG_ERROR("Invalid input context specified.");
		throw invalid_parameter();
	}
}


void engine::create_audio_context()
{
	/** Create audio context for current configuration. */


	// Resolve default audio context
	// to any actual implementation.
	engine_config::audio_context_type audio_context =
		engine_config_->audio_context;

	if (audio_context == engine_config::default_audio_context)
	{
		#if defined(VIRAL__OS_TARGET_WINDOWS)
			audio_context = engine_config::ms_audio_context;
		#elif defined(VIRAL__OS_TARGET_UNIX)
			audio_context = engine_config::port_audio_context;
		#else
			#error "Not implemented for current OS target."
		#endif
	}


	// Create selected audio context.
	#if defined(VIRAL__HAVE_MS_AUDIO)
		if (audio_context == engine_config::ms_audio_context)
			audio_.reset
				(new ms_audio_context
					(application_name + "_audio_context"));
	#endif

	#if defined(VIRAL__HAVE_PORT_AUDIO)
		if (audio_context == engine_config::port_audio_context)
			audio_.reset
				(new port_audio_context
					(application_name + "_audio_context"));
	#endif

	if (audio_context == engine_config::null_audio_context)
		audio_.reset
			(new null_audio_context
				(application_name + "_audio_context"));

	if (!audio_)
	{
		LOG_ERROR("Invalid audio context specified.");
		throw invalid_parameter();
	}


	// Create audio task utility.
	atask_.reset
		(new audio_task
			(application_name + "_audio_task",
			 *audio_));
}


void engine::create_render_context()
{
	/** Create rendering context for current configuration. */


	// Resolve default render context
	// to any actual implementation.
	engine_config::render_context_type render_context =
		engine_config_->render_context;

	if (render_context == engine_config::default_render_context)
	{
		#if defined(VIRAL__OS_TARGET_WINDOWS)
			render_context = engine_config::gl3_render_context;
		#elif defined(VIRAL__OS_TARGET_UNIX)
			render_context = engine_config::gl3_render_context;
		#else
			#error "Not implemented for current OS target."
		#endif
	}


	// Create selected render context.
	#if defined(VIRAL__HAVE_GL3_RENDER)
		if (render_context == engine_config::gl3_render_context)
			renderer_.reset
				(new gl3_render_context
					(application_name + "_render_context",
					 *window_));
	#endif

	if (render_context == engine_config::null_render_context)
		renderer_.reset
			(new null_render_context
				(application_name + "_render_context"));

	if (!renderer_)
	{
		LOG_ERROR("Invalid render context specified.");
		throw invalid_parameter();
	}


	// Create render task utility.
	rtask_.reset
		(new render_task
			(application_name + "_render_task",
			 *renderer_, true));
}


void engine::shutdown() NOTHROW
{
	/**
	 * Shutdown the engine due to proper destruction
	 * or due to an exception during construction.
	 *
	 * @attent Contexts and other objects may be @NULL
	 * if shutdown() is called due to any exception.
	 */


	LOG_CAPTION("Goodbye!");
	LOG_CAPTION("Engine shutdown");
	LOG_INFO("Shutting down.");


	// Save current engine configuration, if any.
	// Ignore any subsequent errors for NOTHROW.
	try
	{
		if (engine_config_)
			engine_config_->save();
	}
	catch (...)
		{ LOG_ERROR("Failed to save engine configuration."); }


	// Destroy objects in inverse order of creation.
	// We do not let member destructors do clean up,
	// so that the log (if already and still alive)
	// can catch destructor messages.
	rtask_.reset();
	renderer_.reset();
	atask_.reset();
	audio_.reset();
	input_.reset();
	window_.reset();
	ntask_.reset();
	network_.reset();
	files_.reset();

	#if defined(VIRAL__OS_TARGET_WINDOWS)
		ms_com_init_.reset();
		ms_sched_fix_.reset();
	#endif

	engine_config_.reset();

	LOG_INFO("Completed shutdown.");


	// Properly terminate log file.
	log_singleton::destroy();
}




} /* namespace viral_core */
