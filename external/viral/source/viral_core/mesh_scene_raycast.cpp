/**
 *************************************************************************
 *
 * @file mesh_scene_raycast.cpp
 *
 * Raycasting through a mesh_scene, implementation.
 *
 ************************************************************************/


#include "mesh_scene_raycast.hpp"


#include "log.hpp"
#include "geo_raycast.hpp"
#include "bounding_tree.hpp"
#include "bounding_combiner.hpp"
#include "bounding_raycast.hpp"
#include "bounding_raycast_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_raycast_test
//
//////////////////////////////////////////////////////////////////////////


bool mesh_scene_raycast_test::try_node_raycast
	(const segment& s,
	 const mesh_scene_triangles& stris,
	 const mesh_scene_triangles_const_node_iterator& stris_node,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	// Check incoming iterator for general validity.
	// This is redundant to bounding_raycast checks,
	// but bounding_combiner_iterator is complex.
	if (!stris_node.coarse_node())
	{
		LOG_ERROR("Testing for ray intersection in invalid mesh scene node.");
		throw assert_failed();
	}


	// If we have not arrived at a mesh yet,
	// test against bounding nodes of the mesh scene.
	if (!stris_node.detail_node())
		return 
			bounding_raycast_util::try_shape_raycast
				(s, stris_node.coarse_node()->shape(),
				 hit_distance_lower_bound, evaluation_potential);


	// We are inside a mesh_scene_object already,
	// so test the bounding nodes of its mesh.
	// In contrast to mesh_scene nodes, we must
	// transform mesh nodes to the scene space.
	const mesh_scene::object_iterator& scene_object =
		stris_node.coarse_node()->object();

	if (!scene_object)
	{
		LOG_ERROR("Testing for ray-shape intersection in invalid mesh scene object.");
		throw assert_failed();
	}

	mesh_scene_mesh_bounding::shape_type shape
		(scene_object->mesh_bounding()().shape(stris_node.detail_node()));
	mesh_scene_mesh_bounding::shape_type transformed_shape
		(shape.transform_and_bound(scene_object->to_world_matrix()));

	return
		bounding_raycast_util::try_shape_raycast
			(s, transformed_shape,
			 hit_distance_lower_bound, evaluation_potential);
}


bool mesh_scene_raycast_test::try_leaf_raycast
	(const segment& s,
	 const mesh_scene_triangles& stris,
	 const mesh_scene_triangles_const_node_iterator& stris_node,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	// Fetch mesh from mesh_scene_triangles node
	// and find triangle index in this mesh.
	if (!stris_node.coarse_node() || !stris_node.detail_node())
	{
		LOG_ERROR("Testing for ray-triangle intersection in invalid or non-leaf node.");
		throw assert_failed();
	}

	const mesh_scene::object_iterator& scene_object =
		stris_node.coarse_node()->object();

	if (!scene_object)
	{
		LOG_ERROR("Testing for ray-triangle intersection in invalid mesh scene object.");
		throw assert_failed();
	}

	const mesh& m = scene_object->mesh()();
	int64 m_triangle_index =
		scene_object->mesh_bounding()().object
			(stris_node.detail_node());


	// Extract triangle from mesh, transform, and test for intersection.
	// Checked access to ensure valid bounding_tree indices.
	// Later triangle intersection anyways is much more expensive.
	const mesh_stream_vector& m_v = m.vertices();
	const mesh_stream_triangle& m_t = m.triangles();

	const matrix& to_world_matrix =
		scene_object->to_world_matrix();

	triangle m_tri
		(to_world_matrix * m_v.at(m_t.at(m_triangle_index).i),
		 to_world_matrix * m_v.at(m_t.at(m_triangle_index).j),
		 to_world_matrix * m_v.at(m_t.at(m_triangle_index).k));

	return
		geo_raycast::try_raycast
			(s, m_tri, hit_point, hit_distance);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_raycast
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_raycast_result
	mesh_scene_raycast::raycast
		(const segment& s, const mesh_scene& scene,
 		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	// Put together temporary mesh_scene_triangles for triangle traversal.
	// Then perform bounding_raycast test on the scene and our input ray. 
	mesh_scene_triangles stris(scene);
	mesh_scene_raycast_test test;	

	bounding_raycast_segment_mesh_scene::raycast_heap heap;

	bounding_raycast_segment_mesh_scene::raycast_result result
		(bounding_raycast_segment_mesh_scene::raycast
			(s, stris, test, heap,
			 hit_distance_bound, hit_distance_bound_valid,
			 hit_distance_tolerance));

	if (!result.intersecting)
		return mesh_scene_raycast_result();

	
	// Assert validity of resulting iterators and indices,
	// then repackage into a more manageable client format.
	const mesh_scene_meshes_bounding::const_node_iterator& meshes_bounding_node =
		result.hit_node.coarse_node();
	const mesh_scene_mesh_bounding::const_node_iterator& mesh_bounding_node =
		result.hit_node.detail_node();

	if (!meshes_bounding_node)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting mesh scene node.");
		throw assert_failed();
	}

	if (!mesh_bounding_node)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting mesh node.");
		throw assert_failed();
	}

	const mesh_scene::object_iterator& scene_object =
		meshes_bounding_node->object();

	if (!scene_object)
	{
		LOG_ERROR("Found intersection, but had invalid intersecting object.");
		throw assert_failed();
	}

	return mesh_scene_raycast_result
				(meshes_bounding_node,
				 scene_object,
				 scene_object->mesh_bounding()().object(mesh_bounding_node),
				 result.hit_point, result.hit_distance);
}




} /* namespace viral_core */
