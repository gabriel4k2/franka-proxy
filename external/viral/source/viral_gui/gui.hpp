/**
 *************************************************************************
 *
 * @file gui.hpp
 *
 * Main GUI header file.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_HPP)
#define INCLUDED__VIRAL_GUI__GUI_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/window_forward.hpp>
#include <viral_core/input_forward.hpp>
#include <viral_core/render_forward.hpp>


#include <viral_core/util.hpp>
#include <viral_core/shared_pointer.hpp>
#include <viral_core/string.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @defgroup gui_module GUI Module
 *
 * Manages user interaction via onscreen gui_element%s
 * that reside in a gui_context.
 *
 * High-level GUI overview:
 * - Most clients should use the high-level gui_app utility,
 *		which handles typical GUI logics and rendering.
 *		See gui_app and gui_bootstrap for details.
 * - Only clients with specific requirements (e.g. a 3D gui)
 *		might need to fall back to the low-level interface.
 *
 * Low-level GUI overview:
 * - Clients create a gui_context and insert gui_element%s
 *		to assemble the GUI. Elements need gui_style%s
 *		(see gui_style_loader) to define GUI layout and look.
 * - Clients call gui_context::update() to perform GUI logics.
 *		Notably, each update returns gui_output_event%s
 *		as reactions to user input (e.g. button clicks).
 * - Clients use gui_context::commit_render() to store any
 *		pending GUI display updates in a render_command_queue.
 *		A render_context can execute this queue later on.
 * - To enable rendering of the GUI, clients must explicitly
 *		add the gui_context::render_layer() to a window-sized
 *		rendering canvas (see render_canvas_data).
 *
 * General notes:
 * - Shared pointers track the lifetime of gui_element%s
 *		both across internal use (e.g. as children pointers)
 *		and external use (e.g. in gui_output_event handlers).
 * - All elements within a GUI must use the same gui_context.
 *		Insertion of children with a mismatching context throws.
 * - If any GUI elements outlive their bound gui_context due to
 *		stray client-side pointers, these elements remain safe
 *		for no other use but later destruction.
 * - All GUI coordinates (e.g. cursor positions, GUI element sizes)
 *		are pixel-based window coordinates with a top-left origin.
 *
 * Safety notes:
 * - The GUI has not been optimized for performance or stability.
 *		Hence the GUI not suited for end user applications,
 *		but it rather is a utility to realize internal tools.
 * - Elements provide strong exception safety where adequate
 *		(e.g. inserting into a list, setting a button text),
 *		but clients should never expect more than basic safety. 
 * - Unlike most other subsystems, the GUI is not threadsafe.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @namespace viral_gui
 * @ingroup gui_module
 *
 * Separate namespace for the GUI module.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class gui_context
 * @ingroup gui_module 
 *
 * Manages input handling and graphics output for an entire GUI.
 *
 * Usage notes:
 * - See @ref gui_module "GUI Module".
 *
 ************************************************************************/
class gui_context :
	private non_copyable
{

	public:

		gui_context
			(const string& name,
			 window_context& window,
			 input_context& input,
			 gui_modal_context& modal_context,
			 const shared_instance<gui_cursor_style>& cursor_style,
			 bool pixel_integral_rendering);


		~gui_context() NOTHROW;


		/**
		 * Fetch the root-level modal gui_stack,
		 * which contains any further gui_element%s
		 * that are currently part of the GUI.
		 */
		//@{
		gui_stack& modal_stack() NOTHROW;
		const gui_stack& modal_stack() const NOTHROW;
		//@}


		/**
		 * Determine if the GUI currently holds a GUI @bref{element}
		 * in the root-level modal_stack() or its children.
		 */		
		bool contains(const gui_element& element) NOTHROW;


		/**
		 * Perform GUI-internal logics, such as automatic
		 * layout updates and input handling.
		 * Returns any resulting @bref{output} events.
		 */
		void update
			(gui_output_event_list& output);


		/**
		 * Commit current GUI state to a command @bref{queue}.
		 * 
		 * Usage notes:
		 * - This call does not create a canvas for the GUI.
		 *		Instead clients must insert render_layer()
		 *		into some render_canvas_data of their own.
		 * - A call to commit_render() does not automatically purge
		 *		rendering IDs (e.g. those of destroyed gui_element%s).
		 *		Clients must rather call render_command_queue::purge()
		 *		at opportune moments to avoid a memory leak.
		 * - See gui_context documentation for higher-level usage.
		 */
		void commit_render
			(render_command_queue& queue);


		/**
		 * Make the next gui_context::commit_render() call rebuild
		 * all GUI meshes, for instance to update element display.
		 * Usually, clients should not explicitly call this function.
		 */
		void raise_render_dirty() NOTHROW;


		/**
		 * Fetch the ID of a render layer that shows the entire GUI.
		 * The GUI automatically applies an appropriate @ref camera,
		 * but clients must explicitly use a window-sized canvas.
		 */
		const shared_instance<render_layer_id>& render_layer() const;


		/**
		 * Manage the global keyboard and mouse focus.
		 *
		 * Focus semantics:
		 * - Keyboard focus and mouse focus (both potentially @NULL)
		 *		mark gui_element%s to consume any relevant user input
		 *		(e.g. user typing or button mouse-over effects).
		 * -  Applications can arbitrarily assign focus to gui_element%s
		 *		to guide user interactions. Independently, GUI elements 
		 *		can also acquire focus (e.g. because of user interaction)
		 *		to disable other elements that consume relevant input.
		 *
		 * Implementation notes:
		 * - GUI elements without focus may still consume hot input,
		 *		for example to acquire key focus after a mouse click.
		 * - An active focus does not keep the target gui_element alive.
		 *		Clients therefore do not have to explicitly clear focus
		 *		in order to destroy memory-intensive GUI elements.
		 */
		//@{
		void set_key_focus
			(const shared_pointer<gui_element>& key_focus) NOTHROW;
		void set_mouse_focus
			(const shared_pointer<gui_element>& mouse_focus) NOTHROW;

		void clear_key_focus() NOTHROW;
		void clear_mouse_focus() NOTHROW;

		weak_pointer<gui_element> key_focus() const NOTHROW;
		weak_pointer<gui_element> mouse_focus() const NOTHROW;
		//@}


		/**
		 * Apply a @bref{temporary_cursor_style} to the GUI cursor,
		 * or pass @NULL to revert to the default @bref{cursor_style}.
		 * The next gui_context::update() automatically restores
		 * the default @bref{cursor_style} prior to processing input.
		 * Both gui_element%s (e.g. process_input()) and GUI clients
		 * can override the cursor to provide interactive feedback.
		 */
		void set_temporary_cursor_style
			(const shared_pointer<gui_cursor_style>& temporary_cursor_style) NOTHROW;


		const string name;

		window_context& window;
		input_context& input;

		gui_modal_context& modal_context;

		const shared_instance<gui_cursor_style> cursor_style;


		/**
		 * Whether elements within this GUI should truncate
		 * coordinates to nearest integer pixel before rendering.
		 * Should be enabled in almost all cases to avoid
		 * aliasing-related artifacts.
		 */
		const bool pixel_integral_rendering;


	private:

		void update_layout();

		void handle_input(gui_output_event_list& output);
		void handle_tick_input
			(gui_output_event_list& output, bool event_hot);
		void handle_input_source_input
			(gui_output_event_list& output, bool event_hot,
			 bool shift_down, bool ctrl_down, bool alt_down);
		void handle_mouse_input
			(gui_output_event_list& output, bool event_hot,
			 bool shift_down, bool ctrl_down, bool alt_down);
		void handle_text_input
			(gui_output_event_list& output, bool event_hot,
			 bool shift_down, bool ctrl_down, bool alt_down);

		void commit_render_gui
			(render_stack_data& stack,
			 render_command_queue& queue);
		void commit_render_cursor
			(render_stack_data& stack,
			 render_command_queue& queue);


		static const float gui_size_min_;
		static const float gui_size_max_;

		static const float mouse_wheel_movement_per_line_;

		weak_pointer<gui_element> key_focus_;
		weak_pointer<gui_element> mouse_focus_;

		shared_pointer<gui_cursor_style> temporary_cursor_style_;
		shared_pointer<gui_cursor_style> render_cursor_style_;

		const shared_instance<gui_stack> modal_stack_;

		bool render_dirty_;

		const shared_instance<render_puppet_id> gui_puppet_;
		const shared_instance<render_model_id>  gui_model_;

		const shared_instance<render_puppet_id> cursor_puppet_;
		const shared_instance<render_model_id> cursor_model_;

		const shared_instance<render_stack_id> render_stack_;
		const shared_instance<render_scene_id> render_scene_;
		const shared_instance<render_layer_id> render_layer_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_HPP) */