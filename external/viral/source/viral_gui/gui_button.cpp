/**
 *************************************************************************
 *
 * @file gui_button.cpp
 *
 * GUI buttons, implementation.
 *
 ************************************************************************/


#include "gui_button.hpp"


#include <viral_core/log.hpp>
#include <viral_core/timer.hpp>


#include "gui_event.hpp"
#include "gui_anchor.hpp"
#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_frame.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_button
//
//////////////////////////////////////////////////////////////////////////


gui_button::gui_button
	(const string& name, gui_context& gui,
	 const shared_instance<gui_button_style>& button_style,
	 behaviour_type behaviour)
	:
	gui_element(name, gui),
	button_style(button_style),
	behaviour(behaviour),

	hot_area_valid_(false),
	in_click_release_cycle_(false),

	down_(false),
	read_only_(false),

	// Timers have quite some overhead, so only use when required.
	hold_timer_(behaviour == hold_button ? new free_timer() : NULL),
	lock_button_already_locked_(false),

	repeat_timer_(button_style().allow_repeat ? new free_timer() : NULL),
	repeat_timer_valid_(false),

	frame_default_(new gui_frame(name + "_frame_default", gui,
								 button_style().default_style)),
	frame_down_(new gui_frame(name + "_frame_down", gui,
							  button_style().down_style)),
	frame_read_only_(new gui_frame(name + "_frame_read_only", gui,
								   button_style().read_only_style)),

	frame_key_focus_overlay_
		(new gui_frame(name + "_frame_key_focus_overlay", gui,
					   button_style().key_focus_overlay_style)),
	frame_mouse_focus_overlay_
		(new gui_frame(name + "_frame_mouse_focus_overlay", gui,
					   button_style().mouse_focus_overlay_style))
{
	insert_child(frame_default_);
	insert_child(frame_down_);
	insert_child(frame_read_only_);

	insert_child(frame_key_focus_overlay_);
	insert_child(frame_mouse_focus_overlay_);
}


vector2f gui_button::preferred_size() const
{
	return
		frame_default_().frame_size_for_content_size
			(calculate_content_size());
}


void gui_button::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (layout_invalid() ||
		!hot_area_valid_ || read_only_)
	{
		abort_click_release_cycle(output);
		return;
	}

	if (typeid(input) == typeid(gui_tick_event))
		process_tick_event(output);

	if (typeid(input) == typeid(gui_input_source_event))
		process_input_source_event
			(static_cast<gui_input_source_event&>(input), output);

	if (typeid(input) == typeid(gui_mouse_event))
		process_mouse_event
			(static_cast<gui_mouse_event&>(input), output);
}


void gui_button::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (read_only_)
		frame_read_only_().commit_render(packer, queue);
	else if (down_)
		frame_down_().commit_render(packer, queue);
	else
		frame_default_().commit_render(packer, queue);

	if (has_key_focus())
		frame_key_focus_overlay_().commit_render(packer, queue);
	if (has_mouse_focus())
		frame_mouse_focus_overlay_().commit_render(packer, queue);

	if (label_) label_->commit_render(packer, queue);
	if (image_) image_->commit_render(packer, queue);
}


bool gui_button::try_accept_key_focus() NOTHROW
{
	want_commit_render();
	return true;
}

void gui_button::lost_key_focus() NOTHROW
	{ want_commit_render(); }


bool gui_button::try_accept_mouse_focus() NOTHROW
{
	want_commit_render();
	return true;
}

void gui_button::lost_mouse_focus() NOTHROW
	{ want_commit_render(); }


void gui_button::set_down(bool down) NOTHROW
{
	if (behaviour != toggle_button &&
		behaviour != lock_button &&
		behaviour != probe_button)
			return;


	// Discard any ongoing click cycle
	// if button down state is toggled.
	abort_click_release_cycle_no_events();
	update_down(down);


	// Lock buttons switch to unlocked state
	// only with set_down(false).
	if (behaviour == lock_button)
		lock_button_already_locked_ = down;
}

bool gui_button::down() const NOTHROW
	{ return down_; }


void gui_button::set_read_only(bool read_only) NOTHROW
{
	if (read_only_ == read_only) return;

	update_read_only(read_only);

	// Discard ongoing user interactions
	// if the read-only state is toggled.
	// We must pass key focus to our parent
	// so we keep any owning window active.

	abort_click_release_cycle_no_events();
	release_key_focus_to_parent();
	release_mouse_focus();
}

bool gui_button::read_only() const NOTHROW
	{ return read_only_; }


void gui_button::set_text(const string& text)
{
	shared_instance<gui_label> new_label
		(new gui_label
			(name + "_button_label", gui,
			 button_style().label_style, text));

	insert_child(new_label);

	// NOTHROW from here.
	label_ = new_label;
	want_layout();
}

string gui_button::text() const
	{ return label_ ? label_->text() : string(); }

void gui_button::clear_text() NOTHROW
{
	label_.reset();
	want_layout();
}


void gui_button::set_graphics
	(const image_rgba& graphics,
	 const render_material_data& material_data)
{
	shared_instance<gui_image> new_image
		(new gui_image(name + "_button_image", gui,
					   button_style().image_style,
					   graphics, material_data));

	insert_child(new_image);

	// NOTHROW from here.
	image_ = new_image;
	want_layout();
}

void gui_button::set_graphics
	(const vector2i& image_size,
	 const shared_instance<render_material_id>& material)
{
	shared_instance<gui_image> new_image
		(new gui_image(name + "_button_image", gui,
					   button_style().image_style,
					   material, image_size));

	insert_child(new_image);

	// NOTHROW from here.
	image_ = new_image;
	want_layout();
}

void gui_button::clear_graphics() NOTHROW
{
	image_.reset();
	want_layout();
}


void gui_button::realize_layout()
{
	// Handle clipping of entire button.
	if (layout_invalid())
	{
		hot_area_valid_ = false;

		frame_default_().clip_entirely();
		frame_down_().clip_entirely();
		frame_read_only_().clip_entirely();

		frame_key_focus_overlay_().clip_entirely();
		frame_mouse_focus_overlay_().clip_entirely();

		if (label_) label_->clip_entirely();
		if (image_) image_->clip_entirely();

		return;
	}


	// Provide layout for button frames.
	// Bound minimum frame size by full content size.
	rect2f frame_area(arrange_area());
	frame_area.size = frame_area.size.max(preferred_size());
		
	frame_default_().layout(frame_area, clip_area());
	frame_down_().layout(frame_area, clip_area());
	frame_read_only_().layout(frame_area, clip_area());

	frame_key_focus_overlay_().layout(frame_area, clip_area());
	frame_mouse_focus_overlay_().layout(frame_area, clip_area());


	// Fetch content and hot areas from frames.
	// Frames already match bound content size,
	// so no need to pass in content size again.
	hot_area_valid_ = frame_default_().build_border_layout
		(vector2f(), hot_area_, hot_area_clipped_);

	rect2f content_arrange;
	rect2f content_clip;

	if (!frame_default_().build_content_layout
			(vector2f(), content_arrange, content_clip))
	{
		if (label_) label_->clip_entirely();
		if (image_) image_->clip_entirely();
		return;
	}


	// Anchor content in content arrange area,
	// then position individual children.
	rect2f content_anchored
		(gui_anchor::build_layout
			(calculate_content_size(), content_arrange,
			 button_style().content_anchor_style()));

	float position_x = content_anchored.pos.x;

	if (image_)
	{
		vector2f image_preferred
			(image_->preferred_size());

		float image_y =
			content_anchored.pos.y +
			(content_anchored.size.y - image_preferred.y) * 0.5f;

		rect2f image_arrange
			(vector2f(position_x, image_y), image_preferred);
		
		rect2f image_clip;
		if (image_arrange.try_intersect(clip_area(), image_clip))
			image_->layout(image_arrange, image_clip);
		else
			image_->clip_entirely();

		float offset_to_label =
			image_preferred.x + button_style().image_label_spacing;

		position_x += offset_to_label;
	}

	if (label_)
	{
		vector2f label_preferred
			(label_->preferred_size());

		float label_y =
			content_anchored.pos.y +
			(content_anchored.size.y - label_preferred.y) * 0.5f;

		rect2f label_arrange
			(vector2f(position_x, label_y), label_preferred);

		rect2f label_clip;
		if (label_arrange.try_intersect(clip_area(), label_clip))
			label_->layout(label_arrange, label_clip);
		else
			label_->clip_entirely();
	}
}


vector2f gui_button::calculate_content_size() const
{
	vector2f label_size
		(label_ ? label_->preferred_size() : vector2f());
	vector2f image_size
		(image_ ? image_->preferred_size() : vector2f());

	return vector2f
		(label_size.x + image_size.x +
				((label_ && image_) ? button_style().image_label_spacing : 0),
			 max(label_size.y, image_size.y));
}


void gui_button::process_tick_event
	(gui_output_event_list& output)
{
	if (behaviour != hold_button || !down_)
		return;

	if (hold_timer_->seconds_passed() < button_style().hold_delay_seconds)
		return;
	
	auto_pointer<gui_output_event> event
		(new gui_button_hold_event(*this));
	output.insert
		(std::move(event));
}


void gui_button::process_input_source_event
	(gui_input_source_event& input_source_event,
	 gui_output_event_list& output)
{
	if (!has_key_focus()) return;
	if (read_only_) return;
	

	// While we have keyboard focus,
	// react to space with click cycle.
	if (input_source_event.source == input_source::k_space)
	{
		if (input_source_event.hot)
		{
			if (input_source_event.event == gui_input_source_event::source_activated)
				start_click_release_cycle(output);
		
			if (input_source_event.event == gui_input_source_event::source_deactivated)
				finish_click_release_cycle(output);

			input_source_event.hot = false;
		}
		else
		{
			if (input_source_event.event == gui_input_source_event::source_deactivated)
				abort_click_release_cycle(output);
		}
	}
}


void gui_button::process_mouse_event
	(gui_mouse_event &mouse_event,
	 gui_output_event_list& output)
{
	// Read-only buttons never react to any sort of events,
	// never have any focus, but still consume all input.
	// See set_read_only() for rationale.
	if (read_only_)
	{
		if (hot_area_clipped_.contains(mouse_event.cursor))
			mouse_event.hot = false;

		release_key_focus_to_parent();
		release_mouse_focus();

		return;
	}


	// Update mouse focus to button.
	// Avoid to capture mouse on cold events,
	// e.g. cursor over window over button.
	if (mouse_event.hot &&
		hot_area_clipped_.contains(mouse_event.cursor))
	{
		acquire_mouse_focus();
	}
	else
		release_mouse_focus();


	// Detect click-release cycle:
	// - Mouse must be left-clicked, then released,
	//		inside the hot zone of the button for success.
	// - Abort click-release cycle on mouse release
	//		outside the hot area of the button.
	// - On each mouse click, also request key focus.
	//		This enables keyboard input to the button.
	if (mouse_event.button == gui_mouse_event::left_button)
	{
		if (mouse_event.hot &&
			hot_area_clipped_.contains(mouse_event.cursor))
		{
			if (mouse_event.event == gui_mouse_event::button_click)
			{
				acquire_key_focus();
				start_click_release_cycle(output);
			}

			if (mouse_event.event == gui_mouse_event::button_release)
				finish_click_release_cycle(output);
		}	
		else
		{
			if (mouse_event.event == gui_mouse_event::button_release)
				abort_click_release_cycle(output);
		}
	}


	// Consume all mouse events within
	// the hot area of the button.
	if (hot_area_clipped_.contains(mouse_event.cursor))
		mouse_event.hot = false;
}


void gui_button::start_click_release_cycle
	(gui_output_event_list& output)
{
	// Pretest, since there may be inconsistent cycle calls
	// (e.g. from conflicting keyboard and mouse input).
	if (in_click_release_cycle_) return;
	in_click_release_cycle_ = true;

	if (behaviour == probe_button)
	{
		// Probe buttons react directly on initial click.
		// Otherwise, users would perceive input latency
		// because we do not set down() during click cycle.
		create_button_event(output);
		return;
	}

	update_down
		((behaviour == toggle_button) ?
			!down_ : true);

	if (behaviour == hold_button)
	{
		create_button_event(output);
		hold_timer_->restart();
	}
}


void gui_button::finish_click_release_cycle
	(gui_output_event_list& output)
{
	// Pretest, since there may be inconsistent cycle calls
	// (e.g. from conflicting keyboard and mouse input).
	if (!in_click_release_cycle_) return;
	in_click_release_cycle_ = false;

	if (behaviour == click_button ||
		behaviour == hold_button)
	{
		update_down(false);
		create_button_event(output);
	}
	else if (behaviour == toggle_button)
	{		
		create_button_event(output);
	}
	else if (behaviour == lock_button)
	{
		lock_button_already_locked_ ?
			create_button_locked_event(output) :
			create_button_event(output);

		lock_button_already_locked_ = true;
	}
	else if (behaviour == probe_button)
	{
		return; // For the sake of completeness.
	}
}


void gui_button::abort_click_release_cycle
	(gui_output_event_list& output)
{
	// Pretest, since there may be inconsistent cycle calls
	// (e.g. from conflicting keyboard and mouse input).
	if (!in_click_release_cycle_) return;
	abort_click_release_cycle_no_events(); // Resets in_click_release_cycle_.
	
	if (behaviour == hold_button)
		create_button_event(output);
}


void gui_button::abort_click_release_cycle_no_events() NOTHROW
{
	/**
	 * This variant of abort_click_release_cycle()
	 * creates no output event and does not throw,
	 * which is useful for utility NOTHROW methods
	 * such as set_read_only().
	 */

	// Pretest, since there may be inconsistent cycle calls
	// (e.g. from conflicting keyboard and mouse input).
	if (!in_click_release_cycle_) return;
	in_click_release_cycle_ = false;

	if (behaviour == click_button)
	{
		update_down(false);
	}
	else if (behaviour == hold_button)
	{
		update_down(false);
	}
	else if (behaviour == toggle_button)
	{
		update_down(!down_);
	}
	else if (behaviour == lock_button)
	{
		if (!lock_button_already_locked_)
			update_down(false);
	}
	else if (behaviour == probe_button)
	{
		return; // For the sake of completeness.
	}
}


void gui_button::create_button_event
	(gui_output_event_list& output)
{
	if (create_button_repeat_event(output))
		return;

	auto_pointer<gui_output_event> event
		(new gui_button_event(*this));
	output.insert
		(std::move(event));
}


void gui_button::create_button_locked_event
	(gui_output_event_list& output)
{
	auto_pointer<gui_output_event> event
		(new gui_button_locked_event(*this));
	output.insert
		(std::move(event));
}


bool gui_button::create_button_repeat_event(gui_output_event_list& output)
{
	// Check if the button supports repetition.
	// Notably, some button behaviours (e.g. hold_button)
	// have no intuitive double-click semantics.
	if (behaviour != click_button &&
		behaviour != probe_button)
		return false;

	if (!button_style().allow_repeat)
		return false;


	// Check for repetition interval.
	// Unfortunately, we must separately keep track of
	// whether our timer was seeded by an initial click.
	if (!repeat_timer_valid_)
	{
		repeat_timer_->restart();

		// Exception safety: Update validity flag
		// only after timer success.
		repeat_timer_valid_ = true;
		return false;
	}

	float seconds_since_last_click =
		repeat_timer_->seconds_passed();

	if (seconds_since_last_click > button_style().repeat_seconds)
	{
		repeat_timer_->restart();
		return false;
	}


	// Generate repetition event.
	// We reset the timer even when we produce a repeated click.
	// This allows to create multiple repeat notifications
	// without an intermediate normal click event.
	repeat_timer_->restart();

	auto_pointer<gui_output_event> event
		(new gui_button_repeat_event(*this));
	output.insert
		(std::move(event));

	return true;
}


void gui_button::update_down(bool down) NOTHROW
{
	if (down_ == down) return;
	down_ = down;
	want_commit_render();
}

void gui_button::update_read_only(bool read_only) NOTHROW
{
	if (read_only_ == read_only) return;
	read_only_ = read_only;
	want_commit_render();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_button_style
//
//////////////////////////////////////////////////////////////////////////


gui_button_style::gui_button_style
	(const shared_instance<gui_frame_style>& default_style,
	 const shared_instance<gui_frame_style>& down_style,
	 const shared_instance<gui_frame_style>& read_only_style,
	 const shared_instance<gui_frame_style>& key_focus_overlay_style,
	 const shared_instance<gui_frame_style>& mouse_focus_overlay_style,
	 const shared_instance<gui_anchor_style>& content_anchor_style,
	 const shared_instance<gui_label_style>& label_style,
	 const shared_instance<gui_image_style>& image_style,
	 float image_label_spacing,
	 float hold_delay_seconds,
	 bool allow_repeat,
	 float repeat_seconds)
	:
	default_style(default_style),
	down_style(down_style),
	read_only_style(read_only_style),
	key_focus_overlay_style(key_focus_overlay_style),
	mouse_focus_overlay_style(mouse_focus_overlay_style),
	content_anchor_style(content_anchor_style),
	label_style(label_style),
	image_style(image_style),
	image_label_spacing(image_label_spacing),
	hold_delay_seconds(hold_delay_seconds),
	allow_repeat(allow_repeat),
	repeat_seconds(repeat_seconds)
{
	if (image_label_spacing < 0)
	{
		LOG_ERROR("Spacing between image and label must be zero or greater.");
		throw invalid_parameter();
	}

	if (hold_delay_seconds <= 0)
	{
		LOG_ERROR("Seconds for hold delay must be greater than zero.");
		throw invalid_parameter();
	}

	if (repeat_seconds <= 0)
	{
		LOG_ERROR("Seconds for click repetition must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
