/**
 *************************************************************************
 *
 * @file window.cpp
 *
 * Application window interface, implementation file.
 *
 ************************************************************************/


#include "window.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  window_context
//
//////////////////////////////////////////////////////////////////////////


window_context::~window_context()
	{ LOG_INFO("Destroyed: " + context_name); }

window_context::window_context(const string& context_name)
	: context_name(context_name)
		{ LOG_INFO("Creating: " + context_name); }


} /* namespace viral_core */
