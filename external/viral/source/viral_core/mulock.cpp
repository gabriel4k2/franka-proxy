/**
 *************************************************************************
 *
 * @file mulock.cpp
 *
 * Multi-lock with reader-writer support, implementation.
 *
 ************************************************************************/


#include "mulock.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mulock
//
//////////////////////////////////////////////////////////////////////////


mulock::mulock
	(mulock_context& context,
	 const mulock_request& request)
	:
	context_(context),
	requests_(to_request_list(request))
{
	acquire_lock();
}


mulock::mulock
	(mulock_context& context,
	 const mulock_request_list& requests)
	:
	context_(context),
	requests_(requests)
{	
	acquire_lock();
}


mulock::~mulock() NOTHROW
	{ release_lock(); }


mulock_result mulock::at(const shared_instance<mulock_id>& id)
	{ return context_.fetch_lock_result(id); }


mulock_request_list mulock::to_request_list
	(const mulock_request& request)
{
	mulock_request_list request_list;
	request_list.push_back(request);
	return request_list;
}


void mulock::acquire_lock()
	{ context_.acquire_lock(requests_); }

void mulock::release_lock() NOTHROW
	{ context_.release_lock(); }




//////////////////////////////////////////////////////////////////////////
//
// mulock_state
//
//////////////////////////////////////////////////////////////////////////


mulock_state::mulock_state
	(auto_pointer<mulock_data> data)
	:
	data_(std::move(data)),
	lock_type_(mulock_request::write),
	lock_count_(0)
{ }




//////////////////////////////////////////////////////////////////////////
//
// mulock_context
//
//////////////////////////////////////////////////////////////////////////


mulock_context::mulock_context() { }


mulock_context::~mulock_context() NOTHROW
{
	purge();

	if (!id_to_state_.empty())
	{
		LOG_ERROR
			("Some mulock_ids have not been released\n"
			 "before the destruction of the mulock_context.\n"			 
			 "Threadsafe data destruction is not guaranteed,\n"
			 "and the application will potentially crash.");

		id_to_state_.clear();
	}
}


void mulock_context::register_data
	(const shared_instance<mulock_id>& id,
	 auto_pointer<mulock_data> data)
{
	MUTEX_SCOPE(mutex_);

	if (!data)
	{
		LOG_ERROR("Cannot register NULL data.");
        throw null_pointer();
	}

	if (id_to_state_.contains(id))
	{
		LOG_ERROR("ID: " + id().debug_name + " is already registered.");
        throw object_registration_failed();
	}

	auto_pointer<mulock_state> new_state
		(new mulock_state(std::move(data)));

	id_to_state_.insert
		(id, std::move(new_state));
}


void mulock_context::purge() NOTHROW
{
	MUTEX_SCOPE(mutex_);

	bool any_deleted;

	// There might be a mulock_data that has mulock_id%s.
	// Thus, we must multipass here for complete purging.
	do
	{
		any_deleted = false;

		for (id_to_state_map::iterator it(id_to_state_.first()); it; )
		{
			id_to_state_map::iterator test_id(it);
			++it;

			if (test_id->key.lock())
				continue;


			// If we delete the marker for limited purge,
			// continue purging on the next mulock_data.
			if (test_id == id_to_state_next_purge_)
				++id_to_state_next_purge_;					


			// Mulock ID was deleted and can never be lock()ed again.
			// Thus, delete attached back-end mulock_data.
			id_to_state_.remove(test_id);
			any_deleted = true;
		}
	}
	while (any_deleted);
}


void mulock_context::purge(int64 purge_limit) NOTHROW
{
	MUTEX_SCOPE(mutex_);

	if (purge_limit < 0)
	{
		LOG_ERROR("Invalid purge limit: " + purge_limit);
		return; // NOTHROW
	}


	// Purge logics: Start on first mulock ID,
	// or continue from preceding limited purge.
	// We wrap around after the last mulock ID
	// to handle mulock_data with mulock_id%s.
	for (int64 i = 0; i < purge_limit; ++i)
	{
		if (!id_to_state_next_purge_)
			id_to_state_next_purge_ = id_to_state_.first();

		if (!id_to_state_next_purge_)
			return; // Mulock context has no mulock_data.

		id_to_state_map::iterator test_id(id_to_state_next_purge_);
		++id_to_state_next_purge_;

		if (test_id->key.lock())
			continue;


		// Mulock ID was deleted and can never be lock()ed again.
		// Thus, delete attached back-end mulock_data.
		id_to_state_.remove(test_id);
	}
}


void mulock_context::acquire_lock
	(const mulock_request_list& requests)
{
	bool yield_mutex = false;

	while (true)
	{
		// If there was a read-write conflict,
		// yield to other threads before retrying.
		// This favors probably conflict-free threads
		// over the pending conflict in our thread,
		// and thus avoids possible resource hogging.
		if (yield_mutex)
			thread_util::sleep_slice();


		MUTEX_SCOPE(mutex_);


		// Avoid deadlock by prohibiting requests
		// for additional locks on single threads.
		if (thread_to_locked_.contains(thread_id::this_thread()))
		{
			LOG_ERROR("Duplicate mulock detected on current thread.");
			throw invalid_operation();
		}


		// Check if all lock requests are conflict free:
		// A single writing lock prohibits any readers.
		// On any conflict, release global mutex and retry.
		bool write_conflict = false;
		for (mulock_request_list::const_iterator it
				(requests.first()); it; ++it)
		{
			id_to_state_map::const_iterator current
				(id_to_state_.find(it->id));

			if (!current || !current->value)
			{
				LOG_ERROR("No data registered for ID: " + it->id().debug_name);
				throw invalid_operation();
			}

			if (current->value->lock_count_ == 0)
				continue; // Data currently unlocked.

			if (it->lock == mulock_request::write ||
				current->value->lock_type_ == mulock_request::write)
			{
				write_conflict = true;
				break;
			}
		}

		if (write_conflict)
		{
			yield_mutex = true;
			continue;
		}


		// Store locked IDs per-thread, undo on failure.
		// Detect redundant requests, as these break locking.
		thread_to_locked_map::iterator thread_to_locked_it
			(thread_to_locked_.insert
				(thread_id::this_thread(), locked_set()));

		SCOPE_GUARD
			(lock_guard,
				(thread_to_locked_,
				 &thread_to_locked_map::remove_by_iterator,
				 thread_to_locked_it));

		for (mulock_request_list::const_iterator it
				(requests.first()); it; ++it)
		{
			if (thread_to_locked_it->value.contains(it->id))
			{
				LOG_ERROR("Redundant request for ID: " + it->id().debug_name);
				throw invalid_operation();
			}

			thread_to_locked_it->value.insert(it->id);
		}

		lock_guard.dismiss();

		
		// Store lock type and increase lock counter for each ID.
		// Our lock is conflict free, thus overwriting lock_type_
		// only affects mulock_states with a zero lock_count_.
		// Exception safety: NOTHROW.
		for (mulock_request_list::const_iterator it
				(requests.first()); it; ++it)
		{
			id_to_state_map::const_iterator current
				(id_to_state_.find(it->id));

			if (!current || !current->value)
				continue; // NOTHROW, should not happen.

			current->value->lock_type_ = it->lock;
			current->value->lock_count_ += 1;
		}


		// Locking succeeded, so abort loop.
		// Exception safety: NOTHROW.
		lock_guard.dismiss();
		return;
	}
}


void mulock_context::release_lock() NOTHROW
{
	MUTEX_SCOPE(mutex_);


	// Find set of per-thread locked IDs.
	thread_to_locked_map::iterator thread_locked_it
		(thread_to_locked_.find(thread_id::this_thread()));

	if (!thread_locked_it)
	{
		LOG_ERROR("Thread did not lock before release. "
				  "This should not actually happen, "
				  "unless the mulock RAII is broken.");
		return; // Satisfy NOTHROW.
	}


	// Undo acquire_lock():
	// Decrease lock counters in global state map,
	// and delete per-thread locked set.
	for (locked_set::iterator it
			(thread_locked_it->value.first()); it; ++it)
	{
		id_to_state_map::const_iterator current
			(id_to_state_.find(*it));

		if (!current || !current->value)
			continue; // NOTHROW, should not happen.

		current->value->lock_count_ -= 1;
	}

	thread_to_locked_.remove(thread_locked_it);
}


mulock_result mulock_context::fetch_lock_result
	(const shared_instance<mulock_id>& id)
{
	MUTEX_SCOPE(mutex_);


	// Ensure that the incoming ID was locked:
	// Find set of all locked IDs for our thread
	// and check if this set contains the given ID.
	thread_to_locked_map::iterator thread_locked_it
		(thread_to_locked_.find(thread_id::this_thread()));

	if (!thread_locked_it)
	{
		LOG_ERROR("Current thread has no locked IDs. "
				  "This should not actually happen, "
				  "unless the mulock RAII is broken.");
		throw assert_failed();
	}

	if (!thread_locked_it->value.contains(id))
	{
		LOG_ERROR("Requested ID: " + id().debug_name +
				  " was not part of mulock request.");
        throw invalid_state();
	}


	// Fetch lock type and data from global state map.
	// The later mulock_access or mulock_const_access
	// checks for a const-matching lock_type_.
	id_to_state_map::iterator state_it
		(id_to_state_.find(id));

	if (!state_it || !state_it->value)
	{
		LOG_ERROR("ID is locked, but has no state. "
				  "This should not actually happen, "
				  "unless the mulock RAII is broken.");
        throw assert_failed();
	}

	if (!state_it->value->data_)
	{
		LOG_ERROR("ID is locked, but has no data. "
				  "This should not actually happen, "
				  "unless the mulock RAII is broken.");
        throw assert_failed();
	}

	return mulock_result
				(mulock_request(id, state_it->value->lock_type_),
				 state_it->value->data_.get());
}




} /* namespace viral_core */
