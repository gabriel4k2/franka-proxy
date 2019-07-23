/**
 *************************************************************************
 *
 * @file gui_window.cpp
 *
 * GUI window facilities, implementation.
 *
 ************************************************************************/


#include "gui_window.hpp"


#include <viral_core/log.hpp>
#include <viral_core/mesh.hpp>


#include "gui.hpp"
#include "gui_event.hpp"
#include "gui_label.hpp"
#include "gui_frame.hpp"
#include "gui_button.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_window
//
//////////////////////////////////////////////////////////////////////////


gui_window::gui_window
	(const string& name, gui_context& gui,
	 const shared_instance<gui_window_style>& window_style,
	 const string& caption,
	 bool allow_close,
	 const window_resize_policy resize_policy)
	:
	gui_element(name, gui),
	window_style(window_style),
	allow_close(allow_close),
	resize_policy(resize_policy),

	client_size_(window_style().min_client_size),
	hot_areas_valid_(false),
	drag_type_x_(no_drag),
	drag_type_y_(no_drag),
	center_on_next_layout_(false),

	frame_default_
		(new gui_frame(name + "_frame_default", gui,
					   window_style().default_style)),
	frame_key_focus_overlay_
		(new gui_frame(name + "_frame_key_focus_overlay", gui,
					   window_style().key_focus_overlay_style)),

	caption_label_
		(new gui_label(name + "_caption_label", gui,
					   window_style().caption_label_style,
					   caption)),
	close_button_
		(new gui_button(name + "_close_button", gui,
						window_style().close_button_style,
						gui_button::click_button))
{
	insert_child(frame_default_);
	insert_child(frame_key_focus_overlay_);

	insert_child(caption_label_);

	// Add close button to hierarchy even if allow_close is @false.
	// This avoids branching for a @NULL pointer at various points.
	// However, a disallowed close button receives neither a layout
	// (and thus remains hidden), nor any input events.
	insert_child(close_button_);

	close_button_().set_graphics
		(window_style().close_icon_pixel_size,
		 window_style().close_icon);
}


vector2f gui_window::preferred_size() const
	{ return frame_default_().frame_size_for_content_size(client_size_); }


void gui_window::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (layout_invalid()) return;

	if (allow_close)
		process_window_button_input(input, output);

	process_window_drag_input(input, output);

	if (content_)
		content_->process_input(input, output);

	process_window_background_input(input);
}

	
void gui_window::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	frame_default_().commit_render(packer, queue);

	if (contains_key_focus())
		frame_key_focus_overlay_().commit_render(packer, queue);
	
	caption_label_().commit_render(packer, queue);

	if (allow_close)
		close_button_().commit_render(packer, queue);

	if (content_)
		content_->commit_render(packer, queue);
}


bool gui_window::try_accept_key_focus() NOTHROW
{
	want_commit_render();
	return true;
}

void gui_window::lost_key_focus() NOTHROW
	{ want_commit_render(); }


void gui_window::set_caption(const string& caption)
{
	caption_label_().set_text(caption);
	want_layout();
}

const string& gui_window::caption() const
	{ return caption_label_().text(); }


void gui_window::set_position(const vector2f& position)
{
	if (position_ == position) return;
	position_ = position;
	want_layout();
}

const vector2f& gui_window::position() const
	{ return position_; }


void gui_window::set_client_size(const vector2f& client_size)
{
	if (client_size_ == client_size) return;
	client_size_ = client_size;
	want_layout();
}

const vector2f& gui_window::client_size() const
	{ return client_size_; }


void gui_window::center_window_delayed()
{
	center_on_next_layout_ = true;
	want_layout();
}


void gui_window::resize_to_preferred_content()
{
	set_client_size
		(content_ ?
		 content_->preferred_size() : vector2f());
}


void gui_window::set_content
	(const shared_pointer<gui_element>& content)
{
	if (content)
		insert_child(content.instance_or_throw());

	// Exception safety: NOTHROW from here.
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_ = content;
	want_layout();
}

const shared_pointer<gui_element>&
	gui_window::content() const NOTHROW
		{ return content_; }

void gui_window::clear_content() NOTHROW
{
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_.reset();
	want_layout();
}


void gui_window::realize_layout()
{
	// Clip everything within the window if parent
	// did not provide us with valid areas.
	if (layout_invalid())
	{
		hot_areas_valid_ = false;

		frame_default_().clip_entirely();
		frame_key_focus_overlay_().clip_entirely();

		caption_label_().clip_entirely();
		close_button_().clip_entirely();

		if (content_) content_->clip_entirely();

		return;
	}


	// Apply automatic centering before constraint checks,
	// so we bring the movement area of a very large window
	// back into visible arrangement area.
	if (center_on_next_layout_)
	{
		position_ =
			(arrange_area().size -
			 (frame_default_().preferred_size() + client_size_)) * 0.5f;

		center_on_next_layout_ = false;
	}


	// Ensure size and visibility constraints are met
	// at all times, e.g. if window content or parent
	// arrange area changed in size, or if the user
	// set an invalid client size.
	enforce_position_and_size_constraints
		(false, false, false, false);


	// Determine arrange and clip areas for window frame
	// within arrange_area() of parent gui_window_area.
	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());

	rect2f frame_arrange
		(ar_area.pos + position_,
		 frame_default_().preferred_size() + client_size_);

	rect2f frame_clip;
	if (!frame_arrange.try_intersect(cl_area, frame_clip))
	{
		clip_entirely();
		return;
	}

	frame_default_().layout(frame_arrange, frame_clip);
	frame_key_focus_overlay_().layout(frame_arrange, frame_clip);


	// Update hot areas for moving and resizing the window.
	// We need both the unclipped areas (to determine click actions)
	// and the clipped areas (to determine click validity).
	hot_area_resize_ =
		rect2f(frame_arrange.pos + window_style().hot_areas_padding_00,
			   frame_arrange.size -
					(window_style().hot_areas_padding_00 +
					 window_style().hot_areas_padding_22));
	
	hot_area_move_ = hot_area_resize_;
	hot_area_move_.size.y = 
		min(hot_area_move_.size.y, window_style().hot_area_move_height);

	hot_areas_valid_ =
		hot_area_resize_.size.min() > 0 &&
		hot_area_move_.size.min() > 0 &&
		hot_area_resize_.try_intersect(frame_clip, hot_area_resize_clipped_) &&
		hot_area_move_.try_intersect(frame_clip, hot_area_move_clipped_);
				

	// If enabled, arrange close button
	// to the right of the window caption.
	rect2f caption_arrange
		(frame_arrange.pos + window_style().caption_offset,
		 vector2f
			(frame_arrange.size.x - window_style().caption_offset.x - window_style().caption_right_spacing,
			 window_style().caption_height));

	if (allow_close)
	{
		vector2f close_button_size
			(close_button_().preferred_size());

		rect2f close_button_arrange
			(caption_arrange.pos +
			 vector2f(caption_arrange.size.x - close_button_size.x,
					  window_style().caption_height * 0.5f - close_button_size.y * 0.5f),
			 close_button_size);

		rect2f close_button_clip;
		if (close_button_arrange.pos.x >= caption_arrange.pos.x &&
			close_button_arrange.try_intersect(frame_clip, close_button_clip))
		{
			close_button_().layout(close_button_arrange, close_button_clip);
		}
		else
			close_button_().clip_entirely();

		caption_arrange.size.x -=
			(close_button_size.x + window_style().caption_button_spacing);
	}
	else
	{
		close_button_().clip_entirely();
	}


	// Arrange caption label in the remainder
	// of the caption line.
	vector2f label_preferred
		(caption_label_().preferred_size());

	rect2f label_arrange
		(caption_arrange.pos +
		 vector2f(0, window_style().caption_height * 0.5f -
					 label_preferred.y * 0.5f),
		 vector2f(caption_arrange.size.x,
				  label_preferred.y));
				  
	rect2f label_clip;
	if (label_arrange.size.x > 0 &&
		label_arrange.try_intersect(frame_clip, label_clip))
	{
		caption_label_().layout(label_arrange, label_clip);
	}
	else
		caption_label_().clip_entirely();


	// Place content within window frame.
	if (content_)
	{
		rect2f content_arrange;
		rect2f content_clip;

		if (frame_default_().build_content_layout
				(client_size_, content_arrange, content_clip))
		{	
			content_->layout(content_arrange, content_clip);
		}
		else
			content_->clip_entirely();
	}
}


void gui_window::process_window_button_input
	(gui_input_event& input, gui_output_event_list& output)
{
	/**
	 * Check if the user clicked the close button of the window,
	 * then send back an appropriate notification.
	 */

	gui_output_event_list button_events;
	close_button_().process_input(input, button_events);

	for (gui_output_event_list::const_iterator it
			(button_events.first()); it; ++it)
	{
		if (!*it) continue;
		if (typeid(**it) != typeid(gui_button_event)) continue;
		if ((*it)->source_untyped != close_button_) continue;

		auto_pointer<gui_output_event> event
			(new gui_window_close_event(*this));
		output.insert
			(std::move(event));
	}
}


void gui_window::process_window_drag_input
	(gui_input_event& input, gui_output_event_list& output)
{
	/**
	 * Determine if we should start a new drag action,
	 * or if we should update an already running one.
	 * In either case, mark drag input as processed
	 * for subsequential content input handling.
	 */

	if (typeid(input) != typeid(gui_mouse_event))
		return;
	
	gui_mouse_event& mouse_event =
		static_cast<gui_mouse_event&>(input);

	if (layout_invalid() || !hot_areas_valid_)
	{
		stop_dragging();
		return;
	}


	// Update cursor to show resizing arrows
	// while resizing or over resizing areas.
	if (mouse_event.hot)
		try_apply_resizing_cursor(mouse_event.cursor);


	// While not dragging, consume hot click events
	// into the dragging areas and start dragging.
	// Acquire key focus to move window to front,
	// but do not steal focus from any child.
	if (drag_type_x_ == no_drag && drag_type_y_ == no_drag)
	{
		if (mouse_event.hot &&
			mouse_event.button == gui_mouse_event::left_button &&
			mouse_event.event == gui_mouse_event::button_click)
		{
			if (try_start_dragging(mouse_event.cursor))
			{
				if (!contains_key_focus())
					acquire_key_focus();

				mouse_event.hot = false;
			}
		}
	}


	// When dragging the window, consume all (hot or cold) events,
	// update window position and size, and stop on button release.
	// Finally, ensure that we keep key focus while dragging.
	if (drag_type_x_ != no_drag || drag_type_y_ != no_drag)
	{
		if (!contains_key_focus())
			acquire_key_focus();

		mouse_event.hot = false;

		update_dragging(mouse_event.cursor);

		if (mouse_event.button == gui_mouse_event::left_button &&
			mouse_event.event == gui_mouse_event::button_release)
				stop_dragging();
	}
}


void gui_window::process_window_background_input(gui_input_event& input)
{
	/**
	 * Capture any remaining hot input to prohibit
	 * the user from clicking through the window.
	 * On left background click, acquire key focus
	 * to become foreground in the window area,
	 * but do not steal focus from any children.
	 */

	if (typeid(input) != typeid(gui_mouse_event))
		return;
	
	gui_mouse_event& mouse_event =
		static_cast<gui_mouse_event&>(input);

	if (hot_areas_valid_ &&
		hot_area_resize_clipped_.contains(mouse_event.cursor))
	{
		if (mouse_event.hot &&
			mouse_event.button == gui_mouse_event::left_button &&
			mouse_event.event == gui_mouse_event::button_click &&
			!contains_key_focus())
		{
			acquire_key_focus();
		}

		mouse_event.hot = false;
	}
}


void gui_window::drag_types_from_cursor
	(const vector2f& cursor,
	 drag_type& drag_type_x, drag_type& drag_type_y) const NOTHROW
{
	/**
	 * Determine the hot area below a @bref{cursor} position,
	 * then return matching drag actions, or no_drag if there
	 * is no active hot area below the position.
	 */


	// Cannot drag on clipped hot areas.
	drag_type_x = no_drag;
	drag_type_y = no_drag;

	if (layout_invalid()) return;
	if (!hot_areas_valid_) return;


	// First, check hot area for window resizing.
	// Use a constant thickness that extends into this hot area.
	// Order is relevant, as resizing and movement areas overlap.
	// Do only allow resizing for resizeable windows.
	if (resize_policy == user_resize ||
		resize_policy == user_resize_enlarge)
	{
		float thickness =
			window_style().hot_area_resize_thickness;

		if (hot_area_resize_clipped_.contains(cursor) &&
			hot_area_resize_.size.min() >= 2 * thickness)
		{
			vector2f delta_pos(cursor - hot_area_resize_.pos);
			vector2f delta_end(hot_area_resize_.end() - cursor);

			drag_type_x =
				(delta_pos.x >= 0 && delta_pos.x <= thickness) ? drag_resize_neg :
				(delta_end.x >= 0 && delta_end.x <= thickness) ? drag_resize_pos :
					no_drag;

			drag_type_y =
				(delta_pos.y >= 0 && delta_pos.y <= thickness) ? drag_resize_neg :
				(delta_end.y >= 0 && delta_end.y <= thickness) ? drag_resize_pos :
					no_drag;
		}
	}


	// If we are not in the resizing area,
	// check hot area for window movement.
	if (drag_type_x == no_drag && drag_type_y == no_drag &&
		hot_area_move_clipped_.contains(cursor))
	{
		drag_type_x = drag_move;
		drag_type_y = drag_move;
	}
}


void gui_window::try_apply_resizing_cursor(const vector2f& cursor) const
{
	/**
	 * Indicate resizing action with suitable cursor
	 * while the mouse hovers over the resizing areas
	 * or while a resizing action is currently underway.
	 */


	// Avoid showing a redundant resizing cursor
	// when the cursor touches the window borders
	// during window movement.
	if (drag_type_x_ == drag_move || drag_type_y_ == drag_move)
		return;


	// If there currently is no ongoing dragging action,
	// determine what dragging area the cursor hovers over.
	drag_type drag_type_x = drag_type_x_;
	drag_type drag_type_y = drag_type_y_;

	if (drag_type_x == no_drag && drag_type_y == no_drag)
		drag_types_from_cursor(cursor, drag_type_x, drag_type_y);


	// Choose a suitable cursor style for ongoing
	// or hovered resizing actions.
	if ((drag_type_x == drag_resize_pos && drag_type_y == drag_resize_pos) ||
		(drag_type_x == drag_resize_neg && drag_type_y == drag_resize_neg))
	{
		gui.set_temporary_cursor_style(window_style().cursor_resize_ul_lr);
	}
	else if ((drag_type_x == drag_resize_pos && drag_type_y == drag_resize_neg) ||
			 (drag_type_x == drag_resize_neg && drag_type_y == drag_resize_pos))
	{
		gui.set_temporary_cursor_style(window_style().cursor_resize_ll_ur);
	}
	else if (drag_type_x == drag_resize_pos ||
			 drag_type_x == drag_resize_neg)
	{
		gui.set_temporary_cursor_style(window_style().cursor_resize_lr);
	}
	else if (drag_type_y == drag_resize_pos ||
			 drag_type_y == drag_resize_neg)
	{
		gui.set_temporary_cursor_style(window_style().cursor_resize_ud);
	}
}


bool gui_window::try_start_dragging(const vector2f& cursor)
{
	/**
	 * Try to start moving or resizing the window
	 * because of a click at a @bref{cursor} position.
	 * Returns @true if the cursor position corresponds
	 * to a valid action region, @false otherwise.
	 */
	

	// Check for click into any dragging area
	// and update current dragging action.
	drag_types_from_cursor
		(cursor, drag_type_x_, drag_type_y_);	
	
	if (drag_type_x_ == no_drag && drag_type_y_ == no_drag)
		return false;


	// Start either moving or resizing,
	// so store initial parameters.
	drag_start_ = cursor;
	drag_start_position_ = position_;
	drag_start_client_size_ = client_size_;
	return true;
}


void gui_window::update_dragging(const vector2f& cursor)
{
	/**
	 * Update window position and client size
	 * for current drag types due to dragging
	 * from @bref{drag_start_} to @bref{cursor}.
	 */

	vector2f drag_delta(cursor - drag_start_);

	vector2f position_influence
		(drag_type_x_ == drag_move ||
			drag_type_x_ == drag_resize_neg ? 1.f : 0.f,
		 drag_type_y_ == drag_move ||
			drag_type_y_ == drag_resize_neg ? 1.f : 0.f);

	vector2f client_size_influence
		(drag_type_x_ == drag_resize_neg ? -1.f :
			drag_type_x_ == drag_resize_pos ? 1.f : 0.f,
		 drag_type_y_ == drag_resize_neg ? -1.f :
			drag_type_y_ == drag_resize_pos ? 1.f : 0.f);

	set_position
		(drag_start_position_ +
		 drag_delta * position_influence);

	set_client_size
		(drag_start_client_size_ +
		 drag_delta * client_size_influence);

	enforce_position_and_size_constraints
		(drag_type_x_ == drag_resize_pos ||
			drag_type_x_ == drag_resize_neg,
		 drag_type_y_ == drag_resize_pos ||
			drag_type_y_ == drag_resize_neg,
		 drag_type_x_ != drag_resize_pos,
		 drag_type_y_ != drag_resize_pos);
}


void gui_window::stop_dragging()
{
	drag_type_x_ = no_drag;
	drag_type_y_ = no_drag;
}


void gui_window::enforce_position_and_size_constraints
	(bool resize_to_make_visible_x,
	 bool resize_to_make_visible_y,
	 bool resize_maintain_end_x,
	 bool resize_maintain_end_y)
{
	/**
	 * Fix relative window position and client size if these do not
	 * satisfy constraints, e.g. style-specific minimum client size,
	 * resizing policy, and minimum visible size of hot movement area.
	 *
	 * Parameters control how violated constraints are restored.
	 * In particular, if all parameters are @false, then updates
	 * keep the client size and change only the window position.
	 *
	 * Parameters are required to avoid usability bugs where a default
	 * restore operation might not undo the actual cause of violation
	 * (e.g. a window that incomprehensibly increases its client size
	 * on moving to maintain minimum visible size of the movement area
	 * instead of ignoring movement).
	 *
	 * Note that enforce_position_and_size_constraints() is required
	 * even for fixed-size windows (e.g. to ensure these stay visible).
	 * In this case, all resizing parameters are @false.
	 */


	vector2f target_position(position_);
	vector2f target_client_size(client_size_);


	// Update client size to match size constraints,
	// i.e. content size and min/max style-specific size.
	//
	// Notes:
	// - Style constraints override content constraints.
	// - We need not handle a fixed_size policy in here,
	//		as there is no separate size to synchronize to.
	if (resize_policy == match_content_size)
		target_client_size =
			(content_ ? content_->preferred_size() : vector2f());
	else if (resize_policy == user_resize_enlarge)
		target_client_size =
			target_client_size.max
				(content_ ? content_->preferred_size() : vector2f());

	target_client_size = 
		target_client_size.max
			(window_style().min_client_size).min
				(window_style().max_client_size);


	// Optionally, realize client size constraints by moving
	// the upper-left instead of the lower-right window corner.
	if (resize_maintain_end_x)
		target_position.x +=
			(client_size_.x - target_client_size.x);

	if (resize_maintain_end_y)
		target_position.y +=
			(client_size_.y - target_client_size.y);


	// Find hot movement area in absolute coordinates
	// from previously size-corrected window placement,
	// in particular from relative target_position.
	rect2f target_hot_move =
		rect2f(target_position + arrange_area().pos +
					window_style().hot_areas_padding_00,
			   target_client_size -
					(window_style().hot_areas_padding_00 +
					 window_style().hot_areas_padding_22));
	
	target_hot_move.size.y = 
		min(target_hot_move.size.y,
			window_style().hot_area_move_height);


	// Calculate the part of the (absolute) hot movement area
	// that remains visible in the (absolute) arrange area.
	// This disregards any other protruding parts of the window.
	float remainder_left = arrange_area().end_x() - target_hot_move.pos.x;
	float remainder_right = target_hot_move.end_x() - arrange_area().pos.x;
	float remainder_top = arrange_area().end_y() - target_hot_move.pos.y;	
	float remainder_bottom = target_hot_move.end_y() - arrange_area().pos.y;


	// Determine the minimum visible size required
	// for the hot movement area at its current size.
	// Notably, we cannot enforce the style-specified
	// minimum visible size of the hot movement area
	// if the hot movement area already is too small.
	// Otherwise, we would prohibit the user from moving
	// small windows to the borders of the arrange area.
	vector2f min_area_00
		(window_style().min_visible_hot_area_move_00.min
			(target_hot_move.size));
	vector2f min_area_22
		(window_style().min_visible_hot_area_move_00.min
			(target_hot_move.size));


	// Determine and apply position or size correction
	// to show the hot movement area in the arrange area
	// with at least the required minimum visible size.
	//
	// Notes:
	// - Order is vital here. We give the upper-left remainder
	//		priority over the lower-right one. This ensures
	//		that the window title remains visible even for
	//		small arrange areas.
	// - We need special handling for y coordinates.
	//		The hot movement area does not change with
	//		vertical size, so we always must adjust our
	//		position to satisfy visibility constraints.
	vector2f correction
		((remainder_left < min_area_00.x) ?
			-(min_area_00.x - remainder_left) :
		 (remainder_right < min_area_22.x) ?
			+(min_area_22.x - remainder_right) : 0.f,
		 (remainder_top < min_area_00.y) ?
			-(min_area_00.y - remainder_top) :
		 (remainder_bottom < min_area_22.y) ?
			+(min_area_22.y - remainder_bottom) : 0.f);

	vector2f correction_position_influence
		((resize_to_make_visible_x && correction.x > 0) ? 0.f : 1.f,
		 1.f);

	vector2f correction_client_size_influence
		(!resize_to_make_visible_x ? 0.f : (correction.x > 0) ? 1.f : -1.f,
		 !resize_to_make_visible_y ? 0.f : -1.f);

	target_position += correction * correction_position_influence;
	target_client_size += correction * correction_client_size_influence;


	// Apply constraint-corrected position and size.
	set_position(target_position);
	set_client_size(target_client_size);
}




//////////////////////////////////////////////////////////////////////////
//
// gui_window_style
//
//////////////////////////////////////////////////////////////////////////


gui_window_style::gui_window_style
	(const shared_instance<gui_frame_style>& default_style,
	 const shared_instance<gui_frame_style>& key_focus_overlay_style,

	 const vector2f& caption_offset,
	 float caption_height,
	 float caption_right_spacing,
	 float caption_button_spacing,

	 const shared_instance<gui_label_style>& caption_label_style,

	 const shared_instance<gui_button_style>& close_button_style,
	 const shared_instance<render_material_id>& close_icon,
	 const vector2i& close_icon_pixel_size,

	 const shared_instance<gui_cursor_style>& cursor_resize_lr,
	 const shared_instance<gui_cursor_style>& cursor_resize_ud,
	 const shared_instance<gui_cursor_style>& cursor_resize_ul_lr,
	 const shared_instance<gui_cursor_style>& cursor_resize_ll_ur,

	 const vector2f& hot_areas_padding_00,
	 const vector2f& hot_areas_padding_22,
	 float hot_area_move_height,
	 float hot_area_resize_thickness,

	 const vector2f& min_visible_hot_area_move_00,
	 const vector2f& min_visible_hot_area_move_22,
	 const vector2f& min_client_size,
	 const vector2f& max_client_size)
	:
	default_style(default_style),
	key_focus_overlay_style(key_focus_overlay_style),

	caption_offset(caption_offset),
	caption_height(caption_height),
	caption_right_spacing(caption_right_spacing),
	caption_button_spacing(caption_button_spacing),

	caption_label_style(caption_label_style),

	close_button_style(close_button_style),
	close_icon(close_icon),
	close_icon_pixel_size(close_icon_pixel_size),

	cursor_resize_lr(cursor_resize_lr),
	cursor_resize_ud(cursor_resize_ud),
	cursor_resize_ul_lr(cursor_resize_ul_lr),
	cursor_resize_ll_ur(cursor_resize_ll_ur),

	hot_areas_padding_00(hot_areas_padding_00),
	hot_areas_padding_22(hot_areas_padding_22),
	hot_area_move_height(hot_area_move_height),
	hot_area_resize_thickness(hot_area_resize_thickness),

	min_visible_hot_area_move_00(min_visible_hot_area_move_00),
	min_visible_hot_area_move_22(min_visible_hot_area_move_22),
	min_client_size(min_client_size),
	max_client_size(max_client_size)
{
	if (close_icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("Pixel size of close icon must be greater than zero.");
		throw invalid_parameter();
	}

	if (hot_area_move_height <= 0)
	{
		LOG_ERROR("Height of window moving area must be greater than zero.");
		throw invalid_parameter();
	}

	if (hot_area_resize_thickness <= 0)
	{
		LOG_ERROR("Thickness of window sizing area must be greater than zero.");
		throw invalid_parameter();	
	}

	if (min_visible_hot_area_move_00.min() < 0 ||
		min_visible_hot_area_move_22.min() < 0)
	{
		LOG_ERROR("Minimum visible hot area must not be negative.");
		throw invalid_parameter();	
	}

	if (min_client_size.min() <= 0)
	{
		LOG_ERROR("Minimum client size must be greater than zero.");
		throw invalid_parameter();
	}

	if ((max_client_size - min_client_size).min() < 0)
	{
		LOG_ERROR("Maximum client size must not be smaller than minimum client size.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
