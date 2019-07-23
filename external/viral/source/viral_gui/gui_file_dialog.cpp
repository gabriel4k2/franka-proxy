/**
 *************************************************************************
 *
 * @file gui_file_dialog.cpp
 *
 * GUI file selection dialog, implementation.
 *
 ************************************************************************/


#include "gui_file_dialog.hpp"


#include <viral_core/log.hpp>
#include <viral_core/file.hpp>
#include <viral_core/file_util.hpp>


#include "gui_event.hpp"
#include "gui_label.hpp"
#include "gui_frame.hpp"
#include "gui_anchor.hpp"
#include "gui_grid.hpp"
#include "gui_button.hpp"
#include "gui_list.hpp"
#include "gui_listbox.hpp"
#include "gui_dropdown.hpp"
#include "gui_editbox.hpp"
#include "gui_window.hpp"
#include "gui_msgbox.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_file_dialog
//
//////////////////////////////////////////////////////////////////////////


bool gui_file_dialog::show
	(gui_context& gui,
	 file_context& files,
	 const shared_instance<gui_file_dialog_style>& file_dialog_style,
	 dialog_mode mode,
	 const string& caption,
	 const string& base_path,
	 const list<string>& extensions,
	 const list<string>& descriptions,
	 bool allow_cancel,
	 list<string>& selected_files)
{
	gui_file_dialog file_dialog
		(gui, files, file_dialog_style,
		 mode, caption, base_path,
		 extensions, descriptions,
		 allow_cancel, selected_files);

	file_dialog.execute();
	
	if (file_dialog.success_)
		selected_files = file_dialog.selected_files_;

	return file_dialog.success_;
}


gui_file_dialog::gui_file_dialog
	(gui_context& gui,
	 file_context& files,
	 const shared_instance<gui_file_dialog_style>& file_dialog_style,
	 dialog_mode mode,
	 const string& caption,
	 const string& base_path,
	 const list<string>& extensions,
	 const list<string>& descriptions,
	 bool allow_cancel,
	 const list<string>& selected_files_initial)
	:
	gui_modal_dialog(gui),
	files_(files),
	file_dialog_style_(file_dialog_style),
	mode_(mode),
	caption_(caption),
	base_path_(base_path),
	extensions_(extensions),
	descriptions_(descriptions),
	allow_cancel_(allow_cancel),
	selected_files_(selected_files_initial),
	current_parent_path_(base_path),

	window_(new gui_window
		("file_dialog_window", gui, file_dialog_style().window_style,
		 caption, allow_cancel_, gui_window::user_resize_enlarge)),

	background_(new gui_frame
		("file_dialog_background", gui,
		 file_dialog_style().background_style)),

	go_up_path_button_(new gui_button
		("file_dialog_go_up_path_button", gui,
		 file_dialog_style().button_style, gui_button::click_button)),	
	parent_path_label_(new gui_label
		("file_dialog_parent_path_label", gui,
		 file_dialog_style().label_style, "")),
	refresh_button_(new gui_button
		("file_dialog_refresh_button", gui,
		 file_dialog_style().button_style, gui_button::click_button)),

	paths_listbox_(new gui_listbox
		("file_dialog_paths_listbox", gui, file_dialog_style().listbox_style,
		 gui_element::horizontal,
		 mode == open_multiple_files ?
			gui_list::select_advanced : gui_list::select_single)),

	path_editbox_(new gui_editbox
		("file_dialog_path_editbox", gui,
		 file_dialog_style().editbox_style, false, true)),
	extension_dropdown_(new gui_dropdown
		("file_dialog_extension_dropdown", gui,
		 file_dialog_style().dropdown_style)),

	ok_button_(new gui_button
		("file_dialog_ok_button", gui,
		 file_dialog_style().button_style, gui_button::click_button)),
	cancel_button_(new gui_button
		("file_dialog_cancel_button", gui,
		 file_dialog_style().button_style, gui_button::click_button)),

	success_(false)
{
	go_up_path_button_().set_text("Up");
	refresh_button_().set_text("Refresh");

	ok_button_().set_text("OK");
	cancel_button_().set_text("Cancel");

	window_().set_content(create_vertical_grid());
	window_().set_client_size(file_dialog_style().initial_client_size);
	window_().center_window_delayed();

	update_parent_path_label();
	update_extension_dropdown();
	update_listbox_content();
}


void gui_file_dialog::setup()
{
	gui_modal_dialog::setup();
	ok_button_().acquire_key_focus();
}


shared_instance<gui_window> gui_file_dialog::dialog_window()
	{ return window_; }
shared_pointer<gui_element> gui_file_dialog::dialog_background()
	{ return background_; }


void gui_file_dialog::process_events
	(const gui_output_event_list& output)
{
	for (gui_output_event_list::const_iterator it
			(output.first()); it; ++it)
	{
		if (!*it) continue;

		const shared_instance<gui_element> &source =
			(*it)->source_untyped;
		
		if (typeid(**it) == typeid(gui_button_event))
		{
			if (source == go_up_path_button_)
				go_up_path();
			else if (source == refresh_button_)
				update_listbox_content();
			else if (source == ok_button_)
				accept();
			else if (source == cancel_button_)
				cancel();
		}

		if (typeid(**it) == typeid(gui_dropdown_event) &&
			source == extension_dropdown_)
				update_listbox_content();

		if (typeid(**it) == typeid(gui_listbox_event) &&
			source == paths_listbox_)
				update_editbox_from_listbox();

		if (typeid(**it) == typeid(gui_editbox_event) &&
			source == path_editbox_)
				update_listbox_from_editbox();

		if (typeid(**it) == typeid(gui_listbox_accept_event) &&
			source == paths_listbox_)
				accept();
	
		if (typeid(**it) == typeid(gui_editbox_accept_event) &&
			source == path_editbox_)
				accept();
		
		if (typeid(**it) == typeid(gui_window_close_event) &&
			source == window_)
				cancel();
	}
}


void gui_file_dialog::go_up_path()
{
	current_parent_path_ =
		virtual_path::extract_parent(current_parent_path_);

	update_parent_path_label();
	update_listbox_content();
}


void gui_file_dialog::accept()
{
	if (check_change_folder())
		return;

	update_selected_files();

	if (!check_selected_files())
		return;

	success_= true;
	finish();
}


void gui_file_dialog::cancel()
{
	// Safety precaution: Ignore if cancel is not allowed.
	// This should not happen, as the window close button
	// and the cancel button have been suppressed.
	if (!allow_cancel_) return;

	selected_files_.clear();
	success_ = false;
	finish();
}


shared_instance<gui_grid> gui_file_dialog::create_vertical_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("file_dialog_vgrid", gui, vector2i(1, 4)));
	grid().set_column_weight(0, 1.f);
	grid().set_row_weight(1, 1.f);

	grid().insert_content(create_parent_path_grid(), vector2i(0, 0));
	grid().insert_content(paths_listbox_, vector2i(0, 1));
	grid().insert_content(create_edit_dropdown_grid(), vector2i(0, 2));
	grid().insert_content(create_button_grid(), vector2i(0, 3));

	return grid;
}


shared_instance<gui_grid> gui_file_dialog::create_parent_path_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("file_dialog_ppgrid", gui, vector2i(3, 1)));
	grid().set_column_weight(1, 1.f);

	grid().insert_content(go_up_path_button_, vector2i(0, 0));
	grid().insert_content(parent_path_label_, vector2i(1, 0));	
	grid().insert_content(refresh_button_, vector2i(2, 0));	

	return grid;
}


shared_instance<gui_grid> gui_file_dialog::create_edit_dropdown_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("file_dialog_edgrid", gui, vector2i(2, 1)));
	grid().set_column_weight(0, 0.9f);
	grid().set_column_weight(1, 0.1f);

	if (mode_ != open_multiple_files)
		grid().insert_content(path_editbox_, vector2i(0, 0));

	grid().insert_content(extension_dropdown_, vector2i(1, 0));	

	return grid;
}


shared_instance<gui_grid> gui_file_dialog::create_button_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("file_dialog_bgrid", gui, vector2i(3, 1)));
	grid().set_column_weight(0, 1.f);

	if (allow_cancel_)
	{
		grid().insert_content(ok_button_, vector2i(1, 0));
		grid().insert_content(cancel_button_, vector2i(2, 0));
	}
	else
	{
		grid().insert_content(ok_button_, vector2i(2, 0));
	}
	
	return grid;
}


void gui_file_dialog::update_parent_path_label()
{
	parent_path_label_().set_text
		(current_parent_path_);
}


void gui_file_dialog::update_extension_dropdown()
{
	extension_dropdown_().items().clear();

	for (list<string>::const_iterator
			it_ext(extensions_.first()), it_desc(descriptions_.first());
			it_ext && it_desc; ++it_ext, ++it_desc)
	{
		auto_pointer<gui_list_item> item
			(new gui_list_item(*it_desc + " (*." + *it_ext + ")"));

		extension_dropdown_().items().insert
			(std::move(item));
	}

	extension_dropdown_().reset_known_item_size();
	extension_dropdown_().synchronize_item_indices();
	extension_dropdown_().select_index(0);
}


void gui_file_dialog::update_listbox_content()
{
	/**
	 * Rebuild listbox items in current parent path
	 * and with current extension from a path index
	 * retrieved through the @bref{files_} context.
	 */

	paths_listbox_().items().clear();

	int64 extindex =
		extension_dropdown_().selected_index();

	string extension
		((extindex >= 0 && extindex < extensions_.size()) ?
			extensions_.at(extindex) : string());

	path_index index;
	files_.build_path_index
		(index, current_parent_path_, extension, true, false);

	for (path_index::folder_path_set::const_iterator it
			(index.folder_paths.first()); it; ++it)
	{
		auto_pointer<gui_list_item> new_item
			(new gui_file_dialog_item
				(virtual_path::extract_target(*it),
				 file_dialog_style_().icon_folder, true));

		paths_listbox_().items().insert
			(std::move(new_item));
	}

	for (path_index::file_path_set::const_iterator it
			(index.file_paths.first()); it; ++it)
	{
		auto_pointer<gui_list_item> new_item
			(new gui_file_dialog_item
				(virtual_path::extract_target(*it),
				 file_dialog_style_().icon_file, false));

		paths_listbox_().items().insert
			(std::move(new_item));
	}

	paths_listbox_().reset_known_item_size();
	paths_listbox_().synchronize_item_indices();
	paths_listbox_().select_index(-1);
}


void gui_file_dialog::update_editbox_from_listbox()
{
	/**
	 * On any single-selection dialog, synchronize the editbox text
	 * to some file that the user selected in the listbox.
	 */

	if (mode_ == open_multiple_files)
		return;

	int64 selection =
		paths_listbox_().first_selected_index();

	const gui_list::item_array& items =
		paths_listbox_().items();

	if (selection < 0 || selection >= items.size())
		return;

	if (!items[selection])
		return;

	if (typeid(*items[selection]) != typeid(gui_file_dialog_item))
		return;

	const gui_file_dialog_item& fdi =
		static_cast<const gui_file_dialog_item&>(*items[selection]);
	
	if (fdi.is_folder)
		return;

	path_editbox_().set_text(fdi.text);
}


void gui_file_dialog::update_listbox_from_editbox()
{
	/**
	 * On any single-selection dialog, synchronize the listbox selection
	 * to the filename that the user typed into the editbox.
	 */

	if (mode_ == open_multiple_files)
		return;

	string text(path_editbox_().text());

	const gui_list::item_array& items =
		paths_listbox_().items();

	for (int64 i = 0; i < items.size(); ++i)
	{
		if (!items[i]) continue;
		if (typeid(*items[i]) != typeid(gui_file_dialog_item))
			continue;

		const gui_file_dialog_item& fdi =
			static_cast<const gui_file_dialog_item&>(*items[i]);

		if (fdi.is_folder) continue;
		if (fdi.text != text) continue;
		
		paths_listbox_().select_index(i);
		return;
	}	

	paths_listbox_().select_index(-1);
}


bool gui_file_dialog::check_change_folder()
{
	/**
	 * On OK or listbox double click, change into another folder
	 * if at least a single folder is currently selected.
	 * Otherwise, return @false, which makes the dialog finish().
	 */

	const gui_list::item_array& items =
		paths_listbox_().items();

	for (int64 i = 0; i < items.size(); ++i)
	{
		if (!items[i]) continue;
		if (!items[i]->selected) continue;

		if (typeid(*items[i]) != typeid(gui_file_dialog_item))
			continue;

		const gui_file_dialog_item& fdi =
			static_cast<const gui_file_dialog_item&>(*items[i]);

		if (!fdi.is_folder) continue;
		
		current_parent_path_ =
			current_parent_path_.empty() ?
				fdi.text :
				(current_parent_path_ + string("/") + fdi.text);

		update_parent_path_label();
		update_listbox_content();
		return true;
	}

	return false;
}


void gui_file_dialog::update_selected_files()
{
	if (mode_ == open_multiple_files)
		update_selected_files_from_listbox();
	else
		update_selected_file_from_editbox();
}


void gui_file_dialog::update_selected_file_from_editbox()
{
	/**
	 * Return a single selected file from the editbox.
	 * Note that the editbox also tracks listbox selection.
	 */

	if (mode_ == open_multiple_files)
		return;

	list<string> new_selected_files;
	new_selected_files.push_back
		(current_parent_path_.empty() ?
				path_editbox_().text() :
				(current_parent_path_ + "/" + path_editbox_().text()));

	swap(selected_files_, new_selected_files);
}


void gui_file_dialog::update_selected_files_from_listbox()
{
	/**
	 * Build a list of selected files from the current parent path
	 * and the current selection in the paths listbox.
	 * Only reasonable in @bref{open_multiple_files} mode.
	 */

	if (mode_ != open_multiple_files)
		return;

	list<string> new_selected_files;

	const gui_list::item_array& items =
		paths_listbox_().items();

	for (int64 i = 0; i < items.size(); ++i)
	{
		if (mode_ != open_multiple_files &&
			!new_selected_files.empty())
				break;

		if (!items[i]) continue;
		if (!items[i]->selected) continue;

		if (typeid(*items[i]) != typeid(gui_file_dialog_item))
			continue;

		const gui_file_dialog_item& fdi =
			static_cast<gui_file_dialog_item&>(*items[i]);

		if (fdi.is_folder) continue;

		new_selected_files.push_back
			(current_parent_path_.empty() ?
				fdi.text :
				(current_parent_path_ + "/" + fdi.text));
	}

	swap(new_selected_files, selected_files_);
}


bool gui_file_dialog::check_selected_files()
{
	/**
	 * Check whether current list of selected files is OK.
	 * Otherwise, clear selected files, display error message,
	 * and return @false.
	 */


	SCOPE_GUARD
		(clear_selected_files,
			(selected_files_, &list<string>::clear));


	// Check if multi-file selection is valid.
	if (mode_ == open_multiple_files && selected_files_.empty())
	{
		gui_msgbox::show
			(gui, file_dialog_style_().msgbox_style,
			 "Please select at least a single file.",
			 "No file selected!",
			 gui_msgbox::icon_error,
			 gui_msgbox::choice_ok,
			 gui_msgbox::default_ok);

		return false;
	}


	// Check if single-file selection is valid.
	if (mode_ != open_multiple_files)
	{
		if (selected_files_.empty() ||
			selected_files_.first()->empty() ||
			*selected_files_.first() == current_parent_path_)
		{
			gui_msgbox::show
				(gui, file_dialog_style_().msgbox_style,
				 "Please select a file or type a file name.",
				 "No file selected!",
				 gui_msgbox::icon_error,
				 gui_msgbox::choice_ok,
				 gui_msgbox::default_ok);

			return false;	
		}

		if (selected_files_.size() > 1)
		{
			// Note this error is just a precaution:
			// Single-selection uses only the text
			// from the path edit box as file name.
			gui_msgbox::show
				(gui, file_dialog_style_().msgbox_style,
				 "Please select only a single file.",
				 "Too many files selected!",
				 gui_msgbox::icon_error,
				 gui_msgbox::choice_ok,
				 gui_msgbox::default_ok);

			return false;
		}
	}


	// Check individual file names.
	for (list<string>::const_iterator it
			(selected_files_.first()); it; ++it)
	{
		if (!virtual_path::is_valid(*it))
		{
			gui_msgbox::show
				(gui, file_dialog_style_().msgbox_style,
					string("Selected file:\n") + *it +
					"\nis not a valid virtual path.",
					"Invalid file name!",
					gui_msgbox::icon_error,
					gui_msgbox::choice_ok,
					gui_msgbox::default_ok);

			return false;
		}

		if ((mode_ == open_file || mode_ == open_multiple_files) &&
			!files_.does_file_exist(*it))
		{
			gui_msgbox::show
				(gui, file_dialog_style_().msgbox_style,
				 string("Selected file:\n") + *it +
				 "\ndoes not exist.",
				 "Invalid file selection!",
				 gui_msgbox::icon_error,
				 gui_msgbox::choice_ok,
				 gui_msgbox::default_ok);

			return false;
		}

		if (mode_ == save_file && files_.does_file_exist(*it))
		{
			gui_msgbox::result_type result = gui_msgbox::show
				(gui, file_dialog_style_().msgbox_style,
				 string("Selected file:\n") + *it +
				 "\nexists already.\n\nOverwrite?",
				 "Overwrite file?",
				 gui_msgbox::icon_question,
				 gui_msgbox::choice_ok_cancel,
				 gui_msgbox::default_cancel);

			if (result != gui_msgbox::result_ok)
				return false;
		}
	}


	clear_selected_files.dismiss();
	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// gui_file_dialog_style
//
//////////////////////////////////////////////////////////////////////////


gui_file_dialog_style::gui_file_dialog_style
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
	 const vector2f& initial_client_size)
	:
	label_style(label_style),
	button_style(button_style),
	listbox_style(listbox_style),
	dropdown_style(dropdown_style),
	editbox_style(editbox_style),
	window_style(window_style),
	icon_folder(icon_folder),
	icon_file(icon_file),
	msgbox_style(msgbox_style),
	background_style(background_style),
	initial_client_size(initial_client_size)
{
	if (initial_client_size.min() <= 0)
	{
		LOG_ERROR("Initial client size must be positive.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
