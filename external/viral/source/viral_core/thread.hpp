/**
 *************************************************************************
 *
 * @file thread.hpp
 *
 * Main multithreading header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__THREAD_HPP)
#define INCLUDED__VIRAL_CORE__THREAD_HPP


#include "build_config.hpp"


#if defined(VIRAL__USE_MS_THREAD)
	#include <windows.h>
#endif


#if defined(VIRAL__USE_CPP11_THREAD)
	#include <thread>
#endif


#include "thread_synch.hpp"
#include "thread_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup thread_module Thread Module
 *
 * Offers multithreading capabilities within the Viral Framework.
 *
 * <b>
 * As the entire Viral Framework is based on multithreading,
 * the following thread safety guidelines must always be considered.
 * </b>
 *
 * Thread safety guidelines:
 * - Most framework classes only provide basic thread safety.
 *		An arbitrary number of readers may call constant, read-only
 *		methods on any number of objects. However, there only may
 *		be a single, exclusive writer on any given object at a time.
 *		Otherwise behaviour is unspecified.
 * - Stronger thread safety guarantees (such as reentrant calls) are
 *		documented where appropriate. Certain use cases may require
 *		explicit locking within client code.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class threaded_task
 * @ingroup thread_module
 *
 * Executes some task in a separate thread.
 *
 * Task usage:
 * - Derive from threaded_task and implement task_main().
 * - Tasks initially are ready(). An explicit start() call starts
 *		or restarts a ready task. A new OS thread then executes
 *		the task_main() function.
 * - All started tasks (even self-terminating ones) must be join()ed.
 *		In particular, tasks do not automatically reset to ready state
 *		once the OS-side thread_has_terminated().
 * - @attent Tasks must be join()ed (i.e. ready()) before destruction.
 *		Otherwise, there most likely are racing conditions. Usually,
 *		derived task classes should call join() in their destructor.
 * - A join_now() flag inside the task instance advises the task_main
 *		function to return shortly. Long-running or looping tasks
 *		should regularly check the join flag and should return from
 *		their main function if it is set. Short or one-shot worker
 *		tasks may return on their own and may ignore the join flag.
 *
 * General notes:
 * - Tasks are directly mapped to OS-internal threads.
 *		Thus, there may be OS quirks such as arbitrary scheduling.
 * - One should regularly test tasks with thread_has_terminated()
 *		to detect unexpected or exceptional OS thread termination.
 * - Tasks provide strong exception safety: An exception is thrown
 *		in case of failure, but the task object remains unchanged.
 * - Tasks are threadsafe for use by arbitrary threads.
 *
 ************************************************************************/
class threaded_task :
	private non_copyable
{
	public:

		threaded_task();
		virtual ~threaded_task() NOTHROW;


		bool ready() const NOTHROW;
		bool thread_has_terminated() const NOTHROW;
		bool join_now() const NOTHROW;


		/** Start or restart a ready() task. */
		void start();


		/**
		 * If the task is not ready(), raise join_now() and block
		 * until the OS thread_has_terminated(). Thereafter, reset
		 * the task to ready state. If the OS thread has terminated
		 * due to an unhandled exception, throw unhandled_exception.
		 *
		 * If the task is ready() already, joining is ignored.
		 * Join fails if another thread is joining concurrently.
		 * Also note a thread cannot self-join with its task.
		 */
		void join();


		/**
		 * If @true and @ref VIRAL__BUILD_MODE_DEBUG is set,
		 * do not catch unhandled exceptions in this thread,
		 * but instead break with a proper stack trace.
		 */
		//@{
		void set_debug_exception_trace(bool debug_exception_trace);
		bool debug_exception_trace() const NOTHROW;
		//@}


		/**
		 * Fetch the thread_id for the OS thread of this task.
		 * Returns thread_id::invalid() if the task is ready(),
		 * such as after join()ing a formerly running task.
		 */
		thread_id id() const NOTHROW;


	private:

		/** Executes within a separate thread. */
		virtual void task_main() = 0;


		#if defined(VIRAL__USE_MS_THREAD)
			void task_startup_wrapper();
			static unsigned int __stdcall
				static_task_startup_wrapper(void *untyped_task);

			static const DWORD wait_time_for_thread_termination_;

			mutable mutex task_lock_;
			bool join_now_;
			bool had_unhandled_exception_;
			bool debug_exception_trace_;

			HANDLE thread_handle_;
			unsigned int thread_id_;
		#endif

		#if defined(VIRAL__USE_CPP11_THREAD)
			void task_startup_wrapper();
			static void static_task_startup_wrapper(threaded_task *task);

			static const float wait_time_for_thread_termination_;

			mutable mutex task_lock_;
			bool join_now_;
			bool thread_has_terminated_;
			bool had_unhandled_exception_;
			bool debug_exception_trace_;

			std::thread std_thread_;	
		#endif

		#if !defined(VIRAL__USE_MS_THREAD) && \
			!defined(VIRAL__USE_CPP11_THREAD)
				#error "Not implemented for current thread back-end."
		#endif
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__THREAD_HPP) */

