/**
 *************************************************************************
 *
 * @file mulock_access.cpp
 *
 * Access to multilock results, implementation.
 *
 ************************************************************************/


#include "mulock_access.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mulock_access_util
//
//////////////////////////////////////////////////////////////////////////


void mulock_access_util::throw_data_type_mismatch()
{
	// Avoid header dependency on log.hpp
	LOG_ERROR("Access type does not match type of locked data.");
	throw type_mismatch();
}


void mulock_access_util::throw_const_access_only()
{
	LOG_ERROR("Cannot allow non-const access on read-only lock.");
	throw invalid_state();
}




} /* namespace viral_core */
