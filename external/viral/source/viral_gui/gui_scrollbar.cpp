/**
 *************************************************************************
 *
 * @file gui_scrollbar.cpp
 *
* GUI scrollbars, implementation.
 *
 ************************************************************************/


#include "gui_scrollbar.hpp"


#include <math.h>


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_frame.hpp"
#include "gui_button.hpp"
#include "gui_knob.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_scrollbar
//
//////////////////////////////////////////////////////////////////////////


gui_scrollbar::gui_scrollbar
	(const string& name, gui_context& gui,
	 const shared_instance<gui_scrollbar_style>& scrollbar_style,
	 orientation_type orientation)
	:
	gui_element(name, gui),

	scrollbar_style(scrollbar_style),
	orientation(orientation),

	range_min_(0), range_max_(100),
	page_size_(10), item_size_(5), overlap_factor_(0.9f),
	continuous_increment_(1.f),
	position_(0),
	hot_area_band_valid_(false),

	knob_(new gui_knob(name + "_knob", gui, scrollbar_style().knob_style, orientation)),

	increment_(new gui_button(name + "_inc", gui,
							  scrollbar_style().button_style, gui_button::hold_button)),
	decrement_(new gui_button(name + "_dec", gui,
							  scrollbar_style().button_style, gui_button::hold_button)),
	band_(new gui_frame(name + "_band", gui,
						scrollbar_style().band_style))
{
	insert_child(knob_);
	insert_child(increment_);
	insert_child(decrement_);
	insert_child(band_);

	if (orientation == horizontal)
	{
		increment_().set_graphics
			(scrollbar_style().icon_pixel_size, scrollbar_style().right_icon);
		decrement_().set_graphics
			(scrollbar_style().icon_pixel_size, scrollbar_style().left_icon);
	}
	else if (orientation == vertical)
	{
		increment_().set_graphics
			(scrollbar_style().icon_pixel_size, scrollbar_style().down_icon);
		decrement_().set_graphics
			(scrollbar_style().icon_pixel_size, scrollbar_style().up_icon);
	}
	else
	{
		LOG_ERROR("Unknown orientation for scrollbar.");
		throw invalid_parameter();
	}
}


vector2f gui_scrollbar::preferred_size() const
{
	return
		orientation == horizontal ?
			vector2f(2 * scrollbar_style().button_size + scrollbar_style().min_band_size,
					 max(scrollbar_style().button_size, band_().preferred_size().y)) :
			vector2f(max(scrollbar_style().button_size, band_().preferred_size().x),
					 2 * scrollbar_style().button_size + scrollbar_style().min_band_size);
}


void gui_scrollbar::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	process_child_input(input, output);
	process_band_input(input, output);
}


void gui_scrollbar::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (!scrollbar_style().buttons_only)
	{
		band_().commit_render(packer, queue);
		knob_().commit_render(packer, queue);
	}

	increment_().commit_render(packer, queue);
	decrement_().commit_render(packer, queue);
}


void gui_scrollbar::set_range
	(float range_min, float range_max)
{
	if (range_min > range_max)
	{
		LOG_ERROR("Range minimum must not be greater than range maximum.");
		throw invalid_parameter();
	}

	range_min_ = range_min;
	range_max_ = range_max;

	position_ = clamp_to_range(position_);

	want_layout();
	knob_().want_layout();
}


float gui_scrollbar::range_min() const NOTHROW { return range_min_; }
float gui_scrollbar::range_max() const NOTHROW { return range_max_; }


void gui_scrollbar::set_page_size(float page_size)
{
	if (page_size < 0)
	{
		LOG_ERROR("Page size must not be negative.");
		throw invalid_parameter();
	}

	page_size_ = page_size;

	want_layout();
	knob_().want_layout();
}

float gui_scrollbar::page_size() const NOTHROW
	{ return page_size_; }


void gui_scrollbar::set_item_size(float item_size)
	{ item_size_ = item_size; }
float gui_scrollbar::item_size() const NOTHROW
	{ return item_size_; }

void gui_scrollbar::set_overlap_factor(float overlap_factor)
	{ overlap_factor_ = overlap_factor; }
float gui_scrollbar::overlap_factor() const NOTHROW
	{ return overlap_factor_; }

void gui_scrollbar::set_continuous_increment(float continuous_increment)
	{ continuous_increment_ = continuous_increment; }
float gui_scrollbar::continuous_increment() const NOTHROW
	{ return continuous_increment_; }


void gui_scrollbar::set_position(float position) NOTHROW
{
	position_ = clamp_to_range(position);
	want_layout();
	knob_().want_layout();
}

float gui_scrollbar::position() const NOTHROW
	{ return position_; }


float gui_scrollbar::clamp_to_range(float position) const NOTHROW
	{ return max(min(position, range_max_), range_min_); }


void gui_scrollbar::set_position_normalized(float position_normalized) NOTHROW
{
	float range = range_max_ - range_min_;
	set_position(range_min_ + position_normalized * range);
}

float gui_scrollbar::position_normalized() const NOTHROW
{
	// Avoid divide-by-zero on empty scrolling range,
	// e.g. if entire scrolled content is visible.
	float range = range_max_ - range_min_;
	return (range >= range_epsilon_) ? (position_ - range_min_) / range : 0;
}


void gui_scrollbar::scroll_page_to(float point) NOTHROW
{
	if (point > position_ + page_size_)
		set_position(point - page_size_);
	else if (point < position_)
		set_position(point);

	// Otherwise, point is on current page.
}

void gui_scrollbar::scroll_page_to(float start, float end) NOTHROW
{
	bool start_visible = is_visible_on_page(start);
	bool end_visible = is_visible_on_page(end);

	if (start_visible && end_visible) return;

	float delta_start =
		start_visible ? 0.f : fabsf(start - position_);
	float delta_end =
		end_visible ? 0.f : fabsf(end - position_);

	scroll_page_to
		(delta_start >= delta_end ? start : end);
}


bool gui_scrollbar::is_visible_on_page(float point) const NOTHROW
{
	return point >= position_ &&
		   point <= position_ + page_size_;
}


void gui_scrollbar::realize_layout()
{
	if (layout_invalid())
	{
		hot_area_band_valid_ = false;
		knob_().clip_entirely();
		increment_().clip_entirely();
		decrement_().clip_entirely();
		band_().clip_entirely();
		return;
	}


	// Disable scrollbar buttons and knob
	// at utmost scrolling limits.
	increment_().set_read_only
		(position_ - range_max_ > -range_epsilon_);
	decrement_().set_read_only
		(position_ - range_min_ < range_epsilon_); 
	knob_().set_read_only
		(range_max_ - range_min_ < range_epsilon_);

	
	// A scrollbar exploits any size past preferred_size(),
	// and clips if less than the preferred size is available.
	// In particular, scrollbar buttons and knobs consume
	// all excess space available against orientation direction.
	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());

	ar_area.size =
		ar_area.size.max(preferred_size());


	// Provide layout for scrollbar buttons.
	vector2f button_size
		(scrollbar_style().button_size,
		 scrollbar_style().button_size);

	button_size = button_size.max
		(orientation == horizontal ?
			vector2f(0, ar_area.size.y) :
			vector2f(ar_area.size.x, 0));

	rect2f decrement_arrange
		(ar_area.pos, button_size);
	rect2f decrement_clip;
	if (decrement_arrange.try_intersect(cl_area, decrement_clip))
		decrement_().layout(decrement_arrange, decrement_clip);
	else
		decrement_().clip_entirely();

	rect2f increment_arrange
		(ar_area.end() - button_size, button_size);
	rect2f increment_clip;
	if (increment_arrange.try_intersect(cl_area, increment_clip))
		increment_().layout(increment_arrange, increment_clip);
	else
		increment_().clip_entirely();


	// Provide layout and page size for knob and band.
	// We cannot do this on set_page_size(), as knob size
	// depends on available knob area.
	rect2f band_arrange
		(orientation == horizontal ?
			rect2f(vector2f(decrement_arrange.end_x(), decrement_arrange.pos.y),
				   vector2f(increment_arrange.pos.x - decrement_arrange.end_x(),
							button_size.y)) :
			rect2f(vector2f(decrement_arrange.pos.x, decrement_arrange.end_y()),
				   vector2f(button_size.x,
							increment_arrange.pos.y - decrement_arrange.end_y())));

	rect2f band_clip;	
	if (band_arrange.try_intersect(cl_area, band_clip))
	{
		// Reset knob to current scrollbar position and size,
		// just in case these have been changed by other means
		// (in which case want_layout() was called).
		// Finally, we handle (hypothetical) divide-by-zero
		// on zero page size and zero scrolling range.
		float range = range_max_ - range_min_;
		float total_range = range + page_size_;

		float page_size_normalized =
			(total_range < range_epsilon_) ? 0 :
				max(0.f, min(1.f, page_size_ / total_range));

		knob_().set_position_normalized
			((range < range_epsilon_) ? 0 :
			 (position_ - range_max_) > -range_epsilon_ ? 1 :
			 (position_ - range_min_) < range_epsilon_ ? 0 :	
			 (position_ / range));

		knob_().set_knob_size
			(page_size_normalized *
				(orientation == horizontal ?
					band_arrange.size.x : band_arrange.size.y));

		knob_().layout(band_arrange, band_clip);
		band_().layout(band_arrange, band_clip);

		// Update hot area, NOTHROW.
		hot_area_band_ = band_arrange;
		hot_area_band_clipped_ = band_clip;
		hot_area_band_valid_ = true;
	}
	else
	{
		knob_().clip_entirely();
		band_().clip_entirely();
	}
}


void gui_scrollbar::process_child_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	/**
	 * Let children elements handle incoming input,
	 * and react appropriately to generated output.
	 */

	gui_output_event_list child_output;
	increment_().process_input(input, child_output);
	decrement_().process_input(input, child_output);

	if (!scrollbar_style().buttons_only)
		knob_().process_input(input, child_output);

	for (gui_output_event_list::const_iterator it(child_output.first()); it; ++it)
	{
		if (!*it) continue;


		// Apply normalized knob position to current range
		// each time the knob reacts to user input.
		if (typeid(**it) == typeid(gui_knob_event))
		{
			if (scrollbar_style().buttons_only) continue;

			set_position
				(knob_().position_normalized() *
					(range_max_ - range_min_) + range_min_);

			create_output_event(output);
		}


		// Move knob position by item size increment
		// on scroll button clicks.
		if (typeid(**it) == typeid(gui_button_event))
		{
			gui_button_event& button_event =
				static_cast<gui_button_event&>(**it);

			// Only react to click, not release.
			if (!button_event.source().down())
				continue;

			if (button_event.source == increment_)
				set_position(position_ + item_size_ * overlap_factor_);			
			else if (button_event.source == decrement_)
				set_position(position_ - item_size_ * overlap_factor_);
			
			create_output_event(output);
		}


		// Move knob position by continuous increment
		// on held scroll button.
		if (typeid(**it) == typeid(gui_button_hold_event))
		{
			gui_button_hold_event& button_event =
				static_cast<gui_button_hold_event&>(**it);

			if (button_event.source == increment_)
				set_position(position_ + continuous_increment_);
			else if (button_event.source == decrement_)
				set_position(position_ - continuous_increment_);

			create_output_event(output);
		}
	}
}


void gui_scrollbar::process_band_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	/**
	 * Frames do not perform appropriate input handling
	 * for clicks on the scroll band, so do this ourselves.
	 */

	if (layout_invalid()) return;
	if (!input.hot) return;
	if (typeid(input) != typeid(gui_mouse_event)) return;
	if (scrollbar_style().buttons_only) return;

	gui_mouse_event& mouse_event =
		static_cast<gui_mouse_event&>(input);

	if (!hot_area_band_valid_ ||
		!hot_area_band_clipped_.contains(mouse_event.cursor)) return;

	mouse_event.hot = false;
	gui_output_event_list ignore_band_output;
	band_().process_input(input, ignore_band_output);

	if (mouse_event.button != gui_mouse_event::left_button ||
		mouse_event.event != gui_mouse_event::button_click) return;


	// Determine page scroll direction.
	// Note that the normalized position is always
	// in the middle of the scrollbar knob, regardless
	// of actual knob size. Hence, we can use this position
	// to determine which side the user clicked on.
	vector2f knob_center
		(hot_area_band_.pos + hot_area_band_.size * position_normalized());

	float delta =
		(orientation == horizontal) ?
			(mouse_event.cursor.x - knob_center.x) :
			(mouse_event.cursor.y - knob_center.y);

	set_position(delta < 0 ?
					(position_ - page_size_ * overlap_factor_) :
					(position_ + page_size_ * overlap_factor_));

	create_output_event(output);
}


void gui_scrollbar::create_output_event
	(gui_output_event_list& output)
{
	auto_pointer<gui_output_event> event
		(new gui_scrollbar_event(*this));
	output.insert
		(std::move(event));
}


const float gui_scrollbar::range_epsilon_ = 0.001f;




//////////////////////////////////////////////////////////////////////////
//
// gui_scrollbar_style
//
//////////////////////////////////////////////////////////////////////////


gui_scrollbar_style::gui_scrollbar_style
	(const shared_instance<gui_knob_style>& knob_style,
	 const shared_instance<gui_button_style>& button_style,
	 const shared_instance<gui_frame_style>& band_style,
	 bool buttons_only,
	 float button_size,
	 float min_band_size,
	 const shared_instance<render_material_id>& left_icon,
	 const shared_instance<render_material_id>& up_icon,
	 const shared_instance<render_material_id>& right_icon,
	 const shared_instance<render_material_id>& down_icon,
	 const vector2i& icon_pixel_size)
	:
	knob_style(knob_style),
	button_style(button_style),
	band_style(band_style),
	buttons_only(buttons_only),
	button_size(button_size),
	min_band_size(min_band_size),
	left_icon(left_icon), up_icon(up_icon),
	right_icon(right_icon), down_icon(down_icon),
	icon_pixel_size(icon_pixel_size)
{
	if (knob_style().min_drag_range != 0)
	{
		LOG_ERROR("Knob style of a scrollbar must have a zero minimum dragging range.");
		throw invalid_parameter();
	}

	if (button_size <= 0)
	{
		LOG_ERROR("Scrollbar button size must be greater than zero.");
		throw invalid_parameter();
	}

	if (min_band_size < 0)
	{
		LOG_ERROR("Scrollbar minimum band size must not be negative.");
		throw invalid_parameter();
	}

	if (icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("Pixel size of scrollbar icons must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */