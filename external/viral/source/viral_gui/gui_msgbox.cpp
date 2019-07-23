/**
 *************************************************************************
 *
 * @file gui_msgbox.cpp
 *
 * GUI message boxes, implementation.
 *
 ************************************************************************/


#include "gui_msgbox.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_grid.hpp"
#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_frame.hpp"
#include "gui_button.hpp"
#include "gui_window.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_msgbox
//
//////////////////////////////////////////////////////////////////////////


gui_msgbox::result_type gui_msgbox::show
	(gui_context& gui,
	 const shared_instance<gui_msgbox_style>& msgbox_style,
	 const string& text, const string& caption,
	 icon_type icon, choice_type choice, default_type default)
{
	gui_msgbox msgbox
		(gui, msgbox_style,
		 text, caption,
		 icon, choice, default);

	msgbox.execute();

	if (msgbox.result_ == result_unknown)
	{
		LOG_ERROR("Message box returned unknown result.");
		throw assert_failed();
	}

	return msgbox.result_;
}


gui_msgbox::gui_msgbox
	(gui_context& gui,
	 const shared_instance<gui_msgbox_style>& msgbox_style,
	 const string& text, const string& caption,
	 icon_type icon, choice_type choice, default_type default)
	:
	gui_modal_dialog(gui),
	msgbox_style_(msgbox_style),
	text_(text), caption_(caption),
	icon_(icon), choice_(choice), default_(default),

	ok_button_
		(new gui_button("msgbox_ok", gui, msgbox_style_().button_style,
						gui_button::click_button)),
	cancel_button_
		(new gui_button("msgbox_cancel", gui, msgbox_style_().button_style,
						gui_button::click_button)),
	yes_button_
		(new gui_button("msgbox_yes", gui, msgbox_style_().button_style,
						 gui_button::click_button)),
	no_button_
		(new gui_button("msgbox_no", gui, msgbox_style_().button_style,
						gui_button::click_button)),

	window_
		(new gui_window("msgbox_window", gui, msgbox_style_().window_style,
						caption_, true, gui_window::match_content_size)),

	background_
		(new gui_frame("msgbox_background", gui,
					   msgbox_style_().background_style)),

	result_(result_unknown)
{
	ok_button_().set_text("OK");
	cancel_button_().set_text("Cancel");
	yes_button_().set_text("Yes");
	no_button_().set_text("No");

	window_().set_content(create_vertical_grid());
	window_().resize_to_preferred_content();
	window_().center_window_delayed();
}


void gui_msgbox::setup()
{
	gui_modal_dialog::setup();
	

	// Determine which buttons are available
	// for the current choice type.
	bool has_ok_button =
		choice_ == choice_ok ||
		choice_ == choice_ok_cancel;
	bool has_cancel_button =
		choice_ == choice_ok_cancel ||
		choice_ == choice_cancel ||
		choice_ == choice_yes_no_cancel;
	bool has_yes_button =
		choice_ == choice_yes_no_cancel;
	bool has_no_button =
		choice_ == choice_yes_no_cancel;


	// Only apply default focus to available buttons.
	if (has_ok_button && default_ == default_ok)
		ok_button_().acquire_key_focus();
	else if (has_cancel_button && default_ == default_cancel)
		cancel_button_().acquire_key_focus();
	else if (has_yes_button && default_ == default_yes)	
		yes_button_().acquire_key_focus();
	else if (has_no_button && default_ == default_no)
		no_button_().acquire_key_focus();
	else if (default_ != default_none)
	{
		LOG_WARN("Invalid default button, ignoring focus.");
	}
}


shared_instance<gui_window> gui_msgbox::dialog_window()
	{ return window_; }
shared_pointer<gui_element> gui_msgbox::dialog_background()
	{ return background_; }


void gui_msgbox::process_events
	(const gui_output_event_list& output)
{
	/**
	 * Wait until the user clicks one of the buttons
	 * within the message box, then store result
	 * and finish() modal interaction.
	 */

	for (gui_output_event_list::const_iterator it
			(output.first()); it; ++it)
	{
		if (!*it) continue;

		const shared_instance<gui_element> &source =
			(*it)->source_untyped;

		if (typeid(**it) == typeid(gui_button_event))
		{
			if (source == ok_button_)
				result_ = result_ok;
			else if (source == cancel_button_)
				result_ = result_cancel;
			else if (source == yes_button_)
				result_ = result_yes;
			else if (source == no_button_)
				result_ = result_no;
		}

		if (typeid(**it) == typeid(gui_window_close_event) &&
			source == window_)
		{
			result_ = result_cancel;
		}

		if (result_ != result_unknown)
		{
			finish();
			return; // Ignore any further events.
		}
	}
}


shared_instance<gui_grid> gui_msgbox::create_vertical_grid()
{
	/**
	 * Create and return vertical grid that stacks
	 * message and icon over the message box buttons.
	 */

	shared_instance<gui_grid> vertical_grid
		(new gui_grid("msgbox_vgrid", gui, vector2i(1, 2)));
	vertical_grid().set_row_weight(0, 1);
	vertical_grid().set_row_weight(1, 0);
	vertical_grid().set_column_weight(0, 1);
	vertical_grid().insert_content(create_icon_text_grid(), vector2i(0, 0));
	vertical_grid().insert_content(create_button_grid(), vector2i(0, 1));

	return vertical_grid;
}


shared_instance<gui_grid> gui_msgbox::create_icon_text_grid()
{
	/** Create and return grid with icon and text. */

	// Create grid.
	shared_instance<gui_grid> icon_text_grid
		(new gui_grid("msgbox_itgrid", gui, vector2i(2, 1)));
	icon_text_grid().set_row_weight(0, 1);
	icon_text_grid().set_column_weight(0, 0);
	icon_text_grid().set_column_weight(1, 1);


	// Create label element with message box text.
	shared_instance<gui_label> text_label
		(new gui_label("msgbox_label", gui,
					   msgbox_style_().label_style, text_)); 

	icon_text_grid().insert_content
		(text_label, vector2i(1, 0));


	// Choose appropriate icon material.
	shared_pointer<render_material_id> icon_material;

	if (icon_ == icon_question)
		icon_material = msgbox_style_().icon_question;
	else if (icon_ == icon_info)
		icon_material = msgbox_style_().icon_info;
	else if (icon_ == icon_warn)
		icon_material = msgbox_style_().icon_warn;
	else if (icon_ == icon_error)
		icon_material = msgbox_style_().icon_error;
	else if (icon_ != no_icon)
	{
		LOG_ERROR("Unknown icon type for message box.");
		throw assert_failed();
	}


	// Create image element with message box icon.
	if (icon_material)
	{
		shared_instance<gui_image> icon_image
			(new gui_image("mesgbox_image", gui,
						   msgbox_style_().icon_image_style,
						   icon_material.instance_or_throw(),
						   msgbox_style_().icon_pixel_size));

		icon_text_grid().insert_content(icon_image, vector2i(0, 0));
	}


	return icon_text_grid;
}


shared_instance<gui_grid> gui_msgbox::create_button_grid()
{
	/** Create and return grid with message box buttons. */

	// Create grid.
	shared_instance<gui_grid> button_grid
		(new gui_grid("msgbox_bgrid", gui, vector2i(4, 1)));
	button_grid().set_row_weight(0, 0);
	button_grid().set_column_weight(0, 1);
	button_grid().set_column_weight(1, 0);
	button_grid().set_column_weight(2, 0);
	button_grid().set_column_weight(3, 0);


	// Insert buttons into matching cells.
	// Note the OK button changes its position
	// if cancel is present as well.
	if (choice_ == choice_ok ||
		choice_ == choice_ok_cancel)
			button_grid().insert_content
				(ok_button_, vector2i(choice_ == choice_ok ? 3 : 2, 0));

	if (choice_ == choice_ok_cancel ||
		choice_ == choice_cancel ||
		choice_ == choice_yes_no_cancel)
			button_grid().insert_content
				(cancel_button_, vector2i(3, 0));

	if (choice_ == choice_yes_no_cancel)
	{
		button_grid().insert_content(yes_button_, vector2i(1, 0));
		button_grid().insert_content(no_button_, vector2i(2, 0));
	}

	return button_grid;
}




//////////////////////////////////////////////////////////////////////////
//
// gui_msgbox_style
//
//////////////////////////////////////////////////////////////////////////


gui_msgbox_style::gui_msgbox_style
	(const shared_instance<gui_label_style>& label_style,
	 const shared_instance<gui_button_style>& button_style,
	 const shared_instance<gui_window_style>& window_style,

	 const shared_instance<gui_image_style>& icon_image_style,
	 const shared_instance<render_material_id>& icon_question,
	 const shared_instance<render_material_id>& icon_info,
	 const shared_instance<render_material_id>& icon_warn,
	 const shared_instance<render_material_id>& icon_error,
	 const vector2i& icon_pixel_size,

	 const shared_instance<gui_frame_style>& background_style)
	:
	label_style(label_style),
	button_style(button_style),
	window_style(window_style),

	icon_image_style(icon_image_style),
	icon_question(icon_question),
	icon_info(icon_info),
	icon_warn(icon_warn),
	icon_error(icon_error),
	icon_pixel_size(icon_pixel_size),

	background_style(background_style)
{
	if (icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("Message box icon size must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
