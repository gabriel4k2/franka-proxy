/**
 *************************************************************************
 *
 * @file gui_editbox.hpp
 *
 * Edit elements with frames and scrolling.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_EDITBOX_HPP)
#define INCLUDED__VIRAL_GUI__GUI_EDITBOX_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_editbox
 * @ingroup gui_module
 *
 * Bundles a gui_edit with a gui_scroll_area and a gui_frame.
 *
 * Layout notes:
 * - An editbox clips on an arrange_area() smaller than preferred_size(),
 *		and fills any excess arrange space with frames and edit().
 * - The @bref{editbox_style.scroll_area_style} should usually
 *		@bref{prefer_override_size}. In this case, the editbox
 *		prefers the override size of the scroll area plus frames,
 *		and shows scrollbars for any excess text within edit().
 * - If the scroll area does not @bref{prefer_override_size},
 *		the editbox prefers size for all text lines instead.
 *		The editbox then clips, but never shows any scrollbars.
 *
 ************************************************************************/
class gui_editbox :
	public gui_element
{

	public:

		gui_editbox
			(const string& name, gui_context& gui,
			 const shared_instance<gui_editbox_style>& editbox_style,
			 bool multiline, bool notify_accept);


		vector2f preferred_size() const override;


		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		bool try_accept_key_focus() NOTHROW override;


		/** Forward calls to the internal edit(). */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;

		void set_text(const string& text);
		void set_text(const string& text, bool allow_undo);
		string text() const;

		void append_text(const string& text);
		void append_text(const string& text, bool allow_undo);

		void set_history_step_limit(int64 history_step_limit) NOTHROW;
		int64 history_step_limit() const NOTHROW;
		void set_history_memory_limit(int64 history_memory_limit) NOTHROW;
		int64 history_memory_limit() const NOTHROW;

		float known_line_width() const NOTHROW;
		void reset_known_line_width() NOTHROW;
		//@}


		/** Access the internal gui_edit. */
		//@{
		gui_edit& edit() NOTHROW;
		const gui_edit& edit() const NOTHROW;
		//@}


		const shared_instance<gui_editbox_style> editbox_style;


	private:

		void realize_layout() override;


		const shared_instance<gui_frame> frame_;
		const shared_instance<gui_scroll_area> scroll_area_;
		const shared_instance<gui_edit> edit_;
};


/**
 * See gui_edit_event, gui_edit_accept_event, and gui_edit_cancel_event.
 * @ingroup gui_module
 */
//@{
typedef gui_simple_output_event<gui_editbox, 0> gui_editbox_event;
typedef gui_simple_output_event<gui_editbox, 1> gui_editbox_accept_event;
typedef gui_simple_output_event<gui_editbox, 2> gui_editbox_cancel_event;
//@}




/**
 *************************************************************************
 *
 * @class gui_editbox_style
 * @ingroup gui_module
 *
 * Styling for a gui_editbox.
 *
 ************************************************************************/
class gui_editbox_style :
	public gui_style
{
	public:

		gui_editbox_style
			(const shared_instance<gui_frame_style>& frame_style,
			 const shared_instance<gui_scroll_area_style>& scroll_area_style,
			 const shared_instance<gui_edit_style>& edit_style);

		const shared_instance<gui_frame_style> frame_style;
		const shared_instance<gui_scroll_area_style> scroll_area_style;
		const shared_instance<gui_edit_style> edit_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_EDITBOX_HPP) */