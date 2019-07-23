/**
 *************************************************************************
 *
 * @file ms_util.cpp
 *
 * Assorted utilities for Microsoft Windows, implementation.
 *
 ************************************************************************/


#include "ms_util.hpp"


#if defined(VIRAL__OS_TARGET_WINDOWS)


#include <objbase.h>


#include "log.hpp"
#include "exception.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_sched_fix
//
//////////////////////////////////////////////////////////////////////////


ms_sched_fix::ms_sched_fix()
	: minimum_period_(1)
{
	while (minimum_period_ < maximum_minimum_period_ &&
		   timeBeginPeriod(minimum_period_) != TIMERR_NOERROR)
				++minimum_period_;

	if (minimum_period_ == maximum_minimum_period_)
	{
		LOG_ERROR("Failed to reduce thread scheduling granularity.");
	}
	else
	{
		LOG_INFO("Reduced thread scheduling granularity to: " +
				 minimum_period_ + "ms");
	}
}


ms_sched_fix::~ms_sched_fix() NOTHROW
{
	if (minimum_period_ != maximum_minimum_period_)
	{
		if (timeEndPeriod(minimum_period_) != TIMERR_NOERROR)
			LOG_ERROR("Failed to reset thread scheduling granularity.");		
	}
}




//////////////////////////////////////////////////////////////////////////
//
// ms_com_init
//
//////////////////////////////////////////////////////////////////////////


ms_com_init::ms_com_init()
{
	LOG_INFO("Initializing COM.");

	HRESULT hr =
		CoInitializeEx(0, COINIT_MULTITHREADED);

	if (hr == RPC_E_CHANGED_MODE)
	{
		LOG_ERROR("Cannot change COM apartment model.");
		throw api_call_failed();
	}
	
	if (FAILED(hr))
	{
		LOG_ERROR("Failed to initialize COM.");
		throw api_call_failed();
	}

	// Success.
	// Exception safety: NOTHROW from here.
	if (hr == S_FALSE)
		{ LOG_INFO("COM was already initialized."); }
}


ms_com_init::~ms_com_init() NOTHROW
{
	LOG_INFO("Uninitializing COM.");
	CoUninitialize();
}




} /* namespace viral_core */


#endif /* defined(VIRAL__OS_TARGET_WINDOWS) */

