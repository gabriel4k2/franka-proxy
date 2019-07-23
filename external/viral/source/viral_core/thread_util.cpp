/**
 *************************************************************************
 *
 * @file thread_util.cpp
 *
 * Thread utility classes, implementation.
 *
 ************************************************************************/


#include "thread_util.hpp"


#if defined(VIRAL__USE_MS_THREAD)
	#include <windows.h>
#endif


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	thread_id
//
//////////////////////////////////////////////////////////////////////////


thread_id& thread_id::operator =(const thread_id& other) NOTHROW
	{ id_ = other.id_; return *this; }


bool thread_id::operator ==(const thread_id& other) const NOTHROW
	{ return id_ == other.id_; }
bool thread_id::operator !=(const thread_id& other) const NOTHROW
	{ return id_ != other.id_; }

bool thread_id::operator <(const thread_id& other) const NOTHROW
	{ return id_ < other.id_; }
bool thread_id::operator >(const thread_id& other) const NOTHROW
	{ return id_ > other.id_; }
bool thread_id::operator <=(const thread_id& other) const NOTHROW
	{ return id_ <= other.id_; }
bool thread_id::operator >=(const thread_id& other) const NOTHROW
	{ return id_ >= other.id_; }


#if defined(VIRAL__USE_MS_THREAD)

	thread_id thread_id::this_thread() NOTHROW
		{ return thread_id(GetCurrentThreadId()); }

	thread_id thread_id::invalid_thread() NOTHROW
	{
		// Zero is not explicitly documented as an invalid ID,
		// but various Win32 API calls return zero thread IDs
		// to indicate failure.
		return thread_id(0);
	}

	thread_id::thread_id(DWORD id) NOTHROW
		: id_(id) { }

#endif


#if defined(VIRAL__USE_CPP11_THREAD)

	thread_id thread_id::this_thread() NOTHROW
		{ return thread_id(std::this_thread::get_id()); }

	thread_id thread_id::invalid_thread() NOTHROW
		{ return thread_id(std::thread::id()); }

	thread_id::thread_id(const std::thread::id& id) NOTHROW
		: id_(id) { }

#endif




//////////////////////////////////////////////////////////////////////////
//
//	thread_util
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_THREAD)
	void thread_util::sleep_slice() NOTHROW
		{ ::Sleep(0); }

	void thread_util::sleep_seconds(float time_seconds) NOTHROW
	{
		time_seconds =
			max(0.f, min(max_sleep_seconds, time_seconds));

		::Sleep(static_cast<DWORD>(time_seconds * 1000));
	}
#endif


#if defined(VIRAL__USE_CPP11_THREAD)
	void thread_util::sleep_slice() NOTHROW
		{ std::this_thread::yield(); }

	void thread_util::sleep_seconds(float time_seconds) NOTHROW
	{
		time_seconds =
			max(0.f, min(max_sleep_seconds, time_seconds));

		std::this_thread::sleep_for
			(std::chrono::milliseconds
				(static_cast<int64>(time_seconds * 1000)));
	}
#endif


const float thread_util::max_sleep_seconds = 3600; // ~1h




} /* namespace viral_core */

