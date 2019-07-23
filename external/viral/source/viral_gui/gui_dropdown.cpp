/**
 *************************************************************************
 *
 * @file gui_dropdown.cpp
 * 
 * GUI dropdown boxes, implementation.
 *
 ************************************************************************/


#include "gui_dropdown.hpp"


#include <viral_core/log.hpp>


#include "gui.hpp"
#include "gui_event.hpp"
#include "gui_stack.hpp"
#include "gui_label.hpp"
#include "gui_frame.hpp"
#include "gui_button.hpp"
#include "gui_list.hpp"
#include "gui_listbox.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_dropdown
//
//////////////////////////////////////////////////////////////////////////


gui_dropdown::gui_dropdown
	(const string& name, gui_context& gui,
	 const shared_instance<gui_dropdown_style>& dropdown_style)
	:
	gui_element(name, gui),
	dropdown_style(dropdown_style),

	label_(new gui_label
		(name + "_label", gui,
		 dropdown_style().label_style, "")),

	item_button_(new gui_button
		(name + "_item_button", gui,
		 dropdown_style().button_style, gui_button::click_button)),
	icon_button_(new gui_button
		(name + "_icon_button", gui,
		 dropdown_style().button_style, gui_button::click_button)),

	listbox_(new gui_listbox
		(name + "_listbox", gui,
		 dropdown_style().listbox_style,
		 gui_list::vertical, gui_list::select_single)),

	modal_helper_
		(new gui_dropdown_modal_helper
			(name + "_modal_helper", gui,
			 dropdown_style, *this, listbox_)),

	known_item_button_size_(dropdown_style().min_item_button_size),
	listbox_notified_(false)
{
	insert_child(label_);
	insert_child(item_button_);
	insert_child(icon_button_);

	// Our listbox_ is a child of modal_helper_.
	// Both appear on the GUI stack on demand only.

	icon_button_().set_graphics
		(dropdown_style().icon_pixel_size,
		 dropdown_style().dropdown_icon);
}


gui_dropdown::~gui_dropdown() NOTHROW
{
	// We might be destroyed separately from our modal_helper_
	// due to some stray shared_pointer to the modal helper.
	// Thus, reset non-managed back-pointer on this occasion.
	modal_helper_().dropdown_ = NULL;
}


vector2f gui_dropdown::preferred_size() const
{
	vector2f label_size(label_().preferred_size());
	vector2f icon_button_size(icon_button_().preferred_size());

	return vector2f
		(label_size.x + known_item_button_size_.x + icon_button_size.x,
		 max(label_size.y, max(known_item_button_size_.y, icon_button_size.y)));
}


void gui_dropdown::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// The incoming event (or another preceding event,
	// e.g. due to an event-consuming parent) potentially
	// triggered an earlier selection on our listbox_.
	// Therefore, hide the listbox and notify client.
	if (listbox_notified_)
	{
		hide_listbox();
		listbox_notified_ = false;

		want_layout();

		auto_pointer<gui_output_event> event
			(new gui_dropdown_event(*this));
		output.insert
			(std::move(event));
	}


	// Once the user clicks either of the buttons,
	// show the modal dropdown listbox.
	gui_output_event_list child_output;
	item_button_().process_input(input, child_output);
	icon_button_().process_input(input, child_output);

	for (gui_output_event_list::iterator it
			(child_output.first()); it; ++it)
	{
		if (!*it) continue;

		if (typeid(**it) != typeid(gui_button_event) &&
			typeid(**it) != typeid(gui_button_repeat_event)) continue;

		if ((*it)->source_untyped != item_button_ &&
			(*it)->source_untyped != icon_button_) continue;

		show_listbox();
	}
}


void gui_dropdown::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;
	label_().commit_render(packer, queue);
	item_button_().commit_render(packer, queue);
	icon_button_().commit_render(packer, queue);
}


void gui_dropdown::set_label_text(const string& label_text)
{
	label_().set_text(label_text);
	want_layout();
}

string gui_dropdown::label_text() const NOTHROW
	{ return label_().text(); }


const vector2f& gui_dropdown::known_item_button_size() const NOTHROW
	{ return known_item_button_size_; }

void gui_dropdown::reset_known_item_button_size() NOTHROW
{
	known_item_button_size_ = dropdown_style().min_item_button_size;
	want_layout();
}


void gui_dropdown::select_index(int64 index) NOTHROW
{
	listbox_().select_index(index);
	want_layout();
}

int64 gui_dropdown::selected_index() const NOTHROW
	{ return listbox_().first_selected_index(); }

void gui_dropdown::synchronize_item_indices() NOTHROW
{
	listbox_().synchronize_item_indices();
	want_layout();
}

vector2f gui_dropdown::known_item_size() const NOTHROW
	{ return listbox_().known_item_size(); }
int64 gui_dropdown::known_items_per_stack() const NOTHROW
	{ return listbox_().known_items_per_stack(); }
void gui_dropdown::reset_known_item_size() NOTHROW
	{ listbox_().reset_known_item_size(); }


gui_dropdown::item_array& gui_dropdown::items()
	{ return listbox_().items(); }
const gui_dropdown::item_array& gui_dropdown::items() const
	{ return listbox_().items(); }


gui_list& gui_dropdown::list() NOTHROW
	{ return listbox_().list(); }
const gui_list& gui_dropdown::list() const NOTHROW
	{ return listbox_().list(); }


void gui_dropdown::realize_layout()
{
	if (layout_invalid())
	{
		label_().clip_entirely();
		item_button_().clip_entirely();
		icon_button_().clip_entirely();
		return;
	}


	// Synchronize item button to current listbox selection.
	// If this increases known_item_button_size_, stay_dirty() 
	// to make our parent provide more space.
	const item_array& items = listbox_().items();
	int64 selection = listbox_().first_selected_index();

	if (selection >= 0 &&
		selection < items.size() &&
		items[selection])
	{
		item_button_().set_text(items[selection]->text);

		if (items[selection]->icon)
			item_button_().set_graphics
				(dropdown_style().listbox_style().list_style().icon_pixel_size,
				 items[selection]->icon.instance_or_throw());
		else
			item_button_().clear_graphics();
	}
	else
	{
		item_button_().set_text("");
		item_button_().clear_graphics();
	}

	vector2f button_preferred
		(item_button_().preferred_size());

	if (button_preferred.x > known_item_button_size_.x ||
		button_preferred.y > known_item_button_size_.y)
	{
		known_item_button_size_ =
			known_item_button_size_.max(button_preferred);

		stay_dirty();
	}


	// Fetch button arrangement area.
	// Clip on less than preferred_size().
	rect2f ar_area(arrange_area());
	ar_area.size = ar_area.size.max(preferred_size());


	// Position right-hand side icon button.
	vector2f icon_button_size(icon_button_().preferred_size());
	rect2f icon_button_arrange
		(ar_area.pos + vector2f(ar_area.size.x - icon_button_size.x, 0),
		 vector2f(icon_button_size.x, ar_area.size.y));

	rect2f icon_button_clip;
	if (icon_button_arrange.size.min() > 0 &&
		icon_button_arrange.try_intersect(clip_area(), icon_button_clip))
	{
		icon_button_().layout(icon_button_arrange, icon_button_clip);
	}
	else
		icon_button_().clip_entirely();


	// Position left-hand side label,
	// depending on current label size.
	vector2f label_size(label_().preferred_size());
	rect2f label_arrange
		(ar_area.pos, vector2f(label_size.x, ar_area.size.y));

	rect2f label_clip;
	if (label_arrange.size.min() > 0 &&
		label_arrange.try_intersect(clip_area(), label_clip))
	{
		label_().layout(label_arrange, label_clip);
	}
	else
		label_().clip_entirely();


	// Position item button in center.
	// The button grows to occupy any excess space,
	// and clips on less than preferred_size().
	rect2f item_button_arrange
		(ar_area.pos + vector2f(label_size.x, 0),
		 ar_area.size - vector2f(label_size.x, 0)
					  - vector2f(icon_button_size.x, 0));

	rect2f item_button_clip;
	if (item_button_arrange.size.min() > 0 &&
		item_button_arrange.try_intersect(clip_area(), item_button_clip))
	{
		item_button_().layout(item_button_arrange, item_button_clip);
	}
	else
		item_button_().clip_entirely();
}


void gui_dropdown::show_listbox()
{
	gui.modal_stack().insert_layer(modal_helper_);

	// Dropdown listbox starts out with key focus.
	listbox_().acquire_key_focus();

	// Rebuild layout of modal helper to synchronize
	// listbox size and position with dropdown buttons.
	modal_helper_().want_layout();
}

void gui_dropdown::notify_listbox() NOTHROW
	{ listbox_notified_ = true; }

void gui_dropdown::hide_listbox() NOTHROW
{
	gui.modal_stack().remove_layer(modal_helper_);

	shared_pointer<gui_element> this_dropdown
		(fetch_shared_from_parent());

	// Retrieve focus from dropdown listbox.
	if (this_dropdown)
		this_dropdown->acquire_key_focus();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_dropdown_modal_helper
//
//////////////////////////////////////////////////////////////////////////


vector2f gui_dropdown_modal_helper::preferred_size() const
	{ return listbox_().preferred_size(); }


void gui_dropdown_modal_helper::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// Notify bound gui_dropdown element of selection inside
	// and clicks outside the listbox. In either case, our
	// dropdown will remove us from the GUI stack later on.

	gui_output_event_list child_output;
	listbox_().process_input(input, child_output);

	for (gui_output_event_list::iterator it
			(child_output.first()); it; ++it)
	{
		if (!*it) continue;

		if (typeid(**it) != typeid(gui_listbox_event) &&
			typeid(**it) != typeid(gui_listbox_accept_event)) continue;

		if (dropdown_) dropdown_->notify_listbox();
	}

	if (input.hot &&
		typeid(input) == typeid(gui_mouse_event))
	{
		gui_mouse_event& mouse_event =
			static_cast<gui_mouse_event&>(input);

		if (mouse_event.event == gui_mouse_event::button_click &&
			mouse_event.button == gui_mouse_event::left_button)
		{
			if (dropdown_)
				dropdown_->notify_listbox();
		}
	}

	input.hot = false;
}


void gui_dropdown_modal_helper::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;	
	background_().commit_render(packer, queue);
	listbox_().commit_render(packer, queue);
}
	

gui_dropdown_modal_helper::gui_dropdown_modal_helper
	(const string& name, gui_context& gui,
	 const shared_instance<gui_dropdown_style>& dropdown_style,
	 gui_dropdown& dropdown,
	 const shared_instance<gui_listbox>& listbox)
	:
	gui_element(name + "_modal_utility", gui),
	dropdown_style_(dropdown_style),
	dropdown_(&dropdown),
	listbox_(listbox),

	background_(new gui_frame
		(name + "_background", gui,
		 dropdown_style().background_style))
{
	insert_child(listbox_);
	insert_child(background_);
}


void gui_dropdown_modal_helper::realize_layout()
{
	if (layout_invalid())
	{
		listbox_().clip_entirely();
		background_().clip_entirely();
		return;
	}

	background_().layout(arrange_area(), clip_area());


	// Occupy at least preferred size,
	// but also match button width if possible.
	vector2f listbox_size
		(listbox_().preferred_size());

	if (dropdown_ && !dropdown_->layout_invalid())
	{
		listbox_size.x =
			max(listbox_size.x,
				dropdown_->item_button_().arrange_area().size.x +
				dropdown_->icon_button_().arrange_area().size.x);
	}


	// Right-align listbox with dropdown icon button,
	// or show at top-left if dropdown layout is invalid
	// (which could happen under rare circumstances).
	vector2f listbox_position
		(!dropdown_ || dropdown_->layout_invalid() ?
			vector2f() :
			vector2f(dropdown_->arrange_area().end_x() - listbox_size.x,
				     dropdown_->arrange_area().end_y()));


	// If the listbox exceeds available vertical space,
	// we raise it until its bottom becomes visible.
	// This fix is sufficient because the listbox shows
	// directly under the usually visible dropdown_.
	float list_end_y = listbox_position.y + listbox_size.y;
	float arrange_end_y = arrange_area().end_y();
	if (list_end_y > arrange_end_y)
		listbox_position.y -= (list_end_y - arrange_end_y);


	listbox_().layout
		(rect2f(listbox_position, listbox_size), clip_area());
}




//////////////////////////////////////////////////////////////////////////
//
// gui_dropdown_style
//
//////////////////////////////////////////////////////////////////////////


gui_dropdown_style::gui_dropdown_style
	(const shared_instance<gui_label_style>& label_style,
	 const shared_instance<gui_button_style>& button_style,
	 const shared_instance<gui_listbox_style>& listbox_style,
	 const vector2f& min_item_button_size,
	 const shared_instance<render_material_id>& dropdown_icon,
	 const vector2i& icon_pixel_size,
	 const shared_instance<gui_frame_style>& background_style)
	:
	label_style(label_style),
	button_style(button_style),
	listbox_style(listbox_style),
	min_item_button_size(min_item_button_size),
	dropdown_icon(dropdown_icon),
	icon_pixel_size(icon_pixel_size),
	background_style(background_style)
{
	if (min_item_button_size.min() <= 0)
	{
		LOG_ERROR("Minimum item button size must be greater than zero.");
		throw invalid_parameter();
	}

	if (icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("List icon size must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
