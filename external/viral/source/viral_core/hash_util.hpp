/**
 *************************************************************************
 *
 * @file hash_util.hpp
 *
 * Hash utility functions.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__HASH_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__HASH_UTIL_HPP


#include "build_config.hpp"


namespace viral_core
{


/**
 * Converts an intrinsic @bref{value}
 * into an unsigned integer hashcode.
 *
 * @ingroup container_module
 */
//@{
inline uint64 hash(char value) NOTHROW
	{ return static_cast<uint64>(value) * 524287 ^
			 static_cast<uint64>(value); }
inline uint64 hash(int32 value) NOTHROW
	{ return static_cast<uint64>(value) * 131071 ^
			 static_cast<uint64>(value); }
inline uint64 hash(int64 value) NOTHROW
	{ return static_cast<uint64>(value) * 131071 ^
			 static_cast<uint64>(value); }

inline uint64 hash(float value) NOTHROW
{
	// Reinterpret float bit pattern as integer.
	// Assumes sizeof(uint32) == sizeof(float).
	return *reinterpret_cast<uint32*>(&value) * 131071 ^
		   *reinterpret_cast<uint32*>(&value);
}

inline uint64 hash(void *value) NOTHROW
{
	// Reinterpret pointer bit pattern as integer.
	return static_cast<uint64>
		(*reinterpret_cast<pointer_int*>(&value) * 131071 ^
		 *reinterpret_cast<pointer_int*>(&value));
}
//@}




/**
 * Converts a provided @bref{object}
 * into an unsigned integer hashcode.
 *
 * To support the global hash() utility,
 * objects of type @bref{T} must provide
 * a hashing member method:
 * @code
 *     uint64 T::hash() const NOTHROW { return hash_code_; }
 * @endcode
 * 
 * @attent
 * Hash code calculation must be const
 * and must not throw.
 *
 * @ingroup container_module
 */	
template <typename T>
	uint64 hash(const T& object) NOTHROW
		{ return object.hash(); }




/**
 * Hash @bref{count} bytes with unknown structure
 * at the indicated @bref{pointer}.
 *
 * If @bref{pointer} is @NULL or if @bref{count} is zero or less,
 * then hash_array() always returns a hash code of @const{0}
 * without accessing the @bref{pointer}.
 *
 * @ingroup container_module
 */
uint64 hash_array(const char *pointer, int64 count) NOTHROW;




/**
 * Merge two hash codes to another hash code.
 * Use this, for instance, as a @ref NOTHROW alternative
 * to calculating a hash on concattenated strings.
 * 
 * @ingroup container_module
 */
uint64 merge_hash(uint64 a, uint64 b) NOTHROW;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__HASH_UTIL_HPP) */
