/**
 *************************************************************************
 *
 * @file gui_progress_bar.hpp
 *
 * GUI progress bars.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_PROGRESS_BAR_HPP)
#define INCLUDED__VIRAL_GUI__GUI_PROGRESS_BAR_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_progress_bar
 * @ingroup gui_module
 *
 * A bar that shows the progress of some operation.
 *
 ************************************************************************/
class gui_progress_bar :
	public gui_element
{

	public:

		gui_progress_bar
			(const string& name, gui_context& gui,
			 const shared_instance<gui_progress_bar_style>& progress_bar_style,
			 orientation_type orientation, bool auto_text);

		
		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Current progress in [0,1] range.
		 * Invalid values silently clamp to valid range.
		 */
		//@{
		void set_progress_normalized(float progress_normalized);
		float progress_normalized() const NOTHROW;
		//@}


		/** Text shown on progress bar. */
		//@{
		void set_text(const string& text);
		const string& text() const NOTHROW;
		//@}


		/**
		 * Explicitly synch text to progress_normalized().
		 * Useful if @bref{auto_text} is disabled.
		 */
		void set_text_from_progress();


		const shared_instance<gui_progress_bar_style> progress_bar_style;
		const orientation_type orientation;


		/**
		 * If @true, automatically update progress bar text
		 * whenever the current progress_normalized() changes.
		 */
		const bool auto_text;


	private:

		void realize_layout() override;


		float progress_normalized_;
		string text_;

		const shared_instance<gui_frame> background_;
		const shared_instance<gui_frame> fill_;
		const shared_instance<gui_label> label_;
};




/**
 *************************************************************************
 *
 * @class gui_progress_bar_style
 * @ingroup gui_module
 *
 * Styling for a gui_progress_bar.
 *
 ************************************************************************/
class gui_progress_bar_style :
	public gui_style
{

	public:

		gui_progress_bar_style
			(const shared_instance<gui_frame_style>& background_style,
			 const shared_instance<gui_frame_style>& fill_style,
			 const shared_instance<gui_label_style>& label_style,
			 const float min_length);

		const shared_instance<gui_frame_style> background_style;
		const shared_instance<gui_frame_style> fill_style;
		const shared_instance<gui_label_style> label_style;
		const float min_length;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_PROGRESS_BAR_HPP) */
