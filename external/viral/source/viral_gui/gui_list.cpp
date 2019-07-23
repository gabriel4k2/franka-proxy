/**
 *************************************************************************
 *
 * @file gui_list.cpp
 *
 * Standalone GUI lists, implementation.
 *
 ************************************************************************/


#include "gui_list.hpp"


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>


#include "gui_event.hpp"
#include "gui_button.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_list
//
//////////////////////////////////////////////////////////////////////////


gui_list::gui_list
	(const string& name, gui_context& gui,
	 const shared_instance<gui_list_style>& list_style,
	 orientation_type orientation,
	 item_select_mode select_mode)
	:
	gui_element(name, gui),
	list_style(list_style),
	orientation(orientation),
	select_mode(select_mode),
	known_item_size_(list_style().min_item_size),
	known_items_per_stack_(1),
	synchronize_item_indices_pending_(true),
	select_sequence_start_(-1)
{ }


vector2f gui_list::preferred_size() const
{
	// Return size for only a single item per stack.
	// This enables higher-level containers to dynamically
	// adjust arrange_area() against orientation direction,
	// to which we react by adapting known_items_per_stack_.

	float stack_count =
		static_cast<float>
			(max(1, (items.size() + known_items_per_stack_ - 1) /
							known_items_per_stack_));

	return (orientation == horizontal) ?
				(known_item_size_ * vector2f(stack_count, 1)) :
				(known_item_size_ * vector2f(1, stack_count));
}


void gui_list::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// Let each item button process incoming input events,
	// then evaluate resulting button output events.
	// We ignore button output while our buttons are stale
	// to avoid mismatched input handling.
	for (item_button_map::const_iterator bit
			(item_buttons_.first()); bit; ++bit)
	{
		gui_button& button = bit->value();
		gui_output_event_list button_output;
		button.process_input(input, button_output);

		if (synchronize_item_indices_pending_)
			continue;

		for (gui_output_event_list::iterator eit
				(button_output.first()); eit; ++eit)
		{
			if (!*eit) continue;

			if (typeid(**eit) == typeid(gui_button_event))
				update_selection(input, bit->key, output);
			else if (typeid(**eit) == typeid(gui_button_repeat_event))
				create_list_accept_event(output);
		}
	}


	// Fetch any key focus acquired by current buttons.
	// Button key focus does not persist over realize_layout()
	// destroying and recreating buttons, which would cause
	// the overall GUI to lose focus.
	if (!has_key_focus() && contains_key_focus())
		acquire_key_focus();
}


void gui_list::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	for (item_button_map::const_iterator it
			(item_buttons_.first()); it; ++it)
	{
		it->value().commit_render(packer, queue);
	}
}


void gui_list::select_index(int64 index) NOTHROW
{
	for (int64 i = 0; i < items.size(); ++i)
	{
		if (!items[i]) continue;
		items[i]->selected = (i == index);
	}

	want_layout();
}


int64 gui_list::first_selected_index() const NOTHROW
{
	for (int64 i = 0; i < items.size(); ++i)
		if (items[i] && items[i]->selected)
			return i;

	return -1;
}


void gui_list::synchronize_item_indices() NOTHROW
{
	// Delete and recreate all buttons on next layout
	// and ignore any further input up to that point.
	synchronize_item_indices_pending_ = true;
	select_sequence_start_ = -1;
	want_layout();
}


vector2f gui_list::known_item_size() const NOTHROW
	{ return known_item_size_; }
int64 gui_list::known_items_per_stack() const NOTHROW
	{ return known_items_per_stack_; }


void gui_list::reset_known_item_size() NOTHROW
{
	known_item_size_ = list_style().min_item_size;
	want_layout();
}


void gui_list::realize_layout() 
{
	// Directly abort if we do not show any buttons,
	// either because the list got clipped or is empty.
	if (layout_invalid() || items.empty())
	{
		item_buttons_.clear();
		synchronize_item_indices_pending_ = false;
		return;
	}


	// If buttons are stale, delete these
	// to enforce button recreation later on.
	if (synchronize_item_indices_pending_)
		item_buttons_.clear();


	// Find index of first and last currently visible stack,
	// then find index of first and last visible item on these.
	// Notably, consider some items even if all are clipped.
	// Also ensure we do not show too many buttons.
	float first_visible_offset;
	float last_visible_offset;
	float stack_size;

	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());

	if (orientation == horizontal)
	{
		first_visible_offset = cl_area.pos.x - ar_area.pos.x;
		last_visible_offset = cl_area.end_x() - ar_area.pos.x;
		stack_size = known_item_size_.x;
	}
	else
	{
		first_visible_offset = cl_area.pos.y - ar_area.pos.y;
		last_visible_offset = cl_area.end_y() - ar_area.pos.y;
		stack_size = known_item_size_.y;
	}

	int64 first_visible_stack =
		integer_operations::cast_or_clamp<int64>
			(first_visible_offset / stack_size);
	int64 last_visible_stack =
		integer_operations::cast_or_clamp<int64>
			(last_visible_offset / stack_size);

	int64 first_visible_item =
		max(0, min(items.size() - 1,
			first_visible_stack * known_items_per_stack_));

	int64 last_visible_item =
		max(0, min(items.size() - 1,
			last_visible_stack * known_items_per_stack_ +
				(known_items_per_stack_ - 1)));

	if (1 + last_visible_item - first_visible_item > max_visible_buttons)
		last_visible_item = first_visible_item + max_visible_buttons - 1;
		

	// Drop buttons that became invisible,
	// e.g. due to scrolling or clipping.
	for (item_button_map::iterator it
			(item_buttons_.first()); it;)
	{
		item_button_map::iterator del_it(it);
		++it;

		if (del_it->key < first_visible_item ||
			del_it->key > last_visible_item)
				item_buttons_.remove(del_it);
	}


	// Synchronize buttons to visible items.
	// This may mean creating or dropping buttons,
	// as well as synchronizing button and item data.
	// Finally, layout() individual buttons.
	for (int64 i = first_visible_item; i <= last_visible_item; ++i)
	{
		// Ignore NULL items and drop any
		// previously associated button.
		if (!items[i])
		{
			item_buttons_.remove(i);
			continue;
		}

		gui_list_item& item = *items[i];


		// Item is valid, so create button
		// if it does not exist yet.
		item_button_map::iterator it(item_buttons_.find(i));

		if (!it)
		{
			shared_instance<gui_button> item_button
				(new gui_button
					(name + "_button_" + i, gui,
					 list_style().button_style,
					 gui_button::probe_button));

			insert_child(item_button);
			it = item_buttons_.insert(i, item_button);
		}

		gui_button& button = it->value();


		// Synchronize button to item content.
		button.set_text(item.text);

		if (item.icon)
			button.set_graphics
				(list_style().icon_pixel_size,
				 item.icon.instance_or_throw());
		else
			button.clear_graphics();

		button.set_down(item.selected);
		button.set_read_only(item.read_only);
	

		// Update button layout.
		// Build stacks from fixed-size buttons,
		// optionally distribute excess space.
		vector2f stack_position
			(orientation == horizontal ?
				vector2f(static_cast<float>(i / known_items_per_stack_),
						 static_cast<float>(i % known_items_per_stack_)) :
				vector2f(static_cast<float>(i % known_items_per_stack_),
						 static_cast<float>(i / known_items_per_stack_)));

		vector2f button_pos
			(ar_area.pos + known_item_size_ * stack_position);
		vector2f button_size
			(known_item_size_);

		if (list_style().allow_stretch)
		{
			vector2f excess_space_per_item
				((ar_area.size / static_cast<float>(known_items_per_stack_)
					- known_item_size_).max(vector2f()));

			if (orientation == horizontal)
			{
				button_pos.y += excess_space_per_item.y * stack_position.y;
				button_size.y += excess_space_per_item.y;
			}
			else
			{
				button_pos.x += excess_space_per_item.x * stack_position.x;
				button_size.x += excess_space_per_item.x;
			}
		}

		rect2f button_arrange(button_pos, button_size);
		rect2f button_clip;
		if (button_arrange.try_intersect(cl_area, button_clip))
			button.layout(button_arrange, button_clip);
		else
			button.clip_entirely();
	}


	// Buttons are now in synch to actual items.
	// Allow process_input() to run again.
	synchronize_item_indices_pending_ = false;


	// If any button wants more than the known_item_size_
	// and the list allows for growing, increase known size
	// and request new layout.
	if (list_style().allow_grow)
	{
		vector2f max_button_size;

		for (item_button_map::const_iterator it
				(item_buttons_.first()); it; ++it)
		{
			max_button_size =
				max_button_size.max(it->value().preferred_size());
		}

		if (max_button_size.x > known_item_size_.x ||
			max_button_size.y > known_item_size_.y)
		{
			known_item_size_ =
				known_item_size_.max(max_button_size);
			stay_dirty();
		}
	}


	// Potentially adjust item count per stack
	// to arrange area, then request re-layout().
	// Bounding known_items_per_steck_ to at least one
	// simplifies layout() and preferred_size().
	if (list_style().allow_stack)
	{
		int64 items_per_stack =
			max(1, min(items.size(), min(max_items_per_stack_,
				integer_operations::cast_or_clamp<int64>
					(orientation == horizontal ?
						(ar_area.size.y / known_item_size_.y) :
						(ar_area.size.x / known_item_size_.x)))));

		if (items_per_stack != known_items_per_stack_)
		{
			known_items_per_stack_ = items_per_stack;
			stay_dirty();
		}
	}
}


void gui_list::update_selection
	(const gui_input_event& input, int64 selection,
	 gui_output_event_list& output)
{
	if (select_mode == select_single)
		update_selection_single(input, selection);
	else if (select_mode == select_toggle)
		update_selection_toggle(input, selection);
	else if (select_mode == select_advanced)
		update_selection_advanced(input, selection);
	else
	{
		LOG_INFO("Unknown selection mode.");
		throw assert_failed();
	}

	// Aftermath: Create appropriate notification event,
	// and refresh down state of all buttons.
	create_list_event(output);
	want_layout();
}


void gui_list::update_selection_single
	(const gui_input_event& input, int64 selection)
{
	for (int64 i = 0; i < items.size(); ++i)
	{
		if (!items[i]) continue;
		items[i]->selected = (i == selection);
	}
}


void gui_list::update_selection_toggle
	(const gui_input_event& input, int64 selection)
{
	if (selection < 0 || selection >= items.size()) return;
	if (!items[selection]) return;
	items[selection]->selected =
		!items[selection]->selected;
}


void gui_list::update_selection_advanced
	(const gui_input_event& input, int64 selection)
{
	// Validate item index and selection start,
	// just in case items got modified without
	// a consecutive synchronize_item_indices().
	if (selection < 0 || selection >= items.size()) return;
	if (!items[selection]) return;

	if (select_sequence_start_ < 0 ||
		select_sequence_start_ >= items.size())
			select_sequence_start_ = -1;
	

	// Determine modifiers active while selecting.
	bool select_sequence = false;
	bool select_incremental = false;

	if (typeid(input) == typeid(gui_mouse_event))
	{
		const gui_mouse_event& mouse_event =
			static_cast<const gui_mouse_event&>(input);

		select_sequence = mouse_event.shift_down;
		select_incremental = mouse_event.ctrl_down;
	}

	if (select_sequence_start_ == -1)
		select_sequence = false;


	// Update selection accordingly.
	for (int64 i = 0; i < items.size(); ++i)
	{
		if (!items[i]) continue;

		bool in_sequence =
			select_sequence &&
			min(selection, select_sequence_start_) <= i &&
			max(selection, select_sequence_start_) >= i;

		bool select =
			in_sequence || i == selection;

		if (select)
			items[i]->selected = true;
		else if (!select_incremental)
			items[i]->selected = false;		
	}

	if (!select_sequence)
		select_sequence_start_ = selection;
}


void gui_list::create_list_event
	(gui_output_event_list& output)
{
	auto_pointer<gui_output_event> event
		(new gui_list_event(*this));
	output.push_back
		(std::move(event));
}


void gui_list::create_list_accept_event
	(gui_output_event_list& output)
{
	auto_pointer<gui_output_event> event
		(new gui_list_accept_event(*this));
	output.push_back
		(std::move(event));
}




//////////////////////////////////////////////////////////////////////////
//
// gui_list_style
//
//////////////////////////////////////////////////////////////////////////


gui_list_style::gui_list_style
	(const shared_instance<gui_button_style>& button_style,
	 const vector2f& min_item_size,
	 const vector2i& icon_pixel_size,
	 bool allow_grow,
	 bool allow_stack,
	 bool allow_stretch)
	:
	button_style(button_style),
	min_item_size(min_item_size),
	icon_pixel_size(icon_pixel_size),
	allow_grow(allow_grow),
	allow_stack(allow_stack),
	allow_stretch(allow_stretch)
{
	if (min_item_size.min() < 0)
	{
		LOG_ERROR("Minimum list item size must not be negative.");
		throw invalid_parameter();
	}

	if (icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("List icon size must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
