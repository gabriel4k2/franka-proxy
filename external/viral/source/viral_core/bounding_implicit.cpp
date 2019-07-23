/**
 *************************************************************************
 *
 * @file bounding_implicit.cpp
 *
 * Bounding volume hierarchy with implicit geometry, implementation.
 *
 ************************************************************************/


#include "bounding_implicit.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_util
//
//////////////////////////////////////////////////////////////////////////


void bounding_implicit_util::log_shape_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no shape."); }
void bounding_implicit_util::log_shape_broken_iterator() NOTHROW
	{ LOG_ERROR("Cannot get shape, iterator does not point to valid node."); }

void bounding_implicit_util::log_validation_failed(const char* error) NOTHROW
	{ LOG_ERROR("Validation of bounding_implicit failed, detailed error: " + error); }




} /* namespace viral_core */
