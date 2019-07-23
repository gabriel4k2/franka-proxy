/**
 *************************************************************************
 *
 * @file gui_window_area.hpp
 *
 * GUI window area.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_WINDOW_AREA_HPP)
#define INCLUDED__VIRAL_GUI__GUI_WINDOW_AREA_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_window_area
 * @ingroup gui_module
 *
 * An area that holds several gui_window%s.
 *
 * Layout notes:
 * - See gui_window for window-specific layout guidelines.
 * - A window area automatically sorts its children gui_window%s.
 *		In particular, any gui_window that @bref{has_key_focus}
 *		or whose @bref{child_has_key_focus} becomes foreground.
 * - At any time, clients can acquire_key_focus() on a gui_window
 *		or contained gui_element%s to move the window to foreground.
 * - Window areas support a background GUI element behind all windows.
 *		This background element can for example contain gui_split%s
 *		or custom application-specific views. It occupies the same
 *		arrange_area() and clip_area() as the parent gui_window_area.
 *
 ************************************************************************/
class gui_window_area :
	public gui_element
{
	public:

		gui_window_area
			(const string& name, gui_context& gui);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Insert a not-@NULL @bref{window} into the window area.
		 *
		 * The @bref{window} does not automatically gain key_focus().
		 * Thus the window appears above other windows without key focus,
		 * but behind any window with key focus. If desired, applications
		 * therefore must explicitly move the @bref{window} to foreground
		 * after insertion (e.g. with acquire_key_focus()).
		 */
		void insert_window
			(const shared_instance<gui_window>& window);


		/**
		 * Remove a @bref{window} from the window area.
		 * Silently ignores non-contained windows.
		 */
		void remove_window
			(const shared_instance<gui_window>& window) NOTHROW;


		/** Remove all windows from the window area. */
		void clear_windows() NOTHROW;


		/** Manage background of the window area. */
		//@{
		void set_background
			(const shared_pointer<gui_element>& background);

		const shared_pointer<gui_element>& background() const NOTHROW;
		void clear_background() NOTHROW;
		//@}


	private:

		void realize_layout() override;

		void sort_windows() NOTHROW;


		typedef list<shared_instance<gui_window>> window_list;
		window_list windows_;

		shared_pointer<gui_element> background_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_WINDOW_AREA_HPP) */
