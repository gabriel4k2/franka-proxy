/**
 *************************************************************************
 *
 * @file gui_slider.cpp
 *
 * Sliders for convenient value entry, implementation.
 *
 ************************************************************************/


#include "gui_slider.hpp"


#include <math.h>


#include <viral_core/log.hpp>
#include <viral_core/mesh.hpp>


#include "gui.hpp"
#include "gui_event.hpp"
#include "gui_render.hpp"
#include "gui_frame.hpp"
#include "gui_button.hpp"
#include "gui_knob.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_slider
//
//////////////////////////////////////////////////////////////////////////


gui_slider::gui_slider
	(const string& name, gui_context& gui,
	 const shared_instance<gui_slider_style>& slider_style,
	 orientation_type orientation,
	 snap_mode snap)
	:
	gui_element(name, gui),

	slider_style(slider_style),
	orientation(orientation),
	snap(snap),

	read_only_(false),
	range_min_(0),
	range_max_(1),
	click_increment_(0.1f),
	continuous_increment_(0.01f),
	value_(0),
	hot_area_band_valid_(false),

	knob_(new gui_knob(name + "_knob", gui,
					   slider_style().knob_style, orientation)),

	increment_(new gui_button
					(name + "_increment", gui,
					 slider_style().button_style, gui_button::hold_button)),

	decrement_(new gui_button
					(name + "_decrement", gui,
					 slider_style().button_style, gui_button::hold_button)),

	band_(new gui_frame(name + "band", gui,
						slider_style().band_style))
{
	if (slider_style().min_band_size < slider_style().knob_style().min_drag_range)
	{
		LOG_ERROR("Minimum slider band size must not be less than knob drag range.");
		throw invalid_parameter();
	}

	knob_().set_knob_size(0);
	insert_child(knob_);

	increment_().set_graphics
		(slider_style().button_icon_pixel_size, slider_style().increment_icon);
	decrement_().set_graphics
		(slider_style().button_icon_pixel_size, slider_style().decrement_icon);

	insert_child(increment_);
	insert_child(decrement_);
	insert_child(band_);
}


vector2f gui_slider::preferred_size() const
{
	vector2f size;
	int64 slider_dir = (orientation == horizontal) ? 0 : 1;
	int64 center_dir = (orientation == horizontal) ? 1 : 0;


	// Fetch button influence on preferred size.
	if (slider_style().show_buttons)
	{
		vector2f button_size
			(slider_style().button_size, slider_style().button_size);
		vector2f increment_size
			(increment_().preferred_size().max(button_size));
		vector2f decrement_size
			(decrement_().preferred_size().max(button_size));

		size[slider_dir] +=
			increment_size[slider_dir] + decrement_size[slider_dir] +
			2 * slider_style().button_spacing;

		size[center_dir] =
			max(size[center_dir],
				max(increment_size[center_dir],
					decrement_size[center_dir]));
	}


	// Apply knob and band influence.
	// Note the knob needs an extra minimum_knob_size() space,
	// see realize_layout() for rationale.
	vector2f inner_size(band_().preferred_size());
	inner_size = inner_size.max(knob_().preferred_size());

	inner_size[slider_dir] =
		max(inner_size[slider_dir], slider_style().min_band_size);
	inner_size[center_dir] =
		max(inner_size[center_dir], slider_style().fixed_band_size);

	size[slider_dir] +=
		inner_size[slider_dir] + knob_().minimum_knob_size();
	size[center_dir] =
		max(size[center_dir], inner_size[center_dir]);
	
	return size;
}


void gui_slider::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	process_child_input(input, output);
	process_band_input(input, output);
}


void gui_slider::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	band_().commit_render(packer, queue);

	if (slider_style().show_buttons)
	{
		increment_().commit_render(packer, queue);
		decrement_().commit_render(packer, queue);
	}

	pack_marks(packer);

	knob_().commit_render(packer, queue);
}


void gui_slider::set_read_only(bool read_only) NOTHROW
{ 
	// We need a separate read_only_ state
	// because buttons and the knob modify
	// their read_only() at slider limits.
	if (read_only_ == read_only) return;
	read_only_ = read_only;
	update_read_only();
};

bool gui_slider::read_only() const NOTHROW
	{ return read_only_; }


void gui_slider::set_marks(const dynamic_buffer<float>& marks)
{
	if (marks.size() > max_mark_count_)
	{
		LOG_ERROR("Incoming marks exceed maximum allowed mark count.");
		throw bounds_exceeded();
	}

	// Exception safety: NOTHROW from here.

	marks_ = marks;
	set_value(value_); // Potentially snap value_ to new marks_.
	want_layout();
}

const dynamic_buffer<float>& gui_slider::marks() const NOTHROW
	{ return marks_; }

void gui_slider::clear_marks() NOTHROW
	{ marks_.clear(); }


void gui_slider::set_range
	(float range_min, float range_max)
{
	if (range_min > range_max)
	{
		LOG_ERROR("Range minimum must not be greater than range maximum.");
		throw invalid_parameter();
	}

	// Exception safety: NOTHROW from here.

	range_min_ = range_min;
	range_max_ = range_max;
	
	set_value(value_); // Potentially clamp or (re-)snap value_.

	want_layout();
	knob_().want_layout();
}


float gui_slider::range_min() const NOTHROW { return range_min_; }
float gui_slider::range_max() const NOTHROW { return range_max_; }


void gui_slider::set_click_increment(float click_increment)
	{ click_increment_ = click_increment; }
float gui_slider::click_increment() const NOTHROW
	{ return click_increment_; }

void gui_slider::set_continuous_increment(float continuous_increment)
	{ continuous_increment_ = continuous_increment; }
float gui_slider::continuous_increment() const NOTHROW
	{ return continuous_increment_; }


void gui_slider::set_value(float value) NOTHROW
{
	value_ =
		clamp_to_range
			((snap == snap_force) ?
				snap_mark_nearest(value) : value);

	want_layout();
	knob_().want_layout();

	// Update read-only state of slider knob and buttons at limits.
	// This also handles limits due to user input or range changes, 
	// which both call set_value().
	update_read_only();
}

float gui_slider::value() const NOTHROW
	{ return value_; }


float gui_slider::clamp_to_range(float value) const NOTHROW
	{ return max(min(value, range_max_), range_min_); }


void gui_slider::set_value_normalized(float value_normalized) NOTHROW
{
	float range = range_max_ - range_min_;
	set_value(range_min_ + value_normalized * range);
}

float gui_slider::value_normalized() const NOTHROW
{
	// Avoid divide-by-zero on empty slider range.
	float range = range_max_ - range_min_;
	return (range >= range_epsilon_) ? (value_ - range_min_) / range : 0;
}


void gui_slider::realize_layout()
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


	// Fetch utilities for orientation-independent layouting.
	rect2f ar_area(arrange_area());
	ar_area.size = ar_area.size.max(preferred_size());

	int64 slider_dir = (orientation == horizontal) ? 0 : 1;
	int64 center_dir = (orientation == horizontal) ? 1 : 0;


	// Position increment and decrement buttons
	// if currently enabled.
	float increment_offset = 0;
	float decrement_offset = 0;

	if (slider_style().show_buttons)
	{
		vector2f button_size
			(slider_style().button_size, slider_style().button_size);
		vector2f increment_size
			(increment_().preferred_size().max(button_size));
		vector2f decrement_size
			(decrement_().preferred_size().max(button_size));

		rect2f increment_arrange(ar_area.pos, increment_size);
		rect2f decrement_arrange(ar_area.pos, decrement_size);

		increment_arrange.pos[center_dir] +=
			(ar_area.size[center_dir] - increment_size[center_dir]) * 0.5f;
		decrement_arrange.pos[center_dir] +=
			(ar_area.size[center_dir] - decrement_size[center_dir]) * 0.5f;

		increment_arrange.pos[slider_dir] +=
			(ar_area.size[slider_dir] - increment_size[slider_dir]);

		rect2f increment_clip;
		if (increment_arrange.try_intersect(clip_area(), increment_clip))
			increment_().layout(increment_arrange, increment_clip);
		else
			increment_().clip_entirely();

		rect2f decrement_clip;
		if (decrement_arrange.try_intersect(clip_area(), decrement_clip))
			decrement_().layout(decrement_arrange, decrement_clip);
		else
			decrement_().clip_entirely();

		increment_offset +=
			increment_size[slider_dir] + slider_style().button_spacing;
		decrement_offset +=
			decrement_size[slider_dir] + slider_style().button_spacing;
	}
	else
	{
		increment_().clip_entirely();
		decrement_().clip_entirely();
	}


	// Grow slider knob to fill space in-between buttons.
	// Note that the minimum band size is already considered
	// through preferred_size(), which bounds ar_area.size.
	rect2f knob_arrange
		(ar_area.pos, knob_().preferred_size());

	knob_arrange.size[slider_dir] =
		max(ar_area.size[slider_dir] - increment_offset - decrement_offset,
			knob_arrange.size[slider_dir]);

	knob_arrange.pos[slider_dir] += decrement_offset;
	knob_arrange.pos[center_dir] +=
		(ar_area.size[center_dir] - knob_arrange.size[center_dir]) * 0.5f;

	rect2f knob_clip;
	if (knob_arrange.try_intersect(clip_area(), knob_clip))
	{
		// We reset the knob position to the current slider value,
		// just in case the value has been changed by other means
		// (in which case want_layout() was called).
		// Updates to knob position even work during knob dragging,
		// which ensures a correct preview of snapping.
		float range = range_max_ - range_min_;
		knob_().set_position_normalized
			((range < range_epsilon_) ? 0 :
			 (value_ - range_max_) > -range_epsilon_ ? 1 :
			 (value_ - range_min_) < range_epsilon_ ? 0 :
			 ((value_ - range_min_) / range));

		knob_().layout(knob_arrange, knob_clip);
	}
	else
		knob_().clip_entirely();
	

	// Arrange background band.
	// Size into orientation scales with arrange area,
	// size perpendicular to orientation is constant.
	// The band also is inset by minimum_knob_size(),
	// so that band values align with the knob center.
	rect2f band_arrange
		(ar_area.pos, band_().preferred_size());

	band_arrange.size[slider_dir] =
		max(knob_arrange.size[slider_dir] - knob_().minimum_knob_size(),
			band_arrange.size[slider_dir]);
	band_arrange.size[center_dir] =
		max(band_arrange.size[center_dir], slider_style().fixed_band_size);

	band_arrange.pos[slider_dir] +=
		decrement_offset + knob_().minimum_knob_size() * 0.5f;
	band_arrange.pos[center_dir] +=
		(ar_area.size[center_dir] - band_arrange.size[center_dir]) * 0.5f;

	rect2f band_clip;
	if (band_arrange.try_intersect(clip_area(), band_clip))
	{
		band_().layout(band_arrange, band_clip);

		// Update hot area, NOTHROW.
		hot_area_band_ = band_arrange;
		hot_area_band_clipped_ = band_clip;
		hot_area_band_valid_ = true;
	}
	else
		band_().clip_entirely();
}


void gui_slider::process_child_input
	(gui_input_event& input, gui_output_event_list& output)
{
	/**
	 * Let children elements handle incoming input,
	 * and react appropriately to generated output.
	 */

	gui_output_event_list child_output;

	if (slider_style().show_buttons)
	{
		increment_().process_input(input, child_output);
		decrement_().process_input(input, child_output);
	}

	knob_().process_input(input, child_output);

	for (gui_output_event_list::const_iterator it(child_output.first()); it; ++it)
	{
		if (!*it) continue;


		// Apply normalized knob position to slider value
		// each time the knob reacts to user input.
		if (typeid(**it) == typeid(gui_knob_event))
		{
			float new_value =
				knob_().position_normalized() *
					(range_max_ - range_min_) + range_min_;

			if (snap == snap_gui)
				new_value = snap_mark_gui(new_value);

			set_value(new_value);
			create_output_event(output);
		}


		// Change slider value by per-click increment
		// on increment or decrement button clicks.
		if (slider_style().show_buttons &&
			typeid(**it) == typeid(gui_button_event))
		{
			gui_button_event& button_event =
				static_cast<gui_button_event&>(**it);

			// Only react to click, not release.
			if (!button_event.source().down())
				continue;

			float new_value = value_;

			if (button_event.source == increment_)
				new_value += click_increment_;				
			else if (button_event.source == decrement_)
				new_value -= click_increment_;

			if (snap == snap_gui)
				new_value = snap_mark_gui(new_value);

			set_value(new_value);			
			create_output_event(output);
		}


		// Change slider value by continuous increment
		// on held increment or decrement buttons.
		// Continuous updates do not consider snapping,
		// which avoids getting stuck on a slider mark.
		if (slider_style().show_buttons &&
			typeid(**it) == typeid(gui_button_hold_event))
		{
			gui_button_hold_event& button_event =
				static_cast<gui_button_hold_event&>(**it);

			if (button_event.source == increment_)
				set_value(value_ + continuous_increment_);
			else if (button_event.source == decrement_)
				set_value(value_ - continuous_increment_);

			create_output_event(output);
		}
	}
}


void gui_slider::process_band_input
	(gui_input_event& input, gui_output_event_list& output)
{
	/**
	 * Frames do not perform appropriate input handling
	 * for clicks on the slider band, so do this ourselves.
	 */

	if (layout_invalid()) return;
	if (!input.hot) return;
	if (read_only_) return;
	if (typeid(input) != typeid(gui_mouse_event)) return;

	gui_mouse_event& mouse_event =
		static_cast<gui_mouse_event&>(input);

	if (!hot_area_band_valid_ ||
		!hot_area_band_clipped_.contains(mouse_event.cursor)) return;

	mouse_event.hot = false;
	gui_output_event_list ignore_band_output;
	band_().process_input(input, ignore_band_output);

	if (mouse_event.button != gui_mouse_event::left_button ||
		mouse_event.event != gui_mouse_event::button_click) return;

	int64 slider_dir = (orientation == horizontal) ? 0 : 1;
	if (hot_area_band_.size[slider_dir] < range_epsilon_)
		return;

	vector2f relative_click
		(mouse_event.cursor - hot_area_band_.pos);
	float normalized_value =
		relative_click[slider_dir] / hot_area_band_.size[slider_dir];
	float new_value =
		normalized_value * (range_max_ - range_min_) + range_min_;

	if (snap == snap_gui)
		new_value = snap_mark_gui(new_value);

	set_value(new_value);
	create_output_event(output);
}


void gui_slider::create_output_event
	(gui_output_event_list& output)
{
	auto_pointer<gui_output_event> event
		(new gui_slider_event(*this));
	output.insert
		(std::move(event));
}


void gui_slider::update_read_only() NOTHROW
{
	/**
	 * Update read-only state of slider knob and slider buttons,
	 * either due to a change in read_only() state of the slider,
	 * or because the slider value or range have changed.
	 */

	if (slider_style().show_buttons)
	{
		increment_().set_read_only
			(read_only_ ||
			 value_ - range_max_ > -range_epsilon_);
		decrement_().set_read_only
			(read_only_ ||
			 value_ - range_min_ < range_epsilon_); 
	}

	knob_().set_read_only
		(read_only_ ||
		 range_max_ - range_min_ < range_epsilon_);
}


void gui_slider::pack_marks(gui_render_packer& packer)
{
	/** Pack all marks for special values into the GUI mesh. */

	if (!hot_area_band_valid_) return;
	if (range_max_ - range_min_ < range_epsilon_) return;

	vector2f mark_icon_size
		(slider_style().mark_icon_size, slider_style().mark_icon_size);

	for (int64 i = 0; i < marks_.size(); ++i)
	{
		if (marks_[i] < range_min_ ||
			marks_[i] > range_max_)
				continue; // Outside range, ignore.

		float mark_percent =
			max(0.f, min(1.f,
				(marks_[i] - range_min_) / (range_max_ - range_min_)));

		vector2f mark_pos
			(hot_area_band_.pos +
			 hot_area_band_.size * mark_percent -
			 mark_icon_size * 0.5f);

		vector2f mark_center
			(hot_area_band_.pos +
				(hot_area_band_.size - mark_icon_size) * 0.5f);

		rect2f mark_extents
			(orientation == horizontal ?
				vector2f(mark_pos.x, mark_center.y) :
				vector2f(mark_center.x, mark_pos.y),
			 mark_icon_size);

		packer.pack_mesh
			(gui_render_util::create_mesh_from_rect
				(mark_extents, rect2f(0, 0, 1, 1), clip_area(),
				 true, gui.pixel_integral_rendering),
			 slider_style().mark_icon);
	}
}


float gui_slider::snap_mark_gui(float value) const NOTHROW
{
	/**
	 * Find and return valid mark nearest to @bref{value}
	 * within style-specific maximum snap tolerance.
	 */

	if (!hot_area_band_valid_) return value;
	if (range_max_ - range_min_ < range_epsilon_) return value;


	// Instead of snapping in GUI space, we snap to normalized values.
	// To simulate size-dependent GUI snapping, we derive a percentual
	// snap tolerance from the current band size.
	float gui_space_range =
		(orientation == horizontal) ?
			hot_area_band_.size.x : hot_area_band_.size.y;

	if (gui_space_range < range_epsilon_) return value;

	float snap_tolerance_percent =
		slider_style().mark_snap_tolerance / gui_space_range;


	// Perform actual snapping.
	int64 nearest_mark = -1;
	float nearest_mark_distance = 0;
	
	for (int64 i = 0; i < marks_.size(); ++i)
	{
		if (marks_[i] < range_min_ ||
			marks_[i] > range_max_)
				continue; // Outside range, ignore.

		float mark_distance_percent =
			fabsf(value - marks_[i]) /
				(range_max_ - range_min_);

		if (mark_distance_percent > snap_tolerance_percent)
			continue; // Outside snapping tolerance, ignore.

		if (nearest_mark == -1 ||
			mark_distance_percent < nearest_mark_distance)
		{
			nearest_mark = i;
			nearest_mark_distance = mark_distance_percent;
		}
	}

	return
		nearest_mark != -1 ?
			marks_[nearest_mark] : value;
}


float gui_slider::snap_mark_nearest(float value) const NOTHROW
{
	/**
	 * Find and return mark nearest to @bref{value}, if any.
	 * Implicitly also clamps to range_min() and range_max().
	 */

	int64 nearest_mark = -1;
	float nearest_mark_distance = 0;

	for (int64 i = 0; i < marks_.size(); ++i)
	{
		if (marks_[i] < range_min_ ||
			marks_[i] > range_max_)
				continue; // Outside range, ignore.

		float mark_distance = fabsf(value - marks_[i]);
		
		if (nearest_mark == -1 ||
			mark_distance < nearest_mark_distance)
		{
			nearest_mark = i;
			nearest_mark_distance = mark_distance;
		}
	}

	if (value < range_min_ + nearest_mark_distance + range_epsilon_)
		return range_min_;
	else if (value > range_max_ - nearest_mark_distance - range_epsilon_)
		return range_max_;
	else if (nearest_mark != -1)
		return marks_[nearest_mark];

	return value;
}


const int64 gui_slider::max_mark_count_ = 256;
const float gui_slider::range_epsilon_ = 0.001f;




//////////////////////////////////////////////////////////////////////////
//
// gui_slider_style
//
//////////////////////////////////////////////////////////////////////////


gui_slider_style::gui_slider_style
	(const shared_instance<gui_knob_style>& knob_style,
	 const shared_instance<gui_button_style>& button_style,
	 const shared_instance<gui_frame_style>& band_style,
	 bool show_buttons,
	 float button_size,
	 float button_spacing,
	 const shared_instance<render_material_id>& increment_icon,
	 const shared_instance<render_material_id>& decrement_icon,
	 const vector2i& button_icon_pixel_size,
	 float fixed_band_size,
	 float min_band_size,
	 const shared_instance<render_material_id>& mark_icon,
	 float mark_icon_size,
	 float mark_snap_tolerance)
	:
	knob_style(knob_style),
	button_style(button_style),
	band_style(band_style),
	show_buttons(show_buttons),
	button_size(button_size),
	button_spacing(button_spacing),
	increment_icon(increment_icon),
	decrement_icon(decrement_icon),
	button_icon_pixel_size(button_icon_pixel_size),
	fixed_band_size(fixed_band_size),
	min_band_size(min_band_size),
	mark_icon(mark_icon),
	mark_icon_size(mark_icon_size),
	mark_snap_tolerance(mark_snap_tolerance)
{
	if (button_size <= 0)
	{
		LOG_ERROR("Button size must be greater than zero.");
		throw invalid_parameter();
	}

	if (fixed_band_size < 0 || min_band_size < 0)
	{
		LOG_ERROR("Neither fixed nor minimum band size may be negative.");
		throw invalid_parameter();
	}

	if (button_icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("Pixel size of button icons must be greater than zero.");
		throw invalid_parameter();
	}

	if (mark_snap_tolerance < 0)
	{
		LOG_ERROR("Snap tolerance of slider marks must not be negative.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */