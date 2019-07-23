/**
 *************************************************************************
 *
 * @file command_line.cpp
 *
 * Static access to command-line parameters, implementation.
 *
 ************************************************************************/


#include "command_line.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// command_line
//
//////////////////////////////////////////////////////////////////////////


void command_line::init
	(int argc, const char* const* argv)
{
	if (argc < 0 || (argc > 0 && !argv))
	{
		LOG_ERROR("Invalid command line parameters.");
		throw invalid_parameter();
	}

	if (argc > max_argc)
	{
		LOG_ERROR("Too many command line arguments.");
		throw invalid_parameter();
	}


	if (initialized())
	{
		// We do not support redundant initialization
		// to ensure thread safety after first init().
		// Note that the error message will not appear
		// unless log_singleton has been initialized.
		LOG_ERROR("Command line has already been initialized.");
		throw invalid_state();
	}


	// Exception safety:
	// NOTHROW from here.
	initialized() = true;
	argc_store() = argc;
	argv_store() = argv;
}


int command_line::argc() NOTHROW
	{ return argc_store(); }
const char* const* command_line::argv() NOTHROW
	{ return argv_store(); }


list<string> command_line::parameters()
{
	list<string> result;
	for (int i = 0; i < argc(); ++i)
		result.push_back(argv()[i]);

	return std::move(result);
}


bool& command_line::initialized() NOTHROW
{
	// Static, local initialization is threadsafe
	// on more recent (i.e. C++11) compilers.
	static bool initialized_local = false;
	return initialized_local;
}


int& command_line::argc_store() NOTHROW
{
	// Static, local initialization is threadsafe
	// on more recent (i.e. C++11) compilers.
	static int argc_local = 0;
	return argc_local;
}


const char* const*& command_line::argv_store() NOTHROW
{
	// Static, local initialization is threadsafe
	// on more recent (i.e. C++11) compilers.
	static const char* const* argv_local = NULL;
	return argv_local;
}




} /* namespace viral_core */
