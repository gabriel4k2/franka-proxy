/**
 *************************************************************************
 *
 * @file gui_knob.cpp
 *
 * GUI knob, implementation.
 *
 ************************************************************************/


#include "gui_knob.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_button.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_knob
//
//////////////////////////////////////////////////////////////////////////


gui_knob::gui_knob
	(const string& name, gui_context& gui,
	 const shared_instance<gui_knob_style>& knob_style,
	 orientation_type orientation)
	:
	gui_element(name, gui),
	knob_style(knob_style),
	orientation(orientation),
	knob_size_(1),
	position_normalized_(0),
	hot_area_valid_(false),
	drag_start_normalized_(0),
	dragging_(false),
	knob_button_(new gui_button(name + "_knob", gui,
								knob_style().button_style, gui_button::hold_button))
{
	insert_child(knob_button_);
}


vector2f gui_knob::preferred_size() const
{
	vector2f button_size(knob_button_().preferred_size());

	return vector2f
		(orientation == horizontal ?
			max(button_size.x, knob_size_) + knob_style().min_drag_range :
			button_size.x,
		 orientation == vertical ?
			max(button_size.y, knob_size_) + knob_style().min_drag_range :
			button_size.y);
}


void gui_knob::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	/**
	 * Fetch click and release events from knob,
	 * then move knob and notify client.
	 */

	if (layout_invalid() ||
		read_only() || !hot_area_valid_)
	{
		stop_dragging();
		return;
	}

	gui_output_event_list ignore_knob_button_events;
	knob_button_().process_input
		(input, ignore_knob_button_events);

	if (typeid(input) != typeid(gui_mouse_event))
		return;

	gui_mouse_event& mouse_event =
		static_cast<gui_mouse_event&>(input);

	if (dragging_)
	{
		update_dragging
			(mouse_event.cursor, output);

		if (!knob_button_().down())
			stop_dragging();
	}
	else if (knob_button_().down())
		start_dragging(mouse_event.cursor);
}


void gui_knob::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;
	knob_button_().commit_render(packer, queue);	
}


void gui_knob::set_read_only(bool read_only) NOTHROW
	{ knob_button_().set_read_only(read_only); }
bool gui_knob::read_only() const NOTHROW
	{ return knob_button_().read_only(); }


void gui_knob::set_knob_size(float knob_size)
{
	knob_size_ = knob_size;
	want_layout();
}

float gui_knob::knob_size() const
	{ return knob_size_; }

float gui_knob::minimum_knob_size() const
{
	return orientation == horizontal ?
				knob_button_().preferred_size().x :
				knob_button_().preferred_size().y;
}


void gui_knob::set_position_normalized(float position_normalized)
{
	position_normalized_ = max(0.f, min(1.f, position_normalized));
	want_layout();
}

float gui_knob::position_normalized() const
	{ return position_normalized_; }


void gui_knob::realize_layout()
{
	if (layout_invalid())
	{
		hot_area_valid_ = false;
		knob_button_().clip_entirely(); 
		return;
	}


	// Build hot area - i.e. the clipped
	// arrangement area in which the knob
	// responds to clicks and drags.
	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());

	hot_area_valid_ =
		ar_area.try_intersect(cl_area, hot_area_clipped_);


	// Clamp knob size to minimum button size
	// to avoid button clipping at range limits.
	vector2f pref_button_size
		(knob_button_().preferred_size());

	float clamped_knob_size =
		max(knob_size_, 
			orientation == horizontal ?
				pref_button_size.x : pref_button_size.y);
			

	// Knob button moves into the orientation direction,
	// and occupies any layout space in the other direction.
	float button_range =
		max(knob_style().min_drag_range,
			orientation == horizontal ?
			ar_area.size.x - clamped_knob_size :
			ar_area.size.y - clamped_knob_size);

	float button_offset =
		position_normalized_ * button_range;

	rect2f button_area
		(orientation == horizontal ?
			rect2f(ar_area.pos + vector2f(button_offset, 0),
				   vector2f(clamped_knob_size, ar_area.size.y)) :
			rect2f(ar_area.pos + vector2f(0, button_offset),
				   vector2f(ar_area.size.x, clamped_knob_size)));

	knob_button_().layout
		(button_area, hot_area_clipped_);	
}


void gui_knob::start_dragging(const vector2f& drag_start)
{
	dragging_ = true;
	drag_start_ = drag_start;
	drag_start_normalized_ = position_normalized_;
}

void gui_knob::stop_dragging()
	{ dragging_ = false; }


void gui_knob::update_dragging
	(const vector2f& drag_now,
	 gui_output_event_list& output)
{
	/**
	 * Determine new position for knob.
	 *
	 * Consider three effects in hot area:
	 * - Smooth (pixelwise) dragging over range.
	 * - Clamp to maximum outside the range.
	 * - Reset to start position outside tolerance,
	 *		based on the (potentially) clipped area.
	 *
	 * Note both arrange_area() and hot_area_clipped_
	 * are valid in here, see process_input().
	 */
	vector2f tolerance
		(knob_style().drag_tolerance,
		 knob_style().drag_tolerance);
	rect2f tolerance_area
		(hot_area_clipped_.pos - tolerance,
		 hot_area_clipped_.size + tolerance * 2);

	float drag_offset = 0;

	if (tolerance_area.contains(drag_now))
	{
		// Inside tolerance, so determine unclamped movement.
		//
		// Notes:
		// - We must convert between UI dragging and normalized
		//		button position. This is only possible if there
		//		is reasonable (non-zero) drag range available.
		// - Buttons may exceed an unsupported, small knob_size_.
		//		In this case, decrease button range accordingly.
		float button_range =
			max(knob_style().min_drag_range,
				orientation == horizontal ?
				arrange_area().size.x -
					max(knob_size_, knob_button_().preferred_size().x) :
				arrange_area().size.y -
					max(knob_size_, knob_button_().preferred_size().y));

		if (button_range > drag_range_epsilon_)
			drag_offset =
				orientation == horizontal ?
					(drag_now.x - drag_start_.x) / button_range :
					(drag_now.y - drag_start_.y) / button_range;
	}


	// Set and clamp new button position,
	// then request re-layout to apply.
	position_normalized_ =
		max(0.f, min(1.f, drag_start_normalized_ + drag_offset));

	want_layout();


	// Notify client about dragging.
	auto_pointer<gui_output_event> event
		(new gui_knob_event(*this));
	output.insert
		(std::move(event));
}


const float gui_knob::drag_range_epsilon_ = 0.001f;




//////////////////////////////////////////////////////////////////////////
//
// gui_knob_style
//
//////////////////////////////////////////////////////////////////////////


gui_knob_style::gui_knob_style
	(const shared_instance<gui_button_style>& button_style,
	 float min_drag_range,
	 float drag_tolerance)
	:
	button_style(button_style),
	min_drag_range(min_drag_range),
	drag_tolerance(drag_tolerance)
{
	if (min_drag_range < 0)
	{
		LOG_ERROR("Minimum drag range must not be negative.");
		throw invalid_parameter();
	}

	if (drag_tolerance < 0)
	{
		LOG_ERROR("Cannot create knob style from negative drag tolerance.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */