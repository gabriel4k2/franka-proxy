/**
 *************************************************************************
 *
 * @file sphere_tree_scene_intersect.cpp
 *
 * Intersection tests in a sphere_tree_scene, implementation.
 *
 ************************************************************************/


#include "sphere_tree_scene_intersect.hpp"


#include "log.hpp"
#include "bounding_combiner.hpp"
#include "bounding_intersect.hpp"
#include "bounding_intersect_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_intersect_test
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_intersect_test::sphere_tree_scene_intersect_test
	(const si_transform& t_transform,
	 const sphere_tree_scene::object_iterator& t_ignore_object)
	:
	t_transform_(t_transform),
	t_to_world_matrix_(t_transform.build_local_to_world()),
	t_ignore_object_(t_ignore_object)
{ }


bool sphere_tree_scene_intersect_test::node_intersecting
	(const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 const sphere_tree& t,
	 const sphere_tree::const_node_iterator& t_node,
	 float& evaluation_potential) const
{
	throw unsupported_feature();
}


bool sphere_tree_scene_intersect_test::leaf_intersecting
	(const sphere_tree_scene_spheres& stss,
	 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
	 const sphere_tree& t,
	 const sphere_tree::const_node_iterator& t_node,
	 bounding_intersect_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_intersect
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene_intersect_result
	sphere_tree_scene_intersect::intersect
		(const sphere_tree_scene& scene,
		 const sphere_tree& t, const si_transform& t_transform)
{
	return intersect_util
				(scene, t, t_transform,
				 sphere_tree_scene::object_iterator());
}


sphere_tree_scene_intersect_result
	sphere_tree_scene_intersect::intersect
		(const sphere_tree_scene& scene,
		 const sphere_tree_scene::object_iterator& t_object)
{
	if (!t_object)
	{
		LOG_ERROR("Invalid scene object for intersection test.");
		throw invalid_parameter();
	}

	return intersect_util
				(scene, 
				 t_object->sphere_tree()(),
				 t_object->transform(),
				 t_object);
}


sphere_tree_scene_intersect_result
	sphere_tree_scene_intersect::intersect_util
		(const sphere_tree_scene& scene,
		 const sphere_tree& t, const si_transform& t_transform,
		 const sphere_tree_scene::object_iterator& t_ignore_object)
{
	// Put together temporary sphere_tree_scene_spheres for leaf traversal.
	// Then perform bounding_intersect test on the scene and our input tree. 
	sphere_tree_scene_spheres stss(scene);
	sphere_tree_scene_intersect_test test
		(t_transform, t_ignore_object);

	bounding_intersect_sphere_tree_scene_sphere_tree::intersect_heap heap;

	bounding_intersect_sphere_tree_scene_sphere_tree::intersect_result result
		(bounding_intersect_sphere_tree_scene_sphere_tree::intersect
			(stss, t, test, heap));

	if (!result.intersecting)
		return sphere_tree_scene_intersect_result();


	// Assert validity of resulting iterators and indices,
	// then repackage into a more manageable client format.
	const sphere_tree_scene_trees_bounding::const_node_iterator&
		intersecting_scene_node = result.intersecting_node_a.coarse_node();

	if (!intersecting_scene_node)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting scene node.");
		throw assert_failed();
	}

	const sphere_tree_scene::object_iterator&
		intersecting_scene_object = intersecting_scene_node->object();

	if (!intersecting_scene_object)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting scene object.");
		throw assert_failed();
	}

	return sphere_tree_scene_intersect_result
				(intersecting_scene_node,
				 intersecting_scene_object,
				 result.intersecting_node_b);
}




} /* namespace viral_core */
