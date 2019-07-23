/**
 *************************************************************************
 *
 * @file allocator_batch.cpp
 *
 * Generic batch allocator, implementation.
 *
 ************************************************************************/


#include "allocator_batch.hpp"


#include "log.hpp"
#include "integer_operations.hpp"
#include "container_limits.hpp"
#include "auto_pointer.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// allocator_batch_store
//
//////////////////////////////////////////////////////////////////////////


allocator_batch_store::allocator_batch_store
	(int64 bytes_per_entry,
	 int64 entries_per_batch) NOTHROW
	:
	bytes_per_entry_(bytes_per_entry),
	entries_per_batch_(entries_per_batch),
	capacity_(0)
{ }


allocator_batch_store::~allocator_batch_store() NOTHROW
{
	if (unused_lookup_.size() != capacity_)
	{
		LOG_WARN("Not all allocations have been freed explicitly. "
				 "The batch store frees any leaked allocations, "
				 "but issues (e.g. stray pointers) may remain.");

		// Exception safety: NOTHROW
	}
}


void allocator_batch_store::reserve(int64 new_capacity)
{
	// Lazy checks for store parameters.
	validate_parameters();


	// Validate requested capacity.
	// We only test against the limits of the lookup buffer,
	// so that the below loop for batch preallocation works.
	if (new_capacity < 0)
	{
		LOG_ERROR("Requested invalid capacity.");
		throw invalid_parameter();
	}

	if (new_capacity > container_limits<char*>::max_size)
	{
		LOG_ERROR("Requested capacity exceeds container limits.");
		throw bounds_exceeded();
	}


	// Allocate batches to meet capacity.
	// Note that even for a valid new_capacity,
	// batch preallocation may exceed max_size.
	// This in turn causes below call to throw.
	while (capacity_ < new_capacity)
		preallocate_new_batch();
}


void* allocator_batch_store::alloc_untyped(int64 size)
{
	// Early out:
	// Requests for zero bytes always succeed
	// and do not consume an unused entry.
	if (size == 0)
		return NULL;


	// Lazy checks for store parameters.
	validate_parameters();


	// Check requested size.
	if (size < 0)
	{
		LOG_ERROR("Cannot request less than zero bytes.");
		throw invalid_parameter();
	}

	if (size > bytes_per_entry_)
	{
		LOG_ERROR("Requested more bytes than available per entry.");
		throw bounds_exceeded();
	}


	// Preallocate an entire new batch
	// if unused entries are exhausted.
	// Basic exception safety: No undo,
	// capacity() may change on failure.
	if (unused_lookup_.empty())
		preallocate_new_batch();

	if (unused_lookup_.empty())
	{
		LOG_ERROR("Batch preallocation did not add an unused entry.");
		throw assert_failed();
	}


	// Fetch unused allocation.
	// NOTHROW from here.
	void* next_allocation =
		unused_lookup_[unused_lookup_.size() - 1];

	unused_lookup_.pop_back();
	return next_allocation;
}


int64 allocator_batch_store::bytes_per_entry() const NOTHROW
	{ return bytes_per_entry_; }
int64 allocator_batch_store::entries_per_batch() const NOTHROW
	{ return entries_per_batch_; }


int64 allocator_batch_store::capacity() const NOTHROW
	{ return capacity_; }


int64 allocator_batch_store::used_count() const NOTHROW
	{ return capacity_ - unused_lookup_.size(); }
int64 allocator_batch_store::unused_count() const NOTHROW
	{ return unused_lookup_.size(); }


void allocator_batch_store::free(void* data) NOTHROW
{
	// Never feed back NULL pointers,
	// as these have not been acquired
	// from batch entries.
	if (!data) return;


	// This push_back() is NOTHROW, since unused_lookup_
	// has reserved enough memory for all batch entries.
	// See preallocate_new_batch().
	unused_lookup_.push_back
		(reinterpret_cast<char*>(data));
}


void allocator_batch_store::free_batches() NOTHROW
{
	if (unused_lookup_.size() != capacity_)
	{
		LOG_WARN("Cannot free batches when allocations remain.");
		return;
	}

	capacity_ = 0;
	batches_.clear();
	unused_lookup_.free();
}


void allocator_batch_store::validate_parameters()
{
	/**
	 * Lazy check for container parameters.
	 * This enables a NOTHROW constructor.
	 * Call before preallocate_new_batch().
	 */

	if (bytes_per_entry_ <= 0 ||
		bytes_per_entry_ > container_limits<char>::max_size)
	{
		LOG_ERROR("Batch allocator has invalid bytes per entry.");
		throw invalid_state();
	}

	if (entries_per_batch_ <= 0 ||
		entries_per_batch_ > container_limits<char>::max_size)
	{
		LOG_ERROR("Batch allocator has invalid entries per batch.");
		throw invalid_state();
	}

	if (!integer_checks::is_mult_safe
			(bytes_per_entry_, entries_per_batch_) ||
		bytes_per_entry_ * entries_per_batch_ >
			container_limits<char>::max_bytes)
	{
		LOG_ERROR("Batch allocator has invalid bytes per batch.");
		throw invalid_state();
	}
}


void allocator_batch_store::preallocate_new_batch()
{
	/**
	 * Preallocate a new batch and a suitable pointer lookup buffer,
	 * then place preallocated entries into @bref{unused_lookup_}.
	 * This call assumes validate_parameters() has succeeded.
	 */


	// Preallocate lookup buffer:
	// - Preallocation enables NOTHROW on pointer push_back(),
	//		such as in subsequent release().
	// - The dynamic_buffer may overallocate to a capacity()
	//		that exceeds the final allocator_batch::capacity_.
	// - Capacity of the lookup buffer is not visible state,
	//		so we need not undo on failure.
	int64 new_capacity =
		integer_operations::add_or_throw
			(capacity_, entries_per_batch_);

	if (new_capacity > container_limits<void*>::max_size)
	{
		LOG_ERROR("Capacity exceeds maximum container size.");
		throw bounds_exceeded();
	}
							
	unused_lookup_.reserve(new_capacity);


	// Preallocate new batch.
	auto_pointer<batch> new_batch(new batch());	
	new_batch->resize
		(integer_operations::mult_or_throw
			(bytes_per_entry_, entries_per_batch_));

	batch& b = *new_batch;
	batches_.push_back(std::move(new_batch));


	// Store entries of the new batch as unused.
	// NOTHROW from here due to preallocation.
	int64 start_index = unused_lookup_.size();
	unused_lookup_.resize
		(unused_lookup_.size() + entries_per_batch_);
	
	char* VIRAL__RESTRICT_PTR src = b.data();
	char** VIRAL__RESTRICT_PTR dst =
		unused_lookup_.data() + start_index;

	for (int64 i = 0; i < entries_per_batch_; ++i)
	{
		*dst = src;
		src += bytes_per_entry_;
		++dst;
	}


	// Preallocation succeeded.
	// Store new, now valid capacity.
	capacity_ = new_capacity;
}




//////////////////////////////////////////////////////////////////////////
//
// allocator_batch
//
//////////////////////////////////////////////////////////////////////////


allocator_batch::allocator_batch
	(allocator_batch_store& store)
	:
	store_(&store)
{ }


allocator_batch::allocator_batch
	(const allocator_batch& other)
	:
	store_(other.store_)
{ }

allocator_batch::allocator_batch
	(allocator_batch&& other) NOTHROW
	:
	store_(other.store_)
{ }


allocator_batch&
	allocator_batch::operator =
		(const allocator_batch& other)
{
	store_ = other.store_;
	return *this;
}

allocator_batch&
	allocator_batch::operator =
		(allocator_batch&& other) NOTHROW
{
	store_ = other.store_;
	return *this;
}


void allocator_batch::swap_with
	(allocator_batch& other) NOTHROW
		{ swap(store_, other.store_); }


void* allocator_batch::alloc_untyped(int64 size)
{
	// The store never is NULL.
	return store_->alloc_untyped(size);
}


void allocator_batch::free(void* data) NOTHROW
{
	// The store never is NULL.
	return store_->free(data);
}


bool allocator_batch::can_free_for
	(const allocator_batch& other) const NOTHROW
		{ return store_ == other.store_; }


allocator_batch_store&
	allocator_batch::store() const NOTHROW
{
	// The store never is NULL.
	return *store_;
}




} /* namespace viral_core */

