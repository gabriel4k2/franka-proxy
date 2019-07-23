/**
 *************************************************************************
 *
 * @file mesh_scene_k_nearest.cpp
 *
 * Nearest neighbors on a mesh_scene, implementation.
 *
 ************************************************************************/


#include "mesh_scene_k_nearest.hpp"


#include "log.hpp"
#include "bounding_k_nearest.hpp"
#include "bounding_k_nearest_util.hpp"
#include "mesh.hpp"
#include "mesh_k_nearest.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_k_nearest_metric
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_k_nearest_metric::mesh_scene_k_nearest_metric()
	{ }


bool mesh_scene_k_nearest_metric::try_node_distance_squared
	(const mesh_scene& scene,
	 const mesh_scene::const_node_iterator& node,
	 const vector& point,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	// Check for distance to boundings over meshes.
	return
		bounding_k_nearest_util::try_shape_distance_squared
			(node->shape(), point,
			 min_distance_squared_lower_bound, evaluation_potential);
}

	
bool mesh_scene_k_nearest_metric::try_leaf_distance_squared
	(const mesh_scene& scene,
	 const mesh_scene::const_node_iterator& node,
	 const vector& point,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	// Arrived at a leaf mesh.
	// We do not want to separately return all mesh triangles,
	// but instead just check for the nearest triangle neighbor.
	// Keep variable mesh_scene_mesh_bounding types by using
	// a dependent return type.
	bounding_k_nearest_result
		<mesh_scene_mesh_bounding, bounding_k_nearest_no_extended_result>
			nearest_mesh_triangle
				(mesh_k_nearest::k_nearest
					(node->object()->mesh_bounding()(),
					 node->object()->mesh()(),
					 node->object()->transform(),
					 point, 1, 0.f, false, 0.f));

	if (nearest_mesh_triangle.entries.empty())
		return false;

	closest_distance_squared =
		nearest_mesh_triangle.entries[0].closest_distance_squared;

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_k_nearest
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_k_nearest_result
	mesh_scene_k_nearest::k_nearest
		(const mesh_scene& scene,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	// Perform bounding_k_nearest query on the scene and our point. 
	// No mesh_scene_triangles, we do not want to return individual triangles.
	mesh_scene_k_nearest_metric metric;

	bounding_k_nearest_mesh_scene_vector::k_nearest_node_heap node_heap;
	bounding_k_nearest_mesh_scene_vector::k_nearest_neighbor_heap neighbor_heap;

	bounding_k_nearest_mesh_scene_vector::k_nearest_result result
		(bounding_k_nearest_mesh_scene_vector::k_nearest
			(scene, point, k, metric,
			 node_heap, neighbor_heap,
			 distance_squared_bound, distance_squared_bound_valid,
			 distance_squared_tolerance));


	// Assert validity of resulting iterators,
	// then repackage into a more manageable client format.
	mesh_scene_k_nearest_result::entry_array entries;

	for (int64 i = 0; i < result.entries.size(); ++i)
	{
		const bounding_k_nearest_mesh_scene_vector::k_nearest_entry& entry =
			result.entries[i];
		const mesh_scene_meshes_bounding::const_node_iterator& meshes_bounding_node =
			entry.leaf_node;

		if (!meshes_bounding_node)
		{
			LOG_ERROR("Found neighbor, but had invalid closest mesh scene node.");
			throw assert_failed();
		}

		const mesh_scene::object_iterator& scene_object =
			meshes_bounding_node->object();

		if (!scene_object)
		{
			LOG_ERROR("Found neighbor, but had invalid closest object.");
			throw assert_failed();
		}

		entries.push_back
			(mesh_scene_k_nearest_entry
				(meshes_bounding_node,
				 scene_object,
				 entry.closest_distance_squared));
	}

	return mesh_scene_k_nearest_result(std::move(entries));
}




} /* namespace viral_core */
