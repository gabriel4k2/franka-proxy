/**
 *************************************************************************
 *
 * @file thread_read_write.hpp
 *
 * Single reader-writer locks for threads.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__THREAD_READ_WRITE_HPP)
#define INCLUDED__VIRAL_CORE__THREAD_READ_WRITE_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mutex_read_write
 * @ingroup thread_module
 *
 * A mutex with exclusive writing and shared reading access.
 *
 * Reader-writer locking:
 * - A mutex_read_write allows an arbitrary number of readers
 *		(e.g. as in try_acquire_read()), but only one writer
 *		(e.g. as in try_acquire_write()) at any given time.
 * - To manage potential starving of reading or writing threads,
 *		 clients can specifiy a suitable @bref{fairness_policy}.
 * - @attent Clients must release access of the acquired type
 *		(e.g. only use release_read() for try_acquire_read()).
 *		Otherwise, behaviour of the mutex becomes undefined.
 *
 * Usage notes:
 * - See @ref mutex for general usage notes.
 * - All @ref mutex utilities exist as variants for mutex_read_write
 *		(e.g. mutex_scope, mutex_scope_read and mutex_scope_write).
 * - @attent Unlike @ref mutex, mutex_read_write is only reentrant
 *		for one thread with multiple reader locks, but not for one
 *		thread with multiple writer locks.
 *
 ************************************************************************/
class mutex_read_write :
	private non_copyable
{

	public:

		enum fairness_policy
		{
			/**
			 * If multiple readers and writers are waiting,
			 * pick next locking thread by arbitrary choice.
			 * This is not necessarily random and may starve
			 * both readers and writers.
			 */
			fairness_arbitrary,

	
			/**
			 * If multiple readers and writers are waiting,
			 * always choose an arbitrary reading thread.
			 * Perpetual reading thus starves writers.
			 */
			fairness_prefer_readers,


			/**
			 * If multiple readers and writers are waiting,
			 * always choose an arbitrary writing thread.
			 * Perpetual writing thus starves readers.
			 */
			fairness_prefer_writers
		};


		mutex_read_write(fairness_policy policy);


		/**
		 * Acquire the mutex for reading or writing,
		 * and block until success.
		 */
		//@{
		void acquire_read() NOTHROW;
		void acquire_write() NOTHROW;
		//@}


		/**
		 * Try to acquire the mutex for reading or writing,
		 * and immediately return @false on concurrent access.
		 */
		//@{
		bool try_acquire_read() NOTHROW;
		bool try_acquire_write() NOTHROW;
		//@}


		/**
		 * Release the mutex if previously acquired
		 * as either read or write access.
		 */
		//@{
		void release_read() NOTHROW;
		void release_write() NOTHROW;
		//@}


		/** Construction-set fairness policy. */
		const fairness_policy policy;
};




/**
 *************************************************************************
 *
 * Macros for protecting some scope, such as a method call,
 * with a @bref{mutex_read_write} for reading or writing.
 * @ingroup thread_module
 *
 ************************************************************************/
//@{
#define MUTEX_SCOPE_READ(mutex_read_write) \
	mutex_scope_read \
		MAKE_UNIQUE_VARIABLE_NAME(mutex_read_write, __LINE__)(mutex_read_write);

#define MUTEX_SCOPE_WRITE(mutex_read_write) \
	mutex_scope_write \
		MAKE_UNIQUE_VARIABLE_NAME(mutex_read_write, __LINE__)(mutex_read_write);
//@}




/**
 *************************************************************************
 *
 * @class mutex_scope_read
 * @ingroup thread_module
 *
 * Automatically acquires and releases a @bref{target_mutex} for reading.
 *
 ************************************************************************/
class mutex_scope_read :
	private non_copyable
{

	public:

		mutex_scope_read(mutex_read_write& target_mutex) NOTHROW
			: target_mutex_(target_mutex) { target_mutex_.acquire_read(); }

		~mutex_scope_read() NOTHROW
			{ target_mutex_.release_read(); }

	private:

		mutex_read_write& target_mutex_;
};




/**
 *************************************************************************
 *
 * @class mutex_scope_write
 * @ingroup thread_module
 *
 * Automatically acquires and releases a @bref{target_mutex} for writing.
 *
 ************************************************************************/
class mutex_scope_write :
	private non_copyable
{

	public:

		mutex_scope_write(mutex_read_write& target_mutex) NOTHROW
			: target_mutex_(target_mutex) { target_mutex_.acquire_write(); }

		~mutex_scope_write() NOTHROW
			{ target_mutex_.release_write(); }

	private:

		mutex_read_write& target_mutex_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__THREAD_READ_WRITE_HPP) */
