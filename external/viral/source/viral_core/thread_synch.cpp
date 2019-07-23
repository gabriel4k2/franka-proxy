/**
 *************************************************************************
 *
 * @file thread_synch.cpp
 *
 * Utilities for thread synchronization, implementation.
 *
 ************************************************************************/


#include "thread_synch.hpp"


#if defined(VIRAL__USE_CPP11_THREAD)
	#include <thread>
#endif


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	mutex, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_THREAD)


mutex::mutex()
{
	// Create the critical section to use, including the notification event.
	// The event is preallocated on Win2000 and earlier, so that later enter
	// and exit calls are failure proof (NOTHROW!).
	if (!InitializeCriticalSectionAndSpinCount
			(&critical_section_, spin_count_ | prealloc_event_flag_))
	{

		// Logging is bug proof:
		// Although the global log carries its own mutex,
		// this mutex is created before the global log is
		// fully initialized, and any global log messages
		// are ignored in this phase.
		LOG_CRITICAL("Failed to create OS-side mutex object.");
		throw api_call_failed();
	}
}


mutex::~mutex() NOTHROW
	{ DeleteCriticalSection(&critical_section_); }

void mutex::acquire() NOTHROW
	{ EnterCriticalSection(&critical_section_); }

bool mutex::try_acquire() NOTHROW
	{ return TryEnterCriticalSection(&critical_section_) != 0; }

void mutex::release() NOTHROW
	{ LeaveCriticalSection(&critical_section_); }


#endif /* defined(VIRAL__USE_MS_THREAD) */




//////////////////////////////////////////////////////////////////////////
//
//	mutex, C++11 std implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_CPP11_THREAD)

mutex::mutex() { }
mutex::~mutex() NOTHROW { }

void mutex::acquire() NOTHROW
{
	try { std_mutex_.lock(); }
	catch (...)
	{
		// We can't possibly do anything in this case,
		// e.g. if the C++ library detects a deadlock.
		// Notably, we can't throw, since many functions
		// depend on a non-throwing mutex acquire().
		LOG_CRITICAL("Emergency exit: Exception in std::mutex::lock().");
		exit(-1);
	}
}

bool mutex::try_acquire() NOTHROW
	{ return std_mutex_.try_lock(); }

void mutex::release() NOTHROW
	{ std_mutex_.unlock(); }

#endif /* defined(VIRAL__USE_CPP11_THREAD) */




//////////////////////////////////////////////////////////////////////////
//
//	signal, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_THREAD)


signal::signal()
	:
	signal_value_(false),
	value_true_event_(NULL),
	value_false_event_(NULL)
{
	SCOPE_GUARD
		(signal_guard, (*this, &signal::destroy));	

	value_true_event_ = CreateEvent(NULL, TRUE, FALSE, NULL);
	value_false_event_ = CreateEvent(NULL, TRUE, TRUE, NULL);

	if (!value_true_event_ || !value_false_event_)
	{
		LOG_CRITICAL("Failed to create OS-side signal object.");
		throw api_call_failed();
	}

	signal_guard.dismiss();
}


signal::~signal() NOTHROW { destroy(); }


void signal::set(bool value) NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);

	if (signal_value_ == value)
		return;

	signal_value_ = value;
	update_events();
}


bool signal::get() const NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);
	return signal_value_;
}


bool signal::get_and_set(bool value) NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);

	if (signal_value_ == value)
		return signal_value_;

	signal_value_ = value;
	update_events();

	return !signal_value_; // consider: !signal_value == old_signal_value
}


bool signal::toggle() NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);

	signal_value_ = !signal_value_;
	update_events();

	return !signal_value_; // consider: !signal_value == old_signal_value
}


void signal::wait_for(bool value) const NOTHROW
	{ wait_for(value, default_timeout_seconds_); }

void signal::wait_for
	(bool value, float timeout_seconds) const NOTHROW
{
	// No mutex here since our handles are constant
	// after creation, and waiting is thread safe.

	timeout_seconds =
		max(0.f, min(max_timeout_seconds_, timeout_seconds));

	WaitForSingleObject
		(value ? value_true_event_ : value_false_event_,
		 static_cast<DWORD>(timeout_seconds * 1000.f));

	// The return code from WaitForSingleObject is irrelevant,
	// since clients must recheck the signal value after wait_for().
}


void signal::update_events() NOTHROW
{
	/**	
	 * We don't do event updates atomically here.
	 *
	 * However, threads anyways must not make assumptions about
	 * when exactly they are blocked or awakened by wait_for(),
	 * since the call also might time-out at any time.
	 *
	 * Since set() and get_and_set() still keep a mutex while calling
	 * update_events(), threads will be synchronized to event states
	 * once they query the current signal value.
	 *
	 * For the same reasons, we don't escalate any event problems
	 * (e.g. in the form of an exception) either.
	 */

	if (!ResetEvent(signal_value_ ? value_false_event_ : value_true_event_))
		LOG_WARN("Signal failed to reset event.");

	if (!SetEvent(signal_value_ ? value_true_event_ : value_false_event_))
		LOG_WARN("Signal failed to set event.");
}


void signal::destroy() NOTHROW
{
	if (value_true_event_)
	{
		CloseHandle(value_true_event_);
		value_true_event_ = NULL;
	}

	if (value_false_event_)
	{
		CloseHandle(value_false_event_);
		value_false_event_ = NULL;
	}
}


const float signal::default_timeout_seconds_ = 0.25f;
const float signal::max_timeout_seconds_ = 60;


#endif /* defined(VIRAL__USE_MS_THREAD) */




//////////////////////////////////////////////////////////////////////////
//
//	signal, C++11 std implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_CPP11_THREAD)


signal::signal() : signal_value_(false) { }
signal::~signal() NOTHROW { }


void signal::set(bool value) NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);
	signal_value_ = value;
	condition_toggle_.notify_all();
}


bool signal::get() const NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);
	return signal_value_;
}


bool signal::get_and_set(bool value) NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);
	bool oldvalue = signal_value_;
	signal_value_ = value;
	condition_toggle_.notify_all();
	return oldvalue;
}


bool signal::toggle() NOTHROW
{
	MUTEX_SCOPE(signal_mutex_);
	bool oldvalue = signal_value_;
	signal_value_ = !signal_value_;
	condition_toggle_.notify_all();
	return oldvalue;
}


void signal::wait_for(bool value) const NOTHROW
	{ wait_for(value, default_timeout_seconds_); }

void signal::wait_for
	(bool value, float timeout_seconds) const NOTHROW
{
	// Check current value to avoid waiting
	// if the signal already has the desired value.
	{
		MUTEX_SCOPE(signal_mutex_);
		if (signal_value_ == value) return;
	}


	timeout_seconds =
		max(0.f, min(max_timeout_seconds_, timeout_seconds));

	
	// Notes:
	// - We use but a single waiting condition to wait
	//		for both signal states. This is not a problem,
	//		since premature timeouts anyways enforce clients
	//		to recheck the current signal value.
	// - We ensure NOTHROW in the case of any lock errors.
	//		We can't do anything here, but the client must
	//		anyways recheck the current signal value on return.
	try
	{
		std::unique_lock<std::mutex> lock(condition_mutex_);
		condition_toggle_.wait_for
			(lock,
			 std::chrono::milliseconds
				(static_cast<int64>(timeout_seconds * 1000)));
	}
	catch (...)
		{ LOG_WARN("Signal waiting failed."); }
}


const float signal::default_timeout_seconds_ = 0.25f;
const float signal::max_timeout_seconds_ = 60;


#endif /* defined(VIRAL__USE_CPP11_THREAD) */




} /* namespace viral_core */
