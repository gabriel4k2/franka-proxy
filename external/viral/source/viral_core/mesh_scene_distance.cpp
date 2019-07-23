/**
 *************************************************************************
 *
 * @file mesh_scene_distance.cpp
 *
 * Distances on mesh_scenes, implementation.
 *
 ************************************************************************/


#include "mesh_scene_distance.hpp"


#include "log.hpp"
#include "geo_distance.hpp"
#include "bounding_combiner.hpp"
#include "bounding_distance.hpp"
#include "bounding_distance_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_distance_metric
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_distance_metric::mesh_scene_distance_metric
	(const mesh& m, const si_transform& m_transform,
	 const mesh_scene::object_iterator& m_ignore_object)
	:
	m_(m),
	m_transform_(m_transform),
	m_to_world_matrix_(m_transform.build_local_to_world()),
	m_ignore_object_(m_ignore_object)
{ }


bool mesh_scene_distance_metric::try_node_distance_squared
	(const mesh_scene_triangles& stris,
	 const mesh_scene_triangles_const_node_iterator& stris_node,
	 const mesh_scene_mesh_bounding& m_bounding,
	 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	// Check incoming iterator for general validity.
	// This is redundant to bounding_distance checks,
	// but bounding_combiner_iterator is complex.
	if (!stris_node.coarse_node())
	{
		LOG_ERROR("Testing for shape distance in invalid mesh scene node.");
		throw assert_failed();
	}


	// If we have not arrived at a mesh yet,
	// test against bounding nodes of the mesh scene.
	if (!stris_node.detail_node())
	{

		// Skip any leaf-level node (in regards to the mesh_scene)
		// that we are supposed to ignore. Note that invalid objects
		// indicate inner mesh_scene nodes, so we must only compare
		// against valid m_ignore_object_ iterators.
		if (m_ignore_object_ &&
			stris_node.coarse_node()->object() == m_ignore_object_)
				return false;

		return
			bounding_distance_util::try_shape_distance_squared
				(stris_node.coarse_node()->shape(), matrix(),
				 m_bounding.shape(m_node), m_to_world_matrix_,
				 min_distance_squared_lower_bound, evaluation_potential);
	}


	// We are inside a mesh_scene_object already,
	// so test the bounding nodes of its mesh.
	const mesh_scene::object_iterator& scene_object =
		stris_node.coarse_node()->object();

	if (!scene_object)
	{
		LOG_ERROR("Testing for shape distance in invalid mesh scene object.");
		throw assert_failed();
	}

	return
		bounding_distance_util::try_shape_distance_squared
			(scene_object->mesh_bounding()().shape(stris_node.detail_node()),
			 scene_object->to_world_matrix(),
			 m_bounding.shape(m_node), m_to_world_matrix_,
			 min_distance_squared_lower_bound, evaluation_potential);
}


bool mesh_scene_distance_metric::try_leaf_distance_squared
	(const mesh_scene_triangles& stris,
	 const mesh_scene_triangles_const_node_iterator& stris_node,
	 const mesh_scene_mesh_bounding& m_bounding,
	 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
	 vector& stris_closest_point, vector& m_closest_point,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
{
	// Fetch mesh from mesh_scene_triangles node
	// and find triangle index in this mesh.
	if (!stris_node.coarse_node() || !stris_node.detail_node())
	{
		LOG_ERROR("Testing for triangle distance in invalid or non-leaf node.");
		throw assert_failed();
	}

	const mesh_scene::object_iterator& scene_object =
		stris_node.coarse_node()->object();

	if (!scene_object)
	{
		LOG_ERROR("Testing for triangle distance in invalid mesh scene object.");
		throw assert_failed();
	}

	const mesh& stris_m = scene_object->mesh()();
	int64 stris_triangle_index =
		scene_object->mesh_bounding()().object
			(stris_node.detail_node());
	
	
	// Extract triangles from meshes and test for distance.
	// Checked access to ensure valid bounding_tree indices.
	// Later triangle distance anyways is much more expensive.
	const mesh_stream_vector& stris_v = stris_m.vertices();
	const mesh_stream_triangle& stris_t = stris_m.triangles();

	const mesh_stream_vector& m_v = m_.vertices();
	const mesh_stream_triangle& m_t = m_.triangles();
	int64 m_triangle_index = m_bounding.object(m_node);

	triangle stris_tri
		(stris_v.at(stris_t.at(stris_triangle_index).i),
		 stris_v.at(stris_t.at(stris_triangle_index).j),
		 stris_v.at(stris_t.at(stris_triangle_index).k));
	triangle m_tri
		(m_v.at(m_t.at(m_triangle_index).i),
		 m_v.at(m_t.at(m_triangle_index).j),
		 m_v.at(m_t.at(m_triangle_index).k));

	closest_distance_squared =
		geo_distance::distance_squared
			(stris_tri, scene_object->to_world_matrix(),
			 m_tri, m_to_world_matrix_,
			 stris_closest_point, m_closest_point);

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_distance
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_distance_result
	mesh_scene_distance::distance
		(const mesh_scene& scene,
		 const mesh& m, const si_transform& m_transform,
		 const mesh_scene_mesh_bounding& m_bounding,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	return distance_util
				(scene, m, m_transform, m_bounding,
				 mesh_scene::object_iterator(),
				 distance_squared_bound,
				 distance_squared_bound_valid,
				 distance_squared_tolerance);
}


mesh_scene_distance_result
	mesh_scene_distance::distance
		(const mesh_scene& scene,
		 const mesh_scene::object_iterator& m_object,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	if (!m_object)
	{
		LOG_ERROR("Invalid scene object for distance test.");
		throw invalid_parameter();
	}

	return distance_util
				(scene, m_object->mesh()(),
				 m_object->transform(), m_object->mesh_bounding()(),
				 m_object,
				 distance_squared_bound,
				 distance_squared_bound_valid,
				 distance_squared_tolerance);
}


mesh_scene_distance_result
	mesh_scene_distance::distance_util
		(const mesh_scene& scene,
		 const mesh& m, const si_transform& m_transform,
		 const mesh_scene_mesh_bounding& m_bounding,
		 const mesh_scene::object_iterator& m_ignore_object,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	// Put together temporary mesh_scene_triangles for triangle traversal.
	// Then perform bounding_distance test on the scene and our input mesh. 
	mesh_scene_triangles stris(scene);
	mesh_scene_distance_metric metric
		(m, m_transform, m_ignore_object);

	bounding_distance_mesh_scene_mesh::distance_heap heap;

	bounding_distance_mesh_scene_mesh::distance_result result
		(bounding_distance_mesh_scene_mesh::distance
			(stris, m_bounding, metric, heap,
			 distance_squared_bound, distance_squared_bound_valid,
			 distance_squared_tolerance));

	if (!result.valid)
		return mesh_scene_distance_result();


	// Assert validity of resulting iterators and indices,
	// then repackage into a more manageable client format.
	const mesh_scene_meshes_bounding::const_node_iterator& meshes_bounding_node =
		result.closest_node_a.coarse_node();
	const mesh_scene_mesh_bounding::const_node_iterator& mesh_bounding_node =
		result.closest_node_a.detail_node();

	if (!meshes_bounding_node)
	{
		LOG_ERROR("Found distance, but had invalid closest mesh scene node.");
		throw assert_failed();
	}

	if (!mesh_bounding_node)
	{
		LOG_ERROR("Found distance, but had invalid closest mesh node.");
		throw assert_failed();
	}

	const mesh_scene::object_iterator& scene_object =
		meshes_bounding_node->object();

	if (!scene_object)
	{
		LOG_ERROR("Found distance, but had invalid closest object.");
		throw assert_failed();
	}

	return mesh_scene_distance_result
				(meshes_bounding_node,
				 scene_object,
				 mesh_bounding_node,
				 scene_object->mesh_bounding()().object(mesh_bounding_node),
				 result.closest_node_b,
				 m_bounding.object(result.closest_node_b),
				 result.closest_point_a,
				 result.closest_point_b,
				 result.closest_distance_squared);
}




} /* namespace viral_core */
