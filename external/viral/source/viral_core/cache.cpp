/**
 *************************************************************************
 *
 * @file cache.cpp
 *
 * Name-to-object bindings, implementation.
 *
 ************************************************************************/


#include "cache.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// cache_errors
//
//////////////////////////////////////////////////////////////////////////


void cache_errors::log_and_throw_registration_failed
	(const string& cache_name, const string& name)
{
	LOG_ERROR("Cache: " + cache_name + " already contains an entry for: " + name);
	throw object_registration_failed();
}

void cache_errors::log_and_throw_not_found
	(const string& cache_name, const string& name)
{
	LOG_ERROR("Cache: " + cache_name + " has no entry for: " + name);
	throw not_found();
}

void cache_errors::log_and_throw_null_pointer
	(const string& cache_name, const string& name)
{
	LOG_ERROR("Cache: " + cache_name + " stores NULL entry for: " + name);
	throw null_pointer();
}

void cache_errors::log_and_throw_type_mismatch
	(const string& cache_name, const string& name)
{
	LOG_ERROR("Cache: " + cache_name + " holds mismatching type for entry: " + name);
	throw type_mismatch();
}




} /* namespace viral_core */
