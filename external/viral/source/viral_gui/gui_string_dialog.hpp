/**
 *************************************************************************
 *
 * @file gui_string_dialog.hpp
 *
 * Modal string entry.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_STRING_DIALOG_HPP)
#define INCLUDED__VIRAL_GUI__GUI_STRING_DIALOG_HPP


#include <viral_core/build_config.hpp>


#include "gui.hpp"
#include "gui_modal.hpp"
#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_string_dialog
 * @ingroup gui_module
 *
 * A modal dialog that prompts the user for some string.
 *
 ************************************************************************/
class gui_string_dialog :
	public gui_modal_dialog
{
	public:

		/**
		 * Show a dialog that prompts the user to enter a string.
		 *
		 * Usage notes:
		 * - The dialog window shows given @bref{caption} and @bref{message}.
		 * - On success, the dialog returns @true and fills @bref{result}.
		 * - The dialog returns @false on user cancel and throws on failure.
		 *		In either case, the @bref{result} string remains unchanged.
		 *		If @bref{allow_cancel} is @false, cancel and close are hidden,
		 *		the user cannot cancel, and the dialog never returns @false.
		 * - The string prompt defaults to the initial value of @bref{result}
		 *		(e.g. from a previous gui_string_dialog invocation).
		 */
		static bool show
			(gui_context& gui,
			 const shared_instance<gui_string_dialog_style>& string_dialog_style,
			 const string& caption,
			 const string& message,
			 bool allow_cancel,
			 string& result); 


	private:

		gui_string_dialog
			(gui_context& gui,
			 const shared_instance<gui_string_dialog_style>& string_dialog_style,
			 const string& caption,
			 const string& message,
			 bool allow_cancel,
			 const string& result_initial);


		void setup() override;

		shared_instance<gui_window> dialog_window() override;
		shared_pointer<gui_element> dialog_background() override;


		void process_events
			(const gui_output_event_list& output) override;

		void accept();
		void cancel();

		shared_instance<gui_grid> create_vertical_grid();
		shared_instance<gui_grid> create_button_grid();


		const shared_instance<gui_string_dialog_style> string_dialog_style_;
		const string caption_;
		const string message_;
		const bool allow_cancel_;

		string result_;

		const shared_instance<gui_window> window_;
		const shared_instance<gui_frame> background_;
		const shared_instance<gui_label> message_label_;
		const shared_instance<gui_editbox> editbox_;
		const shared_instance<gui_button> ok_button_;
		const shared_instance<gui_button> cancel_button_;

		bool success_;
};




/**
 *************************************************************************
 *
 * @class gui_string_dialog_style
 * @ingroup gui_module
 *
 * Styling options for a gui_string_dialog.
 *
 ************************************************************************/
class gui_string_dialog_style :
	public gui_style
{
	public:

		gui_string_dialog_style
			(const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_editbox_style>& editbox_style,
			 const shared_instance<gui_window_style>& window_style,
			 const shared_instance<gui_frame_style>& background_style,
			 const vector2f& initial_client_size);

		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_editbox_style> editbox_style;
		const shared_instance<gui_window_style> window_style;
		const shared_instance<gui_frame_style> background_style;
		const vector2f initial_client_size;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_STRING_DIALOG_HPP) */