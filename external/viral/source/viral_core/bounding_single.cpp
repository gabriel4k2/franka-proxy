/**
 *************************************************************************
 *
 * @file bounding_single.cpp
 *
 * Single-node bounding volume hierarchy, implementation.
 *
 ************************************************************************/


#include "bounding_single.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_single_util
//
//////////////////////////////////////////////////////////////////////////


void bounding_single_util::log_shape_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no shape."); }
void bounding_single_util::log_shape_broken_iterator() NOTHROW
	{ LOG_ERROR("Cannot get shape, iterator does not point to single node."); }

void bounding_single_util::log_object_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no geometry object."); }
void bounding_single_util::log_object_broken_iterator() NOTHROW
	{ LOG_ERROR("Cannot get geometry object, iterator does not point to single node."); }




} /* namespace viral_core */
