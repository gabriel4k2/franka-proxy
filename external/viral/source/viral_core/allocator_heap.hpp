/**
 *************************************************************************
 *
 * @file allocator_heap.hpp
 *
 * Heap-based memory allocator.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ALLOCATOR_HEAP_HPP)
#define INCLUDED__VIRAL_CORE__ALLOCATOR_HEAP_HPP


#include "build_config.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup allocator_module Allocator Module
 * @ingroup container_module
 *
 * Realizes memory allocators, mainly for use in containers.
 *
 * General usage notes:
 * - Allocators fetch memory from some unspecified backing store
 *		(e.g. from the global heap or from a shared memory pool).
 *		See individual allocators for backing store requirements.
 * - To ensure correctness (e.g. no leaks) for arbitrary allocators,
 *		clients must free() allocations explicitly and exactly once,
 *		either on the originating allocator, or on an allocator that
 *		can_free_for() the originating one, notably before losing
 *		(e.g. by assigning or destroying) the last such allocator.
 * - Specific implementations may offer additional lifetime options
 *		(e.g. free()ing any leaked allocations on destruction).
 *
 * Allocation and deallocation:
 * - void* A::alloc_untyped(int64 size)
 *		must allocate @bref{size} bytes of continuous client memory
 *		and must return a not-@NULL pointer to the first client byte,
 *		or a @NULL pointer if the requested byte @bref{size} is zero.
 *		Initial values of bytes are unspecified (e.g. random values),
 *		hence clients must call constructors explicitly if required.
 *		The allocation may have padding before or after client bytes.
 *		On any error, alloc_untyped() must throw (e.g. out_of_memory).
 * - void A::free(void* data) NOTHROW
 *		must release a possibly @NULL @bref{data} that was allocated
 *		by either this allocator or one this allocator can_free_for().
 *		If required, clients must call destructors before free()ing.
 *		Due to @ref NOTHROW, free() ignores any errors, but may log.
 * - bool A::can_free_for(const A& other) const NOTHROW
 *		must return @false if the left-hand allocator cannot free()
 *		all allocations made by an@bref{other} allocator, and should,
 *		but need not, return @true otherwise.
 *
 * Propagating allocators:
 * - Allocators should be default-constructable, copy-constructable,
 *		copy-assignable, and move-assignable. All of these operations
 *		must be @ref NOTHROW. If operations are missing, related calls
 *		on containers (e.g. copy assignment) do not compile.
 * - Allocator copy-construction, copy-assignment and move-assignment
 *		must give allocators that can_free_for() the source allocator.
 *		Existing allocations of source allocators are not duplicated,
 *		and existing allocations of target allocators are not free()d.
 * - By default, operations that copy or transfer entire containers
 *		(e.g. copy-construction, move-assignment) copy the allocator
 *		of the source container to the target container.
 * - Allocators must support @ref NOTHROW swap()ping, which exchanges
 *		the allocations that either allocator can free().
 *
 * Thread safety notes:
 * - To avoid overhead, allocators need not necessarily be threadsafe. 
 *		Thread safety is documented on individual allocators.
 *
 ************************************************************************/



/**
 *************************************************************************
 *
 * @class allocator_heap
 * @ingroup allocator_module
 *
 * An allocator that stores its allocations on the global heap.
 *
 * Usage notes:
 * - Each allocator_heap employs the global heap as its backing store.
 *		Thus, any and all allocator_heap%s can_free_for() each other,
 *		and clients need not manage the lifetime of the backing store.
 * - All allocator_heap%s are threadsafe for arbitrary client threads.
 *		Multiple containers may use multiple allocator_heap instances
 *		to concurrently allocate memory.
 * - @attent Allocations through allocator_heap may be incompatible
 *		with other heap calls (e.g. C-style malloc() and free()).
 *
 ************************************************************************/
class allocator_heap
{

	public:

		/** Allocate @bref{size} bytes on the heap. */
		void* alloc_untyped(int64 size);

		/** Free a previously alloc_untyped() @bref{data} block. */
		void free(void* data) NOTHROW;


		/**
		 * Whether this allocator can free() for an@bref{other} one.
		 * Always returns @true for allocator_heap.
		 */
		bool can_free_for
			(const allocator_heap& other) const NOTHROW;


		/** Swap with another allocator. */
		void swap_with(allocator_heap& other) NOTHROW;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ALLOCATOR_HEAP_HPP) */
