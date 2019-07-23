/**
 *************************************************************************
 *
 * @file gui_progress_bar.cpp
 *
 * GUI progress bars, implementation.
 *
 ************************************************************************/


#include "gui_progress_bar.hpp"


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>


#include "gui_event.hpp"
#include "gui_frame.hpp"
#include "gui_label.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_progress_bar
//
//////////////////////////////////////////////////////////////////////////


gui_progress_bar::gui_progress_bar
	(const string& name, gui_context& gui,
	 const shared_instance<gui_progress_bar_style>& progress_bar_style,
	 orientation_type orientation, bool auto_text)
	:
	gui_element(name, gui),
	progress_bar_style(progress_bar_style),
	orientation(orientation),
	auto_text(auto_text),

	progress_normalized_(0),

	background_
		(new gui_frame(name + "_background", gui,
					   progress_bar_style().background_style)),

	fill_(new gui_frame(name + "_fill", gui,
					    progress_bar_style().fill_style)),

	label_(new gui_label(name + "_label", gui,
						 progress_bar_style().label_style, ""))
{
	insert_child(background_);
	insert_child(fill_);
	insert_child(label_);

	if (auto_text)
		set_text_from_progress();
}


vector2f gui_progress_bar::preferred_size() const
{
	vector2f min_size
		(orientation == horizontal ? progress_bar_style().min_length : 0,
		 orientation == vertical ? progress_bar_style().min_length : 0);

	return
		min_size.max(background_().preferred_size())
				.max(fill_().preferred_size())
				.max(label_().preferred_size());
}


void gui_progress_bar::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	gui_output_event_list ignore_child_output;
	background_().process_input(input, ignore_child_output);
	fill_().process_input(input, ignore_child_output);
}


void gui_progress_bar::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	background_().commit_render(packer, queue);
	fill_().commit_render(packer, queue);
	label_().commit_render(packer, queue);
}


void gui_progress_bar::set_progress_normalized(float progress_normalized)
{
	progress_normalized_ =
		max(0.f, min(1.f, progress_normalized));

	if (auto_text)
		set_text_from_progress();

	want_layout();
}

float gui_progress_bar::progress_normalized() const NOTHROW
	{ return progress_normalized_; }


void gui_progress_bar::set_text(const string& text)
{
	text_ = text;
	want_layout();
}

const string& gui_progress_bar::text() const NOTHROW
	{ return text_; }


void gui_progress_bar::set_text_from_progress()
{
	int64 progress_percent =
		max(0, min(100,
			integer_operations::cast_or_clamp<int64>
				(progress_normalized_ * 100)));

	text_ = string(progress_percent) + "%";
	label_().set_text(text_);  

	want_layout();
}


void gui_progress_bar::realize_layout()
{
	if (layout_invalid())
	{
		background_().clip_entirely();
		fill_().clip_entirely();
		label_().clip_entirely();
		return;
	}


	// Background and label always use the same clip
	// and arrange areas as the entire progress bar.
	rect2f ar_area(arrange_area());
	ar_area.size =
		ar_area.size.max(preferred_size());

	background_().layout(ar_area, clip_area());
	label_().layout(ar_area, clip_area());


	// We build separate arrange and clip areas
	// for our fill_ frame to show current progress.
	// For low progress values, the fill_ frame
	// exceeds fill_arrange, and fill_clip must
	// cut off the excess parts.
	rect2f fill_arrange(ar_area);
	fill_arrange.size
		[orientation == horizontal ? 0 : 1] *=
			progress_normalized_;

	rect2f fill_clip;
	if (fill_arrange.try_intersect(clip_area(), fill_clip))
		fill_().layout(fill_arrange, fill_clip);
	else
		fill_().clip_entirely();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_progress_bar_style
//
//////////////////////////////////////////////////////////////////////////


gui_progress_bar_style::gui_progress_bar_style
	(const shared_instance<gui_frame_style>& background_style,
	 const shared_instance<gui_frame_style>& fill_style,
	 const shared_instance<gui_label_style>& label_style,
	 const float min_length)
	:
	background_style(background_style),
	fill_style(fill_style),
	label_style(label_style),
	min_length(min_length)
{
	if (min_length < 0)
	{
		LOG_ERROR("Progress bar minimum length must not be negative.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
