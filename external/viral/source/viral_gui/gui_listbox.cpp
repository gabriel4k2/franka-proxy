/**
 *************************************************************************
 *
 * @file gui_listbox.cpp
 *
 * GUI lists with frame and scrolling, implementation.
 *
 ************************************************************************/


#include "gui_listbox.hpp"


#include "gui_frame.hpp"
#include "gui_scroll_area.hpp"
#include "gui_list.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_listbox
//
//////////////////////////////////////////////////////////////////////////


gui_listbox::gui_listbox
	(const string& name, gui_context& gui,
	 const shared_instance<gui_listbox_style>& listbox_style,
	 orientation_type orientation,
	 gui_list::item_select_mode select_mode)
	:
	gui_element(name, gui),
	listbox_style(listbox_style),

	frame_(new gui_frame(name + "_frame", gui,
						 listbox_style().frame_style)),

	scroll_area_(new gui_scroll_area
		(name + "_scroll", gui,
		 listbox_style().scroll_area_style)), 

	list_(new gui_list(name + "_list", gui,
					   listbox_style().list_style,
					   orientation, select_mode))
{
	insert_child(frame_);
	frame_().set_content(scroll_area_);
	scroll_area_().set_content(list_);
}


vector2f gui_listbox::preferred_size() const
	{ return frame_().preferred_size(); }


void gui_listbox::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	gui_output_event_list child_output;
	frame_().process_input(input, child_output);

	for (gui_output_event_list::const_iterator it
			(child_output.first()); it; ++it)
	{
		if (!*it) continue;
		if ((*it)->source_untyped != list_) continue;

		if (typeid(**it) == typeid(gui_list_event))
		{
			auto_pointer<gui_output_event> event
				(new gui_listbox_event(*this));
			output.insert
				(std::move(event));
		}

		if (typeid(**it) == typeid(gui_list_accept_event))
		{
			auto_pointer<gui_output_event> event
				(new gui_listbox_accept_event(*this));
			output.insert
				(std::move(event));
		}
	}
}


void gui_listbox::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
		{ frame_().commit_render(packer, queue); }


bool gui_listbox::try_accept_key_focus() NOTHROW
{
	// Pass on focus to contained list element.
	list_().acquire_key_focus();
	return false;
}


void gui_listbox::select_index(int64 index) NOTHROW
	{ list_().select_index(index); }

int64 gui_listbox::first_selected_index() const NOTHROW
	{ return list_().first_selected_index(); }

void gui_listbox::synchronize_item_indices() NOTHROW
	{ list_().synchronize_item_indices(); }

vector2f gui_listbox::known_item_size() const NOTHROW
	{ return list_().known_item_size(); }

int64 gui_listbox::known_items_per_stack() const NOTHROW
	{ return list_().known_items_per_stack(); }

void gui_listbox::reset_known_item_size() NOTHROW
	{ list_().reset_known_item_size(); }


gui_listbox::item_array& gui_listbox::items() NOTHROW
	{ return list_().items; }
const gui_listbox::item_array& gui_listbox::items() const NOTHROW
	{ return list_().items; }


gui_list& gui_listbox::list() NOTHROW
	{ return list_(); }
const gui_list& gui_listbox::list() const NOTHROW
	{ return list_(); }


vector2f gui_listbox::listbox_size_for_item_count(float item_count)
{
	return 
		frame_().frame_size_for_content_size
			(known_item_size() *
				(list_().orientation == horizontal ?
					vector2f(item_count, 1.f) :
					vector2f(1.f, item_count)));
}


void gui_listbox::realize_layout()
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
// gui_listbox_style
//
//////////////////////////////////////////////////////////////////////////


gui_listbox_style::gui_listbox_style
	(const shared_instance<gui_frame_style>& frame_style,
	 const shared_instance<gui_scroll_area_style>& scroll_area_style,
	 const shared_instance<gui_list_style>& list_style)
	:
	frame_style(frame_style),
	scroll_area_style(scroll_area_style),
	list_style(list_style)
{ }




} /* namespace viral_gui */
