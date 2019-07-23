/**
 *************************************************************************
 *
 * @file ms_util.hpp
 *
 * Assorted utilities for Microsoft Windows.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__MS_UTIL_HPP


#include "build_config.hpp"


#if defined(VIRAL__OS_TARGET_WINDOWS)


#include <windows.h>


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup ms_util_module Microsoft Utility Module
 *
 * Assorted utilities for use with Microsoft Windows.
 * 
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class ms_sched_fix
 * @ingroup ms_util_module
 *
 * Fixes various scheduling-related problems on MS Windows,
 * such as missing vsyncs when the DWM is running.
 *
 * Notes:
 * - To use, instantiate a ms_sched_fix, and keep alive up to termination. 
 *		While alive, a ms_sched_fix reduces thread scheduling granularity
 *		down to the system-allowed minimum. The core @ref engine already
 *		creates a ms_sched_fix for its client applications.
 * - By default, MS Windows schedules threads with at most 15ms granularity.
 *		This causes thread_util::sleep() to overshoot short sleeping times,
 *		which in turn results in various timing glitches. Most notable
 *		are missed frames, e.g. with yielding or sleeping main loops or
 *		with graphics drivers that sleep to the next vsync.
 *
 ************************************************************************/
class ms_sched_fix :
	private non_copyable
{

	public:

		ms_sched_fix();
		~ms_sched_fix() NOTHROW;


	private:

		static const UINT maximum_minimum_period_ = 16;
		UINT minimum_period_;
};




/**
 *************************************************************************
 *
 * @class ms_com_init
 * @ingroup ms_util_module
 *
 * Wrapper around CoInitializeEx and CoUninitialize for Microsoft COM.
 *
 * Usage notes:
 * - COM initialization is required for most COM objects (e.g. DirectX).
 *		Without explicit initialization, all calls to COM objects fail.
 * - The @ref engine applies ms_com_init to the constructing thread.
 *		This implicitly initializes all threads without a COM setup
 *		to the multithreaded apartment COM model.
 * 
 * Explicit COM initialization:
 * - Clients need only explicitly apply additional ms_com_init instances
 *		to establish a longer COM lifetime (e.g. over multiple engine%s).
 * - @attent In this case, the first thread to initialize COM must also
 *		be the last thread to deinitialize COM. Otherwise, certain later
 *		COM operations (e.g. re-initializations) fail.
 *
 * Implementation notes:
 * - We cannot easily initialize COM on demand in subsystem code,
 *		since the initializing thread must also uninitialize COM.
 *		We instead prefer a single point of failure in @ref engine.
 *
 ************************************************************************/
class ms_com_init :
	private non_copyable
{

	public:

		ms_com_init();
		~ms_com_init() NOTHROW;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__OS_TARGET_WINDOWS) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_UTIL_HPP) */

