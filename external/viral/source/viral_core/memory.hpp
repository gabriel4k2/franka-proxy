/**
 *************************************************************************
 *
 * @file memory.hpp
 *
 * Header for memory utility methods.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MEMORY_HPP)
#define INCLUDED__VIRAL_CORE__MEMORY_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class memory
 * @ingroup container_module
 *
 * Low-level memory helpers.
 *
 * General usage notes:
 * - Allocation and reallocation throw with strong exception safety,
 *		other calls silently ignore errors for @ref NOTHROW behaviour.
 * - @attent For a more reliable overflow handling on byte @bref{sizes}
 *		and for increased robustness (e.g. wrong or forgotten sizeof()),
 *		clients should prefer typed variants over byte-wise variants.
 *
 * Low-level memory allocation:
 * - @attent Memory calls are not completely compatible or exchangeable
 *		with C-style malloc / realloc / free or with C++ new and delete.
 * - In difference to C-style alloc / realloc, there are no empty blocks.
 *		Any zero-sized allocations return a @NULL pointer instead.
 * - Allocations from different heaps are not compatible to each other
 *		(e.g. when statically linking to the CRT and allocating memory
 *		in two separately loaded modules).
 *
 ************************************************************************/
class memory :
	private non_constructable
{

	public:

		/**
		 * Allocate, reallocate, and free byte-@bref{size}d memory blocks,
		 * or arrays of @bref{count} objects of type @bref{T}.
		 *
		 * Usage notes:
		 * - Zero-@bref{size}d (re-)allocations return @NULL pointers.
		 * - An input @NULL pointer to reallocation may create a new array.
		 * - Freeing a @NULL pointer is silently ignored for @ref NOTHROW.
		 * - See @ref memory class documentation for more generic notes.
		 */
		//@{
		static void* alloc_untyped(int64 size);

		template <typename T>
			static T* alloc_typed(int64 count)
		{
			throw_on_byte_size_overflow(count, size_of<T>());
			return reinterpret_cast<T*>
						(alloc_untyped(count * size_of<T>()));
		}


		static void* realloc_untyped(void* array, int64 size);

		template <typename T>
			static T* realloc_typed(T* array, int64 count)
		{
			throw_on_byte_size_overflow(count, size_of<T>());
			return reinterpret_cast<T*>
						(realloc_untyped(array, count * size_of<T>()));
		}


		static void free(void* array) NOTHROW;
		//@}


		/**
		 * Byte-wise clear or copy of byte @bref{size}d memory blocks,
		 * or of arrays with @bref{count} objects of type @bref{T}.
		 * Silently ignores invalid parameters for @ref NOTHROW.
		 */
		//@{
		static void fill_with_zero_untyped
			(void* array, int64 size) NOTHROW;

		template <typename T>
			static void fill_with_zero_typed
				(T* array, int64 count) NOTHROW
		{
			if (detect_byte_size_overflow(count, size_of<T>())) return;
			fill_with_zero_untyped(array, count * size_of<T>());
		}


		static void copy_block_untyped
			(void* dest, const void* source, int64 size) NOTHROW;

		template <typename T>
			static void copy_block_typed
				(T* dest, const T* source, int64 count) NOTHROW
		{
			if (detect_byte_size_overflow(count, size_of<T>())) return;
			copy_block_untyped(dest, source, count * size_of<T>());
		}
		//@}


		/**
		 * Byte-wise comparison (as in memcmp) or equals test
		 * for two not-@NULL blocks of given byte @bref{size},
		 * or for @bref{count} objects of type @bref{T}.
		 *
		 * On a @bref{size} of 0 or on invalid parameters,
		 * the input blocks are silently ignored and always
		 * compare as equal to one another.
		 */
		//@{	
		enum compare_result
		{
			compare_result_equal,
			compare_result_a_before_b,
			compare_result_b_before_a
		};


		static compare_result compare_blocks_untyped
			(const void* a, const void* b, int64 size) NOTHROW;

		template <typename T>
			static compare_result compare_blocks_typed
				(const T* a, const T* b, int64 count) NOTHROW
		{
			if (detect_byte_size_overflow(count, size_of<T>()))
				return compare_result_equal;

			return compare_blocks_untyped(a, b, count * size_of<T>());
		}


		static bool are_blocks_equal_untyped
			(const void* a, const void* b, int64 size) NOTHROW;

		template <typename T>
			static bool are_blocks_equal_typed
				(const T* a, const T* b, int64 count) NOTHROW
		{
			if (detect_byte_size_overflow(count, size_of<T>()))
				return true;

			return are_blocks_equal_untyped(a, b, count * size_of<T>());
		}
		//@}


		/**
		 * For debug builds, enable the display of memory leak
		 * information on the console window on shutdown.
		 */
		static void enable_debug_output();


	private:

		static void alloc_fail();

		static bool detect_byte_size_overflow
			(int64 count, int64 size_per_object) NOTHROW;
		static void throw_on_byte_size_overflow
			(int64 count, int64 size_per_object);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MEMORY_HPP) */
