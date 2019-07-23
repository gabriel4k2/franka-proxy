/**
 *************************************************************************
 *
 * @file gui_event.cpp
 *
 * Base events for use in a GUI, implementation.
 *
 ************************************************************************/


#include "gui_event.hpp"


#include <viral_core/log.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_output_event
//
//////////////////////////////////////////////////////////////////////////


gui_output_event::gui_output_event(gui_element& source_untyped)
	:
	source_untyped(source_untyped.fetch_shared_from_parent()
								 .instance_or_throw())
{ }




} /* namespace viral_gui */
