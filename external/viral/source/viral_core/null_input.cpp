/**
 *************************************************************************
 *
 * @file null_input.cpp
 *
 * Null input handling, implementation.
 *
 ************************************************************************/


#include "null_input.hpp"


#include "log.hpp"
#include "buffer.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// null_input_context
//
//////////////////////////////////////////////////////////////////////////


null_input_context::null_input_context
	(const string& context_name)
	:
	input_context(context_name)
{
	LOG_INFO("Using null_input_context implementation.");
}


null_input_context::~null_input_context() NOTHROW
	{ LOG_INFO("Destroying."); }


void null_input_context::update()
	{ /** No updates on null_input_context. */ }


float null_input_context::value_of
	(input_source_type source) const
		{ return 0; }
bool null_input_context::is_inactive
	(input_source_type source) const
		{ return true; }
bool null_input_context::is_active
	(input_source_type source) const
		{ return false; }


bool null_input_context::was_deactivated
	(input_source_type source) const
		{ return false; }
bool null_input_context::was_activated
	(input_source_type source) const
		{ return false; }


void null_input_context::get_text_input
	(text_input_buffer &text_input) const
		{ text_input.clear(); }


void null_input_context::freeze_mouse(bool freeze)
	{ /** No mouse freezing on null_input_context. */ }




} /* namespace viral_core */
