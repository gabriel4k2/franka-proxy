/**
 *************************************************************************
 *
 * @file engine.hpp
 *
 * Engine header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ENGINE_HPP)
#define INCLUDED__VIRAL_CORE__ENGINE_HPP


#include "build_config.hpp"


#include "thread_forward.hpp"
#include "file_forward.hpp"
#include "network_forward.hpp"
#include "window_forward.hpp"
#include "input_forward.hpp"
#include "audio_forward.hpp"
#include "render_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "string.hpp"
#include "ms_util.hpp"
#include "engine_config.hpp"


/**
 *************************************************************************
 *
 * @mainpage
 *
 * The Viral Framework is a virtual reality abstraction layer.
 * It allows for intuitive creation of interactive simulations,
 * multimedia applications, and computer games.
 *
 * For example, Viral offers classes and methods for 3D mathematics,
 * multi-threading, input handling, audio output, and 3D rendering.
 * A series of helper and example applications (e.g. content editors)
 * further easen the development process.
 *
 * The following pages are entry points into framework functionality:
 *
 * - The @ref build_and_usage_page "Build and Usage" guide offers
 *		general instructions on how to build the Viral Framework
 *		and explains how to link Viral into client applications.
 *		Finally, the guide gives vital information on multithreading,
 *		exception support, and other usage pitfalls. Thus, the guide
 *		<b>should always be read first</b>.
 *		
 * - The viral_core::engine class creates and bundles all objects
 *		for a simple client application in an easy-to-use format.
 *		In other words, the engine provides client applications
 *		with an intuitive entry point to the Viral Framework.
 *
 * The remaining documentation may be reviewed when needed.
 * In particular, each functionality module has an annotated
 * module description page.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @page build_and_usage_page Build and Usage
 *
 * This page provides general design, build, and usage guidelines
 * for the Viral Framework.
 * 
 * General design:
 * - <b>The Viral Framework inherently is multithreaded.</b>
 *		A typical application has at least a separate rendering thread.
 *		This implies thread safety requirements for framework clients.
 *		The thread_module "Thread Module" gives an appropriate overview.
 * - <b>The Viral Framework relies on exceptions.</b>
 *		Most methods report failure by exception instead of return code.
 *		Compiler exception support must not be disabled, and client code
 *		must be aware of exception safety rules. Respective guidelines
 *		are given in the @ref exception_module "Exception Module".
 *
 * Using the Viral Framework in a client application:
 * - Build all framework libraries as described below.
 * - Add the <tt>source</tt> path to the include folders of the compiler.
 *		All framework headers then are available through another subfolder
 *		(e.g. <tt>\#include \<%viral_core/engine.hpp\></tt>).
 * - Add the <tt>%build/[configuration]</tt> path to the library list
 *		of the linker and link against all contained libraries.
 *
 * Building process overview:
 * - Download and extract the Viral Framework package.
 * - Build all framework libraries and the %viral_test application.
 *		System-specific build notes can be found below.
 * - Run the %viral_test application to ensure build integrity.
 *		Test failure is reported by crash, unhandled exception,
 *		or leaked memory. Error messages are no sign of failure,
 *		as several error-reporting functions are tested as well.
 * - Build and run example applications as desired.
 * - @attent Building might require an explicit copy of files
 *		from the <tt>assets</tt> to the <tt>build</tt> folder.
 *		Scripts that automate the copy operations are available
 *		in the directory <tt>tools/copy_assets</tt>.
 *
 * MS Windows specific build notes:
 * - A premade Visual Studio solution and associated projects
 *		with suitable compiler options are available under
 *		the <tt>tools/visual_studio_****</tt> directory.
 * - For efficient build management, Visual Studio projects
 *		are mostly configured via shared property sheets.
 * - @attent
 *		Several of the compiler options used to build any libraries
 *		must match with the options of the later client application
 *		(e.g. CRT type, multithreading, debug mode, static linking).
 *		Client applications thus might need to modify build options.
 *
 * External library dependencies:
 * - The included <tt>external</tt> folder holds the following libraries:
 *		- cameron_gjk, http://www.cs.ox.ac.uk/stephen.cameron/distances
 *		- expat, http://expat.sourceforge.net
 *		- libjpeg, http://www.ijg.org
 *		- libpng, http://www.libpng.org
 *		- libogg, http://www.xiph.org/ogg
 *		- libvorbis, http://www.xiph.org/vorbis
 *		- zlib, http://www.zlib.net
 * - The following external libraries must be retrieved from other sources:
 *		- C/C++ runtime, vendor/compiler-specific
 *		- Windows API, included in Visual C++, MS Windows only
 * - Certain external libraries are optional. If not available,
 *		these can be excluded from compiling via build_config.hpp
 *		or @ref VIRAL__CUSTOM_EXTERNAL_LIBS. Respective methods
 *		then throw viral_core::unsupported_feature on use.
 *
 * Doxygen documentation guidelines:
 * - Always consider higher-level class and module descriptions
 *		when searching for information. Only non-obvious usage,
 *		side effects, or invariants are documented.
 * - Doxygen @bref{EXTRACT_ALL} is required, since many elements have
 *		no explicit documentation. Custom tags denote C++ constants,
 *		blind (non-linking) references or formatting elements. Finally,
 *		a define @ref VIRAL__DOXYGEN_DOCUMENTATION is set if Doxygen
 *		currently is running. 
 * - A preconfigured control file and documentation layout is available
 *		within the <tt>tools/doxygen</tt> subfolder.
 *
 ***********************************************************************/




/**
 ************************************************************************
 *
 * @page license_page License, Contact, Credits
 *
 * This page covers framework licensing, provides developer contacts,
 * and credits external tools.
 *
 * The Viral Framework has been written and is maintained by:
 *
 * <pre>
 * Wernaeh
 * Tobias Werner
 *
 * Postal:
 * Albert-Einstein-Ring 12
 * 95448 Bayreuth
 * Germany
 *
 * EMail:
 * wernaeh@googlemail.com
 *
 * ICQ:
 * #110924697
 * </pre>
 *
 * It has been released under the following license:
 *
 * <pre>
 * The Viral Framework has been released under the GNU General Public
 * License, Version 3, 29 June 2007:
 *
 * Viral Framework for intuitive creation of multimedia applications
 * Copyright (C) 2000-2018 Tobias Werner
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see
 * http://www.gnu.org/licenses/gpl.html
 * </pre>
 *
 * If required, commercial licensing options are available as well.
 * Please contact me for any such requests.
 *
 * The following software has been used to create the Viral Framework
 * and accompanying sample applications:
 *	- Visual Studio Community, http://www.visualstudio.com
 *	- Doxygen, http://www.doxygen.nl
 *	- GIMP, http://www.gimp.org 
 *	- Krita, http://www.krita.org 
 *	- Blender, http://www.blender.org
 *  - BMFont, http://www.angelcode.com/products/bmfont
 *	- Reaper, http://www.reaper.fm
 *
 * Thank you for your great work!
 *
 ***********************************************************************/




/**
 *************************************************************************
 * Global program version defines.
 ************************************************************************/
//@{
#define VIRAL__VERSION_MAJOR "0"
#define VIRAL__VERSION_MINOR "11"
#define VIRAL__VERSION_HOTFIX "0"
#define VIRAL__VERSION_STRING \
	VIRAL__VERSION_MAJOR "." \
	VIRAL__VERSION_MINOR "." \
	VIRAL__VERSION_HOTFIX
//@}




/**
 *************************************************************************
 * The root framework namespace.
 ************************************************************************/
namespace viral_core
{


/**
 *************************************************************************
 *
 * @class engine
 *
 * Bundles together all contexts for a basic multimedia application.
 *
 * Usage overview:
 * - On construction, the engine instance automatically creates
 *		default contexts - window, input, rendering, audio, ...
 * - Explicit calls to both window_context::set_display() and
 *		window_context::update() are required to show the window.
 * - The application then enters its main loop, and loops up to
 *		some exit condition (e.g. window_context::close_flag()).
 * - The main loop works with engine contexts for application logics,
 *		e.g. with window_context::update(), input_context::update(),
 *		audio_context::execute(), and render_task::execute().
 *		See the respective classes for usage details.
 * - After return from the main loop, the application first destroys
 *		all objects of its own. Thereafter, it destroys the engine,
 *		which in turn cleans up all default contexts. This shutdown
 *		must run in the same thread that created the engine.
 * - Any errors on engine initialization are reported by exception.
 *		See the @ref exception_module "Exception Module" for details.
 *		Errors can easily be reported via critical_error().
 *
 * Thread safety:
 * - An engine instance is constant after construction,
 *		and may be used by arbitrary threads at a time.
 *
 ************************************************************************/
class engine :
	private non_copyable
{

	public:

		/**
		 * Create engine instance.
		 *
		 * Requires a unique @bref{application_name} to initialize
		 * various system parameters, from application icons over
		 * log files to file_context folders.
		 */
		engine(const string& application_name);


		~engine() NOTHROW;


		/** Retrieve construction-created contexts. */
		//@{
		file_context& files() const;
		network_context& network() const;
		network_task& ntask() const;
		window_context& window() const;
		input_context& input() const;
		audio_context& audio() const;
		audio_task& atask() const;
		render_context& renderer() const;
		render_task& rtask() const;
		//@}


		/** Report a critical error to the user. */
		//@{
		static void critical_error(const char* error_text);
		static void critical_error(const string& error_text);
		static void critical_error(const exception& ex);
		//@}


		const string application_name;


	private:

		void init_memory();
		void init_log();
		void startup_info();

		void create_contexts();
		void create_file_context();
		void create_network_context();
		void create_window_context();
		void create_input_context();
		void create_audio_context();
		void create_render_context();

		void shutdown() NOTHROW;


		auto_pointer<engine_config> engine_config_;

		#if defined(VIRAL__OS_TARGET_WINDOWS)
			auto_pointer<ms_sched_fix> ms_sched_fix_;
			auto_pointer<ms_com_init> ms_com_init_;
		#endif

		auto_pointer<file_context> files_;
		auto_pointer<network_context> network_;
		auto_pointer<network_task> ntask_;
		auto_pointer<window_context> window_;
		auto_pointer<input_context> input_;
		auto_pointer<audio_context> audio_;
		auto_pointer<audio_task> atask_;
		auto_pointer<render_context> renderer_;
		auto_pointer<render_task> rtask_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ENGINE_HPP) */
