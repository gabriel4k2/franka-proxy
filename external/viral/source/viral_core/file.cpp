/**
 *************************************************************************
 *
 * @file file.cpp
 *
 * Base implementation for the virtual file system.
 *
 ************************************************************************/


#include "file.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  file_context
//
//////////////////////////////////////////////////////////////////////////


file_context::file_context (const string& context_name) :
	context_name(context_name)
		{ LOG_INFO("Creating: "  + context_name); }

file_context::~file_context() NOTHROW
	{ LOG_INFO("Destroyed: " + context_name); }




} /* namespace viral_core */
