/**
 *************************************************************************
 *
 * @file gui_tristate.cpp
 *
 * Clickable GUI elements with three states, implementation.
 *
 ************************************************************************/


#include "gui_tristate.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_button.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_tristate
//
//////////////////////////////////////////////////////////////////////////


gui_tristate::gui_tristate
	(const string& name, gui_context& gui,
	 const shared_instance<gui_tristate_style>& tristate_style,
	 select_mode mode)
	:
	gui_element(name, gui),
	tristate_style(tristate_style),
	mode(mode),
	state_(state_no),
	button_(new gui_button(name + "_button", gui,
						   tristate_style().button_style, gui_button::click_button)),
	label_(new gui_label(name + "_label", gui,
						 tristate_style().button_style().label_style, ""))
{
	insert_child(button_);
	insert_child(label_);

	update_icon();
}


vector2f gui_tristate::preferred_size() const
{
	vector2f button_size = button_().preferred_size();
	vector2f label_size = label_().preferred_size();

	return vector2f
		(button_size.x + label_size.x + tristate_style().button_label_spacing,
		 max(button_size.y, label_size.y));
}


void gui_tristate::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// Toggle tristate and notify client
	// if the user clicked the child button.	 
	gui_output_event_list button_output;
	button_().process_input(input, button_output);

	if (button_output.empty()) return;
	if (!*button_output.first()) return;
	if (typeid(**button_output.first()) != typeid(gui_button_event)) return;

	if (mode == mode_choice) set_state(state_yes);
		else toggle_state();

	auto_pointer<gui_output_event> toggle_event
		(new gui_tristate_event(*this));
	output.push_back
		(std::move(toggle_event));
}


void gui_tristate::commit_render
	(gui_render_packer& packer, 
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	button_().commit_render(packer, queue);
	label_().commit_render(packer, queue);
}


void gui_tristate::set_read_only(bool read_only) NOTHROW
	{ button_().set_read_only(read_only); }
bool gui_tristate::read_only() const NOTHROW
	{ return button_().read_only(); }


void gui_tristate::set_state(select_state state)
{
	state_ = state;
	update_icon();
	update_choices();
}

void gui_tristate::toggle_state()
{
	state_ = (state_ == state_no) ? state_yes : state_no;
	update_icon();
	update_choices();
}

gui_tristate::select_state gui_tristate::state() const
	{ return state_; }


void gui_tristate::set_text(const string& text)
{
	label_().set_text(text);
	want_layout();
}

string gui_tristate::text() const
	{ return label_().text(); }


void gui_tristate::set_choices
	(const choice_list& choices)
{
	if (choices.contains(weak_pointer<gui_tristate>()))
	{
		LOG_ERROR("Cannot group with NULL choice tristate.");
		throw null_pointer();
	}

	choices_ = choices;
}

const gui_tristate::choice_list& gui_tristate::choices() const
	{ return choices_; }


void gui_tristate::realize_layout()
{
	if (layout_invalid())
	{
		button_().clip_entirely();
		label_().clip_entirely();
		return;
	}	


	// Build content clipping area that applies
	// on insufficient arrange_area() size.
	const rect2f& ar_area = arrange_area();
	const rect2f& cl_area = clip_area();

	rect2f content_clip;
	if (!ar_area.try_intersect(cl_area, content_clip))
	{
		button_().clip_entirely();
		label_().clip_entirely();
		return;
	}


	// Determine minimum size occupied by content,
	// in particular the tristate label.
	vector2f button_size(button_().preferred_size());
	vector2f label_size(label_().preferred_size());

	vector2f content_size
		(min(button_size.x + label_size.x + tristate_style().button_label_spacing,
			 ar_area.size.x),
		 max(button_size.y, label_size.y));


	// Tristate button always occupies fixed space,
	// but clips against combined clipping area.
	rect2f button_area
		(vector2f(ar_area.pos.x,
				  ar_area.pos.y + (content_size.y - button_size.y) * 0.5f),
		 button_size);

	if (button_area.size.min() > 0)
		button_().layout(button_area, content_clip);
	else
		button_().clip_entirely();


	// Tristate label occupies remaining space,
	// potentially less than label_size.
	rect2f label_area
		(vector2f(ar_area.pos.x + button_size.x + tristate_style().button_label_spacing,
				  ar_area.pos.y + (content_size.y - label_size.y) * 0.5f),
		 vector2f(content_size.x - button_size.x - tristate_style().button_label_spacing,
				  label_size.y));

	if (label_area.size.min() > 0)
		label_().layout(label_area, content_clip);
	else
		label_().clip_entirely();
}


void gui_tristate::update_icon()
{
	button_().set_graphics
		(tristate_style().icon_pixel_size,
			(state_ == state_yes) ? tristate_style().yes_icon :
			(state_ == state_maybe) ? tristate_style().maybe_icon : 
									  tristate_style().no_icon);
}


void gui_tristate::update_choices()
{
	if (mode != mode_choice) return;

	for (choice_list::const_iterator it(choices_.first()); it; ++it)
	{
		shared_pointer<gui_tristate> shared(it->lock());

		if (!shared) continue;
		if (shared.get() == this) continue;

		// Do not use set_state here to avoid infinite recursion
		// from set_state() to update_choices() to set_state().
		shared->state_ = state_no;
		shared->update_icon();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gui_tristate_style
//
//////////////////////////////////////////////////////////////////////////


gui_tristate_style::gui_tristate_style
	(const shared_instance<gui_button_style>& button_style,
	 const shared_instance<render_material_id>& yes_icon,
	 const shared_instance<render_material_id>& no_icon,
	 const shared_instance<render_material_id>& maybe_icon,
	 const vector2i& icon_pixel_size,
	 float button_label_spacing)
	:
	button_style(button_style),
	yes_icon(yes_icon),
	no_icon(no_icon),
	maybe_icon(maybe_icon),
	icon_pixel_size(icon_pixel_size),
	button_label_spacing(button_label_spacing)
{
	if (icon_pixel_size.min() <= 0)
	{
		LOG_ERROR("Pixel size of tristate icons must be greater than zero.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
