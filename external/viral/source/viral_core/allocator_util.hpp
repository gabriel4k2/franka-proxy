/**
 *************************************************************************
 *
 * @file allocator_util.hpp
 *
 * Utilities for use with allocators.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ALLOCATOR_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__ALLOCATOR_UTIL_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "integer_operations.hpp"
#include "scope_guard.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class allocator_util
 * @ingroup allocator_module
 *
 * Static allocation and deallocation utilities,
 * including construction and destruction.
 *
 ************************************************************************/
class allocator_util :
	private non_constructable
{

	public:

		/**
		 * Allocate enough memory for @bref{count} objects
		 * of type @bref{T} through some @bref{allocator}.
		 * Objects remain uninitialized.
		 */
		template <typename T, typename A>
			static T* allocate
				(A& allocator, int64 count)
		{
			int64 byte_count =
				integer_operations::mult_or_throw
					(size_of<T>(), count);

			return static_cast<T*>
						(allocator.alloc_untyped(byte_count));
		}


		/**
		 * Allocate enough memory for an object of type @bref{T}
		 * with some @bref{allocator}, then construct the object
		 * from given @bref{parameters} and return the result.
		 */
		template <typename T, typename A, typename ... P>
			static T* allocate_and_construct
				(A& allocator, P && ... parameters)
		{
			void* memory =
				allocator.alloc_untyped(size_of<T>());
			SCOPE_GUARD
				(memory_guard,
					(allocator, &A::free, memory));

			T* object =
				new(memory) T(std::forward<P>(parameters) ...);
			
			// Success.
			// NOTHROW from here.
			memory_guard.dismiss();
			return object;
		}


		/**
		 * Destroy given, possibly @NULL @bref{object},
		 * then free its memory on @bref{allocator}.
		 */
		template <typename T, typename A>
			static void destroy_and_free
				(A& allocator, T* object) NOTHROW
		{
			if (object)
				object->~T();

			allocator.free(object);
		}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ALLOCATOR_UTIL_HPP) */
