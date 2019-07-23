/**
 *************************************************************************
 *
 * @file sphere_tree_scene_distance.cpp
 *
 * Distances in a sphere_tree_scene, implementation.
 *
 ************************************************************************/


#include "sphere_tree_scene_distance.hpp"


#include "log.hpp"
#include "bounding_combiner.hpp"
#include "bounding_distance.hpp"
#include "bounding_distance_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_distance_metric
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_distance_metric::sphere_tree_scene_distance_metric
	(const si_transform& t_transform,
	 const sphere_tree_scene::object_iterator& t_ignore_object)
	:
	t_transform_(t_transform),
	t_to_world_matrix_(t_transform.build_local_to_world()),
	t_ignore_object_(t_ignore_object)
{ }


bool sphere_tree_scene_distance_metric::try_node_distance_squared
	(const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 const sphere_tree& t,
	 const sphere_tree::const_node_iterator& t_node,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	throw unsupported_feature();
}


bool sphere_tree_scene_distance_metric::try_leaf_distance_squared
	(const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 const sphere_tree& t,
	 const sphere_tree::const_node_iterator& t_node,
	 vector& stss_closest_point, vector& t_closest_point,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_distance
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_distance_result
	sphere_tree_scene_distance::distance
		(const sphere_tree_scene& scene,
		 const sphere_tree& t, const si_transform& t_transform,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	return distance_util
				(scene, t, t_transform,
				 sphere_tree_scene::object_iterator(),
				 distance_squared_bound,
				 distance_squared_bound_valid,
				 distance_squared_tolerance);
}


sphere_tree_scene_distance_result
	sphere_tree_scene_distance::distance
		(const sphere_tree_scene& scene,
		 const sphere_tree_scene::object_iterator& t_object,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	if (!t_object)
	{
		LOG_ERROR("Invalid scene object for distance test.");
		throw invalid_parameter();
	}

	return distance_util
				(scene,
				 t_object->sphere_tree()(),
				 t_object->transform(),
				 t_object,
				 distance_squared_bound,
				 distance_squared_bound_valid,
				 distance_squared_tolerance);
}


sphere_tree_scene_distance_result
	sphere_tree_scene_distance::distance_util
		(const sphere_tree_scene& scene,
		 const sphere_tree& t, const si_transform& t_transform,
		 const sphere_tree_scene::object_iterator& t_ignore_object,
	 	 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	// Put together temporary sphere_tree_scene_spheres for leaf traversal.
	// Then perform bounding_distance test on the scene and our input tree. 
	sphere_tree_scene_spheres stss(scene);
	sphere_tree_scene_distance_metric metric
		(t_transform, t_ignore_object);

	bounding_distance_sphere_tree_scene_sphere_tree::distance_heap heap;

	bounding_distance_sphere_tree_scene_sphere_tree::distance_result result
		(bounding_distance_sphere_tree_scene_sphere_tree::distance
			(stss, t, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance));

	if (!result.valid)
		return sphere_tree_scene_distance_result();


	// Assert validity of all resulting iterators,
	// then repackage into a more manageable client format.
	const sphere_tree_scene_trees_bounding::const_node_iterator&
		closest_scene_node = result.closest_node_a.coarse_node();

	if (!closest_scene_node)
	{
		LOG_ERROR("Found distance, but had invalid closest scene node.");
		throw assert_failed();
	}

	const sphere_tree_scene::object_iterator&
		closest_scene_object = closest_scene_node->object();

	if (!closest_scene_object)
	{
		LOG_ERROR("Found distance, but had invalid closest scene object.");
		throw assert_failed();
	}

	return sphere_tree_scene_distance_result
				(closest_scene_node,
				 closest_scene_object,
				 result.closest_node_b,
				 result.closest_point_a,
				 result.closest_point_b,
				 result.closest_distance_squared);
}




} /* namespace viral_core */

