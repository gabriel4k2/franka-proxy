/**
 *************************************************************************
 *
 * @file thread_read_write.cpp
 *
 * Single reader-writer locks for threads, implementation.
 *
 ************************************************************************/


#include "thread_read_write.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mutex_read_write
//
//////////////////////////////////////////////////////////////////////////


mutex_read_write::mutex_read_write
	(fairness_policy policy)
	:
	policy(policy)
{ }


void mutex_read_write::acquire_read() NOTHROW { }
void mutex_read_write::acquire_write() NOTHROW { }


bool mutex_read_write::try_acquire_read() NOTHROW
	{ return false; }
bool mutex_read_write::try_acquire_write() NOTHROW
	{ return false; }


void mutex_read_write::release_read() NOTHROW { }
void mutex_read_write::release_write() NOTHROW { }




} /* namespace viral_core */
