/**
 *************************************************************************
 *
 * @file gui_dropdown.hpp
 * 
 * GUI dropdown boxes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_DROPDOWN_HPP)
#define INCLUDED__VIRAL_GUI__GUI_DROPDOWN_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_dropdown
 * @ingroup gui_module
 *
 * Shows a single-selection gui_listbox on demand. 
 *
 * Layout notes:
 * - A gui_dropdown consists of two buttons, one with the dropdown icon,
 *		and one with the currently selected item. A label to the left
 *		of the dropdown buttons shows a descriptive label_text().
 * - Label and icon button always prefer fixed size, while the item button
 *		increases its known_item_button_size() to fit any selected item
 *		(initially gui_dropdown_style::min_item_button_size).
 * - Only the item button extends with additional horizontal space,
 *		while both buttons and the label grow with excess vertical space.
 *		Less space than preferred causes clipping.
 * - The dropdown listbox at least occupies gui_listbox::preferred_size().
 *		The listbox width also increases to cover both dropdown buttons.
 * - If gui_scroll_area_style::prefer_override_size is set,
 *		the listbox shows a scrollable section of dropdown items().
 *		Otherwise, the listbox shows all items at once.
 *
 ************************************************************************/
class gui_dropdown :
	public gui_element
{
	friend class gui_dropdown_modal_helper;

	public:

		gui_dropdown
			(const string& name, gui_context& gui,
			 const shared_instance<gui_dropdown_style>& dropdown_style);

		~gui_dropdown() NOTHROW override;


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/** Manage dropdown label. */
		//@{
		void set_label_text(const string& label_text);
		string label_text() const NOTHROW;
		//@}


		/**
		 * Item button size, increases on layouting
		 * to accomodate any selected item.
		 *
		 * The cached button size does not shrink on smaller items,
		 * which keeps layouting consistent. However, clients can
		 * explicitly reset_known_item_button_size(), for instance
		 * after resetting the dropdown items().
		 */ 
		//@{
		const vector2f& known_item_button_size() const NOTHROW;
		void reset_known_item_button_size() NOTHROW;
		//@}

		
		/** Forward calls to the internal list(). */
		//@{
		void select_index(int64 index) NOTHROW;
		int64 selected_index() const NOTHROW;
		void synchronize_item_indices() NOTHROW;
		vector2f known_item_size() const NOTHROW;
		int64 known_items_per_stack() const NOTHROW;
		void reset_known_item_size() NOTHROW;
		//@}


		/** Items of the internal list(). */
		//@{
		typedef owning_array<gui_list_item> item_array;
		item_array& items();
		const item_array& items() const;
		//@}


		/** Access the internal gui_list. */
		//@{
		gui_list& list() NOTHROW;
		const gui_list& list() const NOTHROW;
		//@}


		const shared_instance<gui_dropdown_style> dropdown_style;


	private:		

		void realize_layout() override;

		void show_listbox();
		void notify_listbox() NOTHROW;
		void hide_listbox() NOTHROW;


		const shared_instance<gui_label> label_;
		const shared_instance<gui_button> item_button_;
		const shared_instance<gui_button> icon_button_;
		const shared_instance<gui_listbox> listbox_;
		const shared_instance<gui_dropdown_modal_helper> modal_helper_;

		vector2f known_item_button_size_;

		bool listbox_notified_;
};


/**
 * Sent if the selection in a gui_dropdown has potentially changed.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_dropdown> gui_dropdown_event;




/**
 *************************************************************************
 *
 * @class gui_dropdown_modal_helper
 * @ingroup gui_module
 *
 * Internal utility that wraps the gui_listbox of a gui_dropdown
 * into a modal layer of gui_context::modal_stack().
 *
 ************************************************************************/
class gui_dropdown_modal_helper :
	public gui_element
{
	friend class gui_dropdown;

	public:

		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;

	private:

		gui_dropdown_modal_helper
			(const string& name, gui_context& gui,
			 const shared_instance<gui_dropdown_style>& dropdown_style,
			 gui_dropdown& dropdown,
			 const shared_instance<gui_listbox>& listbox);

		void realize_layout() override;

		const shared_instance<gui_dropdown_style> dropdown_style_;
		gui_dropdown* dropdown_;
		const shared_instance<gui_listbox> listbox_;
		const shared_instance<gui_frame> background_;
};




/**
 *************************************************************************
 *
 * @class gui_dropdown_style
 * @ingroup gui_module
 *
 * Styling for a gui_dropdown element.
 *
 ************************************************************************/
class gui_dropdown_style :
	public gui_style
{
	public:

		gui_dropdown_style
			(const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_listbox_style>& listbox_style,
			 const vector2f& min_item_button_size,
			 const shared_instance<render_material_id>& dropdown_icon,
			 const vector2i& icon_pixel_size,
			 const shared_instance<gui_frame_style>& background_style);


		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_listbox_style> listbox_style;

		const vector2f min_item_button_size;

		const shared_instance<render_material_id> dropdown_icon;
		const vector2i icon_pixel_size;

		const shared_instance<gui_frame_style> background_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_DROPDOWN_HPP) */
