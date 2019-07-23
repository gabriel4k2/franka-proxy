/**
 *************************************************************************
 *
 * @file null_window.cpp
 *
 * Null windowing system, implementation.
 *
 ************************************************************************/


#include "null_window.hpp"


#include "log.hpp"
#include "geo_2d.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// null_window_context
//
//////////////////////////////////////////////////////////////////////////


null_window_context::null_window_context
	(const string& context_name)
	:
	window_context(context_name)
{
	LOG_INFO("Using null_window_context implementation.");
}


null_window_context::~null_window_context() NOTHROW
	{ LOG_INFO("Destroying."); }


void null_window_context::set_caption(const string& caption)
	{ /** The null_window_context does not accept other captions. */ }

string null_window_context::caption() const
	{ return context_name; }


void null_window_context::set_display
	(const vector2f& position, const vector2f& size,
	 uint32 display_flags)
		{ /** The null_window_context ignores display options. */ }

vector2f null_window_context::position() const
	{ return vector2f(0.f, 0.f); }
vector2f null_window_context::size() const
	{ return vector2f(640.f, 480.f); }
uint32 null_window_context::display_flags() const
	{ return 0; }


uint32 null_window_context::notify_flags() const
	{ return 0; }


bool null_window_context::close_flag() const
	{ return false; }


void null_window_context::update()
	{ /** The null_window_context ignores updates. */ }




} /* namespace viral_core */
