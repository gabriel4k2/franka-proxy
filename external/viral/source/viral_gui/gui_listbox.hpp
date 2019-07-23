/**
 *************************************************************************
 *
 * @file gui_listbox.hpp
 *
 * GUI lists with frame and scrolling.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_LISTBOX_HPP)
#define INCLUDED__VIRAL_GUI__GUI_LISTBOX_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"
#include "gui_list.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_listbox
 * @ingroup gui_module
 *
 * Bundles a gui_list with a gui_scroll_area and a gui_frame.
 *
 * Layout notes:
 * - A listbox clips on an arrange_area() smaller than preferred_size(),
 *		and fills any excess arrange space with frames and list().
 * - The @bref{listbox_style.scroll_area_style} should usually
 *		@bref{prefer_override_size}. In this case, the listbox
 *		prefers the override size of the scroll area plus frames,
 *		and shows scrollbars for any excess items within list().
 * - If the scroll area does not @bref{prefer_override_size},
 *		the listbox prefers size for all list items instead.
 *		The listbox then clips, but never shows any scrollbars.
 *
 ************************************************************************/
class gui_listbox :
	public gui_element
{

	public:

		gui_listbox
			(const string& name, gui_context& gui,
			 const shared_instance<gui_listbox_style>& listbox_style,
			 orientation_type orientation,
			 gui_list::item_select_mode select_mode);


		vector2f preferred_size() const override;


		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		bool try_accept_key_focus() NOTHROW override;


		/** Forward calls to the internal list(). */
		//@{
		void select_index(int64 index) NOTHROW;
		int64 first_selected_index() const NOTHROW;
		void synchronize_item_indices() NOTHROW;
		vector2f known_item_size() const NOTHROW;
		int64 known_items_per_stack() const NOTHROW;
		void reset_known_item_size() NOTHROW;
		//@}


		/** Items of the internal list(). */
		//@{
		typedef owning_array<gui_list_item> item_array;
		item_array& items() NOTHROW;
		const item_array& items() const NOTHROW;
		//@}


		/** Access the internal gui_list. */
		//@{
		gui_list& list() NOTHROW;
		const gui_list& list() const NOTHROW;
		//@}


		/**
		 * Determine arrange_area().size required to show
		 * the given (potentially fractional) number of items
		 * without any item stacking.
		 */
		vector2f listbox_size_for_item_count(float item_count);


		const shared_instance<gui_listbox_style> listbox_style;


	private:

		void realize_layout() override;


		const shared_instance<gui_frame> frame_;
		const shared_instance<gui_scroll_area> scroll_area_;
		const shared_instance<gui_list> list_;
};


/**
 * See gui_list_event and gui_list_accept_event.
 * @ingroup gui_module
 */
//@{
typedef gui_simple_output_event<gui_listbox, 0> gui_listbox_event;
typedef gui_simple_output_event<gui_listbox, 1> gui_listbox_accept_event;
//@}




/**
 *************************************************************************
 *
 * @class gui_listbox_style
 * @ingroup gui_module
 *
 * Styling for a gui_listbox.
 *
 ************************************************************************/
class gui_listbox_style :
	public gui_style
{
	public:

		gui_listbox_style
			(const shared_instance<gui_frame_style>& frame_style,
			 const shared_instance<gui_scroll_area_style>& scroll_area_style,
			 const shared_instance<gui_list_style>& list_style);

		const shared_instance<gui_frame_style> frame_style;
		const shared_instance<gui_scroll_area_style> scroll_area_style;
		const shared_instance<gui_list_style> list_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_LISTBOX_HPP) */