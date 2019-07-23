/**
 *************************************************************************
 *
 * @file gui_edit_kernel.cpp
 *
 * Editing kernel for gui_edit, implementation.
 *
 ************************************************************************/


#include "gui_edit_kernel.hpp"


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>


#include "gui_edit_lines.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_edit_operation
//
//////////////////////////////////////////////////////////////////////////


gui_edit_operation::gui_edit_operation
	(const vector2i& text_index,
	 const string& current_text,
	 const string& new_text,
	 const vector2i& undo_caret_index,
	 const vector2i& undo_selection_start_index)
	:
	text_index(text_index),
	current_text(current_text),
	new_text(new_text),
	undo_caret_index(undo_caret_index),
	undo_selection_start_index(undo_selection_start_index),

	memory_usage(find_memory_usage(current_text, new_text))
{ }


int64 gui_edit_operation::find_memory_usage
	(const string& current_text, const string& new_text)
{
	/**
	 * All below operations clamp (instead of throwing)
	 * so that we at least allow a single editing operation
	 * of arbitrarily long content.
	 */

	int64 text_size =
		integer_operations::add_or_clamp
			(current_text.size(), new_text.size());

	int64 memory_usage =
		integer_operations::add_or_clamp
			(text_size, size_of<gui_edit_operation>());

	return min(memory_usage, gui_edit_kernel::max_history_memory);
}




//////////////////////////////////////////////////////////////////////////
//
// gui_edit_kernel
//
//////////////////////////////////////////////////////////////////////////


gui_edit_kernel::gui_edit_kernel
	(bool multiline,
	 gui_edit_lines& lines,
	 gui_edit_selection& selection)
	:
	multiline_(multiline),
	lines_(lines),
	selection_(selection),
	history_step_limit_(default_history_steps),
	history_memory_limit_(default_history_memory)
{ }


void gui_edit_kernel::execute
	(const gui_edit_operation& edit_operation)
{
	// Pre-check the edit operation for validity
	// before inserting into history. This avoids
	// clearing our history on invalid edits.
	if (!validate_edit_operation(edit_operation))
	{
		LOG_WARN("Editing would exceed maximum line count or line length.");
		return;
	}

	SCOPE_GUARD
		(clear_history_on_exception,
			(*this, &gui_edit_kernel::clear_history));


	// First remove any previously undone actions from history,
	// then push new action to history, finally redo.
	while (history_.last() != history_last_step_)
		history_.pop_back();

	history_.push_back(edit_operation);
	redo_history();


	// Editing succeeded.
	// Exception safety: NOTHROW from here.
	apply_history_limit();
	clear_history_on_exception.dismiss();
}


void gui_edit_kernel::redo_history()
{
	SCOPE_GUARD
		(clear_history_on_exception,
			(*this, &gui_edit_kernel::clear_history));

	history_list::iterator history_next_step = history_last_step_;

	if (history_next_step) ++history_next_step;
		else history_next_step = history_.first();

	if (history_next_step)
	{
		apply_edit_operation(*history_next_step, false);

		// Exception safety: NOTHROW from here.	
		history_last_step_ = history_next_step;
	}

	clear_history_on_exception.dismiss();
}


void gui_edit_kernel::undo_history()
{
	SCOPE_GUARD
		(clear_history_on_exception,
			(*this, &gui_edit_kernel::clear_history));

	if (history_last_step_)
	{
		// Reuse the gui_edit_operation from history,
		// but flip the roles of @bref{current_text}
		// and @bref{new_text} to undo the operation.
		apply_edit_operation
			(gui_edit_operation
				(history_last_step_->text_index,
				 history_last_step_->new_text,
				 history_last_step_->current_text,
				 history_last_step_->undo_caret_index,
				 history_last_step_->undo_selection_start_index), true);

		// Exception safety: NOTHROW from here.
		--history_last_step_;
	}

	clear_history_on_exception.dismiss();
}


void gui_edit_kernel::clear_history() NOTHROW
{
	history_.clear();
	history_last_step_ = history_list::iterator();
}


void gui_edit_kernel::set_history_step_limit(int64 history_step_limit) NOTHROW
{
	history_step_limit_ =
		max(0, min(history_step_limit, max_history_steps));

	apply_history_limit();
}

int64 gui_edit_kernel::history_step_limit() const NOTHROW
	{ return history_step_limit_; }


void gui_edit_kernel::set_history_memory_limit(int64 history_memory_limit) NOTHROW
{
	history_memory_limit_ =
		max(0, min(history_memory_limit, max_history_memory));

	apply_history_limit();
}

int64 gui_edit_kernel::history_memory_limit() const NOTHROW
	{ return history_memory_limit_; }


void gui_edit_kernel::prepare_edit_operation
	(const gui_edit_operation& edit_operation,
	 list<string>& remove_lines, list<string>& insert_lines, 
 	 vector2i& first_remove_index, vector2i& last_remove_index,
	 vector2i& last_insert_index)
{
	/**
	 * Prepare an @bref{edit_operation} by finding
	 * initial and resulting lines.
	 */


	// Find line fragments remaining
	// after removing current_text.
	remove_lines.clear();
	edit_operation.current_text.split('\n', remove_lines);

	if (remove_lines.empty())
	{
		LOG_ERROR("Splitting former lines gave empty list.");
		throw assert_failed();
	}

	first_remove_index = edit_operation.text_index;
	last_remove_index = vector2i
		(remove_lines.last()->size() +
			(remove_lines.size() == 1 ? first_remove_index.x : 0),
		 first_remove_index.y + remove_lines.size() - 1);

	string left_fragment
		(lines_.line(first_remove_index).left(first_remove_index.x));
	string right_fragment
		(lines_.line(last_remove_index).substring(last_remove_index.x));


	// Build final inserted lines.
	insert_lines.clear();
	edit_operation.new_text.split('\n', insert_lines);

	if (insert_lines.empty())
	{
		LOG_ERROR("Splitting new lines gave empty list.");
		throw assert_failed();
	}

	last_insert_index = vector2i
		(insert_lines.last()->size() +
			((insert_lines.size() == 1) ? left_fragment.size() : 0),
		 first_remove_index.y + insert_lines.size() - 1);

	(*insert_lines.first()) =
		left_fragment + (*insert_lines.first());
	(*insert_lines.last()) =
		(*insert_lines.last()) + right_fragment;
}


bool gui_edit_kernel::validate_edit_operation
	(const gui_edit_operation& edit_operation)
{
	/**
	 * Ensure that some incoming @bref{edit_operation}
	 * does not violate invariants of the edit element.
	 * 
	 * @attent Only validated edit operations may be
	 * executed or added to the edit history.
	 */

	list<string> remove_lines, insert_lines;
	vector2i first_remove_index, last_remove_index;
	vector2i last_insert_index;

	prepare_edit_operation
		(edit_operation, remove_lines, insert_lines,
		 first_remove_index, last_remove_index, last_insert_index);

	for (list<string>::const_iterator it
			(insert_lines.first()); it; ++it)
	{
		if (it->size() > max_line_length) return false;
	}

	if (!integer_checks::is_add_safe
				(lines_.line_count(), insert_lines.size()))
	{
		return false;
	}

	int64 new_line_count =
		lines_.line_count() + insert_lines.size() - remove_lines.size();

	if (new_line_count < 1 || new_line_count > max_line_count)
		return false;

	if (!multiline_ && new_line_count != 1)
		return false;

	return true;
}


void gui_edit_kernel::apply_edit_operation
	(const gui_edit_operation& edit_operation,
	 bool undo_caret_and_selection)
{
	/**
	 * Apply incoming @bref{edit_operation} to @bref{lines_},
	 * potentially move caret, and refresh rendering.
	 *
	 * @attent The @bref{edit_operation} must already
	 * have passed validate_edit_operation().
	 */


	// Fetch off-hand editing results.
	list<string> remove_lines, insert_lines;
	vector2i first_remove_index, last_remove_index;
	vector2i last_insert_index;

	prepare_edit_operation
		(edit_operation, remove_lines, insert_lines,
		 first_remove_index, last_remove_index, last_insert_index);


	// Atomically resize line storage and insert new lines.
	//
	// Exception safety notes:
	// - We did most work off-hand within above temporaries,
	//		and we now swap in temporary results with NOTHROW.
	// - Only lines_.resize() to a greater number of lines
	//		potentially throws. However, the entire operation
	//		still is atomic, as remaining swaps are NOTHROW.
	int64 diff_line_count =
		insert_lines.size() - remove_lines.size();

	if (diff_line_count > 0)
	{
		int64 old_line_count = lines_.line_count();
		lines_.resize(old_line_count + diff_line_count); // Might throw.

		for (int64 y = old_line_count - 1; y > last_remove_index.y; --y)
			lines_.swap_lines(y, y + diff_line_count);
	}
	else if (diff_line_count < 0)
	{
		int64 old_line_count = lines_.line_count();
		for (int64 y = last_remove_index.y + 1; y < old_line_count; ++y)
			lines_.swap_lines(y, y + diff_line_count);
		
		lines_.resize(lines_.line_count() + diff_line_count); // NOTHROW here.
	}

	int64 num_line = first_remove_index.y;
	for (list<string>::iterator it
			(insert_lines.first()); it; ++it, ++num_line)
	{	
		lines_.swap_line(num_line, *it);
	}


	// Appropriately reposition caret.
	if (undo_caret_and_selection)
	{
		selection_.set_caret_index(edit_operation.undo_selection_start_index, true, true);
		selection_.set_caret_index(edit_operation.undo_caret_index, false, true);
	}
	else
	{
		selection_.set_caret_index(last_insert_index, true, true);
	}
}


void gui_edit_kernel::apply_history_limit() NOTHROW
{
	// Count steps and memory backward through history,
	// and find the last history step we must delete.
	// On a NULL delete step, history is within limits.
	history_list::iterator last_delete_step(history_.last());
	int64 history_steps = 0;
	int64 history_memory = 0;

	while (last_delete_step)
	{
		++history_steps;
		history_memory =
			integer_operations::add_or_clamp
				(history_memory, last_delete_step->memory_usage);

		if (history_steps > history_step_limit_ ||
			history_memory > history_memory_limit_)
				break;

		--last_delete_step;
	}


	// Discard all steps up to and including the deletion step.
	// If we must discard steps we have undone and did not yet redo,
	// our future history becomes invalid, so clear all history.
	// This can only happen on calls that explicitly set history limits,
	// as we implicitly limit_history() only at the end of new_history(),
	// which already deletes any undone entries.
	while (last_delete_step)
	{
		if (history_.first() == history_last_step_)
		{
			clear_history();
			break;
		}

		if (history_.first() == last_delete_step)
			last_delete_step = history_list::iterator();

		history_.pop_front();
	}
}




} /* namespace viral_gui */
