/**
 *************************************************************************
 *
 * @file gui_progress_box.cpp
 *
 * GUI progress box, implementation.
 *
 ************************************************************************/


#include "gui_progress_box.hpp"


#include <math.h>


#include <viral_core/log.hpp>
#include <viral_core/timer.hpp>
#include <viral_core/geo_util.hpp>


#include "gui_grid.hpp"
#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_frame.hpp"
#include "gui_knob.hpp"
#include "gui_progress_bar.hpp"
#include "gui_window.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_progress_box
//
//////////////////////////////////////////////////////////////////////////


gui_progress_box::gui_progress_box
	(gui_context& gui,
	 const shared_instance<gui_progress_box_style>& progress_box_style,
	 const string& text,
	 const string& caption,
	 indicator_mode mode)
	:
	gui_modal_dialog(gui),

	progress_box_style(progress_box_style),
	text(text),
	caption(caption),
	mode(mode),

	progress_indicator_
		(new gui_progress_bar
			("progress_box_progress_indicator", gui,
			 progress_box_style().progress_indicator_style,
			 gui_element::horizontal, false)),
	busy_indicator_
		(new gui_knob
			("progress_box_busy_indicator", gui,
			 progress_box_style().busy_indicator_style,
			 gui_element::horizontal)),
			
	window_
		(new gui_window
			("progress_box_window", gui,
			 progress_box_style().window_style,
			 caption, false, gui_window::match_content_size)),
	background_
		(new gui_frame
			("progress_box_background", gui,
			 progress_box_style().background_style)),

	busy_timer_(new free_timer()),

	progress_(0),
	close_(false)
{
	busy_indicator_().set_knob_size(0);
	busy_indicator_().set_read_only(true);

	window_().set_content(create_content_grid());
	window_().resize_to_preferred_content();
	window_().center_window_delayed();
}


void gui_progress_box::set_progress(float value) NOTHROW
{
	// We cannot directly update the progress bar,
	// because the progress bar is not threadsafe.
	progress_ = value;
}


void gui_progress_box::close() NOTHROW
{
	// We cannot directly call finish(),
	// as finish() is not threadsafe.
	close_ = true;
}


void gui_progress_box::reset() NOTHROW
{
	// Clients must reset synchronized members explicitly.
	// Implicit resetting in setup() would cause a data race
	// with concurrent set_progress() or close() invocations
	// of the background thread (e.g. fast background thread
	// calls close(), but slow setup() clears the close flag
	// through the original thread again).

	progress_ = 0.f;
	close_ = false;
}


void gui_progress_box::setup()
{
	gui_modal_dialog::setup();

	progress_indicator_().set_progress_normalized(0.f);
	busy_indicator_().set_position_normalized(0.5f);
}


void gui_progress_box::update()
{
	gui_modal_dialog::update();


	// Regular updates to GUI elements in the progress box:
	// Synchronize progress to changes from other threads,
	// and animate the busy indicator.
	if (mode == progress_indicator)
	{
		progress_indicator_().set_progress_normalized
			(progress_.value());
	}

	if (mode == busy_indicator)
	{
		float busy_offset =
			busy_timer_->seconds_passed() / 
			progress_box_style().busy_indicator_frequency;

		if (busy_offset > 1)
		{
			busy_offset = 1;
			busy_timer_->restart();
		}

		busy_indicator_().set_position_normalized
			(0.5f + 0.5f * sinf(busy_offset * geo_constants::double_pi));
	}


	// Return from execute() in original thread
	// if a background thread called close().
	if (close_.value())
		finish();
}


shared_instance<gui_window> gui_progress_box::dialog_window()
	{ return window_; }
shared_pointer<gui_element> gui_progress_box::dialog_background()
	{ return background_; }


void gui_progress_box::process_events
	(const gui_output_event_list& output)
{
	// The progress box has no elements
	// that the user can interact with,
	// thus no events to process here.
}


shared_instance<gui_grid> gui_progress_box::create_content_grid() const
{
	// Create main layout grid.
	shared_instance<gui_grid> grid
		(new gui_grid("progress_box_itgrid", gui, vector2i(2, 2)));
	grid().set_row_weight(0, 1);
	grid().set_row_weight(1, 0);
	grid().set_column_weight(0, 0);
	grid().set_column_weight(1, 1);
	grid().set_stretch_left(vector2i(1, 1), 1);


	// Create and insert progress box icon.
	shared_instance<gui_image> icon_image
		(new gui_image
			("mesgbox_image", gui,
			 progress_box_style().icon_image_style,
			 progress_box_style().icon_material,
			 progress_box_style().icon_pixel_size));

	grid().insert_content
		(icon_image, vector2i(0, 0));


	// Create and insert progress box label.
	shared_instance<gui_label> text_label
		(new gui_label("progress_box_label", gui,
		               progress_box_style().label_style, text));

	grid().insert_content
		(text_label, vector2i(1, 0));


	// Insert progress or busy indicator.
	if (mode == progress_indicator)
	{
		grid().insert_content
			(progress_indicator_, vector2i(1, 1));
	}
	else if (mode == busy_indicator)
	{
		busy_indicator_().set_position_normalized(0.5f);
		grid().insert_content
			(busy_indicator_, vector2i(1, 1));
	}
	else
	{
		LOG_ERROR("Unknown indicator mode.");
		throw invalid_parameter();
	}

	return grid;
}




//////////////////////////////////////////////////////////////////////////
//
// gui_progress_box_thread
//
//////////////////////////////////////////////////////////////////////////


gui_progress_box_thread::gui_progress_box_thread
	(gui_context& gui,
	 const shared_instance<gui_progress_box_style>& progress_box_style,
	 const string& text,
	 const string& caption,
	 gui_progress_box::indicator_mode mode)
	:
	progress_box_
		(gui, progress_box_style, text, caption, mode)
{
	start();
}


gui_progress_box_thread::~gui_progress_box_thread() NOTHROW
{
	// Allow task_main() to return from gui_progress_box::execute().
	// Use threadsafe close() instead of finish().
	progress_box_.close();

	try { join(); }
	catch (...)
	{
		LOG_CRITICAL
			("Internal thread has thrown an exception on joining.");
	}
}


void gui_progress_box_thread::set_progress(float progress) NOTHROW
{
	// No mutex:
	// gui_progress_box::set_progress() is threadsafe.

	progress_box_.set_progress(progress);
}


void gui_progress_box_thread::task_main()
	{ progress_box_.execute(); }




//////////////////////////////////////////////////////////////////////////
//
// gui_progress_box_style
//
//////////////////////////////////////////////////////////////////////////


gui_progress_box_style::gui_progress_box_style
	(const shared_instance<gui_label_style>& label_style,
	 const shared_instance<gui_progress_bar_style>& progress_indicator_style,
	 const shared_instance<gui_knob_style>& busy_indicator_style,
	 const shared_instance<gui_image_style>& icon_image_style,
	 const shared_instance<render_material_id>& icon_material,
	 const vector2i& icon_pixel_size,
	 const shared_instance<gui_window_style>& window_style,
	 const shared_instance<gui_frame_style>& background_style,
	 float busy_indicator_frequency)
	:
	label_style(label_style),
	progress_indicator_style(progress_indicator_style),
	busy_indicator_style(busy_indicator_style),
	icon_image_style(icon_image_style),
	icon_material(icon_material),
	icon_pixel_size(icon_pixel_size),
	window_style(window_style),
	background_style(background_style),
	busy_indicator_frequency(busy_indicator_frequency)
{
	if (icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("Progress box icon size must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
