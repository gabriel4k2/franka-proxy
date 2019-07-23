/**
 *************************************************************************
 *
 * @file gui_list.hpp
 *
 * Standalone GUI lists.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_LIST_HPP)
#define INCLUDED__VIRAL_GUI__GUI_LIST_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/util.hpp>
#include <viral_core/shared_pointer.hpp>
#include <viral_core/tree_map.hpp>
#include <viral_core/owning_array.hpp>


#include "gui_event.hpp"
#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_list_item
 * @ingroup gui_module
 *
 * A single entry in a gui_list, with some @bref{text} label
 * and an optional @bref{icon}.
 *
 * Usage notes:
 * - Clients may derive from gui_list_item to store application data.
 * - A gui_list owns all contained gui_list_item%s, and deletes these
 *		on obvious occasions (e.g. gui_list::~gui_list()).
 *
 ************************************************************************/
class gui_list_item :
	private non_copyable
{
	public:

		gui_list_item()
			:
			selected(false), read_only(false)
		{ }

		gui_list_item(const string& text)
			:
			text(text),
			selected(false), read_only(false)
		{ }

		gui_list_item
			(const string& text,
			 const shared_pointer<render_material_id>& icon)
			:
			text(text), icon(icon),
			selected(false), read_only(false)
		{ }


		virtual ~gui_list_item() NOTHROW { }


		string text;
		shared_pointer<render_material_id> icon;


		/**
		 * Whether the item is currently selected
		 * in the context of the owning gui_list.
		 */
		bool selected;


		/**
		 * Whether the item is currently read-only.
		 *
		 * Read-only items can not be @bref{selected}
		 * or deselected by user input, but still may
		 * be selected or deselected from code.
		 */
		bool read_only;
};




/**
 *************************************************************************
 *
 * @class gui_list
 * @ingroup gui_module
 *
 * Displays selectable, equally-sized gui_list_item%s
 * in a horizontal or vertical list.
 *
 * Usage notes:
 * - GUI lists efficiently handle a large number of gui_list_item%s,
 *		in particular when many of these items clip entirely. 
 * - Clients can directly work with the @bref{items} array in a list.
 *		Notably, clients can modify any gui_list_item within a list.
 *		@NULL items are supported, but show as whitespace.
 * - If clients change the mapping from array indices to list items
 *		(i.e. by adding, exchanging, or removing list @bref{items}),
 *		clients must explicitly call synchronize_item_indices().
 *		Failing to do so causes mismatched input handling.
 * - When only changing item data (e.g. text, icon, or selection state),
 *		clients use want_layout() instead of synchronize_item_indices().
 *		This avoids accidential cancelling of ongoing user interactions.
 *
 * Layout notes:
 * - A GUI list organizes its @bref{items} within sequential stacks. 
 *		Sequential stacks position in @bref{orientation} direction,
 *		and each stack aligns its items against @bref{orientation}.
 * - A list reports preferred_size() for its current stack count,
 *		each with one item instead of actual known_items_per_stack().
 *		Otherwise, some parents would not reduce items per stack.
 * - Layout()ing updates all stacks for an incoming arrange_area():
 *		First known_items_per_stack() adapt to occupy available space,
 *		then the overall stack count adjusts to match all list items.
 * - Lists clip if they cannot adjust their stack count to show
 *		all items within the provided arrange_area(). 
 *
 * Item size notes:
 * - Both layout()ing and preferred_size() use a known_item_size(),
 *		initially gui_list_style::min_item_size, to avoid expensive
 *		typesetting on each contained item.
 * - Usually, @bref{known_item_size} increases once layout()ing finds
 *		any item with a larger preferred_size(). The list then updates
 *		its own preferred_size() and continues to stay_dirty().
 * - After large-scale operations (such as replacing all list items),
 *		clients should reset_known_item_size() to shrink all items
 *		back to their default size.
 *
 * General notes:
 * - Various flags within gui_list_style (e.g. @bref{allow_grow} or
 *		@bref{allow_stack}) influence layouting of stacks and items.
 * - Lists do not scroll on their own, but show all items by default.
 *		To enable scrolling on a gui_list, clients can either use
 *		a convenient gui_listbox or a parent gui_scroll_area.
 * - Lists have an implementation-specific maximum to visible items.
 *		If this limit is exceeded, lists silently hide some items.
 *		This avoids excessive layout()ing on suboptimal GUI setup.
 *
 ************************************************************************/
class gui_list :
	public gui_element
{
	public:

		/**
		 * User interface for selecting items in a list.
		 * Note selection modes are not radically enforced.
		 * For instance, lists do not have any selection
		 * right after initialization, and client code
		 * can directly write to gui_list_item::selected.
		 */
		enum item_select_mode
		{
			/** Click selects a single item and deselects all others. */
			select_single,

			/** Click selects and deselects individual items. */
			select_toggle,

			/**
			 * Clicking by default selects single items,
			 * but shift and control modifiers allow
			 * incremental and sequential selection.
			 */
			select_advanced
		};


		gui_list
			(const string& name, gui_context& gui,
			 const shared_instance<gui_list_style>& list_style,
			 orientation_type orientation,
			 item_select_mode select_mode);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Select item at given @bref{index}, and deselect
		 * all other items. On invalid @bref{index} (e.g. -1),
		 * clear current selection.
		 */
		void select_index(int64 index) NOTHROW;


		/** Find first selected index, or -1 on no selection. */ 
		int64 first_selected_index() const NOTHROW;


		/**
		 * Discard display state and pending user interaction
		 * (double clicks, sequence selection) on all @bref{items}.
		 * See gui_list documentation for usage guidelines.
		 */
		void synchronize_item_indices() NOTHROW;


		/** Fetch current item size and item per stack count. */
		//@{
		vector2f known_item_size() const NOTHROW;
		int64 known_items_per_stack() const NOTHROW;
		//@}


		/** Reset current item size back to minimum item size. */
		void reset_known_item_size() NOTHROW;


		const shared_instance<gui_list_style> list_style;
		const orientation_type orientation;
		const item_select_mode select_mode;


		/** Array of gui_list_item%s in this list. */
		//@{
		typedef owning_array<gui_list_item> item_array;
		item_array items;
		//@}


		static const int64 max_visible_buttons = 1024;
		static const int64 max_items_per_stack_ = 1024;


	private:

		void realize_layout() override;

		void update_selection
			(const gui_input_event& input, int64 selection,
			 gui_output_event_list& output);

		void update_selection_single
			(const gui_input_event& input, int64 selection);
		void update_selection_toggle
			(const gui_input_event& input, int64 selection);
		void update_selection_advanced
			(const gui_input_event& input, int64 selection);

		void create_list_event(gui_output_event_list& output);
		void create_list_accept_event(gui_output_event_list& output);
		

		vector2f known_item_size_;
		int64 known_items_per_stack_;		

		typedef tree_map<int64, shared_instance<gui_button>> item_button_map;
		item_button_map item_buttons_;

		bool synchronize_item_indices_pending_;

		int64 select_sequence_start_;
};


/**
 * Sent by a gui_list if user input potentially changed
 * the item selection. Notably, selecting an already
 * selected item creates a redundant gui_list_event.
 *
 * A list event provides no further selection details.
 * Instead, clients should check the @bref{source} list.
 * 
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_list, 0> gui_list_event;


/**
 * Sent by a gui_list if the user repeatedly clicks an item.
 * Requires @bref{allow_repeat} in gui_list_style::button_style.
 *
 * Note that the first click creates a gui_list_event as usual.
 * Thereafter, repeated clicks only create gui_list_accept_event%s
 * and do not change item selection state.
 * 
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_list, 1> gui_list_accept_event;




/**
 *************************************************************************
 *
 * @class gui_list_style
 * @ingroup gui_module
 *
 * Styling for a gui_list.
 *
 ************************************************************************/
class gui_list_style :
	public gui_style
{
	public:

		gui_list_style
			(const shared_instance<gui_button_style>& button_style,
			 const vector2f& min_item_size,
			 const vector2i& icon_pixel_size,
			 bool allow_grow,
			 bool allow_stack,
			 bool allow_stretch);


		const shared_instance<gui_button_style> button_style;
		const vector2f min_item_size;
		const vector2i icon_pixel_size;


		/**
		 * Increase gui_list::known_item_size() to fit
		 * items with a larger preferred size.
		 * If unset, always use @bref{min_item_size}.
		 */
		const bool allow_grow;


		/**
		 * Increase the number of items per stack to occupy
		 * excess arrange area. If unset, do not stack items.
		 */
		const bool allow_stack;


		/**
		 * Stretch stack items to occupy remaining free space.
		 * This does not modify @bref{known_item_size}.
		 * If unset, stacks anchor items to the top-left.
		 */
		const bool allow_stretch;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_LIST_HPP) */
