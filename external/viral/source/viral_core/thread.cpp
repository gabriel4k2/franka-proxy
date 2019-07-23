/**
 *************************************************************************
 *
 * @file thread.cpp
 *
 * Implementation file for the main multithreading header.
 *
 ************************************************************************/


#include "thread.hpp"


#if defined(VIRAL__USE_MS_THREAD)
	#include <process.h>
#endif


#include "log.hpp"
#include "exception.hpp"
#include "thread_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	threaded_task, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_THREAD)


threaded_task::threaded_task()
	:
	join_now_(false), had_unhandled_exception_(false),
	debug_exception_trace_(true),
	thread_handle_(NULL), thread_id_(0)
{ }


threaded_task::~threaded_task() NOTHROW
{
	if (thread_handle_)
	{
		LOG_CRITICAL
			("A task was not explicitly join()ed before destruction. "
			 "This most likely creates a race condition. "
			 "The OS thread will be detached from the task, "
			 "and continues execution until it terminates.");

		CloseHandle(thread_handle_);
		thread_handle_ = NULL;
		thread_id_ = 0;
	}
}


bool threaded_task::ready() const NOTHROW
{
	// Joined successfully => OS handle released => ready
	MUTEX_SCOPE(task_lock_);
	return !thread_handle_;
}


bool threaded_task::thread_has_terminated() const NOTHROW
{
	MUTEX_SCOPE(task_lock_);


	if (!thread_handle_) // no handle, no thread
		return true; 

	if (GetCurrentThreadId() == thread_id_) // thread lives and is current
		return false;


	// Check the current thread state, wait time = 0.
	// We use this instead of a simple terminateion flag
	// to guarantee clean thread shutdown in join().
	DWORD retcode =
		WaitForSingleObject(thread_handle_, 0);

	if (retcode == WAIT_FAILED)
	{
		LOG_CRITICAL("Emergency exit: Failed to check for thread termination.");
		exit(-1);
	}

	if (retcode == WAIT_OBJECT_0)
		return true;

	return false;
}


bool threaded_task::join_now() const NOTHROW
	{ MUTEX_SCOPE(task_lock_); return join_now_; }


void threaded_task::start()
{
	MUTEX_SCOPE(task_lock_);

	if (thread_handle_)
	{
		LOG_ERROR("Task is still running (unjoined), cannot restart.");
		throw invalid_state();
	}


	// - Exception safety:
	//		First create and run the OS thread (which may fail),
	//		then update internal threaded_task states (NOTHROW).
	//		This works around state rollback on failure.
	// - There are no problems with an initially running OS thread,
	//		since any queries on the threaded_task anyways block
	//		until this method releases the task_lock_.
	// - On Microsoft Windows, threads are created with _beginthreadex()
	//		rather than the Windows API CreateThread(), since the latter
	//		leaks memory if used together with static CRT versions.
	thread_handle_ = reinterpret_cast<HANDLE>
		(_beginthreadex
			(NULL, 0, &static_task_startup_wrapper, this, 0, &thread_id_));

	if (!thread_handle_)
	{
		LOG_CRITICAL("Windows failed to spawn thread.");
		throw system_call_failed();
	}
	
	join_now_ = false;
	had_unhandled_exception_ = false;
}


void threaded_task::join()
{
	MUTEX_SCOPE(task_lock_);


	if (!thread_handle_) // ready already
		return;

	if (GetCurrentThreadId() == thread_id_)
	{
		LOG_ERROR("Task can't join() with itself.");
		throw invalid_state();
	}

	if (join_now_)
	{
		LOG_ERROR("Another thread is already join()ing with this task.");
		throw invalid_state();
	}


	// - Signal OS thread to exit, then wait for its termination.
	// - If the thread does not terminate instantly, it either
	//		requires more time, or there is a deadlock involving
	//		the task-internal mutex. In either case, temporarily
	//		release the mutex and sleep some bit. Then try again.
	// - Mutex release is still threadsafe, as no other thread can
	//		start or join until the current join attempt succeeds.
	// - This and all following sections must not throw.
	join_now_ = true;

	while (!thread_has_terminated())
	{
		task_lock_.release();
		Sleep(wait_time_for_thread_termination_);
		task_lock_.acquire();
	}


	// Thread terminated.
	// Reset task to ready state and release OS resources.
	// This allows start()ing and join()ing again.
	join_now_ = false;
	
	if (thread_handle_)
	{
		CloseHandle(thread_handle_);
		thread_handle_ = NULL;
	}

	thread_id_ = 0;


	// Pass on any unhandled exception.
	// Reset the respective flag so we can reuse the task.
	if (had_unhandled_exception_)
	{
		had_unhandled_exception_ = false;
		throw unhandled_exception();
	}
}


void threaded_task::set_debug_exception_trace(bool debug_exception_trace)
{
	MUTEX_SCOPE(task_lock_);
	debug_exception_trace_ = debug_exception_trace;
}

bool threaded_task::debug_exception_trace() const NOTHROW
{
	MUTEX_SCOPE(task_lock_);
	return debug_exception_trace_;
}


thread_id threaded_task::id() const NOTHROW
{
	MUTEX_SCOPE(task_lock_);	
	return thread_id(thread_id_);
}


void threaded_task::task_startup_wrapper()
{
	#if defined(VIRAL__BUILD_MODE_DEBUG)	
		// Use accessor method for thread safety.
		if (debug_exception_trace()) task_main();
		else
		{
	#endif
			try { task_main(); }
			catch (...)
			{
				MUTEX_SCOPE(task_lock_);
				had_unhandled_exception_ = true;
			}
	#if defined(VIRAL__BUILD_MODE_DEBUG)	
		}
	#endif
}


unsigned int __stdcall
	threaded_task::static_task_startup_wrapper(void *untyped_task)
{
	if (!untyped_task)
	{
		LOG_CRITICAL("Thread without task instance mysteriously got started.");
		return static_cast<unsigned int>(-1);
	}

	threaded_task *task =
		static_cast<threaded_task*>(untyped_task);

	task->task_startup_wrapper();

	return 0;
}


/**
 * Time to sleep when joining on a running thread
 * before rechecking for thread exit.
 *
 * @attent
 * Excessive wait times for thread exit (e.g. 250 msecs)
 * introduce a noticeable delay on threaded_task::join().
 */
const DWORD threaded_task::wait_time_for_thread_termination_ = 10;


#endif /* defined(VIRAL__USE_MS_THREAD) */




//////////////////////////////////////////////////////////////////////////
//
//	threaded_task, C++11 std implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_CPP11_THREAD)


threaded_task::threaded_task()
	:
	join_now_(false), thread_has_terminated_(true),
	had_unhandled_exception_(false), debug_exception_trace_(true)
{ }


threaded_task::~threaded_task() NOTHROW
{
	if (std_thread_.joinable())
	{
		LOG_CRITICAL
			("A task was not explicitly join()ed before destruction. "
			 "This most likely creates a race condition. "
			 "The OS thread will be detached from the task, "
			 "and continues execution until it terminates.");

		try { std_thread_.detach(); }
		catch (...)
			{ LOG_CRITICAL("Ignoring exception in std::thread::detach()."); }
	}
}


bool threaded_task::ready() const NOTHROW
	{ MUTEX_SCOPE(task_lock_); return !std_thread_.joinable(); }

bool threaded_task::thread_has_terminated() const NOTHROW
	{ MUTEX_SCOPE(task_lock_); return thread_has_terminated_; }

bool threaded_task::join_now() const NOTHROW
	{ MUTEX_SCOPE(task_lock_); return join_now_; }


void threaded_task::start()
{
	MUTEX_SCOPE(task_lock_);

	if (!ready())
	{
		LOG_ERROR("Task is still running (unjoined), cannot restart.");
		throw invalid_state();
	}


	// - Exception safety: First create and run the OS thread (which may fail),
	//		then update internal task states (NOTHROW). This works around state
	//		rollback on failure.
	// - There are no problems with an initially running OS thread, since any
	//		queries on the task anyways block up to return from this method.
	try
	{
		std_thread_ =
			std::thread(&static_task_startup_wrapper, this);
	}
	catch (...)
	{
		LOG_CRITICAL("Failed to spawn std::thread.");
		throw system_call_failed();
	}
	
	join_now_ = false;
	thread_has_terminated_ = false;
	had_unhandled_exception_ = false;
}


void threaded_task::join()
{
	MUTEX_SCOPE(task_lock_);

	if (ready()) return;

	if (std_thread_.get_id() == std::this_thread::get_id())
	{
		LOG_ERROR("Task can't join() with its thread.");
		throw invalid_state();
	}

	if (join_now_)
	{
		LOG_ERROR("Another thread is already join()ing with this task.");
		throw invalid_state();
	}


	// - Signal OS thread to exit, then wait for its termination.
	// - If the thread does not terminate instantly, it either
	//		requires more time, or there is a deadlock involving
	//		the task-internal mutex. In either case, temporarily
	//		release the mutex and sleep some bit. Then try again.
	// - Mutex release is still threadsafe, as no other thread can
	//		start or join until the current join attempt succeeds.
	// - This and all following sections must not throw.
	join_now_ = true;

	while (!thread_has_terminated())
	{
		task_lock_.release();
		thread_util::sleep_seconds(wait_time_for_thread_termination_);
		task_lock_.acquire();
	}


	// Thread terminated.
	// Reset task to ready state and release OS resources.
	// This allows start()ing and join()ing again.
	// We can't properly handle join failure, but we need
	// to exit in order to avoid worse situations.
	try { std_thread_.join(); }
	catch (...)
	{
		LOG_CRITICAL("Emergency exit: Exception in std::thread::join().");
		exit(-1);
	}

	std_thread_ = std::thread();
	join_now_ = false;


	// Pass on any unhandled exception.
	// Reset the respective flag so we can reuse the task.
	if (had_unhandled_exception_)
	{
		had_unhandled_exception_ = false;
		throw unhandled_exception();
	}
}


void threaded_task::set_debug_exception_trace(bool debug_exception_trace)
{
	MUTEX_SCOPE(task_lock_);
	debug_exception_trace_ = debug_exception_trace;
}

bool threaded_task::debug_exception_trace() const NOTHROW
{
	MUTEX_SCOPE(task_lock_);
	return debug_exception_trace_;
}


thread_id threaded_task::id() const NOTHROW
{
	MUTEX_SCOPE(task_lock_);	
	return thread_id(std_thread_.get_id());
}


void threaded_task::task_startup_wrapper()
{
	#if defined(VIRAL__BUILD_MODE_DEBUG)	
		// Use accessor method for thread safety.
		if (debug_exception_trace()) task_main();
		else
		{
	#endif
			try { task_main(); }
			catch (...)
			{
				MUTEX_SCOPE(task_lock_);
				had_unhandled_exception_ = true;
			}
	#if defined(VIRAL__BUILD_MODE_DEBUG)	
		}
	#endif


	// NOTHROW: Always set termination flag.
	// Ensures that join() always returns
	// if the OS-side thread terminates.
	MUTEX_SCOPE(task_lock_);
	thread_has_terminated_ = true;
}


void threaded_task::static_task_startup_wrapper(threaded_task *task)
	{ task->task_startup_wrapper(); }


const float threaded_task::wait_time_for_thread_termination_ = 0.01f;


#endif /* defined(VIRAL__USE_CPP11_THREAD) */




} /* namespace viral_core */
