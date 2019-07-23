/**
 *************************************************************************
 *
 * @file bounding_tree.cpp
 *
 * Bounding volume hierarchy with explicit geometry, implementation.
 *
 ************************************************************************/


#include "bounding_tree.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_tree_util
//
//////////////////////////////////////////////////////////////////////////


void bounding_tree_util::log_shape_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no shape."); }
void bounding_tree_util::log_shape_broken_iterator() NOTHROW
	{ LOG_ERROR("Cannot get shape, iterator does not point to valid node."); }

void bounding_tree_util::log_object_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no geometry object."); }
void bounding_tree_util::log_object_broken_iterator() NOTHROW
	{ LOG_ERROR("Cannot get geometry object, iterator does not point to valid node."); }
void bounding_tree_util::log_object_broken_index() NOTHROW
	{ LOG_ERROR("Cannot get geometry object, object index is invalid."); }

void bounding_tree_util::log_validation_failed(const char* error) NOTHROW
	{ LOG_ERROR("Validation of bounding_tree failed, detailed error: " + error); }




} /* namespace viral_core */

