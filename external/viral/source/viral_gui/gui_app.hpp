/**
 *************************************************************************
 *
 * @file gui_app.hpp
 *
 * GUI application framework.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_APP_HPP)
#define INCLUDED__VIRAL_GUI__GUI_APP_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/util.hpp>
#include <viral_core/owning_list.hpp>
#include <viral_core/owning_map.hpp>
#include <viral_core/cache.hpp>
#include <viral_core/timer.hpp>
#include <viral_core/engine.hpp>


#include "gui.hpp"
#include "gui_modal.hpp"
#include "gui_element.hpp"
#include "gui_event_handler.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_bootstrap
 * @ingroup gui_module
 *
 * Performs common startup operations required for gui_app%s,
 * such as loading textures, shaders, and gui_style%s.
 *
 ************************************************************************/
class gui_bootstrap :
	private non_copyable
{

	public:

		gui_bootstrap
			(file_context& files,
			 render_task& rtask,
			 gui_shader_cache& shader_cache,
			 gui_material_cache& material_cache,
			 gui_style_cache& style_cache,
			 const string& vertex_shader_path,
			 const string& fragment_shader_path,
			 const string& material_path,
			 const list<string>& style_xml_paths);


		static const char* const gui_shader_name;
		const shared_instance<render_shader_id> gui_shader_id;
};




/**
 *************************************************************************
 *
 * @class gui_app
 * @ingroup gui_module
 *
 * Framework for GUI applications centered around a @bref{gui_context}.
 *
 * Usage notes:
 * - Derive from gui_app and implement a default constructor.
 *		This default constructor must hand suitable parameters
 *		to the gui_app() constructor. The default constructor
 *		should also register_event_handler()s for gui_element%s
 *		that are relevant to the client application.
 * - Optionally, reimplement logics_hook() and render_hook()
 *		to run custom code before GUI logics and GUI rendering.
 * - Within the program main(), call gui_app::run() and provide
 *		the custom gui_app implementation as template parameter.
 *
 * Advanced usage:
 * - Clients may access contexts (e.g. render_context, input_context)
 *		through the exposed @bref{engine_} instance.
 * - The gui_app automatically updates the engine and its contexts.
 *		Clients should not redundantly update contexts on their own
 *		to avoid unexpected behaviour.
 *
 ************************************************************************/
class gui_app :
	public gui_modal_context
{

	public:

		template <typename T>
			static void run()
		{
			run_inner<T>();
		}


		/**
		 * Register not-@NULL @bref{event_handler} for all gui_output_event%s
		 * that occur on some GUI @bref{element}. Optionally, fetch the element
		 * from @bref{element_cache_} by its unique @bref{name} in the cache.
		 *
		 * Usage notes:
		 * - Each element allows for multiple event handlers.
		 *		Per-element handlers execute in registration order.
		 * - Registered event handlers do not keep gui_element%s alive.
		 *		The gui_app regularly purges handlers for dead elements,
		 *		but only in reaction to any gui_output_event%s.
		 * - After dynamically creating and destroying gui_element%s,
		 *		clients may wish to explicitly remove_event_handlers()
		 *		or to purge_event_handlers().
		 */
		//@{
		void register_event_handler
			(const string& name,
			 auto_pointer<gui_event_handler> event_handler);
		void register_event_handler
			(const shared_instance<gui_element>& element,
			 auto_pointer<gui_event_handler> event_handler);
		//@}


		/**
		 * Register an event handler that invokes a @bref{callback}
		 * on a generic @bref{object} and filters gui_output_event%s
		 * at least to type @bref{T}. See register_event_handler().
		 *
		 * @attent Any bound @bref{object} must remain valid
		 * for the entire lifetime of its event handler.
		 */
		//@{
		template <typename T, typename O>
			void register_event_callback
				(const string& name,
				 O& object, void (O::*callback)(const T&))
		{
			auto_pointer<gui_event_handler_callback<T, O>> event_handler
				(new gui_event_handler_callback<T, O>(object, callback));
			
			register_event_handler
				(name, std::move(event_handler));
		}

		template <typename T, typename O>
			void register_event_callback
				(const shared_instance<gui_element>& element,
				 O& object, void (O::*callback)(const T&))
		{
			auto_pointer<gui_event_handler_callback<T, O>> event_handler
				(new gui_event_handler_callback<T, O>(object, callback));
			
			register_event_handler
				(element, std::move(event_handler));
		}
		//@}


		/**
		 * Remove all event handlers for a GUI @bref{element}.
		 * Optionally fetch the element from @bref{element_cache_}
		 * by its unique @bref{name} in the cache.
		 *
		 * Both calls log (but otherwise ignore) any errors
		 * to achieve @ref NOTHROW behaviour.
		 */ 
		//@{
		void remove_event_handlers
			(const string& name) NOTHROW;
		void remove_event_handlers
			(const weak_pointer<gui_element>& element) NOTHROW;
		//@}


		/** Discard all event handlers on dead gui_element%s. */
		void purge_event_handlers() NOTHROW;


		/** Discard event handlers on all gui_element%s. */
		void clear_event_handlers() NOTHROW;


		/** Implementation-specific step_timer configuration. */
		//@{
		static const float timer_timestep_seconds;
		static const float timer_maximum_seconds;
		//@}


		/** Application name, see gui_app(). */
		const string application_name;		


	protected:

		/**
		 * Create a GUI application.
		 *
		 * General notes:
		 * - A gui_app internally creates an @ref engine instance.
		 *		Thus, a window_context, a render_context, and
		 *		an input_context are readily available for clients.
		 * - The unique @bref{application_name} initializes various
		 *		system contexts. See engine::engine() for details.
		 * - All paths are virtual_path%s on the engine file_context.
		 *		Usually, they are relative to a subdirectory with
		 *		the application name, next to the executable file.
		 *
		 * Elements and styles:
		 * - The list of @bref{style_paths} provides XML files with
		 *		gui_style%s that a gui_style_loader processes in-order.
		 *		One of the style files must hold a gui_cursor_style
		 *		@bref{default_cursor_style} for the mouse cursor.
		 * - The @bref{element_paths} list indicates XML files with
		 *		gui_element%s that a gui_element_loader reads in-order.
		 *		The specified @bref{root_element} becomes the content
		 *		of the lowest-most modal GUI layer.
		 * - Multiple XML files with gui_style%s and gui_element%s
		 *		easen content management on complex GUIs.
		 *
		 * Rendering notes:
		 * - All shader paths contain shader source code suitable
		 *		for the currently selected rendering backend.
		 *		The @bref{shader_cache_} exports a corresponding
		 *		shader ID under gui_bootstrap::gui_shader_name.
		 * - Loading generates a single material for each PNG file
		 *		directly within the folder @bref{material_path}.
		 * - Materials always are unlit and have their blend mode
		 *		set to alpha blending.
		 */
		gui_app
			(const string& application_name,
			 const string& vertex_shader_path,
			 const string& fragment_shader_path,
			 const string& material_path,
			 const list<string>& style_paths,
			 const string& default_cursor_style,
			 const list<string>& element_paths,
			 const string& root_element);


		/**
		 * Run the GUI from an existing gui_app instance.
		 * Blocks until the GUI exits from its main loop,
		 * e.g. because the user closed the application.
		 *
		 * @attent Unsafe execution allows implementations
		 * to circumvent the safety of a static run() entry.
		 * Depending on target OS and enabled libraries,
		 * execute_unsafe() must run from the main thread.
		 * Otherwise, program behaviour may silently break.
		 */
		void execute_unsafe();


		/**
		 * Client-implementable hooks that execute before GUI logics
		 * (i.e. at each @bref{timer_} step) or before GUI rendering.
		 *
		 * Usage notes:
		 * - Implementations need not call the empty parent hooks.
		 * - Opposed to gui_context::commit_render(), the gui_app
		 *		automatically issues render_command_queue::purge().
		 *		Clients need not explicitly do so in render_hook().
		 */
		//@{
		virtual void logics_hook(gui_modal_interaction* modal_interaction);
		virtual void render_hook(render_command_queue& queue);
		//@}


		step_timer timer_;
		engine engine_;	

		gui_shader_cache shader_cache_;
		gui_material_cache material_cache_;
		gui_style_cache style_cache_;
		gui_element_cache element_cache_;
	
		gui_bootstrap bootstrap_;
		gui_context gui_;

		const shared_instance<render_process_id> process_id_;
		const shared_instance<render_canvas_id> canvas_id_;


	private:

		template <typename T>
			static void run_inner()
		{
			T app;
			app.gui_app::execute(); // Scoped execute() enforces gui_app parent.
		}

		void execute();

		void initialize_elements
			(const list<string>& element_paths, const string& root_element);
		void initialize_window();

		bool main_loop_iteration
			(gui_modal_interaction* modal_interaction);
		void modal_iteration
			(gui_modal_interaction& modal_interaction) override;	

		void logics(gui_modal_interaction* modal_interaction);
		void logics_event_handlers();

		void render();


		typedef owning_list<gui_event_handler> event_handler_list;
		typedef owning_map<weak_pointer<gui_element>,
						   event_handler_list> event_handler_map;
		event_handler_map event_handlers_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_APP_HPP) */
