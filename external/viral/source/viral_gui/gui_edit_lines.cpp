/**
 *************************************************************************
 *
 * @file gui_edit_lines.cpp
 *
 * Lines and line selection for gui_edit, implementation.
 *
 ************************************************************************/


#include "gui_edit_lines.hpp"


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>
#include <viral_core/font.hpp>


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_edit_lines
//
//////////////////////////////////////////////////////////////////////////


gui_edit_lines::gui_edit_lines
	(const font& f)
	:
	font_(f)
{
	// Ensure we have at least one line at all times,
	// so that we also have at least one valid index.
	// See clamp_index(), index_valid() and resize().
	lines_.push_back(string());
}


bool gui_edit_lines::index_valid
	(const vector2i& index) const NOTHROW
{
	if (index.y < 0 || index.y >= lines_.size())
		return false;	

	if (index.x < 0 || index.x > lines_[index.y].size())
		return false; // Terminating \\0 is valid index.

	return true;
}


vector2i gui_edit_lines::clamp_index
	(const vector2i& index) const NOTHROW
{
	int64 clamp_y =
		max(0, min(index.y, lines_.size() - 1));
	int64 clamp_x =
		max(0, min(index.x, lines_[clamp_y].size()));

	return vector2i(clamp_x, clamp_y);
}


vector2i gui_edit_lines::first_index() const NOTHROW
	{ return vector2i(); }

vector2i gui_edit_lines::last_index() const NOTHROW
{
	return vector2i(lines_[lines_.size() - 1].size(),
					lines_.size() - 1);
}


vector2i gui_edit_lines::find_next_index
	(const vector2i& index) const NOTHROW
{
	if (!index_valid(index)) return index;

	if (index.x == lines_[index.y].size())
	{
		if (index.y == lines_.size() - 1) return index;
		return vector2i(0, index.y + 1);
	}

	return vector2i(index.x + 1, index.y);
}


vector2i gui_edit_lines::find_prev_index
	(const vector2i& index) const NOTHROW
{
	if (!index_valid(index)) return index;

	if (index.x == 0)
	{
		if (index.y == 0) return index;
		return vector2i(lines_[index.y - 1].size(), index.y - 1);
	}

	return vector2i(index.x - 1, index.y);
}


vector2i gui_edit_lines::relative_position_to_index
	(const vector2f& relative_position) const NOTHROW
{
	 // Since we store our text as separate lines, we cannot
	 // rely on the newline tests in font::find_letter_index().
	 // Instead we directly determine the correct line from
	 // the cursor y position.
	float position_y_lines_float =
		max(0.f, min(static_cast<float>(lines_.size() - 1),
			relative_position.y / font_.line_height));

	int64 position_y_lines =
		max(0, min(lines_.size() - 1,
			integer_operations::cast_or_clamp<int64>
				(position_y_lines_float)));


	// Find x index from position on correct line.
	int64 position_x_chars =
		font_.find_letter_index
			(lines_[position_y_lines], relative_position);

	return
		vector2i(position_x_chars, position_y_lines);
}


vector2f gui_edit_lines::index_to_relative_position
	(const vector2i& index) const NOTHROW
{
	// Find y offset to correct line.
	// Directly abort for invalid lines.
	vector2f pos;

	if (index.y < 0 || index.y >= lines_.size())
		return pos;

	pos.y +=
		index.y * font_.line_height;
		
	if (lines_[index.y].empty())
		return pos;


	// Handle at-end or out-of-line characters.
	// Position these in front of the first valid char,
	// or past the last valid char.
	if (index.x <= 0) return pos;

	if (index.x >= lines_[index.y].size())
	{
		rect2f letter;
		if (font_.find_letter_extents
				(lines_[index.y], lines_[index.y].size() - 1,
				 rect2f(), false, letter))
		{
			pos.x += letter.end_x();
		}

		return pos;
	}


	// Handle in-line characters.
	rect2f letter;
	if (font_.find_letter_extents
			(lines_[index.y], index.x,
			 rect2f(), false, letter))
	{
		pos.x += letter.pos.x;
	}

	return pos;
}


void gui_edit_lines::resize(int64 line_count)
{
	if (line_count < 0)
	{
		LOG_ERROR("Cannot resize to less than zero lines.");
		throw invalid_parameter();
	}

	if (line_count == 0)
	{
		LOG_ERROR("There must be at least one line.");
		throw invalid_parameter();
	}

	lines_.resize(line_count);
}


void gui_edit_lines::swap_lines
	(int64 line_number_a, int64 line_number_b) NOTHROW
{
	if (line_number_a < 0 || line_number_a >= lines_.size())
	{
		LOG_ERROR("Line number A invalid.");
		return; // Exception safety: NOTHROW
	}

	if (line_number_b < 0 || line_number_b >= lines_.size())
	{
		LOG_ERROR("Line number B invalid.");
		return; // Exception safety: NOTHROW
	}

	swap(lines_[line_number_a], lines_[line_number_b]);
}


void gui_edit_lines::swap_line
	(int64 line_number, string& s) NOTHROW
{
	if (line_number < 0 || line_number >= lines_.size())
	{
		LOG_ERROR("Line number invalid.");
		return; // Exception safety: NOTHROW
	}

	swap(lines_[line_number], s);
}


int64 gui_edit_lines::line_count() const NOTHROW
	{ return lines_.size(); }


string gui_edit_lines::line(const vector2i& index) const
	{ return line(index.y); }

string gui_edit_lines::line(int64 line_number) const
{
	if (line_number < 0 || line_number >= lines_.size())
		return "";

	return lines_[line_number];
}


int64 gui_edit_lines::line_size
	(const vector2i& index) const NOTHROW
		{ return line_size(index.y); }

int64 gui_edit_lines::line_size
	(int64 line_number) const NOTHROW
{
	if (line_number < 0 || line_number >= lines_.size())
		return 0;

	return lines_[line_number].size();
}


char gui_edit_lines::char_at_index
	(const vector2i& index) const NOTHROW
{
	if (!index_valid(index)) return '\0';
	return lines_[index.y][index.x];
}


string gui_edit_lines::text_from_indices
	(const vector2i& first_index,
	 const vector2i& last_index) const
{
	if (!index_valid(first_index) ||
		!index_valid(last_index)) return string();

	if (first_index.y > last_index.y) return string();

	if (first_index.y == last_index.y &&
		first_index.x >= last_index.x) return string();

	string result;
	for (int64 y = first_index.y; y <= last_index.y; ++y)
	{
		int64 first_x =
			(y == first_index.y) ? first_index.x : 0;
		int64 last_x =
			(y == last_index.y) ? last_index.x : lines_[y].size();

		if (y != first_index.y)
			result.append('\n');

		result +=
			lines_[y].substring(first_x, last_x - first_x);
	}

	return result;
}




//////////////////////////////////////////////////////////////////////////
//
// gui_edit_selection
//
//////////////////////////////////////////////////////////////////////////


gui_edit_selection::gui_edit_selection
	(const font& style_font,
	 const gui_edit_lines& lines)
	:
	style_font_(style_font),
	lines_(lines)
{ }


void gui_edit_selection::set_caret_index
	(const vector2i& caret_index,
	 bool reset_selection, bool reset_line_move) NOTHROW
{
	caret_index_ =
		lines_.clamp_index(caret_index);

	if (reset_selection)
		selection_start_index_ = caret_index_;

	if (reset_line_move)
		line_move_start_index_ = caret_index_;
}


void gui_edit_selection::move_caret_horizontal
	(int64 offset, bool select) NOTHROW
{
	// Looping is inefficient, but offset should be small.
	while (offset != 0)
	{
		vector2i move_target
			(offset > 0 ? lines_.find_next_index(caret_index_) :
						  lines_.find_prev_index(caret_index_));

		set_caret_index(move_target, !select, true);
		offset += (offset > 0) ? -1 : 1;
	}
}


void gui_edit_selection::move_caret_wordwise
	(int64 offset, bool select) NOTHROW
{
	// Looping is inefficient, but offset should be small.
	while (offset != 0)
	{
		vector2i move_target(caret_index_);

		char start_class =
			move_caret_wordwise_class
				(offset > 0 ? caret_index_ :
							  lines_.find_prev_index(caret_index_));


		// Search for word boundary.
		while (true)
		{
			vector2i new_move_target
				(offset > 0 ? lines_.find_next_index(move_target) :
							  lines_.find_prev_index(move_target));


			// Abort at the end of text.
			if (new_move_target == move_target) break;
			move_target = new_move_target;


			// Abort at boundaries in character classes.
			char current_class =
				move_caret_wordwise_class
					(offset > 0 ? move_target :
								  lines_.find_prev_index(move_target));

			if (current_class != start_class) break;
		}


		// Move caret to word boundary.
		set_caret_index(move_target, !select, true);
		offset += (offset > 0) ? -1 : 1;
	}
}


void gui_edit_selection::move_caret_vertical
	(int64 offset, bool select) NOTHROW
{
	// We need the epsilon here to ensure that later
	// relative_position_to_index() does not round
	// to the previous character due to numeric precision.
	vector2f gui_space_offset
		(line_move_epsilon_,
		 style_font_.line_height * (static_cast<float>(offset) + 0.5f));

	vector2f mixed_pos
		(lines_.index_to_relative_position(line_move_start_index_).x,
		 lines_.index_to_relative_position(caret_index_).y);

	vector2f next_pos
		(mixed_pos + gui_space_offset);

	set_caret_index
		(lines_.relative_position_to_index(next_pos), !select, false);
}


void gui_edit_selection::move_caret_by_page
	(float page_size, int64 page_offset, bool select) NOTHROW
{
	float scroll_amount =
		page_size * static_cast<float>(page_offset);

	vector2f mixed_pos
		(lines_.index_to_relative_position(line_move_start_index_).x,
		 lines_.index_to_relative_position(caret_index_).y);

	vector2f next_pos
		(mixed_pos + vector2f(0, scroll_amount));

	set_caret_index
		(lines_.relative_position_to_index(next_pos), !select, false);
}


void gui_edit_selection::move_caret_to_line_start(bool select) NOTHROW
	{ set_caret_index(vector2i(0, caret_index_.y), !select, true); }

void gui_edit_selection::move_caret_to_line_end(bool select) NOTHROW
{
	set_caret_index
		(vector2i(lines_.line_size(caret_index_),
				  caret_index_.y), !select, true);
}


vector2i gui_edit_selection::caret_index() const NOTHROW
	{ return lines_.clamp_index(caret_index_); }
vector2i gui_edit_selection::selection_start_index() const NOTHROW
	{ return lines_.clamp_index(selection_start_index_); }


vector2i gui_edit_selection::first_selected_index() const NOTHROW
{
	return vector2i
		(caret_index_.y < selection_start_index_.y ? caret_index_ :
		 caret_index_.y > selection_start_index_.y ? selection_start_index_ :
		 // here: caret_index_.y == selection_start_index_.y
		 caret_index_.x < selection_start_index_.x ? caret_index_ :
		 selection_start_index_);
}


vector2i gui_edit_selection::last_selected_index() const NOTHROW
{
	return vector2i
		(caret_index_.y > selection_start_index_.y ? caret_index_ :
		 caret_index_.y < selection_start_index_.y ? selection_start_index_ :
		 // here: caret_index_.y == selection_start_index_.y
		 caret_index_.x > selection_start_index_.x ? caret_index_ :
		 selection_start_index_);
}


bool gui_edit_selection::empty() const NOTHROW
	{ return caret_index_ == selection_start_index_; }


string gui_edit_selection::selected_text() const
{
	return lines_.text_from_indices
				(first_selected_index(),
				 last_selected_index());
}


char gui_edit_selection::move_caret_wordwise_class
	(const vector2i& index) const NOTHROW
{
	/**
	 * Return a unified representant for the character class
	 * at given @bref{index} for wordwise caret movement.
	 */

	char char_at_index =
		lines_.char_at_index(index);

	if (char_at_index >= '0' && char_at_index <= '9' ||
		char_at_index >= 'a' && char_at_index <= 'z' ||
		char_at_index >= 'A' && char_at_index <= 'Z')
			return 'a'; // Alphanumeric.

	if (char_at_index == ' ' || char_at_index == '\t' ||
		char_at_index == '\r' || char_at_index == '\n')
			return ' '; // Whitespace.

	if (char_at_index >= 33 && char_at_index <= 47 || // ! ... /
		char_at_index >= 58 && char_at_index <= 64 || // : ... @
		char_at_index >= 91 && char_at_index <= 96 || // [ ... `
		char_at_index >= 123 && char_at_index <= 126) // { ... ~
			return '.'; // Interpunctuation.

	return '\0'; //  Other characters or invalid index.
}


const float gui_edit_selection::line_move_epsilon_ = 0.001f;




} /* namespace viral_gui */
