/**
 *************************************************************************
 *
 * @file mesh_scene_region.cpp
 *
 * Region tests on a mesh_scene, implementation.
 *
 ************************************************************************/


#include "mesh_scene_region.hpp"


#include "log.hpp"
#include "bounding_region.hpp"
#include "bounding_region_util.hpp"
#include "mesh.hpp"
#include "mesh_region.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_region_test
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_region_test::mesh_scene_region_test() { }


bool mesh_scene_region_test::try_node_region
	(const mesh_scene& scene,
	 const mesh_scene::const_node_iterator& node,
	 const sphere& s) const
{
	throw unsupported_feature();
}


bool mesh_scene_region_test::try_node_region
	(const mesh_scene& scene,
	 const mesh_scene::const_node_iterator& node,
	 const box& b) const
{
	throw unsupported_feature();
}


bool mesh_scene_region_test::try_leaf_region
	(const mesh_scene& scene,
	 const mesh_scene::const_node_iterator& node,
	 const sphere& s,
	 bounding_region_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}


bool mesh_scene_region_test::try_leaf_region
	(const mesh_scene& scene,
	 const mesh_scene::const_node_iterator& node,
	 const box& b,
	 bounding_region_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_region
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_region_result
	mesh_scene_region::region
		(const mesh_scene& scene, const sphere& s)
{
	// Perform bounding_region query on the scene and our sphere. 
	// No mesh_scene_triangles, we do not want to return individual triangles.
	mesh_scene_region_test test;

	bounding_region_mesh_scene_sphere::region_array region_array;
	
	bounding_region_mesh_scene_sphere::region_result result
		(bounding_region_mesh_scene_sphere::region
			(scene, s, test, region_array));

	return wrap_region_result(result);
}


mesh_scene_region_result
	mesh_scene_region::region
		(const mesh_scene& scene, const box& b)
{
	// Perform bounding_region query on the scene and our box. 
	// No mesh_scene_triangles, we do not want to return individual triangles.
	mesh_scene_region_test test;

	bounding_region_mesh_scene_box::region_array region_array;
	
	bounding_region_mesh_scene_box::region_result result
		(bounding_region_mesh_scene_box::region
			(scene, b, test, region_array));

	return wrap_region_result(result);
}


template <typename T>
	mesh_scene_region_result
		mesh_scene_region::wrap_region_result
			(const T& region_result)
{
	// Assert validity of resulting iterators,
	// then repackage into a more manageable client format.
	mesh_scene_region_result::entry_array entries;

	for (int64 i = 0; i < region_result.entries.size(); ++i)
	{
		const bounding_region_mesh_scene_sphere::region_entry& entry =
			region_result.entries[i];
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
			(mesh_scene_region_entry
				(meshes_bounding_node,
				 scene_object));
	}

	return mesh_scene_region_result(std::move(entries));
}




} /* namespace viral_core */
