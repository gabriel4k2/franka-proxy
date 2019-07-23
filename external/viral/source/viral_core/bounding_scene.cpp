/**
 *************************************************************************
 *
 * @file bounding_scene.cpp
 *
 * Bounding volumes for large-scale scenes, implementation.
 *
 ************************************************************************/


#include "bounding_scene.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_scene_util
//
//////////////////////////////////////////////////////////////////////////


void bounding_scene_util::log_update_past_end() NOTHROW
	{ LOG_ERROR("Ignoring update on past-end iterator."); }
void bounding_scene_util::log_update_no_node() NOTHROW
	{ LOG_ERROR("Ignoring update on iterator without node."); }

void bounding_scene_util::log_remove_past_end() NOTHROW
	{ LOG_ERROR("Ignoring remove on past-end iterator."); }
void bounding_scene_util::log_remove_no_node() NOTHROW
	{ LOG_ERROR("Ignoring remove on iterator without node."); }

void bounding_scene_util::log_shape_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no shape."); }
void bounding_scene_util::log_object_past_end() NOTHROW
	{ LOG_ERROR("Past-end iterator has no geometry object."); }
void bounding_scene_util::log_object_not_leaf() NOTHROW
	{ LOG_ERROR("Inner nodes do not have a geometry object."); }

void bounding_scene_util::log_validation_failed(const char* error) NOTHROW
	{ LOG_ERROR("Validation of bounding_scene failed, detailed error: " + error); }




} /* namespace viral_core */
