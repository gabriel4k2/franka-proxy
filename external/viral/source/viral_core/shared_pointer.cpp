/**
 *************************************************************************
 *
 * @file shared_pointer.cpp
 *
 * Shared pointer implementation details.
 *
 ************************************************************************/


#include "shared_pointer.hpp"


#if defined(VIRAL__USE_MS_THREAD)
	#include <windows.h>
#endif


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	shared_life_manager_base, Microsoft Windows implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_MS_THREAD)


void shared_life_manager_base::add_shared_pointer() NOTHROW
{
	// We are copying from an existing, exclusively touched
	// shared pointer that ensures object lifetime exceeds
	// this call, thus no concurrency problems.
	InterlockedIncrement64(&shared_pointer_count_);
	InterlockedIncrement64(&total_pointer_count_);
}


void shared_life_manager_base::add_weak_pointer() NOTHROW
{
	// As the above, but for an exclusive weak_pointer source.
	InterlockedIncrement64(&total_pointer_count_);
}


void shared_life_manager_base::remove_shared_pointer() NOTHROW
{
	// If a counter reaches 0, we have been the last pointer in
	// category. Thus no concurrent, conflicting add_*_pointer()
	// or remove_*_pointer() call is possible.
	if (InterlockedDecrement64(&shared_pointer_count_) == 0)
		delete_object();
	if (InterlockedDecrement64(&total_pointer_count_) == 0)
		delete this;
}	


void shared_life_manager_base::remove_weak_pointer() NOTHROW
{
	// As the above.
	if (InterlockedDecrement64(&total_pointer_count_) == 0)
		delete this;
}


bool shared_life_manager_base::try_lock_weak_pointer() NOTHROW
{
	/**
	 * Try to lock a weak pointer into a new shared pointer -
	 * which need not succeed if somebody released or releases
	 * the last shared pointer.
	 *
	 * Implementation idea:
	 * Loop until the shared pointer update succeeds against
	 * potential concurrent writes. Success means we performed
	 * an exchange-based, simulated add operation on the correct
	 * starting value that we saved locally on this thread's stack.
	 */

	while (true)
	{
		#if defined(VIRAL__MACHINE_TARGET_X64)

			// Reads on int64 are atomic on 64 bit Windows.
			// We can thus avoid an expensive interlock here.
			int64 current = shared_pointer_count_;
			if (current == 0) return false;

		#else
			#error "Not implemented for current machine target."
		#endif

		int64 next = current + 1;
		if (InterlockedCompareExchange64
				(&shared_pointer_count_, next, current) == current)
		{
			InterlockedIncrement64(&total_pointer_count_);
			return true;					
		}
	}
}


bool shared_life_manager_base::try_destroy_if_unique() NOTHROW
{
	/**
	 * Only release a shared pointer if it currently is unique.
	 * The pointer count must only be decreased from 1 to 0 and
	 * only in an atomic operation to avoid race conditions with
	 * concurrent weak locks or shared pointer copies.
	 */


	// Reads on int64 are atomic on 64 bit Windows.
	// Thus, perform a redundant early-out test
	// to avoid costy atomic write operations.
	#if defined(VIRAL__MACHINE_TARGET_X64)
		if (shared_pointer_count_ > 1) return false;
	#endif


	if (InterlockedCompareExchange64(&shared_pointer_count_, 0, 1) != 1)
		return false;

	delete_object();

	if (InterlockedDecrement64(&total_pointer_count_) == 0)
		delete this;

	return true;
}


shared_life_manager_base::shared_life_manager_base() NOTHROW
	: shared_pointer_count_(1), total_pointer_count_(1) { }


#endif /* defined(VIRAL__USE_MS_THREAD) */





//////////////////////////////////////////////////////////////////////////
//
//	shared_life_manager_base, C++11 std implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__USE_CPP11_THREAD)


void shared_life_manager_base::add_shared_pointer() NOTHROW
{
	// We are copying from an existing, exclusively touched
	// shared pointer that ensures object lifetime exceeds
	// this call, thus no concurrency problems.
	++shared_pointer_count_;
	++total_pointer_count_;
}


void shared_life_manager_base::add_weak_pointer() NOTHROW
{
	// As the above, but for an exclusive weak_pointer source.
	++total_pointer_count_;
}


void shared_life_manager_base::remove_shared_pointer() NOTHROW
{
	// If a counter reaches 0, we have been the last pointer in
	// category. Thus no concurrent, conflicting add_*_pointer()
	// or remove_*_pointer() call is possible.
	if ((--shared_pointer_count_) == 0)
		delete_object();
	if ((--total_pointer_count_) == 0)
		delete this;
}	


void shared_life_manager_base::remove_weak_pointer() NOTHROW
{
	// As the above.
	if ((--total_pointer_count_) == 0)
		delete this;
}


bool shared_life_manager_base::try_lock_weak_pointer() NOTHROW
{
	/**
	 * Try to lock a weak pointer into a new shared pointer -
	 * which need not succeed if somebody released or releases
	 * the last shared pointer.
	 *
	 * Implementation idea:
	 * Loop until the shared pointer update succeeds against
	 * potential concurrent writes. Success means we performed
	 * an exchange-based, simulated add operation on the correct
	 * starting value that we saved locally on this thread's stack.
	 */

	while (true)
	{
		int64 current = shared_pointer_count_;
		if (current == 0) return false;

		int64 next = current + 1;
		if (shared_pointer_count_.compare_exchange_weak(current, next))
		{
			++total_pointer_count_;
			return true;					
		}
	}
}


bool shared_life_manager_base::try_destroy_if_unique() NOTHROW
{
	/**
	 * Only release a shared pointer if it currently is unique.
	 * The pointer count must only be decreased from 1 to 0 and
	 * only in an atomic operation to avoid race conditions with
	 * concurrent weak locks or shared pointer copies.
	 */

	int64 one = 1;
	if (!shared_pointer_count_.compare_exchange_strong(one, 0))
		return false;

	delete_object();

	if ((--total_pointer_count_) == 0)
		delete this;

	return true;
}


shared_life_manager_base::shared_life_manager_base() NOTHROW :
	shared_pointer_count_(1), total_pointer_count_(1)
{
	// We initially have exactly one shared_pointer
	// to our target object.
}


#endif /* defined(VIRAL__USE_CPP11_THREAD) */




} /* namespace viral_core */
