/**
 *************************************************************************
 *
 * @file container_limits.hpp
 *
 * Limits for container classes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__CONTAINER_LIMITS_HPP)
#define INCLUDED__VIRAL_CORE__CONTAINER_LIMITS_HPP


#include "build_config.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class container_limits
 * @ingroup container_module
 *
 * Generic limits for a container that stores entries of type @bref{T}.
 *
 * Container byte and size limits:
 * - All containers enforce @bref{max_bytes} and @bref{max_size} limits
 *		as configured by container_limits (OS limits notwithstanding).
 * - Clients can always use int64 offsets relative to the first entry
 *		to address a past-end element in a @bref{max_size}d container.
 * - Clients can always use int64 offsets relative to the first byte
 *		of a container to address all bytes of the past-end element.
 * - Clients may use negative int64 offsets relative to the first entry 
 *		as special values (e.g. to indicate invalid objects). 
 * - Apart from the above, clients must not make any assumptions about
 *		available bytes, sizes, or valid index ranges. For robustness,
 *		clients should explicitly check index arithmetic for overflow
 *		(e.g. with the integer_checks or integer_operations utilities).
 *
 ************************************************************************/
template <typename T>
	class container_limits
{

	public:

		/**
		 * Maximum number of bytes allowed in a container.
		 * See container_limits documentation for semantics.
		 *
		 * Usage notes:
		 * - Byte limits only consider the size of actual entries,
		 *		not of auxiliary variables (e.g. list pointers).
		 * - Byte limits always are a multiple of @bref{max_size},
		 *		but otherwise may vary per container.
		 */
		static const int64 max_bytes =
			(8589934592 / sizeof(T)) * sizeof(T); // Clamp to multiple of size.


		/**
		 * Maximum number of entries allowed in a container.
		 * See container_limits documentation for semantics.
		 */
		static const int64 max_size = max_bytes / sizeof(T);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__CONTAINER_LIMITS_HPP) */
