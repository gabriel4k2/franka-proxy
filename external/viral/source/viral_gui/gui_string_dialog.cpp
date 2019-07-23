/**
 *************************************************************************
 *
 * @file gui_string_dialog.hpp
 *
 * Modal string entry, implementation.
 *
 ************************************************************************/


#include "gui_string_dialog.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_grid.hpp"
#include "gui_frame.hpp"
#include "gui_label.hpp"
#include "gui_button.hpp"
#include "gui_editbox.hpp"
#include "gui_window.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_string_dialog
//
//////////////////////////////////////////////////////////////////////////


bool gui_string_dialog::show
	(gui_context& gui,
	 const shared_instance<gui_string_dialog_style>& string_dialog_style,
	 const string& caption,
	 const string& message,
	 bool allow_cancel,
	 string& result)
{
	gui_string_dialog string_dialog
		(gui, string_dialog_style,
		 caption, message, allow_cancel, result);

	string_dialog.execute();

	if (string_dialog.success_)
		result = string_dialog.result_;

	return string_dialog.success_;
}


gui_string_dialog::gui_string_dialog
	(gui_context& gui,
	 const shared_instance<gui_string_dialog_style>& string_dialog_style,
	 const string& caption,
	 const string& message,
	 bool allow_cancel,
	 const string& result_initial)
	:
	gui_modal_dialog(gui),
	string_dialog_style_(string_dialog_style),
	caption_(caption),
	message_(message),
	allow_cancel_(allow_cancel),
	result_(result_initial),

	window_(new gui_window
		("string_dialog_window", gui,
		 string_dialog_style().window_style,
		 caption, allow_cancel, gui_window::user_resize_enlarge)),

	background_(new gui_frame
		("string_dialog_background", gui,
		 string_dialog_style().background_style)),

	message_label_(new gui_label
		("string_dialog_message_label", gui,
		 string_dialog_style().label_style, message_)),

	editbox_(new gui_editbox
		("string_dialog_editbox", gui,
		 string_dialog_style().editbox_style,
		 false, true)),

	ok_button_(new gui_button
		("string_dialog_ok_button",  gui,
		 string_dialog_style().button_style,
		 gui_button::click_button)),
	cancel_button_(new gui_button
		("string_dialog_cancel_button",  gui,
		 string_dialog_style().button_style,
		 gui_button::click_button)),

	success_(false)
{
	editbox_().set_text(result_initial);
	ok_button_().set_text("OK");
	cancel_button_().set_text("Cancel");

	window_().set_content(create_vertical_grid());
	window_().set_client_size(string_dialog_style().initial_client_size);
	window_().center_window_delayed();
}


void gui_string_dialog::setup()
{
	gui_modal_dialog::setup();
	editbox_().acquire_key_focus();
}


shared_instance<gui_window> gui_string_dialog::dialog_window()
	{ return window_; }
shared_pointer<gui_element> gui_string_dialog::dialog_background()
	{ return background_; }


void gui_string_dialog::process_events
	(const gui_output_event_list& output)
{
	for (gui_output_event_list::const_iterator it
			(output.first()); it; ++it)
	{
		if (!*it) continue;

		const shared_instance<gui_element> &source =
			(*it)->source_untyped;

		if (typeid(**it) == typeid(gui_button_event))
		{
			if (source == ok_button_)
				{ accept(); return; }
			else if (source == cancel_button_)
				{ cancel(); return; }
		}

		if (typeid(**it) == typeid(gui_editbox_accept_event) &&
			source == editbox_)
		{
			accept();
			return;
		}

		if (typeid(**it) == typeid(gui_window_close_event) &&
			source == window_)
		{
			cancel();
			return;
		}
	}
}


void gui_string_dialog::accept()
{
	result_ = editbox_().text();
	success_ = true;
	finish();
}


void gui_string_dialog::cancel()
{
	// Safety precaution: Ignore if cancel is not allowed.
	// This should not happen, as the window close button
	// and the cancel button have been suppressed.
	if (!allow_cancel_) return;

	success_ = false;
	finish();
}


shared_instance<gui_grid> gui_string_dialog::create_vertical_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("string_dialog_vgrid", gui, vector2i(1, 3)));
	grid().set_column_weight(0, 1.f);
	grid().set_row_weight(0, 1.f);

	grid().insert_content(message_label_, vector2i(0, 0));
	grid().insert_content(editbox_, vector2i(0, 1));
	grid().insert_content(create_button_grid(), vector2i(0, 2));

	return grid;
}


shared_instance<gui_grid> gui_string_dialog::create_button_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("string_dialog_bgrid", gui, vector2i(3, 1)));
	grid().set_column_weight(0, 1.f);

	if (allow_cancel_)
	{
		grid().insert_content(ok_button_, vector2i(1, 0));
		grid().insert_content(cancel_button_, vector2i(2, 0));
	}
	else
	{
		grid().insert_content(ok_button_, vector2i(2, 0));
	}
	
	return grid;
}




//////////////////////////////////////////////////////////////////////////
//
// gui_string_dialog_style
//
//////////////////////////////////////////////////////////////////////////


gui_string_dialog_style::gui_string_dialog_style
	(const shared_instance<gui_label_style>& label_style,
	 const shared_instance<gui_button_style>& button_style,
	 const shared_instance<gui_editbox_style>& editbox_style,
	 const shared_instance<gui_window_style>& window_style,
	 const shared_instance<gui_frame_style>& background_style,
	 const vector2f& initial_client_size)
	:
	label_style(label_style),
	button_style(button_style),
	editbox_style(editbox_style),
	window_style(window_style),
	background_style(background_style),
	initial_client_size(initial_client_size)
{
	if (initial_client_size.min() <= 0)
	{
		LOG_ERROR("Initial client size must be positive.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
