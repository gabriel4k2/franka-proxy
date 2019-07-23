/**
 *************************************************************************
 *
 * @file gui_editbox.hpp
 *
 * Edit elements with frames and scrolling, implementation.
 *
 ************************************************************************/


#include "gui_editbox.hpp"


#include "gui_event.hpp"
#include "gui_frame.hpp"
#include "gui_scroll_area.hpp"
#include "gui_edit.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_editbox
//
//////////////////////////////////////////////////////////////////////////


gui_editbox::gui_editbox
	(const string& name, gui_context& gui,
	 const shared_instance<gui_editbox_style>& editbox_style,
	 bool multiline, bool notify_accept)
	:
	gui_element(name, gui),
	editbox_style(editbox_style),

	frame_(new gui_frame(name + "_frame", gui,
						 editbox_style().frame_style)),

	scroll_area_
		(new gui_scroll_area
			(name + "_scroll", gui,
			 editbox_style().scroll_area_style)), 

	edit_(new gui_edit(name + "_edit", gui,
					   editbox_style().edit_style,
					   multiline, notify_accept))
{
	insert_child(frame_);
	frame_().set_content(scroll_area_);
	scroll_area_().set_content(edit_);
}


vector2f gui_editbox::preferred_size() const
	{ return frame_().preferred_size(); }


void gui_editbox::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	gui_output_event_list child_output;
	frame_().process_input(input, child_output);

	for (gui_output_event_list::const_iterator it
			(child_output.first()); it; ++it)
	{
		if (!*it) continue;
		if ((*it)->source_untyped != edit_) continue;

		if (typeid(**it) == typeid(gui_edit_event))
		{
			auto_pointer<gui_output_event> event
				(new gui_editbox_event(*this));
			output.insert
				(std::move(event));
		}

		if (typeid(**it) == typeid(gui_edit_accept_event))
		{
			auto_pointer<gui_output_event> event
				(new gui_editbox_accept_event(*this));
			output.insert
				(std::move(event));
		}

		if (typeid(**it) == typeid(gui_edit_cancel_event))
		{
			auto_pointer<gui_output_event> event
				(new gui_editbox_cancel_event(*this));
			output.insert
				(std::move(event));
		}
	}
}


void gui_editbox::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
		{ frame_().commit_render(packer, queue); }


bool gui_editbox::try_accept_key_focus() NOTHROW
{
	// Pass on focus to contained edit element.
	edit_().acquire_key_focus();
	return false;
}


void gui_editbox::set_read_only(bool read_only) NOTHROW
	{ edit_().set_read_only(read_only); }
bool gui_editbox::read_only() const NOTHROW
	{ return edit_().read_only(); }


void gui_editbox::set_text(const string& text)
	{ edit_().set_text(text); }
void gui_editbox::set_text(const string& text, bool allow_undo)
	{ edit_().set_text(text, allow_undo); }
string gui_editbox::text() const
	{ return edit_().text(); }


void gui_editbox::append_text(const string& text)
	{ edit_().append_text(text); }
void gui_editbox::append_text(const string& text, bool allow_undo)
	{ edit_().append_text(text, allow_undo); }


void gui_editbox::set_history_step_limit(int64 history_step_limit) NOTHROW
	{ edit_().set_history_step_limit(history_step_limit); }
int64 gui_editbox::history_step_limit() const NOTHROW
	{ return edit_().history_step_limit(); }
void gui_editbox::set_history_memory_limit(int64 history_memory_limit) NOTHROW
	{ edit_().set_history_memory_limit(history_memory_limit); }
int64 gui_editbox::history_memory_limit() const NOTHROW
	{ return edit_().history_memory_limit(); }


float gui_editbox::known_line_width() const NOTHROW
	{ return edit_().known_line_width(); }
void gui_editbox::reset_known_line_width() NOTHROW
	{ edit_().reset_known_line_width(); }


gui_edit& gui_editbox::edit() NOTHROW
	{ return edit_(); }
const gui_edit& gui_editbox::edit() const NOTHROW
	{ return edit_(); }


void gui_editbox::realize_layout()
{
	if (layout_invalid())
	{
		frame_().clip_entirely();
		return;
	}

	frame_().layout(arrange_area(), clip_area());
}




//////////////////////////////////////////////////////////////////////////
//
// gui_editbox_style
//
//////////////////////////////////////////////////////////////////////////


gui_editbox_style::gui_editbox_style
	(const shared_instance<gui_frame_style>& frame_style,
	 const shared_instance<gui_scroll_area_style>& scroll_area_style,
	 const shared_instance<gui_edit_style>& edit_style)
	:
	frame_style(frame_style),
	scroll_area_style(scroll_area_style),
	edit_style(edit_style)
{ }




} /* namespace viral_gui */