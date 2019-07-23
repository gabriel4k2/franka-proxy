/**
 *************************************************************************
 *
 * @file memory.cpp
 *
 * Memory helper implementation.
 *
 ************************************************************************/


#include "memory.hpp"


#if defined(VIRAL__OS_TARGET_WINDOWS)
	#include <windows.h>

	#if defined(VIRAL__BUILD_MODE_DEBUG)
		#include <crtdbg.h>
	#endif
#endif


#include <memory.h>
#include <stdlib.h>


#include "exception.hpp"
#include "integer_checks.hpp"
#include "integer_operations.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// memory
//
//////////////////////////////////////////////////////////////////////////


void* memory::alloc_untyped(int64 size)
{
	if (size < 0)
		throw invalid_parameter();

	// C malloc returns non-NULL pointer even
	// for size == 0, fix that here.
	if (size == 0) return NULL;

	size_t checked_size = 
		integer_operations::cast_or_throw<size_t>(size);

	void* array = ::malloc(checked_size);
	if (!array) alloc_fail();

	return array;
}


void* memory::realloc_untyped(void* array, int64 size)
{
	if (size < 0)
		throw invalid_parameter();

	// See above, 0 size fix.
	if (size == 0)
	{
		::free(array);
		return NULL;
	}

	size_t checked_size = 
		integer_operations::cast_or_throw<size_t>(size);

	void* new_array = ::realloc(array, checked_size);
	if (!new_array) alloc_fail();

	return new_array;
}


void memory::free(void* array) NOTHROW
	{ if (array) ::free(array); }


void memory::copy_block_untyped
	(void* dest, const void* source, int64 size) NOTHROW
{
	if (size <= 0 || !dest || !source)
		return;

	size_t checked_size = 
		integer_operations::cast_or_terminate<size_t>(size);

	::memcpy(dest, source, checked_size);
}


memory::compare_result memory::compare_blocks_untyped
	(const void* a, const void* b, int64 size) NOTHROW
{
	if (size <= 0 || !a || !b)
		return compare_result_equal;

	size_t checked_size = 
		integer_operations::cast_or_terminate<size_t>(size);

	int memcmp_result =
		::memcmp(a, b, checked_size);

	return
		(memcmp_result < 0) ? compare_result_a_before_b :
		(memcmp_result > 0) ? compare_result_b_before_a :
							  compare_result_equal;
}


bool memory::are_blocks_equal_untyped
	(const void* a, const void* b, int64 size) NOTHROW
{
	return compare_blocks_untyped(a, b, size) == compare_result_equal;
}


void memory::alloc_fail()
	{ throw out_of_memory(); }


bool memory::detect_byte_size_overflow
	(int64 count, int64 size_per_object) NOTHROW
{
	// Avoid dependency on integer_checks in header.
	return !integer_checks::is_mult_safe(count, size_per_object);
}


void memory::throw_on_byte_size_overflow
	(int64 count, int64 size_per_object)
{
	// Avoid dependency on integer_checks in header.
	if (!integer_checks::is_mult_safe(count, size_per_object))
		throw integer_overflow();
}




//////////////////////////////////////////////////////////////////////////
//
// memory, Windows-specific implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__OS_TARGET_WINDOWS)


void memory::fill_with_zero_untyped(void *memory, int64 size) NOTHROW
{
	if (size <= 0 || !memory) return;

	size_t checked_size = 
		integer_operations::cast_or_terminate<size_t>(size);

	ZeroMemory(memory, checked_size);
}


void memory::enable_debug_output()
{
	#if defined(VIRAL__BUILD_MODE_DEBUG)
		_CrtSetDbgFlag(_CrtSetDbgFlag(_CRTDBG_REPORT_FLAG) |
					   _CRTDBG_LEAK_CHECK_DF);
	#endif
}


#endif




//////////////////////////////////////////////////////////////////////////
//
// memory, Unix-specific implementation
//
//////////////////////////////////////////////////////////////////////////


#if defined(VIRAL__OS_TARGET_UNIX)


void memory::fill_with_zero_untyped(void *memory, int64 size) NOTHROW
{
	if (size <= 0 || !memory) return;

	size_t checked_size = 
		integer_operations::cast_or_terminate<size_t>(size);

	::memset(memory, 0, checked_size);
}


void memory::enable_debug_output()
	{ /** Not supported under Unix. */ }


#endif




} /* namespace viral_core */
