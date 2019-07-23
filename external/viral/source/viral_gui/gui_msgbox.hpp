/**
 *************************************************************************
 *
 * @file gui_msgbox.hpp
 *
 * GUI message boxes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_MSGBOX_HPP)
#define INCLUDED__VIRAL_GUI__GUI_MSGBOX_HPP


#include <viral_core/build_config.hpp>


#include "gui.hpp"
#include "gui_modal.hpp"
#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_msgbox
 * @ingroup gui_module
 *
 * Run-of-the-mill modal message box.
 *
 ************************************************************************/
class gui_msgbox :
	private gui_modal_dialog
{
	public:

		enum icon_type
			{ no_icon, icon_question, icon_info,
			  icon_warn, icon_error };

		enum choice_type
			{ choice_ok, choice_ok_cancel,
			  choice_cancel, choice_yes_no_cancel };

		enum default_type
			{ default_none,
			  default_ok, default_cancel,
			  default_yes, default_no };


		enum result_type
		{
			/** Never returned, throws assert_failed instead. */
			result_unknown,
				
			result_ok, result_cancel,
			result_yes, result_no
		};


		/**
		 * Show a modal message box with given parameters
		 * and return the button the user clicked.
		 *
		 * @attent
		 * Closing the window always returns result_cancel,
		 * even if the message box has no cancel button.
		 */
		static result_type show
			(gui_context& gui,
			 const shared_instance<gui_msgbox_style>& msgbox_style,
			 const string& text, const string& caption,
			 icon_type icon, choice_type choice, default_type default);
		

	private:

		gui_msgbox
			(gui_context& gui,
			 const shared_instance<gui_msgbox_style>& msgbox_style,
			 const string& text, const string& caption,
			 icon_type icon, choice_type choice, default_type default);


		void setup() override;

		shared_instance<gui_window> dialog_window() override;
		shared_pointer<gui_element> dialog_background() override;

		void process_events
			(const gui_output_event_list& output) override;


		shared_instance<gui_grid> create_vertical_grid();
		shared_instance<gui_grid> create_icon_text_grid();
		shared_instance<gui_grid> create_button_grid();


		const shared_instance<gui_msgbox_style> msgbox_style_;

		const string text_;
		const string caption_;
		const icon_type icon_;
		const choice_type choice_;
		const default_type default_;

		const shared_instance<gui_button> ok_button_;
		const shared_instance<gui_button> cancel_button_;
		const shared_instance<gui_button> yes_button_;
		const shared_instance<gui_button> no_button_;

		const shared_instance<gui_window> window_;
		const shared_instance<gui_frame> background_;

		result_type result_;
};




/**
 *************************************************************************
 *
 * @class gui_msgbox_style
 * @ingroup gui_module
 *
 * Overall styling for gui_msgbox%es.
 *
 ************************************************************************/
class gui_msgbox_style :
	public gui_style
{
	public:

		gui_msgbox_style
			(const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_window_style>& window_style,

			 const shared_instance<gui_image_style>& icon_image_style,
			 const shared_instance<render_material_id>& icon_question,
			 const shared_instance<render_material_id>& icon_info,
			 const shared_instance<render_material_id>& icon_warn,
			 const shared_instance<render_material_id>& icon_error,
			 const vector2i& icon_pixel_size,

			 const shared_instance<gui_frame_style>& background_style);

		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_window_style> window_style;

		const shared_instance<gui_image_style> icon_image_style;
		const shared_instance<render_material_id> icon_question;
		const shared_instance<render_material_id> icon_info;
		const shared_instance<render_material_id> icon_warn;
		const shared_instance<render_material_id> icon_error;
		const vector2i icon_pixel_size;

		const shared_instance<gui_frame_style> background_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_MSGBOX_HPP) */