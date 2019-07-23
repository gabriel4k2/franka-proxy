/**
 *************************************************************************
 *
 * @file mesh_scene_proximate.cpp
 * 
 * Proximity tests on meshes in a mesh_scene, implementation.
 *
 ************************************************************************/


#include "mesh_scene_proximate.hpp"


#include "log.hpp"
#include "geo_distance.hpp"
#include "bounding_combiner.hpp"
#include "bounding_proximate.hpp"
#include "bounding_proximate_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_proximate_test
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_proximate_test::mesh_scene_proximate_test
	(const mesh& m, const si_transform& m_transform,
	 const mesh_scene::object_iterator& m_ignore_object)
	:
	m_(m),
	m_transform_(m_transform),
	m_to_world_matrix_(m_transform.build_local_to_world()),
	m_ignore_object_(m_ignore_object)
{ }


bool mesh_scene_proximate_test::node_proximate
	(const mesh_scene_triangles& stris,
	 const mesh_scene_triangles_const_node_iterator& stris_node,
	 const mesh_scene_mesh_bounding& m_bounding,
	 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	// Check incoming iterator for general validity.
	// This is redundant to bounding_proximate checks,
	// but bounding_combiner_iterator is complex.
	if (!stris_node.coarse_node())
	{
		LOG_ERROR("Testing for shape proximity in invalid mesh scene node.");
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

		return bounding_proximate_util::shape_proximate
			(stris_node.coarse_node()->shape(), matrix(),
			 m_bounding.shape(m_node), m_to_world_matrix_,
			 distance_squared_upper_bound, evaluation_potential);
	}


	// We are inside a mesh_scene_object already,
	// so test the bounding nodes of its mesh.
	const mesh_scene::object_iterator& scene_object =
		stris_node.coarse_node()->object();

	if (!scene_object)
	{
		LOG_ERROR("Testing for shape proximity in invalid mesh scene object.");
		throw assert_failed();
	}

	return bounding_proximate_util::shape_proximate
		(scene_object->mesh_bounding()().shape(stris_node.detail_node()),
		 scene_object->to_world_matrix(),
		 m_bounding.shape(m_node), m_to_world_matrix_,
		 distance_squared_upper_bound, evaluation_potential);
}

	
bool mesh_scene_proximate_test::leaf_proximate
	(const mesh_scene_triangles& stris,
	 const mesh_scene_triangles_const_node_iterator& stris_node,
	 const mesh_scene_mesh_bounding& m_bounding,
	 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	// Fetch mesh from mesh_scene_triangles node
	// and find triangle index in this mesh.
	if (!stris_node.coarse_node() || !stris_node.detail_node())
	{
		LOG_ERROR("Testing for triangle proximity in invalid or non-leaf node.");
		throw assert_failed();
	}

	const mesh_scene::object_iterator& scene_object =
		stris_node.coarse_node()->object();

	if (!scene_object)
	{
		LOG_ERROR("Testing for triangle proximity in invalid mesh scene object.");
		throw assert_failed();
	}

	const mesh& stris_m = scene_object->mesh()();
	int64 stris_triangle_index =
		scene_object->mesh_bounding()().object
			(stris_node.detail_node());
	
	
	// Extract triangles from meshes and test for proximity.
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


	// We must find a correct (expensive) distance
	// to determine proximate geometry objects.
	vector stris_closest_point;
	vector m_closest_point;
	float closest_distance_squared =
		geo_distance::distance_squared
			(stris_tri, scene_object->to_world_matrix(),
			 m_tri, m_to_world_matrix_,
			 stris_closest_point, m_closest_point);


	// We return an exact distance_squared_upper_bound
	// as proximate, consistent to node_proximate_util().
	return closest_distance_squared <= distance_squared_upper_bound;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_proximate
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_proximate_result
	mesh_scene_proximate::proximate
		(const mesh_scene& scene,
		 const mesh& m, const si_transform& m_transform,
		 const mesh_scene_mesh_bounding& m_bounding,
		 float distance_squared_upper_bound)
{
	return proximate_util
				(scene, m, m_transform, m_bounding,
				 mesh_scene::object_iterator(),
				 distance_squared_upper_bound);
}


mesh_scene_proximate_result
	mesh_scene_proximate::proximate
		(const mesh_scene& scene,
		 const mesh_scene::object_iterator& m_object,
		 float distance_squared_upper_bound)
{
	if (!m_object)
	{
		LOG_ERROR("Invalid scene object for proximity test.");
		throw invalid_parameter();
	}

	return proximate_util
				(scene, m_object->mesh()(),
				 m_object->transform(), m_object->mesh_bounding()(),
				 m_object,
				 distance_squared_upper_bound);
}


mesh_scene_proximate_result
	mesh_scene_proximate::proximate_util
		(const mesh_scene& scene,
		 const mesh& m, const si_transform& m_transform,
		 const mesh_scene_mesh_bounding& m_bounding,
		 const mesh_scene::object_iterator& m_ignore_object,
		 float distance_squared_upper_bound)
{
	// Put together temporary mesh_scene_triangles for triangle traversal.
	// Then perform bounding_proximate test on the scene and our input mesh. 
	mesh_scene_triangles stris(scene);
	mesh_scene_proximate_test test
		(m, m_transform, m_ignore_object);

	bounding_proximate_mesh_scene_mesh::proximate_heap heap;

	bounding_proximate_mesh_scene_mesh::proximate_result result
		(bounding_proximate_mesh_scene_mesh::proximate
			(stris, m_bounding, test, heap,
			 distance_squared_upper_bound));

	if (!result.valid)
		return mesh_scene_proximate_result();


	// Assert validity of resulting iterators and indices,
	// then repackage into a more manageable client format.
	const mesh_scene_meshes_bounding::const_node_iterator& meshes_bounding_node =
		result.proximate_node_a.coarse_node();
	const mesh_scene_mesh_bounding::const_node_iterator& mesh_bounding_node =
		result.proximate_node_a.detail_node();

	if (!meshes_bounding_node)
	{
		LOG_ERROR("Found proximity, but had invalid proximate mesh scene node.");
		throw assert_failed();
	}

	if (!mesh_bounding_node)
	{
		LOG_ERROR("Found proximity, but had invalid proximate mesh node.");
		throw assert_failed();
	}

	const mesh_scene::object_iterator& scene_object =
		meshes_bounding_node->object();

	if (!scene_object)
	{
		LOG_ERROR("Found proximity, but had invalid proximate object.");
		throw assert_failed();
	}

	return mesh_scene_proximate_result
				(meshes_bounding_node,
				 scene_object,
				 scene_object->mesh_bounding()().object(mesh_bounding_node),
				 m_bounding.object(result.proximate_node_b));
}




} /* namespace viral_core */

