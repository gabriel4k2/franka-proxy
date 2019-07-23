/**
 *************************************************************************
 *
 * @file allocator_batch.hpp
 *
 * Generic batch allocator.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ALLOCATOR_BATCH_HPP)
#define INCLUDED__VIRAL_CORE__ALLOCATOR_BATCH_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "buffer.hpp"
#include "owning_list.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class allocator_batch_store
 * @ingroup allocator_module
 * 
 * Efficiently stores memory allocations of limited size,
 * typically for use through allocator_batch.
 *
 * Usage notes:
 * - A batch store fulfills allocations from preallocated batches,
 *		where each batch consists of constant entries_per_batch(),
 *		and where each entry can store exactly one alloc_untyped()
 *		with up to bytes_per_entry() of client payload. Conversely,
 *		allocation requests for more than bytes_per_entry() fail.
 * - In contrast to discrete heap allocations, batch stores reduce
 *		both memory and performance overhead (e.g. no heap locking)
 *		when frequently allocating and freeing bytes_per_entry().
 * - Each preallocated entry does always occupy all bytes_per_entry()
 *		plus internal overhead of around 8 bytes. This in particular
 *		does not depend on the actual size of allocation requests.
 * - Batch stores cannot coalesce or free (partially) unused batches.
 *		Batches remain preallocated until clients explicitly free()
 *		all allocations and then free_batches(). Typical containers
 *		free() allocations, but do not free_batches(), higher-level
 *		clients thus must free_batches() manually if required.
 * - The capacity() and thus the limit on the total allocation count
 *		of a batch store may be less than container_limits::max_size,
 *		as capacity() can grow but in multiples of entries_per_batch().
 *
 ************************************************************************/
class allocator_batch_store :
	private non_copyable
{

	public:

		/**
		 * Create an empty batch store
		 * with supplied allocation parameters.
		 *
		 * @attent
		 * To enable a @ref NOTHROW default constructor,
		 * all parameters (e.g. @bref{bytes_per_entry})
		 * are checked but on the first batch allocation.
		 */
		allocator_batch_store
			(int64 bytes_per_entry,
			 int64 entries_per_batch) NOTHROW;


		~allocator_batch_store() NOTHROW;


		/**
		 * Preallocate enough batches and entries to accomodate
		 * at least @bref{new_capacity} calls to alloc_untyped()
		 * without later dynamic heap allocation.
		 *
		 * On failure, the capacity() of the batch allocator
		 * may have partially increased.
		 */
		void reserve(int64 new_capacity);


		/**
		 * Allocate @bref{size} bytes, but at most bytes_per_entry().
		 * Only performs heap allocations if entries are exhausted.
		 *
		 * Throws on any error (e.g. too large allocation request),
		 * but does not free any additionally preallocated batches.
		 * Thus, capacity() may change after an exception.
		 */
		void* alloc_untyped(int64 size);


		int64 bytes_per_entry() const NOTHROW;
		int64 entries_per_batch() const NOTHROW;


		int64 capacity() const NOTHROW;


		/**
		 * Count of in use and of still available,
		 * preallocated entries.
		 */
		//@{
		int64 used_count() const NOTHROW;
		int64 unused_count() const NOTHROW;
		//@}


		/**
		 * Free previously allocated @bref{data}.
		 *
		 * @attent
		 * Undefined and difficult to debug behaviour
		 * if @bref{data} was not allocated previously.
		 */
		void free(void* data) NOTHROW;


		/**
		 * Free preallocated batches, thus resetting capacity().
		 * Unless all allocations have been free()d beforehand,
		 * this call is ignored with a warning.
		 */
		void free_batches() NOTHROW;


	private:

		void validate_parameters();
		void preallocate_new_batch();


		typedef dynamic_buffer<char> batch;
		typedef owning_list<batch> batch_list;

		const int64 bytes_per_entry_;
		const int64 entries_per_batch_;

		int64 capacity_;
		batch_list batches_;
		dynamic_buffer<char*> unused_lookup_;
};




/**
 *************************************************************************
 *
 * @class allocator_batch
 * @ingroup allocator_module
 *
 * Efficiently allocates memory from an allocator_batch_store.
 *
 * Usage notes:
 * - See allocator_batch_store for general documentation.
 * - Each allocator_batch needs a suitable allocator_batch_store.
 *		Therefore, allocator_batch is not default-constructable,
 *		and neither are respective containers.
 * - Multiple batch allocators may share an allocator_batch_store.
 *		In this case, the allocators can_free_for() one another.
 * - Clients (notably, application code) must explicitly ensure
 *		that backing store()s outlive bound allocator_batch%es.
 * - When using allocator_batch for containers, clients must supply
 *		an allocator_batch_store with sufficient bytes_per_entry() 
 *		for internal objects (e.g. nodes) of the container.
 *
 * Thread safety:
 * - Neither allocator_batch nor allocator_batch_store are threadsafe.
 *		Notably, only one client thread at a time may work on containers
 *		that share an allocator_batch_store through allocator_batch%es.
 *
 ************************************************************************/
class allocator_batch
{

	public:

		/**
		 * Create a new allocator_batch that uses
		 * the given backing @bref{store}.
		 */
		allocator_batch
			(allocator_batch_store& store);


		/**
		 * Create a new allocator_batch that shares
		 * backing store() with a source allocator.
		 * The source allocator remains unchanged,
		 * existing allocations are not duplicated.
		 */
		//@{
		allocator_batch
			(const allocator_batch& other);
		allocator_batch
			(allocator_batch&& other) NOTHROW;
		//@}


		/**
		 * Assign or move batch allocator.
		 * Both operations set the @bref{other} store()
		 * as backing store() of the target allocator.
		 * Any existing allocations remain unchanged.
		 */
		//@{
		allocator_batch& operator =
			(const allocator_batch& other);
		allocator_batch& operator =
			(allocator_batch&& other) NOTHROW;
		//@}


		/** Swap two batch allocators. */
		void swap_with(allocator_batch& other) NOTHROW;


		/** Allocate @bref{size} bytes on the backing store(). */
		void* alloc_untyped(int64 size);


		/** Free a previously alloc_untyped() @bref{data} block. */
		void free(void* data) NOTHROW;


		/** Return @true if two allocators share store(). */
		bool can_free_for
			(const allocator_batch& other) const NOTHROW;


		/** Fetch the currently assigned backing store. */
		allocator_batch_store& store() const NOTHROW;


	private:

		allocator_batch_store* store_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ALLOCATOR_BATCH_HPP) */
