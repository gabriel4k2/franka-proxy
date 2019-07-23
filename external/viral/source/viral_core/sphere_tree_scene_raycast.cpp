/**
 *************************************************************************
 *
 * @file sphere_tree_scene_raycast.cpp
 *
 * Raycasting through sphere_tree_scene%s, implementation.
 *
 ************************************************************************/


#include "sphere_tree_scene_raycast.hpp"


#include "log.hpp"
#include "bounding_combiner.hpp"
#include "bounding_raycast.hpp"
#include "bounding_raycast_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_raycast_test
//
//////////////////////////////////////////////////////////////////////////


bool sphere_tree_scene_raycast_test::try_node_raycast
	(const segment& s,
	 const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	throw unsupported_feature();
}


bool sphere_tree_scene_raycast_test::try_leaf_raycast
	(const segment& s,
	 const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_raycast
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_raycast_result
	sphere_tree_scene_raycast::raycast
		(const segment& s, const sphere_tree_scene& scene,
	 	 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	// Put together temporary sphere_tree_scene_spheres for leaf traversal.
	// Then perform bounding_raycast test on the scene and our input ray. 
	sphere_tree_scene_spheres stss(scene);
	sphere_tree_scene_raycast_test test;	

	bounding_raycast_segment_sphere_tree_scene::raycast_heap heap;

	bounding_raycast_segment_sphere_tree_scene::raycast_result result
		(bounding_raycast_segment_sphere_tree_scene::raycast
			(s, stss, test, heap,
			 hit_distance_bound, hit_distance_bound_valid,
			 hit_distance_tolerance));

	if (!result.intersecting)
		return sphere_tree_scene_raycast_result();

	
	// Assert validity of resulting iterators and indices,
	// then repackage into a more manageable client format.
	const sphere_tree_scene_trees_bounding::const_node_iterator&
		hit_scene_node = result.hit_node.coarse_node();

	if (!hit_scene_node)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting scene node.");
		throw assert_failed();
	}

	const sphere_tree_scene::object_iterator&
		hit_scene_object = hit_scene_node->object();

	if (!hit_scene_object)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting scene object.");
		throw assert_failed();
	}

	return sphere_tree_scene_raycast_result
				(hit_scene_node,
				 hit_scene_object,
				 result.hit_point, result.hit_distance);
}




} /* namespace viral_core */
