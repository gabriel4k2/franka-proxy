/**
 *************************************************************************
 *
 * @file thread_atomic.hpp
 *
 * Wrapper for atomic values.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__THREAD_ATOMIC_HPP)
#define INCLUDED__VIRAL_CORE__THREAD_ATOMIC_HPP


#include "build_config.hpp"


#include "thread_synch.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class atomic
 * @ingroup thread_module
 *
 * Mutex-protected wrapper around a value of type @bref{T}.
 *
 * Usage notes:
 * - @attent Construction of atomic values is never NOTHROW
 *		due to initialization of the contained @ref mutex.
 * - Other @ref NOTHROW operations in type @bref{T} are NOTHROW
 *		in the atomic wrapper as well (e.g. assignment operators).
 *		If operations on @bref{T} are strongly exception safe,
 *		then the wrapper is strongly exception safe, too.
 *
 ************************************************************************/
template <typename T>
	class atomic
{

	public:

		atomic()
			: value_() { }
		atomic(const T& value)
			: value_(value) { }


		/** Threadsafe copy of @bref{other} atomic. */
		atomic(const atomic& other)
			:
			value_(other.value()) // Thread safety: Use locking .value()
		{ }


		atomic& operator =(const T& value)
		{
			MUTEX_SCOPE(mutex_);
			value_ = value;
			return *this;
		}


		/**
		 * Threadsafe assignment of @bref{other} atomic.
		 * Note concurrent, cyclic assignment is deadlock-free.
		 */
		atomic& operator =(const atomic& other)
		{
			// Avoid deadlock: Only one lock at a time.
			return operator =(other.value());
		}


		T value() const
		{
			MUTEX_SCOPE(mutex_);
			return value_;
		}


		/**
		 * Set new atomic value and fetch replaced value
		 * in a single, threadsafe transaction.
		 *
		 * Note concurrent, cyclic get_and_set is deadlock-free.
		 */
		//@{
		T get_and_set(const T& value)
		{
			MUTEX_SCOPE(mutex_);
			T old_value(value_);
			value_ = value;
			return old_value;
		}

		T get_and_set(const atomic& other)
		{
			// Avoid deadlock: Only one lock at a time.
			return get_and_set(other.value());
		}
		//@}


	private:

		mutable mutex mutex_;
		T value_;
};


/**
 * Default atomic types.
 * @ingroup thread_module
 */
//@{
typedef atomic<bool> atomic_bool;
typedef atomic<int32> atomic_int32;
typedef atomic<int64> atomic_int64;
typedef atomic<float> atomic_float;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__THREAD_ATOMIC_HPP) */


