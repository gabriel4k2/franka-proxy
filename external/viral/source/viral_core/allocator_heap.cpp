/**
 *************************************************************************
 *
 * @file allocator_heap.cpp
 *
 * Heap-based memory allocator, implementation.
 *
 ************************************************************************/


#include "allocator_heap.hpp"


#include "memory.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// allocator_heap
//
//////////////////////////////////////////////////////////////////////////


void* allocator_heap::alloc_untyped(int64 size)
	{ return memory::alloc_untyped(size); }

void allocator_heap::free(void* data) NOTHROW
	{ memory::free(data); }

bool allocator_heap::can_free_for
	(const allocator_heap& other) const NOTHROW
		{ return true; }


void allocator_heap::swap_with
	(allocator_heap& other) NOTHROW
{
	// No state to swap for allocator_heap.
}




} /* namespace viral_core */
