/**
 *************************************************************************
 *
 * @file gui_progress_box.hpp
 *
 * GUI progress box.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_PROGRESS_BOX_HPP)
#define INCLUDED__VIRAL_GUI__GUI_PROGRESS_BOX_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/timer_forward.hpp>


#include <viral_core/auto_pointer.hpp>
#include <viral_core/thread.hpp>
#include <viral_core/thread_atomic.hpp>


#include "gui.hpp"
#include "gui_modal.hpp"
#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_progress_box
 * @ingroup gui_module
 *
 * Prohibits GUI interactions and shows a progress indicator
 * during a threaded, long-term background operation.
 *
 * Usage notes:
 * - Create a gui_progress_box.
 * - Start a background thread with the background operation
 *		and pass on a reference to the gui_progress_box.
 * - Call gui_progress_box::execute() in the original thread.
 *		The box shows up and the GUI updates automatically.
 * - Call gui_progress_box::close() in the background thread
 *		to return the original thread from execute().
 * - Clients must reset() the gui_progress_box before reuse
 *		(i.e. after joining with the background thread).
 * - See gui_progress_box_thread for a convenience wrapper.
 *
 * Exception safety:
 * - The background thread must call gui_progress_box::close()
 *		even in the case of an exception to avoid a frozen GUI.
 *
 * Thread safety:
 * - @attent The GUI is not threadsafe. The background operation
 *		therefore must not access the gui_context or gui_elements
 *		while a gui_progress_box is execute()ing.
 * - Most calls on gui_progress_box are not threadsafe (e.g. finish()).
 *		Any background threads may only use set_progress() and close()
 *		to control a concurrently execute()ing gui_progress_box.
 *
 ************************************************************************/
class gui_progress_box :
	public gui_modal_dialog
{

	public:

		enum indicator_mode
		{
			/** Show a gui_progress_bar to track progress. */
			progress_indicator,


			/**
			 * Show a gui_knob indicator without explicit progress,
			 * usually when there is no estimate on remaining time.
			 */
			busy_indicator
		};


		gui_progress_box
			(gui_context& gui,
			 const shared_instance<gui_progress_box_style>& progress_box_style,
			 const string& text,
			 const string& caption,
			 indicator_mode mode);


		/**
		 * Set current progress for @bref{progress_indicator}.
		 *
		 * Usage notes:
		 * - Ignored if not in @bref{progress_indicator} mode.
		 * - This call is threadsafe for arbitrary threads.
		 *
		 * Progress values:
		 * - All @bref{progress} values are in [0, 1] range.
		 * - Invalid values silently clamp to valid range.
		 * - Setting the current progress to a value of 1
		 *		does not automatically close() the dialog.
		 */
		void set_progress(float progress) NOTHROW;


		/**
		 * Close the progress box and exit from execute(),
		 * such as when the background operation has finished.
		 * Threadsafe for arbitrary threads.
		 */
		void close() NOTHROW;


		/** Reset progress and close() flag. */
		void reset() NOTHROW;


		const shared_instance<gui_progress_box_style> progress_box_style;

		const string text;
		const string caption;
		const indicator_mode mode;


	private:

		void setup() override;
		void update() override;

		shared_instance<gui_window> dialog_window() override;
		shared_pointer<gui_element> dialog_background() override;

		void process_events
			(const gui_output_event_list& output) override;

		shared_instance<gui_grid> create_content_grid() const;


		const shared_instance<gui_progress_bar> progress_indicator_;
		const shared_instance<gui_knob> busy_indicator_;

		const shared_instance<gui_window> window_;
		const shared_instance<gui_frame> background_;

		const auto_pointer<free_timer> busy_timer_;

		atomic<float> progress_;
		atomic<bool> close_;
};




/**
 *************************************************************************
 *
 * @class gui_progress_box_thread
 * @ingroup gui_module
 *
 * Allows a client thread to execute a long-term operation
 * while showing a gui_progress_box in a separate thread.
 *
 * Usage notes:
 * - Unlike a standalone gui_progress_box, a gui_progress_box_thread
 *		calls gui_modal_dialog::execute() in a separate, new thread,
 *		which allows the creating client thread to run its operation.
 * - The separate, new thread automatically starts on construction,
 *		shows the progress box, and perpetually handles GUI logics.
 * - Clients must destroy the gui_progress_box_thread (e.g. by scope)
 *		to close the progress box. This procedure synchs program flow
 *		to thread termination and thus prohibits races for GUI logics.
 * - @attent Since the GUI is not threadsafe, client threads must not
 *		access the GUI at all (e.g. modify contents of gui_element%s)
 *		while a gui_progress_box_thread is running.
 *
 ************************************************************************/
class gui_progress_box_thread :
	private threaded_task
{

	public:

		gui_progress_box_thread
			(gui_context& gui,
			 const shared_instance<gui_progress_box_style>& progress_box_style,
			 const string& text,
			 const string& caption,
			 gui_progress_box::indicator_mode mode);


		~gui_progress_box_thread() NOTHROW;


		/**
		 * See gui_progress_box::set_progress().
		 * Threadsafe for arbitrary threads.
		 */
		void set_progress(float progress) NOTHROW;


	private:

		void task_main() override;


		gui_progress_box progress_box_;
};




/**
 *************************************************************************
 *
 * @class gui_progress_box_style
 * @ingroup gui_module
 *
 * Overall styling for gui_progress_box%es.
 *
 ************************************************************************/
class gui_progress_box_style :
	public gui_style
{
	public:

		gui_progress_box_style
			(const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_progress_bar_style>& progress_indicator_style,
			 const shared_instance<gui_knob_style>& busy_indicator_style,
			 const shared_instance<gui_image_style>& icon_image_style,
			 const shared_instance<render_material_id>& icon_material,
			 const vector2i& icon_pixel_size,
			 const shared_instance<gui_window_style>& window_style,
			 const shared_instance<gui_frame_style>& background_style,
			 float busy_indicator_frequency);

		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_progress_bar_style> progress_indicator_style;
		const shared_instance<gui_knob_style> busy_indicator_style;

		const shared_instance<gui_image_style> icon_image_style;
		const shared_instance<render_material_id> icon_material;
		const vector2i icon_pixel_size;

		const shared_instance<gui_window_style> window_style;
		const shared_instance<gui_frame_style> background_style;

		float busy_indicator_frequency;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_PROGRESS_BOX_HPP) */
