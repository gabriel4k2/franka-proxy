/**
 *************************************************************************
 *
 * @file gui_spacer.cpp
 *
 * GUI whitespace holder, implementation.
 *
 ************************************************************************/


#include "gui_spacer.hpp"


#include <viral_core/log.hpp>


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_spacer
//
//////////////////////////////////////////////////////////////////////////


gui_spacer::gui_spacer
	(const string& name, gui_context& gui,
	 const shared_instance<gui_spacer_style>& spacer_style)
	:
	gui_element(name, gui),
	spacer_style(spacer_style)
{ }


vector2f gui_spacer::preferred_size() const
	{ return spacer_style().extents; }


void gui_spacer::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{ /** No action. */ }


void gui_spacer::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{ /** No action. */ }


void gui_spacer::realize_layout() { }




//////////////////////////////////////////////////////////////////////////
//
// gui_spacer_style
//
//////////////////////////////////////////////////////////////////////////


gui_spacer_style::gui_spacer_style(const vector2f& extents)
	:
	extents(extents)
{
	if (extents.min() < 0)
	{
		LOG_ERROR("Spacer may not have negative extents.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */

