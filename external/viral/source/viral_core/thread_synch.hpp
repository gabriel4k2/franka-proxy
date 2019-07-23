/**
 *************************************************************************
 *
 * @file thread_synch.hpp
 *
 * Utilities for thread synchronization.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__THREAD_SYNCH_HPP)
#define INCLUDED__VIRAL_CORE__THREAD_SYNCH_HPP


#include "build_config.hpp"


#if defined(VIRAL__USE_MS_THREAD)
	#include <windows.h>
#endif


#if defined(VIRAL__USE_CPP11_THREAD)
	#include <mutex>
	#include <condition_variable>
#endif


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mutex
 * @ingroup thread_module
 *
 * Reentrant mutex that may only be acquired and released
 * by a single thread at a time.
 *
 * Notes:
 * - Mutexes should only be acquired via the @ref MUTEX_SCOPE macro
 *		or mutex_scope RAII objects for deadlock and exception safety.
 * - Mutexes are reentrant, and need to be release()d once for
 *		each time they are acquire()d. Reentrancy is realized
 *		with an internal counter that may exhibit wrap-around
 *		at a (very high) value.
 * - Mutexes must not be release()d by a thread that does not
 *		own the mutex. Otherwise, behaviour is undefined.
 * - Mutex instances must not be deleted while they are still locked.
 *		Otherwise, behaviour is undefined.
 * - Mutexes are threadsafe for use by an arbitrary number of threads.
 *
 ************************************************************************/
class mutex :
	private non_copyable
{

	public:

		mutex();
		~mutex() NOTHROW;


		/** Acquire the mutex, and block up to success. */
		void acquire() NOTHROW;


		/**
		 * Try to acquire the mutex, and immediately return
		 * @false if the mutex is held by another thread.
		 */
		bool try_acquire() NOTHROW;


		/** Release a mutex owned by the current thread. */
		void release() NOTHROW;


	private:

		#if defined(VIRAL__USE_MS_THREAD)
			static const DWORD spin_count_ = 4000;
			static const DWORD prealloc_event_flag_ = 0x80000000;

			CRITICAL_SECTION critical_section_;
		#endif

		#if defined(VIRAL__USE_CPP11_THREAD)
			std::recursive_mutex std_mutex_;
		#endif

		#if !defined(VIRAL__USE_MS_THREAD) && \
			!defined(VIRAL__USE_CPP11_THREAD)
				#error "Not implemented for current thread back-end."
		#endif
};




/**
 *************************************************************************
 *
 * Macro for protecting some scope, such as a method call,
 * with a given @bref{mutex} object.
 * @ingroup thread_module
 *
 ************************************************************************/
#define MUTEX_SCOPE(mutex) \
	mutex_scope MAKE_UNIQUE_VARIABLE_NAME(mutex, __LINE__)(mutex);




/**
 *************************************************************************
 *
 * @class mutex_scope
 * @ingroup thread_module
 *
 * Automatically acquires its associated @bref{target_mutex} on
 * construction, and releases this mutex on destruction, thus ensuring
 * correct release for all (even exceptional) scope exits.
 *
 ************************************************************************/
class mutex_scope :
	private non_copyable
{

	public:

		mutex_scope(mutex& target_mutex) NOTHROW
			: target_mutex_(target_mutex) { target_mutex_.acquire(); }

		~mutex_scope() NOTHROW
			{ target_mutex_.release(); }

	private:

		mutex& target_mutex_;
};




/**
 *************************************************************************
 *
 * @class signal
 * @ingroup thread_module
 *
 * Allows client threads to block until a boolean variable
 * is set to some value or until some timeout has passed.
 *
 * Usage notes:
 * - A signal must not be destroyed while any threads are still waiting
 *		on the signal object. Otherwise, behaviour is undefined.
 * - Signals are threadsafe for an arbitrary number of threads at a time.
 *
 ************************************************************************/
class signal :
	private non_copyable
{

	public:

		/** Create signal with initial value of @false. */
		signal();


		~signal() NOTHROW;


		/** Atomic access to the internal value. */
		//@{
		void set(bool value) NOTHROW;
		bool get() const NOTHROW;
		//@}


		/**
		 * Set signal to @bref{value} and return previous value
		 * in one atomic call. This allows to determine whether
		 * a thread toggled the signal when there are multiple
		 * client threads.
		 */
		bool get_and_set(bool value) NOTHROW;


		/**
		 * Toggle the signal to a value it does not currently have,
		 * and return the previous signal value in one atomic call.
		 */
		bool toggle() NOTHROW;


		/**
		 * Wait for signal @bref{value} either with default timeout
		 * or with user-specified @bref{timeout_seconds} hint.
		 *
		 * Usage notes:
		 * - Invalid @bref{timeout_seconds} silently clamp to valid range.
		 * - @attent Apart from signal state toggles, waiting also returns
		 *		at the default or user-set timeout and on external triggers
		 *		(e.g. OS scheduling). Therefore, clients must always recheck
		 *		the signal value after waiting returns.
		 * - @attent Waiting for a signal does not clear any other mutexes.
		 *		Thus clients still need to be careful to avoid deadlocks.
		 */
		//@{
		void wait_for(bool value) const NOTHROW;
		void wait_for(bool value, float timeout_seconds) const NOTHROW;
		//@}


	private:

		#if defined(VIRAL__USE_MS_THREAD)
			void update_events() NOTHROW;
			void destroy() NOTHROW;

			static const float default_timeout_seconds_;
			static const float max_timeout_seconds_;

			mutable mutex signal_mutex_;
			bool signal_value_;

			HANDLE value_true_event_;
			HANDLE value_false_event_;
		#endif

		#if defined(VIRAL__USE_CPP11_THREAD)
			static const float default_timeout_seconds_;
			static const float max_timeout_seconds_;

			mutable mutex signal_mutex_;
			bool signal_value_;

			mutable std::mutex condition_mutex_;
			mutable std::condition_variable condition_toggle_;
		#endif

		#if !defined(VIRAL__USE_MS_THREAD) && \
			!defined(VIRAL__USE_CPP11_THREAD)
				#error "Not implemented for current thread back-end."
		#endif
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__THREAD_SYNCH_HPP) */
