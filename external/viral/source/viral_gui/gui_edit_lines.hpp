/**
 *************************************************************************
 *
 * @file gui_edit_lines.hpp
 *
 * Lines and line selection for gui_edit.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_EDIT_LINES_HPP)
#define INCLUDED__VIRAL_GUI__GUI_EDIT_LINES_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/image_forward.hpp>


#include <viral_core/util.hpp>
#include <viral_core/array.hpp>
#include <viral_core/string.hpp>
#include <viral_core/geo_2d.hpp>


namespace viral_gui
{


using namespace viral_core;


/**
 *************************************************************************
 *
 * @class gui_edit_lines
 * @ingroup gui_module
 *
 * Manages the lines of a gui_edit element.
 *
 * Index interpretation:
 * - Column indices mark insertion spots between letters.
 *		Hence, valid x values range from 0 to line_size(y),
 *		as opposed to characters from 0 to line_size(y) - 1.
 * - Valid y row indices range from 0 to lines_.size() - 1.
 *
 * Implementation notes:
 * - The gui_edit_lines utility checks all incoming indices
 *		to prohibit invalid access to the internal @bref{lines_}.
 * - @attent There always is at least a single (empty) line.
 *		This ensures that clamping can prohibit invalid indices.
 *
 ************************************************************************/
class gui_edit_lines :
	private non_copyable
{

	public:

		typedef array<string> line_array;


		gui_edit_lines(const font& f);


		/**
		 * Test if @bref{index} denotes a valid line
		 * and a valid insertion spot on that line.
		 */
		bool index_valid(const vector2i& index) const NOTHROW;


		/**
		 * Clamp @bref{index} to the next valid line
		 * and to a valid insertion spot on that line.
		 */
		vector2i clamp_index
			(const vector2i& index) const NOTHROW;


		/**
		 * Return the first or last valid insertion spot.
		 * The last_index() always points to a terminating \\0,
		 * while the first_index() always is (0,0).
		 */
		//@{
		vector2i first_index() const NOTHROW;
		vector2i last_index() const NOTHROW;
		//@}


		/**
		 * Find insertion spot after or before @bref{index}.
		 * Silently returns @bref{index} if @bref{index} was invalid
		 * or if @bref{index} was the first or last insertion spot.
		 */
		//@{
		vector2i find_next_index(const vector2i& index) const NOTHROW;
		vector2i find_prev_index(const vector2i& index) const NOTHROW;
		//@}


		/**
		 * For a @bref{relative_position} to the typesetting origin,
		 * find corresponding character index within our text.
		 *
		 * If there is no character at the given position,
		 * return an approximately nearest insertion spot instead.
		 */
		vector2i relative_position_to_index
			(const vector2f& position) const NOTHROW;


		/**
		 * For a character @bref{index} within our @bref{lines_},
		 * find corresponding upper-left typesetting position.
		 *
		 * If @bref{index} is invalid (e.g. uninitialized or past-end),
		 * then return a position for the nearest valid index.
		 */
		vector2f index_to_relative_position
			(const vector2i& index) const NOTHROW;


		/**
		 * Add or delete lines at the end of storage.
		 *
		 * This call is @ref NOTHROW if @bref{line_count}
		 * is greater than zero and less than line_count().
		 */
		void resize(int64 line_count);


		/**
		 * Swap a line at some @bref{line_number_a}
		 * with the line at another @bref{line_number_b}
		 * or with a separate @bref{s}tring.
		 * Ignored on invalid line numbers.
		 */
		//@{
		void swap_lines(int64 line_number_a, int64 line_number_b) NOTHROW;
		void swap_line(int64 line_number, string& s) NOTHROW;
		//@}


		/** Number of lines in the gui_edit element. */
		int64 line_count() const NOTHROW;


		/**
		 * Returns line at @bref{index} or @bref{line_number},
		 * or an empty string if no such line exists.
		 */
		//@{
		string line(const vector2i& index) const;
		string line(int64 line_number) const;
		//@}


		/**
		 * Fetch the number of characters for line
		 * at @bref{index} or at @bref{line_number},
		 * or return 0 if no such line exists.
		 */
		//@{
		int64 line_size(const vector2i& index) const NOTHROW;
		int64 line_size(int64 line_number) const NOTHROW;
		//@}
		 

		/**
		 * Character to the right of the insertion spot @bref{index}.
		 * Returns \\0 if the index is invalid.
		 */
		char char_at_index
			(const vector2i& index) const NOTHROW;


		/**
		 * Put together a single string from all characters
		 * between @bref{first_index} and @bref{last_index}.
		 * Returns an empty string if either index is invalid,
		 * or if @bref{last_index} precedes @bref{first_index}.
		 */
		string text_from_indices
			(const vector2i& first_index,
			 const vector2i& last_index) const;


	private:

		const font& font_;
		line_array lines_;
};




/**
 *************************************************************************
 *
 * @class gui_edit_selection
 * @ingroup gui_module
 *
 * Manages the editing caret and the selection in a gui_edit element.
 *
 ************************************************************************/
class gui_edit_selection :
	private non_copyable
{
	public:

		typedef array<string> line_array;
	

		gui_edit_selection
			(const font& style_font,
			 const gui_edit_lines& lines);


		/**
		 * Move the caret to @bref{caret_index}.
		 * Silently clamps invalid indices.
		 *
		 * If @bref{reset_selection} is @true,
		 * move selection_start_index() as well.
		 *
		 * If @bref{reset_line_move} is @true,
		 * reset the anchor for vertical caret movement.
		 * See move_vertical() for details.
		 */
		void set_caret_index
			(const vector2i& index,
			 bool reset_selection, bool reset_line_move) NOTHROW;


		/**
		 * Move caret through text by given offset
		 * in characters, words, lines, or pages,
		 * or to start or end of current line.
		 *
		 * Usage notes:
		 * - If @bref{select} is @false, move the selection
		 *		starting point to the new caret position.
		 *		Otherwise, keep the selection starting point
		 *		and thus change the current selection.
		 * - All movement calls require implicit typesetting,
		 *		thus offsets should be rather small.		 
		 *
		 * Movement behaviour:
		 * - Horizontal movement skips to the next or previous line
		 *		if the caret leaves the current line.
		 * - If possible, vertical and page-wise caret movements maintain
		 *		the horizontal on-screen (not index-based) caret position
		 *		from where the user started line movement.
		 * - Character classes (e.g. alphanumerics, interpunctation)
		 *		form the word boundaries for word-wise caret movement.
		 *		See move_caret_wordwise_class().
		 */
		//@{
		void move_caret_horizontal(int64 char_offset, bool select) NOTHROW;
		void move_caret_wordwise(int64 word_offset, bool select) NOTHROW;
		void move_caret_vertical(int64 line_offset, bool select) NOTHROW;

		void move_caret_by_page
			(float page_size, int64 page_offset, bool select) NOTHROW;

		void move_caret_to_line_start(bool select) NOTHROW;
		void move_caret_to_line_end(bool select) NOTHROW;
		//@}


		/**
		 * Fetch the current caret and selection start indices.
		 * Both queries clamp_index() to valid insertion spots
		 * on valid lines, and therefore should never be cached.
		 *
		 * Note that users can drag the caret to the top-left
		 * or to the bottom-right to mark a selection interval,
		 * thus caret and selection start need not be in order.
		 * Use first_selected_index() and last_selected_index()
		 * if an ascending index order is required.
		 */
		//@{
		vector2i caret_index() const NOTHROW;
		vector2i selection_start_index() const NOTHROW;

		vector2i first_selected_index() const NOTHROW;
		vector2i last_selected_index() const NOTHROW;
		//@}


		/**
		 * Returns @true if caret_index() and selection_start_index()
		 * refer to the same insertion spots, @false otherwise.
		 */
		bool empty() const NOTHROW;


		/**
		 * Fetch all text between caret_index() and selection_start_index().
		 * Returns an empty string if the selection is empty().
		 */
		string selected_text() const;


	private:

		char move_caret_wordwise_class
			(const vector2i& index) const NOTHROW;


		static const float line_move_epsilon_;

		const font& style_font_;
		const gui_edit_lines& lines_;

		vector2i caret_index_;
		vector2i selection_start_index_;
		vector2i line_move_start_index_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_EDIT_LINES_HPP) */

