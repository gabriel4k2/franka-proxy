/**
 *************************************************************************
 *
 * @file gui_stack.cpp
 *
 * GUI element stack, implementation.
 *
 ************************************************************************/


#include "gui_stack.hpp"


#include <viral_core/log.hpp>
#include <viral_core/mesh.hpp>


#include "gui_event.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_stack
//
//////////////////////////////////////////////////////////////////////////


gui_stack::gui_stack
	(const string& name, gui_context& gui, bool modal)
	:
	gui_element(name, gui),
	modal(modal)
{ }


vector2f gui_stack::preferred_size() const
{
	vector2f max;

	for (layer_list::const_iterator it(layers_.first()); it; ++it)
		max = max.max((*it)().preferred_size());

	return max;
}


void gui_stack::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	for (layer_list::iterator it(layers_.last()); it; --it)
	{
		(*it)().process_input(input, output);
		if (modal) input.hot = false;
	}
}


void gui_stack::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	for (layer_list::iterator it(layers_.first()); it; ++it)
		(*it)().commit_render(packer, queue);
}


void gui_stack::insert_layer
	(const shared_instance<gui_element>& layer)
{
	insert_child(layer);
	SCOPE_GUARD
		(child_guard, (*this, &gui_stack::remove_child, layer));

	layers_.push_back(layer);
	child_guard.dismiss();

	// Exception safety: NOTHROW from here.
	want_layout();
}


void gui_stack::remove_layer
	(const shared_instance<gui_element>& layer) NOTHROW
{
	remove_child(layer);
	layers_.remove(layer);
	want_layout();
}


void gui_stack::realize_layout()
{
	for (layer_list::iterator it(layers_.first()); it; ++it)
	{
		if (layout_invalid()) (*it)().clip_entirely();
			else (*it)().layout(arrange_area(), clip_area());
	}
}




} /* namespace viral_gui */
