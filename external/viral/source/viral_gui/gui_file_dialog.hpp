/**
 *************************************************************************
 *
 * @file gui_file_dialog.hpp
 *
 * GUI file selection dialog.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__FILE_DIALOG_HPP)
#define INCLUDED__VIRAL_GUI__FILE_DIALOG_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/file_forward.hpp>


#include <viral_core/list.hpp>


#include "gui.hpp"
#include "gui_modal.hpp"
#include "gui_element.hpp"
#include "gui_list.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_file_dialog
 * @ingroup gui_file_module
 *
 * Modal dialog that allows the user to select one or more files.
 *
 ************************************************************************/
class gui_file_dialog :
	public gui_modal_dialog
{
	public:

		enum dialog_mode
			{ save_file, open_file, open_multiple_files };


		/**
		 * Show a file selection dialog.
		 *
		 * Usage notes:
		 * - The dialog @bref{mode} defines what files the user may select.
		 * - The @bref{caption} tells the user what general object to select.
		 * - File selection starts in the given @bref{base_path}.
		 * - File selection is restricted to any of given @bref{extensions},
		 *		which come in a gui_dropdown along their @bref{descriptions}.
		 * - On success, the call returns @true and stores resulting pathes
		 *		(depending on mode) to @bref{selected_files}.
		 * - The call returns @false on user cancel and throws on failure.
		 *		In either case, the @bref{selected_files} remain unchanged.
		 *		If @bref{allow_cancel} is @false, cancel and close are hidden,
		 *		the user cannot cancel, and the dialog never returns @false.
		 */
		static bool show
			(gui_context& gui,
			 file_context& files,
			 const shared_instance<gui_file_dialog_style>& file_dialog_style,
			 dialog_mode mode,
			 const string& caption,
			 const string& base_path,
			 const list<string>& extensions,
			 const list<string>& descriptions,
			 bool allow_cancel,
			 list<string>& selected_files); 


	private:

		gui_file_dialog
			(gui_context& gui,
			 file_context& files,
			 const shared_instance<gui_file_dialog_style>& file_dialog_style,
			 dialog_mode mode,
			 const string& caption,
			 const string& base_path,
			 const list<string>& extensions,
			 const list<string>& descriptions,
			 bool allow_cancel,
			 const list<string>& selected_files_initial);


		void setup() override;

		shared_instance<gui_window> dialog_window() override;
		shared_pointer<gui_element> dialog_background() override;


		void process_events
			(const gui_output_event_list& output) override;

		void go_up_path();

		void accept();
		void cancel();


		shared_instance<gui_grid> create_vertical_grid();
		shared_instance<gui_grid> create_parent_path_grid();
		shared_instance<gui_grid> create_edit_dropdown_grid();
		shared_instance<gui_grid> create_button_grid();


		void update_parent_path_label();
		void update_extension_dropdown();
		void update_listbox_content();

		void update_editbox_from_listbox();
		void update_listbox_from_editbox();

		bool check_change_folder();

		void update_selected_files();
		void update_selected_file_from_editbox();
		void update_selected_files_from_listbox();

		bool check_selected_files();


		file_context& files_;

		const shared_instance<gui_file_dialog_style> file_dialog_style_;		
		const dialog_mode mode_;
		const string caption_;
		const string base_path_;
		const list<string> extensions_;
		const list<string> descriptions_;
		const bool allow_cancel_;

		list<string> selected_files_;

		string current_parent_path_;

		const shared_instance<gui_window> window_;
		const shared_instance<gui_frame> background_;

		const shared_instance<gui_button> go_up_path_button_;
		const shared_instance<gui_label> parent_path_label_;
		const shared_instance<gui_button> refresh_button_;

		const shared_instance<gui_listbox> paths_listbox_;
		const shared_instance<gui_editbox> path_editbox_;
		const shared_instance<gui_dropdown> extension_dropdown_;

		const shared_instance<gui_button> ok_button_;
		const shared_instance<gui_button> cancel_button_;

		bool success_;
};




/**
 *************************************************************************
 *
 * @class gui_file_dialog_style
 * @ingroup gui_module
 *
 * Overall styling for gui_file_dialog%s.
 *
 ************************************************************************/
class gui_file_dialog_style :
	public gui_style
{
	public:

		gui_file_dialog_style
			(const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_listbox_style>& listbox_style,
			 const shared_instance<gui_dropdown_style>& dropdown_style,
			 const shared_instance<gui_editbox_style>& editbox_style,
			 const shared_instance<gui_window_style>& window_style,
			 const shared_instance<render_material_id>& icon_folder,
			 const shared_instance<render_material_id>& icon_file,
			 const shared_instance<gui_msgbox_style>& msgbox_style,
			 const shared_instance<gui_frame_style>& background_style,
			 const vector2f& initial_client_size);

		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_listbox_style> listbox_style;
		const shared_instance<gui_dropdown_style> dropdown_style;
		const shared_instance<gui_editbox_style> editbox_style;
		const shared_instance<gui_window_style> window_style;

		const shared_instance<render_material_id> icon_folder;
		const shared_instance<render_material_id> icon_file;

		const shared_instance<gui_msgbox_style> msgbox_style;
		const shared_instance<gui_frame_style> background_style;

		const vector2f initial_client_size;
};




/**
 *************************************************************************
 *
 * @class gui_file_dialog_item
 * @ingroup gui_file_module
 *
 * A gui_list_item used by gui_file_dialog.
 *
 ************************************************************************/
class gui_file_dialog_item :
	public gui_list_item
{
	public:

		gui_file_dialog_item
			(const string& text,
			 const shared_pointer<render_material_id>& icon,
			 bool is_folder)
			:
			gui_list_item(text, icon),
			is_folder(is_folder)
		{ }

		const bool is_folder;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__FILE_DIALOG_HPP) */
