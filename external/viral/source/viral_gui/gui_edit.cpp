/**
 *************************************************************************
 *
 * @file gui_edit.cpp
 *
 * GUI text edit, implementation.
 *
 ************************************************************************/


#include "gui_edit.hpp"


#if defined(VIRAL__OS_TARGET_WINDOWS)
	#include <windows.h>
#endif


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>
#include <viral_core/timer.hpp>
#include <viral_core/input.hpp>
#include <viral_core/render_scene.hpp>


#include "gui.hpp"
#include "gui_event.hpp"
#include "gui_style_util.hpp"
#include "gui_render.hpp"
#include "gui_anchor.hpp"
#include "gui_label.hpp"
#include "gui_scroll_area.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_edit
//
//////////////////////////////////////////////////////////////////////////


gui_edit::gui_edit
	(const string& name, gui_context& gui,
	 const shared_instance<gui_edit_style>& edit_style,
	 bool multiline, bool notify_accept)
	:
	gui_element(name, gui),
	edit_style(edit_style),
	multiline(multiline),
	notify_accept(notify_accept),

	hot_area_valid_(false),

	read_only_(false),
	overwrite_mode_(false),

	lines_(edit_style().font_style().f()),
	selection_(edit_style().font_style().f(), lines_),
	kernel_(multiline, lines_, selection_),

	caret_dragging_(false),
	caret_visible_(false),
	caret_timer_(new free_timer()),
	delayed_scroll_to_caret_(false),

	known_line_width_(edit_style().min_line_width),

	line_anchor_style_
		(new gui_anchor_style(gui_anchor::anchor_neg, gui_anchor::anchor_neg,
							  gui_anchor::anchor_neg, gui_anchor::anchor_neg)),
	line_label_style_
		(new gui_label_style(edit_style().font_style, line_anchor_style_))
{ }


vector2f gui_edit::preferred_size() const
{
	float num_lines =
		static_cast<float>(lines_.line_count());

	return
		vector2f(edit_style().caret_width + known_line_width_,
				 edit_style().font_style().f().line_height * num_lines);
}


void gui_edit::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (layout_invalid() || !hot_area_valid_)
	{
		stop_caret_dragging();
		return;
	}

	if (typeid(input) == typeid(gui_tick_event))
	{
		process_tick_event
			(static_cast<gui_tick_event&>(input), output);
	}
	else if (typeid(input) == typeid(gui_mouse_event))
	{
		process_mouse_event
			(static_cast<gui_mouse_event&>(input), output);
	}
	else if (typeid(input) == typeid(gui_text_event))
	{
		process_text_event
			(static_cast<gui_text_event&>(input), output);
	}
}


void gui_edit::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	commit_render_selection(packer);

	for (line_label_map::const_iterator it
			(line_labels_.first()); it; ++it)
	{
		it->value().commit_render(packer, queue);
	}

	commit_render_caret(packer);
}


bool gui_edit::try_accept_key_focus() NOTHROW
{
	want_commit_render();
	restart_caret_blink();
	return true;
}

void gui_edit::lost_key_focus() NOTHROW
{
	want_commit_render();
	stop_caret_dragging();
}


void gui_edit::set_read_only(bool read_only) NOTHROW
{
	if (read_only_ == read_only) return;
	read_only_ = read_only;
	want_commit_render();
}

bool gui_edit::read_only() const NOTHROW
	{ return read_only_; }


void gui_edit::set_text(const string& text)
	{ set_text(text, false); }

void gui_edit::set_text
	(const string& text, bool allow_undo)
{
	// Simulate edit by user:
	// Select all text(), then replace with @bref{text}.
	// We standardize incoming text to \n line breaks,
	// and remove all line breaks on single-line edits.
	string new_text(text);
	new_text.remove_all('\r');
	if (!multiline) new_text.remove_all('\n');

	kernel_.execute
		(gui_edit_operation
			(vector2i(), gui_edit::text(), new_text,
			 selection_.caret_index(),
			 selection_.selection_start_index()));

	// Exception safety: NOTHROW from here.
	if (!allow_undo)
		kernel_.clear_history();

	want_layout();
	restart_caret_blink();
}


string gui_edit::text() const
{
	return lines_.text_from_indices
				(lines_.first_index(), lines_.last_index());
}


void gui_edit::append_text(const string& text)
	{ append_text(text, false); }

void gui_edit::append_text(const string& text, bool allow_undo)
{
	// Simulate edit by user:
	// Move cursor to end, then insert @bref{text}.
	// We standardize incoming text to \n line breaks,
	// and remove all line breaks on single-line edits.
	string append(text);
	append.remove_all('\r');
	if (!multiline) append.remove_all('\n');

	kernel_.execute
		(gui_edit_operation
			(lines_.last_index(), "", append,
			 lines_.last_index(), lines_.last_index()));

	// Exception safety: NOTHROW from here.
	if (!allow_undo)
		kernel_.clear_history();

	want_layout();
	restart_caret_blink();
}


void gui_edit::set_history_step_limit(int64 history_step_limit) NOTHROW
	{ kernel_.set_history_step_limit(history_step_limit); }
int64 gui_edit::history_step_limit() const NOTHROW
	{ return kernel_.history_step_limit(); }

void gui_edit::set_history_memory_limit(int64 history_memory_limit) NOTHROW
	{ kernel_.set_history_memory_limit(history_memory_limit); }
int64 gui_edit::history_memory_limit() const NOTHROW
	{ return kernel_.history_memory_limit(); }


float gui_edit::known_line_width() const NOTHROW
	{ return known_line_width_; }

void gui_edit::reset_known_line_width() NOTHROW
{
	known_line_width_ = edit_style().min_line_width;
	want_layout();
}


void gui_edit::realize_layout()
{
	// Directly abort if we got clipped entirely.
	if (layout_invalid())
	{
		line_labels_.clear();
		return;
	}


	// Build hot area from arrange and clip areas.
	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());

	hot_area_valid_ =
		ar_area.try_intersect(cl_area, hot_area_clipped_);


	// Find index of first and last currently visible line.
	// Ensure that first_visible and last_visible are valid.
	// Notably, consider some lines even if all are clipped.
	// Also ensure we do not show too many lines.
	float first_visible_offset = cl_area.pos.y - ar_area.pos.y;
	float last_visible_offset = cl_area.end_y() - ar_area.pos.y;
	float line_height = edit_style().font_style().f().line_height;

	int64 first_visible =
		max(0, min(lines_.line_count() - 1,
			integer_operations::cast_or_clamp<int64>
				(first_visible_offset / line_height)));

	int64 last_visible =
		max(0, min(lines_.line_count() - 1,
			integer_operations::cast_or_clamp<int64>
				(last_visible_offset / line_height)));

	if (1 + last_visible - first_visible > max_visible_lines)
		last_visible = first_visible + max_visible_lines - 1;
		

	// Drop labels for lines that currently are not visible,
	// e.g. due to scrolling or clipping.
	for (line_label_map::iterator it(line_labels_.first()); it;)
	{
		line_label_map::iterator del_it(it);
		++it;

		if (del_it->key < first_visible ||
			del_it->key > last_visible)
				line_labels_.remove(del_it);
	}


	// Synchronize line labels to visible lines.
	for (int64 i = first_visible; i <= last_visible; ++i)
	{

		// Update existing label, or create new label
		// if label does not exist yet.
		line_label_map::iterator it(line_labels_.find(i));

		if (it)
		{
			it->value().set_text(lines_.line(i));
		}
		else
		{
			shared_instance<gui_label> line_label
				(new gui_label
					(name + "_label_" + i, gui,
					 line_label_style_, lines_.line(i)));

			insert_child(line_label);
			it = line_labels_.insert(i, line_label);
		}


		// Finally, provide layout for label, or clip entirely
		// if part of tolerance area beyond clipping area.
		rect2f label_arrange
			(ar_area.pos +
			 vector2f(0, line_height) * static_cast<float>(i),
			 it->value().preferred_size());

		rect2f label_clip;
		if (label_arrange.try_intersect(cl_area, label_clip))
			it->value().layout(label_arrange, label_clip);
		else
			it->value().clip_entirely();
	}


	// Find the currently visible label that requires
	// most horizontal space, use either that space
	// or at least the style-specific minimum line width.
	int64 max_line_index = -1;
	float max_line_width = 0;

	for (line_label_map::const_iterator it
			(line_labels_.first()); it; ++it)
	{
		float line_width =
			it->value().preferred_size().x;

		if (max_line_index == -1 ||
			line_width > max_line_width)
		{
			max_line_index = it->key;
			max_line_width = line_width;
		}
	}

	max_line_width =
		max(max_line_width, edit_style().min_line_width);


	// If the widest label warrants a different known_line_width_,
	// update cached line size and request a new layout.
	// Allow reducing known width only for single-line edits.
	bool changed_known_line_width = false;

	if (max_line_index != -1 &&
		max_line_width != known_line_width_)
	{
		if (!multiline || max_line_width > known_line_width_)
		{	
			known_line_width_ = max_line_width;
			stay_dirty();
			changed_known_line_width = true;
		}
	}


	// If we update known_line_width, the caret may jump out of view.
	// Prohibit this effect by scrolling to the caret later on.
	if (changed_known_line_width)
	{
		if (line_labels_.contains(selection_.caret_index().y))
			delayed_scroll_to_caret_ = true;
	}
	else if (delayed_scroll_to_caret_)
	{
		scroll_to_caret();
		stay_dirty();
		delayed_scroll_to_caret_ = false;		
	}
}


void gui_edit::process_tick_event
	(gui_tick_event& tick_event,
	 gui_output_event_list& output)
{
	if (!hot_area_valid_) return;
	if (!has_key_focus()) return;


	// Let caret blink while editing.
	if (caret_timer_->seconds_passed() > edit_style().caret_blink_seconds)
	{
		caret_visible_ = !caret_visible_;
		want_commit_render();

		caret_timer_->restart();	
	}
}


void gui_edit::process_mouse_event
	(gui_mouse_event& mouse_event,
	 gui_output_event_list& output)
{
	if (!hot_area_valid_) return;


	// Abort caret dragging and reset keyboard focus
	// on (potentially cold) clicks outside the hot area.
	// No need to pass focus to parent, as another element
	// will possibly acquire key focus now.
	if (!hot_area_clipped_.contains(mouse_event.cursor) &&
		mouse_event.event == mouse_event.button_click &&
		mouse_event.button == mouse_event.left_button)
	{
		release_key_focus();
		stop_caret_dragging();
		return;
	}


	// Consume mouse input within hot area.
	// Show caret cursor to indicate editing options.
	if (mouse_event.hot &&
		hot_area_clipped_.contains(mouse_event.cursor))
	{
		gui.set_temporary_cursor_style(edit_style().caret_cursor);
		mouse_event.hot = false;
	

		// Move caret and start dragging
		// on hot click in editing area.
		if (mouse_event.event == mouse_event.button_click &&
			mouse_event.button == mouse_event.left_button)
		{
			acquire_key_focus();

			if (!caret_dragging_)
				start_caret_dragging(mouse_event.cursor);
		}
	}
	

	// Update dragging with all mouse events
	// until we are released or lose key focus.
	if (caret_dragging_)
	{
		update_caret_dragging(mouse_event.cursor);

		if (mouse_event.event == mouse_event.button_release &&
			mouse_event.button == mouse_event.left_button)
				stop_caret_dragging();
	}
}


void gui_edit::process_text_event
	(gui_text_event& text_event,
	 gui_output_event_list& output)
{
	if (!hot_area_valid_) return;
	if (!has_key_focus()) return;
	if (!text_event.hot) return;
	if (caret_dragging_) return;


	// Fetch event data.
	text_input_event::extended_event_type extended_event =
		text_event.text_input.extended_event;
	char localized_char =
		text_event.text_input.localized_char;
	int64 repeat_count =
		text_event.text_input.repeat_count;

	bool shift_down =
		(text_event.text_input.modifiers &
			text_input_event::modifier_shift) != 0;
	bool control_down =
		(text_event.text_input.modifiers &
			text_input_event::modifier_control) != 0;


	// Send event to appropriate handler.
	// If multiple events are generated through auto-repeat,
	// process these as separate method calls for simplicity.
	// Maximum repeat count is bounded to guard against
	// input_context bugs or user misbehaviour.
	int64 repeat_bound =
		max(0, min(repeat_count, max_key_repeat_));

	bool selection_changed = false;
	bool content_changed = false;
	bool had_return_event = false;
	bool had_escape_event = false;

	for (int64 i = 0; i < repeat_bound; ++i)
	{

		if (control_down)
		{
			// Word-wise caret movement with control key.
			if (extended_event == text_input_event::cursor_left_event)
			{
				selection_.move_caret_wordwise(-1, shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::cursor_right_event)
			{
				selection_.move_caret_wordwise(1, shift_down);
				selection_changed = true;
			}


			// Handle editing shortcuts: copy, cut, paste, redo, undo.
			// For read-only gui_edit elements, only copying is allowed.
			if (extended_event == text_input_event::no_extended_event)
			{
				if (localized_char == 'C') copy();
			}

			if (!read_only_)
			{
				if (extended_event == text_input_event::no_extended_event)
				{
					if (localized_char == 'X')
					{
						cut();
						content_changed = true;
					}

					if (localized_char == 'V')
					{
						paste();
						content_changed = true;
					}

					if (localized_char == 'Z')
					{
						kernel_.undo_history();
						content_changed = true;
					}

					if (localized_char == 'Y')
					{
						kernel_.redo_history();
						content_changed = true;
					}
				}					
			}
		}
		else
		{
			// Single-letter and single-line caret movement.
			if (extended_event == text_input_event::cursor_left_event)
			{
				selection_.move_caret_horizontal(-1, shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::cursor_right_event)
			{
				selection_.move_caret_horizontal(1, shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::cursor_up_event)
			{
				selection_.move_caret_vertical(-1, shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::cursor_down_event)
			{
				selection_.move_caret_vertical(1, shift_down);
				selection_changed = true;
			}


			// Special caret movement.
			else if (extended_event == text_input_event::home_event)
			{
				selection_.move_caret_to_line_start(shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::end_event)
			{
				selection_.move_caret_to_line_end(shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::page_up_event)
			{
				selection_.move_caret_by_page(clip_area().size.y, -1, shift_down);
				selection_changed = true;
			}
			else if (extended_event == text_input_event::page_down_event)
			{
				selection_.move_caret_by_page(clip_area().size.y, 1, shift_down);
				selection_changed = true;
			}


			// Relinquish focus on escape.
			else if (extended_event == text_input_event::escape_event)
			{
				cancel_editing();
				selection_changed = true;
				had_escape_event = true;
			}


			// Text editing: Insert localized chars, delete, toggle overwrite.
			// Prohibit editing in read-only edit boxes.
			if (!read_only_)
			{
				if (extended_event == text_input_event::no_extended_event)
				{
					insert_char(localized_char);
					content_changed = true;
				}
				else if (extended_event == text_input_event::backspace_event)
				{
					delete_left();
					content_changed = true;
				}
				else if (extended_event == text_input_event::return_event)
				{
					// Prohibit enter on single-line edit boxes
					// to suppress the warning for ignored newline.
					if (multiline)
					{
						insert_char('\n');
						content_changed = true;
					}

					had_return_event = true;
				}
				else if (extended_event == text_input_event::space_event)
				{
					insert_char(' ');
					content_changed = true;
				}
				else if (extended_event == text_input_event::insert_event)
				{
					toggle_overwrite_mode();
					selection_changed = true;
				}
				else if (extended_event == text_input_event::delete_event)
				{
					delete_right();
					content_changed = true;
				}
			}
		}
	}


	// If content has changed, request new layout and
	// delayed caret scrolling, as parent scroll areas
	// may have to adapt to a different preferred_size().
	// Otherwise, a rendering update is sufficient.
	if (content_changed)
	{
		want_layout();
		delayed_scroll_to_caret_ = true;
		restart_caret_blink();
	}
	else if (selection_changed)
	{
		want_commit_render();
		scroll_to_caret();
		restart_caret_blink();
	}


	// If the incoming keyboard event potentially changed
	// our content, send a confirmation to the client.
	// Additionally, send accept notification on return
	// and cancel on escape.
	if (content_changed)
	{
		auto_pointer<gui_output_event> event
			(new gui_edit_event(*this));
		output.push_back
			(std::move(event));
	}

	if (had_return_event)
	{
		auto_pointer<gui_output_event> event
			(new gui_edit_accept_event(*this));
		output.push_back
			(std::move(event));
	}

	if (had_escape_event)
	{
		auto_pointer<gui_output_event> event
			(new gui_edit_cancel_event(*this));
		output.push_back
			(std::move(event));
	}
}


void gui_edit::cancel_editing()
{
	if (!has_key_focus())
		return;

	// There is no cycle here with gui_editbox,
	// as the next higher parent in this case
	// is a gui_scroll_area element.
	release_key_focus_to_parent();

	stop_caret_dragging();
}


void gui_edit::insert_char(char chr)
{
	if (overwrite_mode_ &&
		selection_.empty() &&
		selection_.caret_index().x != lines_.line_size(selection_.caret_index()))
	{
		// In overwrite mode, assume a one-character selection
		// that extends over the next character after the caret.
		vector2i next_index
			(lines_.find_next_index(selection_.caret_index()));

		kernel_.execute(gui_edit_operation
			(selection_.first_selected_index(),
			 lines_.text_from_indices(selection_.caret_index(), next_index),
			 string(chr),
			 selection_.caret_index(), selection_.selection_start_index()));
	}
	else
	{
		kernel_.execute(gui_edit_operation
			(selection_.first_selected_index(),
			 selection_.selected_text(), string(chr),
			 selection_.caret_index(), selection_.selection_start_index()));
	}
}


void gui_edit::toggle_overwrite_mode()
	{ overwrite_mode_ = !overwrite_mode_; }


void gui_edit::delete_left()
{
	if (!selection_.empty())
	{
		delete_selection();
		return;
	}

	vector2i prev_index
		(lines_.find_prev_index(selection_.caret_index()));

	if (prev_index == selection_.caret_index())
		return;

	kernel_.execute
		(gui_edit_operation
			(prev_index,
			 lines_.text_from_indices(prev_index, selection_.caret_index()), "",
			 selection_.caret_index(), selection_.selection_start_index()));
}


void gui_edit::delete_right()
{
	if (!selection_.empty())
	{
		delete_selection();
		return;
	}

	vector2i next_index
		(lines_.find_next_index(selection_.caret_index()));

	if (next_index == selection_.caret_index())
		return;

	kernel_.execute
		(gui_edit_operation
			(selection_.caret_index(), 
			 lines_.text_from_indices(selection_.caret_index(), next_index), "",
			 selection_.caret_index(), selection_.selection_start_index()));
}


void gui_edit::delete_selection()
{
	if (selection_.empty()) return;

	kernel_.execute
		(gui_edit_operation
			(selection_.first_selected_index(),
			 selection_.selected_text(), "",
			 selection_.caret_index(), selection_.selection_start_index()));
}


void gui_edit::cut()
{
	if (selection_.empty()) return;
	copy();
	delete_selection();
}


#if defined(VIRAL__OS_TARGET_WINDOWS)


	void gui_edit::copy()
	{
		// Prepare selection line-ends for MS Windows.		
		list<string> lines;
		selection_.selected_text().split('\n', lines);
		
		string selection;
		for (list<string>::const_iterator it(lines.first()); it; ++it)
		{
			if (it != lines.first()) selection += "\r\n";
			selection += *it;
		}


		// Copy current text selection to global memory.
		SIZE_T checked_string_size =
			integer_operations::cast_or_throw<SIZE_T>
				(integer_operations::add_or_throw
					(selection.size(), 1));

		HANDLE clipboard_data_handle =
			GlobalAlloc(GMEM_MOVEABLE, checked_string_size);

		if (!clipboard_data_handle)
		{
			LOG_ERROR("Failed to allocate clipboard memory.");
			return;
		}

		SCOPE_GUARD
			(free_clipboard_data_handle_guard,
				(&GlobalFree, clipboard_data_handle));

		void* clipboard_data_pointer =
			GlobalLock(clipboard_data_handle);

		if (!clipboard_data_pointer)
		{
			LOG_ERROR("Failed to lock clipboard data.");
			return;
		}

		memory::copy_block_typed
			(reinterpret_cast<char*>(clipboard_data_pointer),
			 selection.data(),
			 selection.size() + 1);

		GlobalUnlock(clipboard_data_handle);


		// Attach global memory with text selection to system clipboard.
		if (OpenClipboard(NULL) == 0)
		{
			LOG_ERROR("Failed to open clipboard.");
			return;
		}

		SCOPE_GUARD
			(close_clipboard_guard, (&CloseClipboard));

		if (EmptyClipboard() == 0)
		{
			LOG_ERROR("Failed to empty clipboard.");
			return;
		}

		if (!SetClipboardData(CF_TEXT, clipboard_data_handle))
		{
			LOG_ERROR("Failed to attach text to clipboard.");
			return;
		}

		free_clipboard_data_handle_guard.dismiss(); // Clipboard owns memory now.
	}


	void gui_edit::paste()
	{
		// Open clipboard and fetch text data, if any.
		if (OpenClipboard(NULL) == 0)
		{
			LOG_ERROR("Failed to open clipboard.");
			return;
		}

		SCOPE_GUARD
			(close_clipboard_guard, (&CloseClipboard));

		HANDLE clipboard_data_handle = GetClipboardData(CF_TEXT);

		if (!clipboard_data_handle)
		{
			LOG_WARN("Clipboard contains no text data.");
			return;
		}

		
		// Lock data and copy to string representation.
		void* clipboard_data_pointer =
			GlobalLock(clipboard_data_handle);

		if (!clipboard_data_pointer)
		{
			LOG_ERROR("Clipboard text data pointer is NULL.");
			return;
		}

		SCOPE_GUARD
			(unlock_clipboard_data_guard,
				(&GlobalUnlock, clipboard_data_handle));

		string clipboard_content
			(static_cast<const char*>(clipboard_data_pointer));


		// Unify line ends, then apply as edit operation.
		clipboard_content.remove_all('\r');
		if (!multiline) clipboard_content.remove_all('\n');

		kernel_.execute
			(gui_edit_operation
				(selection_.first_selected_index(),
				 selection_.selected_text(), clipboard_content,
				 selection_.caret_index(), selection_.selection_start_index()));
	}


#else

	void gui_edit::copy()
		{ LOG_ERROR("Copy not implemented for current build target."); }
	void gui_edit::paste()
		{ LOG_ERROR("Paste not implemented for current build target."); }

#endif


void gui_edit::start_caret_dragging(const vector2f& cursor)
{
	if (layout_invalid()) return;
	if (caret_dragging_) return;
	
	vector2f relative_cursor(cursor - arrange_area().pos);

	selection_.set_caret_index
		(lines_.relative_position_to_index(relative_cursor),
		 true, true);

	caret_dragging_ = true;

	want_commit_render();
	scroll_to_caret();
	restart_caret_blink();
}

void gui_edit::update_caret_dragging(const vector2f& cursor)
{
	if (layout_invalid()) return;
	if (!caret_dragging_) return;

	vector2f relative_cursor
		(cursor - arrange_area().pos);
	vector2i new_caret_index
		(lines_.relative_position_to_index(relative_cursor));

	if (new_caret_index != selection_.caret_index())
	{
		selection_.set_caret_index
			(new_caret_index, false, true);

		want_commit_render();
		scroll_to_caret();
		restart_caret_blink();
	}
}

void gui_edit::stop_caret_dragging()
	{ caret_dragging_ = false; }


rect2f gui_edit::build_caret_extents() NOTHROW
{
	if (!hot_area_valid_) return rect2f();

	bool show_overwrite_caret =
		selection_.empty() &&
		overwrite_mode_ &&
		selection_.caret_index().x < lines_.line_size(selection_.caret_index());

	float overwrite_width =
		lines_.index_to_relative_position
			(lines_.find_next_index(selection_.caret_index())).x -
		lines_.index_to_relative_position
			(selection_.caret_index()).x;

	return rect2f
		(lines_.index_to_relative_position(selection_.caret_index()) +
				arrange_area().pos,
		 vector2f(show_overwrite_caret ?
						max(overwrite_width, edit_style().caret_overwrite_width) :
						edit_style().caret_width,
				  edit_style().font_style().f().line_height));
}


void gui_edit::scroll_to_caret() NOTHROW
{
	// Find first containing gui_scroll_area (if any).
	gui_scroll_area *scroll_area = NULL;

	gui_element* p = parent();
	while (p)
	{
		if (typeid(*p) == typeid(gui_scroll_area))
		{
			scroll_area = static_cast<gui_scroll_area*>(p);
			break;
		}
		
		p = p->parent();
	}


	// Scroll to caret extents in our gui_scroll_area.
	if (!scroll_area ||
		 scroll_area->layout_invalid())
			return;

	rect2f caret_extents(build_caret_extents());
	caret_extents.pos =
		scroll_area->to_scroll_space(caret_extents.pos);

	scroll_area->scroll_page_to(caret_extents);
}


void gui_edit::restart_caret_blink() NOTHROW
{
	if (!caret_visible_)
		caret_visible_ = true;

	// Consume any timer exceptions to ensure NOTHROW
	// for use in other methods, e.g. set_caret_index()
	// or try_accept_key_focus().
	try { caret_timer_->restart(); }
	catch (...)
		{ LOG_ERROR("Exception on restart of caret timer."); }
}


void gui_edit::commit_render_selection
	(gui_render_packer& packer)
{
	if (!hot_area_valid_) return;
	if (!has_key_focus()) return;
	if (selection_.empty()) return;


	// Fetch selection.
	vector2i first_index(selection_.first_selected_index());
	vector2i last_index(selection_.last_selected_index());


	// Build rectangles for selected portions
	// on each selected line.
	dynamic_buffer<rect2f> extents;
	dynamic_buffer<rect2f> texcoords;

	for (int64 y = first_index.y; y <= last_index.y; ++y)
	{
		vector2f start
			(lines_.index_to_relative_position
				(y == first_index.y ?
					first_index :
					vector2i(0, y)));

		vector2f end
			(lines_.index_to_relative_position
				(y == last_index.y ?
					last_index :
					vector2i(lines_.line_size(y), y)));

		vector2f size
			(end.x - start.x,
			 edit_style().font_style().f().line_height);


		// Translate to GUI coordinates.
		start += arrange_area().pos;
		end += arrange_area().pos;


		// Add some whitespace as marker for newline
		// characters on completely selected lines.
		if (y < last_index.y)
		{
			font::letter_map::const_iterator it
				(edit_style().font_style().f().letters.find(' '));

			if (it)
				size.x += it->value.increment;
		}


		extents.push_back(rect2f(start, size));
		texcoords.push_back(rect2f(0, 0, 1, 1));
	}


	// Build clipped geometry for all selection rectangles,
	// then send results to the render packer.
	mesh msh
		(gui_render_util::create_mesh_from_rects
			(extents.data(), texcoords.data(), extents.size(),
			 clip_area(), true, gui.pixel_integral_rendering));

	packer.pack_mesh
		(msh, edit_style().selection_material);
}


void gui_edit::commit_render_caret
	(gui_render_packer& packer)
{
	if (!hot_area_valid_) return;
	if (!has_key_focus()) return;
	if (!caret_visible_) return;

	mesh msh
		(gui_render_util::create_mesh_from_rect
			(build_caret_extents(), rect2f(0, 0, 1, 1),
			 clip_area(), true, gui.pixel_integral_rendering));

	packer.pack_mesh
		(msh, edit_style().caret_material);
}


const int64 gui_edit::max_key_repeat_ = 32;




//////////////////////////////////////////////////////////////////////////
//
// gui_edit_style
//
//////////////////////////////////////////////////////////////////////////


gui_edit_style::gui_edit_style
	(const shared_instance<gui_font_style>& font_style,
	 float min_line_width,
	 const shared_instance<render_material_id>& selection_material,
	 const shared_instance<render_material_id>& caret_material,
	 float caret_width,
	 float caret_overwrite_width,
	 float caret_blink_seconds,
	 const shared_instance<gui_cursor_style>& caret_cursor)
	:
	font_style(font_style),
	min_line_width(min_line_width),
	selection_material(selection_material),
	caret_material(caret_material),
	caret_width(caret_width),
	caret_overwrite_width(caret_overwrite_width),
	caret_blink_seconds(caret_blink_seconds),
	caret_cursor(caret_cursor)
{
	if (min_line_width <= 0)
	{
		LOG_ERROR("Minimum line width must be greater than zero.");
		throw invalid_parameter();
	}

	if (caret_width <= 0 || caret_overwrite_width <= 0)
	{
		LOG_ERROR("Caret widths must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
