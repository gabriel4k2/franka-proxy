/**
 *************************************************************************
 *
 * @file thread_util.hpp
 *
 * Thread utility classes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__THREAD_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__THREAD_UTIL_HPP


#include "build_config.hpp"


#if defined(VIRAL__USE_MS_THREAD)
	#include <windows.h>
#endif


#if defined(VIRAL__USE_CPP11_THREAD)
	#include <thread>
#endif


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class thread_id
 * @ingroup thread_module
 *
 * Copyable wrapper for an OS-specific thread handle.
 *
 * Usage notes:
 * - A thread_id becomes invalid if the respective OS thread terminates.
 *		Notably, the internal OS-specific thread handle may be reused
 *		for any new threads.
 *
 ************************************************************************/
class thread_id
{
	friend class thread_util;
	friend class threaded_task;


	public:

		thread_id& operator =(const thread_id& other) NOTHROW;

		bool operator ==(const thread_id& other) const NOTHROW;
		bool operator !=(const thread_id& other) const NOTHROW;

		bool operator <(const thread_id& other) const NOTHROW;
		bool operator >(const thread_id& other) const NOTHROW;
		bool operator <=(const thread_id& other) const NOTHROW;
		bool operator >=(const thread_id& other) const NOTHROW;


		/** Get a thread_id for the calling thread. */
		static thread_id this_thread() NOTHROW;

		/** An ID that indicates an invalid OS thread. */
		static thread_id invalid_thread() NOTHROW;


	private:

		#if defined(VIRAL__USE_MS_THREAD)

			thread_id(DWORD id) NOTHROW;
			DWORD id_;

		#endif

		#if defined(VIRAL__USE_CPP11_THREAD)

			thread_id(const std::thread::id& id) NOTHROW;
			std::thread::id id_;

		#endif

		#if !defined(VIRAL__USE_MS_THREAD) && \
			!defined(VIRAL__USE_CPP11_THREAD)
				#error "Not implemented for current thread back-end."
		#endif
};




/**
 *************************************************************************
 *
 * @class thread_util
 * @ingroup thread_module
 *
 * Static utility functions for thread control.
 *
 ************************************************************************/
class thread_util :
	public non_constructable
{

	public:

		/**
		 * Hint for passing the remainder of the current thread's
		 * time slice to other waiting threads or processes.
		 */
		static void sleep_slice() NOTHROW;


		/**
		 * Let the current thread sleep for about the hinted time.
		 * Invalid sleeping times silently clamp to valid range.
		 */
		static void sleep_seconds(float time_seconds) NOTHROW;


		/** Upper limit for sleep_seconds(). */
		static const float max_sleep_seconds;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__THREAD_UTIL_HPP) */
