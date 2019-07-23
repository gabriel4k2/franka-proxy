/**
 *************************************************************************
 *
 * @file gui_value_edit.cpp
 *
 * Combined sliders and text edit boxes, implementation.
 *
 ************************************************************************/


#include "gui_value_edit.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_edit.hpp"
#include "gui_editbox.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_value_edit
//
//////////////////////////////////////////////////////////////////////////


gui_value_edit::gui_value_edit
	(const string& name, gui_context& gui,
	 const shared_instance<gui_value_edit_style>& value_edit_style,
	 orientation_type orientation,
	 gui_slider::snap_mode snap)
	:
	gui_element(name, gui),
	value_edit_style(value_edit_style),
	orientation(orientation),

	monitor_editbox_edit_cancel_(false),

	slider_(new gui_slider
				(name + "_slider", gui, value_edit_style().slider_style, 
				 orientation, snap)),

	editbox_(new gui_editbox
				(name + "_editbox", gui, value_edit_style().editbox_style,
				 false, true))
{
	// Value edits do not require extensive undo,
	// so do not waste memory for the history.
	editbox_().set_history_step_limit
		(default_edit_history_step_limit_);
	editbox_().set_history_memory_limit
		(default_edit_history_memory_limit_);

	insert_child(slider_);
	insert_child(editbox_);

	try_text_from_value();
}


vector2f gui_value_edit::preferred_size() const
{
	vector2f slider_size(slider_().preferred_size());
	vector2f editbox_size(editbox_().preferred_size());

	return orientation == horizontal ?
		vector2f(slider_size.x + value_edit_style().slider_edit_spacing + editbox_size.x,
				 max(slider_size.y, editbox_size.y)) :
		vector2f(max(slider_size.x, editbox_size.x),
				 slider_size.x + value_edit_style().slider_edit_spacing + editbox_size.x);
}


void gui_value_edit::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// Before processing any input, synchronize read_only() states
	// of the independently exposed slider() and editbox() elements.
	// Client code may have set_read_only() on only one of these
	// (e.g. gui_element_loader::to_slider_helper()).
	if (slider_().read_only() != editbox_().read_only())
		editbox_().set_read_only(slider_().read_only());
 

	// Run incoming input event through slider and editbox.
	// Check if either was modified, then synch appropriately.
	// We synch two-ways, just in case the user entered text
	// outside value range or selected values without text.
	// Remaining inconsistencies stem from inexact conversion,
	// which we cannot fix reasonably.
	gui_output_event_list children_output;

	slider_().process_input(input, children_output);
	editbox_().process_input(input, children_output);

	for (gui_output_event_list::const_iterator it
			(children_output.first()); it; ++it)
	{
		if (!*it) continue;

		if (typeid(**it) == typeid(gui_slider_event))
		{
			try_text_from_value();
			try_value_from_text();
			
			auto_pointer<gui_output_event> event
				(new gui_value_edit_event(*this));
			output.push_back
				(std::move(event));
		}

		if (typeid(**it) == typeid(gui_editbox_accept_event))
		{
			// User finished entry in the editbox by pressing enter,
			// so remove focus from the editbox and stop monitoring.
			acquire_key_focus();
			monitor_editbox_edit_cancel_ = false;

			try_value_from_text();
			try_text_from_value();
			
			auto_pointer<gui_output_event> event
				(new gui_value_edit_event(*this));
			output.push_back
				(std::move(event));
		}
	}

	
	// Cancel events are somewhat unreliable, as they do not trigger
	// when the user changes focus by clicking on another GUI element.
	// Instead, we reset text to the slider value once the edit element
	// in the editbox loses its keyboard focus due to incoming input.
	// Note that the editbox itself may still maintain keyboard focus,
	// e.g. in its scroll area or in its frames.
	if (monitor_editbox_edit_cancel_ &&
		!editbox_().edit().has_key_focus())
	{
		try_text_from_value();

		auto_pointer<gui_output_event> event
			(new gui_value_edit_event(*this));
		output.push_back
			(std::move(event));
	}

	monitor_editbox_edit_cancel_ =
		editbox_().edit().has_key_focus();
}


void gui_value_edit::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	editbox_().commit_render(packer, queue);
	slider_().commit_render(packer, queue);
}
	

void gui_value_edit::set_read_only(bool read_only) NOTHROW
{
	slider_().set_read_only(read_only);
	editbox_().set_read_only(read_only);
}

bool gui_value_edit::read_only() const NOTHROW
	{ return slider_().read_only(); }


bool gui_value_edit::try_set_value(float value) NOTHROW
{
	try
	{
		const float old_value = slider_().value();
		const string old_text(editbox_().text());

		SCOPE_GUARD
			(restore_guard,
				(*this, &gui_value_edit::restore_value_and_text,
				 old_value, guard_by_reference(old_text)));
	
		slider_().set_value(value);

		// Slider may have clamped incoming value,
		// so apply new (now valid) value to editbox.
		float accepted_value = slider_().value();
		editbox_().set_text
			(converter_ ?
				converter_->to_text(accepted_value) :
				default_to_text(accepted_value));

		// There still might be inconsistencies,
		// e.g. on contradictory value conversions,
		// but we cannot reasonably resolve these.

		restore_guard.dismiss();

		return true;
	}
	catch (...) { return false; }
}

float gui_value_edit::value() const NOTHROW
	{ return slider_().value(); }


bool gui_value_edit::try_set_text(const string& text) NOTHROW
{
	try
	{
		const float old_value = slider_().value();
		const string old_text(editbox_().text());

		SCOPE_GUARD
			(restore_guard,
				(*this, &gui_value_edit::restore_value_and_text,
				 old_value, guard_by_reference(old_text)));

		editbox_().set_text(text);

		// Editbox might have adjusted text,
		// so apply (now valid) text to slider.
		string accepted_text(editbox_().text());
		slider_().set_value
			(converter_ ?
				converter_->to_value(accepted_text) :
				default_to_value(accepted_text));

		// There still might be inconsistencies,
		// e.g. on contradictory value conversions,
		// but we cannot reasonably resolve these.

		restore_guard.dismiss();

		return true;
	}
	catch (...) { return false; }
}

string gui_value_edit::text() const NOTHROW
	{ return editbox_().text(); }


bool gui_value_edit::try_text_from_value() NOTHROW
	{ return try_set_value(value()); }
bool gui_value_edit::try_value_from_text() NOTHROW
	{ return try_set_text(text()); }


void gui_value_edit::set_converter
	(auto_pointer<gui_value_edit_converter> converter) NOTHROW
		{ converter_ = std::move(converter); }

gui_value_edit_converter* gui_value_edit::converter() NOTHROW
	{ return converter_.get(); }


gui_slider& gui_value_edit::slider() NOTHROW
	{ return slider_(); }
const gui_slider& gui_value_edit::slider() const NOTHROW
	{ return slider_(); }
gui_editbox& gui_value_edit::editbox() NOTHROW
	{ return editbox_(); }
const gui_editbox& gui_value_edit::editbox() const NOTHROW
	{ return editbox_(); }


void gui_value_edit::realize_layout()
{
	if (layout_invalid())
	{
		slider_().clip_entirely();
		editbox_().clip_entirely();
		return;
	}


	// Determine size-correct arrangement areas.
	rect2f ar_area(arrange_area());
	ar_area.size =
		ar_area.size.max(preferred_size());

	rect2f slider_arrange
		(ar_area.pos, slider_().preferred_size());
	rect2f editbox_arrange
		(ar_area.pos, editbox_().preferred_size());


	// Arrange slider and text edit in orientation direction.
	// Center both elements against orientation direction.
	int64 slider_dir = (orientation == horizontal) ? 0 : 1;
	int64 center_dir = (orientation == horizontal) ? 1 : 0;

	slider_arrange.size[slider_dir] =
		max(slider_arrange.size[slider_dir],
			ar_area.size[slider_dir] - editbox_arrange.size[slider_dir] -
				value_edit_style().slider_edit_spacing);
	editbox_arrange.pos[slider_dir] +=
		slider_arrange.size[slider_dir] + value_edit_style().slider_edit_spacing;

	slider_arrange.pos[center_dir] +=
		(ar_area.size[center_dir] - slider_arrange.size[center_dir]) * 0.5f;
	editbox_arrange.pos[center_dir] +=
		(ar_area.size[center_dir] - editbox_arrange.size[center_dir]) * 0.5f;


	// Apply final layout to children elements.
	rect2f slider_clip;
	if (slider_arrange.try_intersect(clip_area(), slider_clip))
		slider_().layout(slider_arrange, slider_clip);
	else
		slider_().clip_entirely();

	rect2f editbox_clip;
	if (editbox_arrange.try_intersect(clip_area(), editbox_clip))
		editbox_().layout(editbox_arrange, editbox_clip);
	else
		editbox_().clip_entirely();
}


float gui_value_edit::default_to_value(const string& text)
	{ return text.to_float(); }

string gui_value_edit::default_to_text(float value)
	{ return string(value, default_text_digits_); }


void gui_value_edit::restore_value_and_text
	(float old_value, const string& old_text) NOTHROW
{
	try { slider_().set_value(old_value); }
	catch (...)
		{ LOG_WARN("Failed to restore former slider value on GUI element: " + name); }

	try { editbox_().set_text(old_text); }
	catch (...)
		{ LOG_WARN("Failed to restore former editbox text on GUI element: " + name); }
}


const int64 gui_value_edit::default_text_digits_ = 4;
const int64 gui_value_edit::default_edit_history_step_limit_ = 8;
const int64 gui_value_edit::default_edit_history_memory_limit_ = 1024;




//////////////////////////////////////////////////////////////////////////
//
// gui_value_edit_style
//
//////////////////////////////////////////////////////////////////////////


gui_value_edit_style::gui_value_edit_style
	(const shared_instance<gui_slider_style>& slider_style,
	 const shared_instance<gui_editbox_style>& editbox_style,
	 float slider_edit_spacing)
	:
	slider_style(slider_style),
	editbox_style(editbox_style),
	slider_edit_spacing(slider_edit_spacing)
{ }




} /* namespace viral_gui */
