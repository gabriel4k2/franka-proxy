/**
 *************************************************************************
 *
 * @file gui_edit_kernel.hpp
 *
 * Editing kernel for gui_edit.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_EDIT_KERNEL)
#define INCLUDED__VIRAL_GUI__GUI_EDIT_KERNEL


#include <viral_core/build_config.hpp>


#include <viral_core/container_forward.hpp>


#include <viral_core/util.hpp>
#include <viral_core/geo_2d.hpp>
#include <viral_core/list.hpp>
#include <viral_core/string.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


using namespace viral_core;


/**
 *************************************************************************
 *
 * @class gui_edit_operation
 * @ingroup gui_module
 *
 * An atomic and undoable editing operation in a gui_edit element.
 *
 * Implementation notes:
 * - An edit operation replaces @bref{current_text} at @bref{text_index}
 *		with @bref{new_text}. Inserting and deleting are special cases
 *		where either @bref{current_text} or @bref{new_text} is empty.
 * - After replacement, the caret moves to the end of @bref{new_text},
 *		and no text is selected. Operations separately record initial
 *		caret and selection indices, so that undo can restore these.
 *
 ************************************************************************/
class gui_edit_operation
{

	public:

		gui_edit_operation
			(const vector2i& text_index,
			 const string& current_text,
			 const string& new_text,
			 const vector2i& undo_caret_index,
			 const vector2i& undo_selection_start_index);

		const vector2i text_index;
		const string current_text;
		const string new_text;

		const vector2i undo_caret_index;
		const vector2i undo_selection_start_index;

		const int64 memory_usage;


	private:

		static int64 find_memory_usage
			(const string& current_text, const string& new_text);
};




/**
 *************************************************************************
 *
 * @class gui_edit_kernel
 * @ingroup gui_module
 *
 * Performs gui_edit_operation%s on the lines of a gui_edit element
 * and tracks the undo and redo history.
 *
 ************************************************************************/
class gui_edit_kernel :
	private non_copyable
{

	public:

		typedef array<string> line_array;


		gui_edit_kernel
			(bool multiline,
			 gui_edit_lines& lines,
			 gui_edit_selection& selection);


		/**
		 * Perform some new @bref{edit_operation}
		 * and insert it into the history queue.
		 * Any undone steps are deleted permanently.
		 *
		 * Editing is atomic and does not change any text on failure.
		 * However, on any exception, we clear the entire history
		 * to avoid corruption of edited text by future undo or redo.
		 */
		void execute
			(const gui_edit_operation& edit_operation);		


		/**
		 * Redo or undo an operation from the history queue.
		 * See execute() for atomic editing and exception behaviour.
		 */
		//@{
		void redo_history();
		void undo_history();
		//@}


		/**
		 * Remove the entire editing history,
		 * which resets any undo or redo steps.
		 */
		void clear_history() NOTHROW;


		/**
		 * Apply memory and step limits to history.
		 * See gui_edit::history_step_limit().
		 */
		//@{
		void set_history_step_limit(int64 history_step_limit) NOTHROW;
		int64 history_step_limit() const NOTHROW;
		void set_history_memory_limit(int64 history_memory_limit) NOTHROW;
		int64 history_memory_limit() const NOTHROW;
		//@}


		/** Safety limits for excessive gui_edit contents. */
		//@{
		static const int64 max_line_length = 512;
		static const int64 max_line_count = 4096;
		//@}


		/**
		 * Maximum and default steps and bytes
		 * in history for undo/redo operations.
		 */
		//@{
		static const int64 max_history_steps = 1024;
		static const int64 default_history_steps = max_history_steps;
		static const int64 max_history_memory = 1048576;
		static const int64 default_history_memory = max_history_memory;
		//@}


	private:

		void prepare_edit_operation
			(const gui_edit_operation& edit_operation,
			 list<string>& remove_lines, list<string>& insert_lines, 
			 vector2i& first_remove_index, vector2i& last_remove_index,
			 vector2i& last_insert_index);

		bool validate_edit_operation
			(const gui_edit_operation& edit_operation);

		void apply_edit_operation
			(const gui_edit_operation& edit_operation,
			 bool undo_caret_and_selection);

		void apply_history_limit() NOTHROW;


		const bool multiline_;

		gui_edit_lines& lines_;
		gui_edit_selection& selection_;

		typedef list<gui_edit_operation> history_list;
		history_list history_;
		history_list::iterator history_last_step_;

		int64 history_step_limit_;
		int64 history_memory_limit_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_EDIT_KERNEL) */
