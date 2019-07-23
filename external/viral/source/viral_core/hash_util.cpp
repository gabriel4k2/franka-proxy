/**
 *************************************************************************
 *
 * @file hash_util.cpp
 *
 * Hash utility functions, implementation.
 *
 ************************************************************************/


#include "hash_util.hpp"


namespace viral_core
{


uint64 hash_array(const char* pointer, int64 count) NOTHROW
{
	if (count <= 0 || !pointer) return 0;

	// Based on public domain FNV hash function.
	uint64 hash = 2166136261;

	for (int64 i = 0; i < count; ++i)
		hash = (hash * 16777619) ^ pointer[i];

	return hash;
}


uint64 merge_hash(uint64 a, uint64 b) NOTHROW
	{ return (a * 524287) ^ (b * 127); }




} /* namespace viral_core */
