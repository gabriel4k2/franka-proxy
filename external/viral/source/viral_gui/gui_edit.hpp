/**
 *************************************************************************
 *
 * @file gui_edit.hpp
 *
 * GUI text edit.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_EDIT_HPP)
#define INCLUDED__VIRAL_GUI__GUI_EDIT_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/timer_forward.hpp>


#include <viral_core/tree_map.hpp>


#include "gui_element.hpp"
#include "gui_edit_lines.hpp"
#include "gui_edit_kernel.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_edit
 * @ingroup gui_module
 *
 * Allows for basic text editing.
 *
 * Usage notes:
 * - Unlike gui_label%s, edit elements efficiently support long text,
 *		in particular when most lines clip.
 * - Edit elements do not render any text background and do not scroll.
 *		Clients can explicitly wrap edit elements into other elements
 *		(e.g. a gui_frame or gui_scroll_area) to enable such features.
 *		See gui_editbox for a gui_edit with frames and scrolling.
 * - There are internal (large) limits to line length and line count.
 *		If these limits are exceeded (e.g. through user text entry),
 *		the respective editing operation is logged and then ignored.
 *
 * Layout notes:
 * - An edit element prefers enough space for display of its entire text().
 *		The edit element clips on less space and ignores excess space.
 * - For performance reasons, @bref{multiline} edit elements guess text size
 *		based on a known_line_width(), at first gui_edit_style::min_line_width.
 *		Once layouting arranges a line longer than the known line width,
 *		layouting increases known_line_width() and updates preferred_size().
 * - Multiline edit elements do not shrink known_line_width() on their own.
 *		To free up excess preferred_size(), clients instead must explicitly
 *		reset_known_line_width(), for instance after major operations.
 * - On single-line edits, layouting always updates known_line_width()
 *		to match the larger of actual line width or min_line_width.
 *
 ************************************************************************/
class gui_edit :
	public gui_element
{
	public:

		gui_edit
			(const string& name, gui_context& gui,
			 const shared_instance<gui_edit_style>& edit_style,
			 bool multiline, bool notify_accept);


		vector2f preferred_size() const override;


		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		bool try_accept_key_focus() NOTHROW override;
		void lost_key_focus() NOTHROW override;


		/**
		 * Disable mouse and keyboard editing. 
		 * Text selection and clipboard copy remain possible,
		 * and applications can still set_text().
		 */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/**
		 * Text within the edit element.
		 *
		 * To avoid excessive history on frequent changes,
		 * set_text() clears any current editing history,
		 * unless @bref{allow_undo} is explicitly @true.
		 */
		//@{
		void set_text(const string& text);
		void set_text(const string& text, bool allow_undo);
		string text() const;
		//@}


		/**
		 * Append text to the edit element,
		 * more efficient than separate text()
		 * and set_text() calls.
		 */
		//@{
		void append_text(const string& text);
		void append_text(const string& text, bool allow_undo);
		//@}


		/**
		 * Change maximum steps or bytes for undo and redo.
		 * Invalid values silently clamp to valid range. 
		 * See gui_edit_kernel for default and maximum values.
		 */
		//@{
		void set_history_step_limit(int64 history_step_limit) NOTHROW;
		int64 history_step_limit() const NOTHROW;
		void set_history_memory_limit(int64 history_memory_limit) NOTHROW;
		int64 history_memory_limit() const NOTHROW;
		//@}


		/**
		 * Fetch or reset currently known line width.
		 * Relevant only for @bref{multiline} edit elements.
		 */
		//@{
		float known_line_width() const NOTHROW;
		void reset_known_line_width() NOTHROW;
		//@}


		const shared_instance<gui_edit_style> edit_style;


		/**
		 * Whether this gui_edit can hold more than one line of text.
		 * If @bref{multiline} is @false, the element removes any newlines
		 * from set_text(), user input, or paste operations.
		 */
		const bool multiline;


		/**
		 * If set, this edit element sends gui_edit_accept_event%s
		 * (potentially in addition to normal gui_edit_event%s)
		 * whenever the user hits return within the text field.
		 * Note that both single-line and multiline edit elements
		 * can send notifications on accept.
		 */
		const bool notify_accept;


		/** Maximum number of lines visible at a time. */
		static const int64 max_visible_lines = 2048;


	private:

		void realize_layout() override;

		void process_tick_event
			(gui_tick_event& tick_event,
			 gui_output_event_list& output);
		void process_mouse_event
			(gui_mouse_event& mouse_event,
			 gui_output_event_list& output);
		void process_text_event
			(gui_text_event& text_event,
			 gui_output_event_list& output);

		void cancel_editing();

		void insert_char(char chr);
		void toggle_overwrite_mode();

		void delete_left();
		void delete_right();
		void delete_selection();

		void cut();
		void copy();
		void paste();

		void start_caret_dragging(const vector2f& cursor);
		void update_caret_dragging(const vector2f& cursor);
		void stop_caret_dragging();

		rect2f build_caret_extents() NOTHROW;
		void scroll_to_caret() NOTHROW;
		void restart_caret_blink() NOTHROW;

		void commit_render_selection
			(gui_render_packer& packer);
		void commit_render_caret
			(gui_render_packer& packer);


		static const int64 max_key_repeat_;

		rect2f hot_area_clipped_;
		bool hot_area_valid_;

		bool read_only_;
		bool overwrite_mode_;

		gui_edit_lines lines_;
		gui_edit_selection selection_;
		gui_edit_kernel kernel_;

		bool caret_dragging_;
		bool caret_visible_;
		auto_pointer<free_timer> caret_timer_;
		bool delayed_scroll_to_caret_;

		typedef tree_map<int64, shared_instance<gui_label>> line_label_map;
		line_label_map line_labels_;
		float known_line_width_;

		const shared_instance<gui_anchor_style> line_anchor_style_;
		const shared_instance<gui_label_style> line_label_style_;
};


/**
 * Sent whenever the user potentially changed the content
 * of a gui_edit element. There might be false-positive events,
 * e.g. if the user pastes from an empty clipboard.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_edit, 0> gui_edit_event;


/**
 * Sent by single-line and multiline gui_edit elements
 * with @bref{notify_accept} whenever the user hits
 * return while editing text.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_edit, 1> gui_edit_accept_event;


/**
 * Sent by all gui_edit%s whenever the user cancels editing
 * by explicitly hitting the escape key. Note that the user
 * can also cancel editing by moving focus to another element,
 * in which case no gui_edit_cancel_event occurs.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_edit, 2> gui_edit_cancel_event;




/**
 *************************************************************************
 *
 * @class gui_edit_style
 * @ingroup gui_module
 *
 * Styling for a gui_edit_box.
 *
 ************************************************************************/
class gui_edit_style :
	public gui_style
{
	public:

		gui_edit_style
			(const shared_instance<gui_font_style>& font_style,
			 float min_line_width,
			 const shared_instance<render_material_id>& selection_material,
			 const shared_instance<render_material_id>& caret_material,
			 float caret_width,
			 float caret_overwrite_width,
			 float caret_blink_seconds,
			 const shared_instance<gui_cursor_style>& caret_cursor);

		const shared_instance<gui_font_style> font_style;
		const float min_line_width;

		const shared_instance<render_material_id> selection_material;
		const shared_instance<render_material_id> caret_material;
		const float caret_width;
		const float caret_overwrite_width;
		const float caret_blink_seconds;

		const shared_instance<gui_cursor_style> caret_cursor;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_EDIT_HPP) */