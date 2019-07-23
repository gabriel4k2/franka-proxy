/**
 *************************************************************************
 *
 * @file sphere_tree_scene_proximate.cpp
 *
 * Proximity tests in a sphere_tree_scene, implementation.
 *
 ************************************************************************/


#include "sphere_tree_scene_proximate.hpp"


#include "log.hpp"
#include "bounding_combiner.hpp"
#include "bounding_proximate.hpp"
#include "bounding_proximate_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_proximate_test
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_proximate_test::sphere_tree_scene_proximate_test
	(const si_transform& t_transform,
	 const sphere_tree_scene::object_iterator& t_ignore_object)
	:
	t_transform_(t_transform),
	t_to_world_matrix_(t_transform.build_local_to_world()),
	t_ignore_object_(t_ignore_object)
{ }


bool sphere_tree_scene_proximate_test::node_proximate
	(const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 const sphere_tree& t,
	 const sphere_tree::const_node_iterator& t_node,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	throw unsupported_feature();
}


bool sphere_tree_scene_proximate_test::leaf_proximate
	(const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 const sphere_tree& t,
	 const sphere_tree::const_node_iterator& t_node,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_proximate
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_proximate_result
	sphere_tree_scene_proximate::proximate
		(const sphere_tree_scene& scene,
		 const sphere_tree& t, const si_transform& t_transform,
		 float distance_squared_upper_bound)
{
	return proximate_util
				(scene, t, t_transform,
				 sphere_tree_scene::object_iterator(),
				 distance_squared_upper_bound);
}


sphere_tree_scene_proximate_result
	sphere_tree_scene_proximate::proximate
		(const sphere_tree_scene& scene,
		 const sphere_tree_scene::object_iterator& t_object,
		 float distance_squared_upper_bound)
{
	if (!t_object)
	{
		LOG_ERROR("Invalid scene object for proximity test.");
		throw invalid_parameter();
	}

	return proximate_util
				(scene,
				 t_object->sphere_tree()(),
				 t_object->transform(),
				 t_object,
				 distance_squared_upper_bound);
}


sphere_tree_scene_proximate_result
	sphere_tree_scene_proximate::proximate_util
		(const sphere_tree_scene& scene,
		 const sphere_tree& t, const si_transform& t_transform,
		 const sphere_tree_scene::object_iterator& t_ignore_object,
		 float distance_squared_upper_bound)
{
	// Put together temporary sphere_tree_scene_spheres for leaf traversal.
	// Then perform bounding_proximate test on the scene and our input tree. 
	sphere_tree_scene_spheres stss(scene);
	sphere_tree_scene_proximate_test test
		(t_transform, t_ignore_object);

	bounding_proximate_sphere_tree_scene_sphere_tree::proximate_heap heap;

	bounding_proximate_sphere_tree_scene_sphere_tree::proximate_result result
		(bounding_proximate_sphere_tree_scene_sphere_tree::proximate
			(stss, t, test, heap,
			 distance_squared_upper_bound));

	if (!result.valid)
		return sphere_tree_scene_proximate_result();


	// Assert validity of all resulting iterators,
	// then repackage into a more manageable client format.
	const sphere_tree_scene_trees_bounding::const_node_iterator&
		proximate_scene_node = result.proximate_node_a.coarse_node();

	if (!proximate_scene_node)
	{
		LOG_ERROR("Found proximity, but had invalid proximate scene node.");
		throw assert_failed();
	}

	const sphere_tree_scene::object_iterator&
		proximate_scene_object = proximate_scene_node->object();

	if (!proximate_scene_object)
	{
		LOG_ERROR("Found proximity, but had invalid proximate scene object.");
		throw assert_failed();
	}

	return sphere_tree_scene_proximate_result
				(proximate_scene_node,
				 proximate_scene_object,
				 result.proximate_node_b);
}




} /* namespace viral_core */
