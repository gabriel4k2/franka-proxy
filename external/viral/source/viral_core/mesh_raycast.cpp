/**
 *************************************************************************
 *
 * @file mesh_raycast.cpp
 *
 * Raycasting through meshes, implementation.
 *
 ************************************************************************/


#include "mesh_raycast.hpp"


#include "geo_transform.hpp"
#include "geo_raycast.hpp"
#include "bounding_tree.hpp"
#include "bounding_raycast.hpp"
#include "bounding_raycast_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_raycast_test_triangle
//
//////////////////////////////////////////////////////////////////////////


bool mesh_raycast_test_triangle::try_node_raycast
	(const segment& s, const sphere_tree_triangle& t,
	 const sphere_tree_triangle_const_node_iterator& n,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	return bounding_raycast_util::try_shape_raycast
				(s, t.shape(n), hit_distance_lower_bound, evaluation_potential);
}


bool mesh_raycast_test_triangle::try_node_raycast
	(const segment& s, const box_tree_triangle& t,
	 const box_tree_triangle_const_node_iterator& n,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	return bounding_raycast_util::try_shape_raycast
				(s, t.shape(n), hit_distance_lower_bound, evaluation_potential);
}


bool mesh_raycast_test_triangle::try_leaf_raycast
	(const segment& s, const sphere_tree_triangle& t,
	 const sphere_tree_triangle_const_node_iterator& n,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	return geo_raycast::try_raycast
				(s, t.object(n), hit_point, hit_distance);
}


bool mesh_raycast_test_triangle::try_leaf_raycast
	(const segment& s, const box_tree_triangle& t,
	 const box_tree_triangle_const_node_iterator& n,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	return geo_raycast::try_raycast
				(s, t.object(n), hit_point, hit_distance);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_raycast_test_index
//
//////////////////////////////////////////////////////////////////////////


mesh_raycast_test_index::mesh_raycast_test_index
	(const mesh& mesh_t) NOTHROW
	:
	mesh_t_(mesh_t)
{ }


bool mesh_raycast_test_index::try_node_raycast
	(const segment& s, const sphere_tree_index& t,
	 const sphere_tree_index_const_node_iterator& n,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	return bounding_raycast_util::try_shape_raycast
				(s, t.shape(n), hit_distance_lower_bound, evaluation_potential);
}


bool mesh_raycast_test_index::try_node_raycast
	(const segment& s, const box_tree_index& t,
	 const box_tree_index_const_node_iterator& n,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	return bounding_raycast_util::try_shape_raycast
				(s, t.shape(n), hit_distance_lower_bound, evaluation_potential);
}


bool mesh_raycast_test_index::try_leaf_raycast
	(const segment& s, const sphere_tree_index& t,
	 const sphere_tree_index_const_node_iterator& n,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	int64 triangle_index = t.object(n);

	const mesh_stream_vector& vertices = mesh_t_.vertices();
	const mesh_stream_triangle& triangles = mesh_t_.triangles();

	// Use checked access here,
	// later triangle tests are much more expensive.
	triangle tri
		(vertices.at(triangles.at(triangle_index).i),
		 vertices.at(triangles.at(triangle_index).j),
		 vertices.at(triangles.at(triangle_index).k));

	return geo_raycast::try_raycast
				(s, tri, hit_point, hit_distance);
}


bool mesh_raycast_test_index::try_leaf_raycast
	(const segment& s, const box_tree_index& t,
	 const box_tree_index_const_node_iterator& n,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	int64 triangle_index = t.object(n);

	const mesh_stream_vector& vertices = mesh_t_.vertices();
	const mesh_stream_triangle& triangles = mesh_t_.triangles();

	// Use checked access here,
	// later triangle tests are much more expensive.
	triangle tri
		(vertices.at(triangles.at(triangle_index).i),
		 vertices.at(triangles.at(triangle_index).j),
		 vertices.at(triangles.at(triangle_index).k));

	return geo_raycast::try_raycast
				(s, tri, hit_point, hit_distance);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_raycast
//
//////////////////////////////////////////////////////////////////////////


mesh_raycast_result_sphere_tree_triangle
	mesh_raycast::raycast
		(const segment& s, const sphere_tree_triangle& t,
		 const si_transform& transform_t,
		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	si_transform transform_t_inv(transform_t.invert());
	segment s_inv_transformed
		(transform_t_inv * s.start,
		 transform_t_inv * s.end);

	mesh_raycast_test_triangle test;
	bounding_raycast_segment_sphere_triangle::raycast_heap heap;

	return
		bounding_raycast_segment_sphere_triangle::raycast
			(s_inv_transformed, t, test, heap,
			 hit_distance_bound, hit_distance_bound_valid, hit_distance_tolerance);
}


mesh_raycast_result_box_tree_triangle
	mesh_raycast::raycast
		(const segment& s, const box_tree_triangle& t,
		 const si_transform& transform_t,
		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	si_transform transform_t_inv(transform_t.invert());
	segment s_inv_transformed
		(transform_t_inv * s.start,
		 transform_t_inv * s.end);

	mesh_raycast_test_triangle test;
	bounding_raycast_segment_box_triangle::raycast_heap heap;

	return
		bounding_raycast_segment_box_triangle::raycast
			(s_inv_transformed, t, test, heap,
			 hit_distance_bound, hit_distance_bound_valid, hit_distance_tolerance);
}


mesh_raycast_result_sphere_tree_index
	mesh_raycast::raycast
		(const segment& s, const sphere_tree_index& t,
		 const mesh& mesh_t, const si_transform& transform_t,
		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	si_transform transform_t_inv(transform_t.invert());
	segment s_inv_transformed
		(transform_t_inv * s.start,
		 transform_t_inv * s.end);

	mesh_raycast_test_index test(mesh_t);
	bounding_raycast_segment_sphere_index::raycast_heap heap;

	return
		bounding_raycast_segment_sphere_index::raycast
			(s_inv_transformed, t, test, heap,
			 hit_distance_bound, hit_distance_bound_valid, hit_distance_tolerance);
}


mesh_raycast_result_box_tree_index
	mesh_raycast::raycast
		(const segment& s, const box_tree_index& t,
		 const mesh& mesh_t, const si_transform& transform_t,
		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	si_transform transform_t_inv(transform_t.invert());
	segment s_inv_transformed
		(transform_t_inv * s.start,
		 transform_t_inv * s.end);

	mesh_raycast_test_index test(mesh_t);
	bounding_raycast_segment_box_index::raycast_heap heap;

	return
		bounding_raycast_segment_box_index::raycast
			(s_inv_transformed, t, test, heap,
			 hit_distance_bound, hit_distance_bound_valid, hit_distance_tolerance);
}


bool mesh_raycast::try_raycast_slow
	(const segment& s,
	 const mesh& m, const si_transform& transform_m,
	 vector& hit_point, float& hit_distance)
{
	// Reset in-out parameters just in case
	// we should early out or throw.
	hit_point = vector();
	hit_distance = 0.f;

	if (m.triangle_count() == 0)
		return false;


	// Transform segment to mesh coordinates.
	si_transform transform_m_inv(transform_m.invert());
	segment s_inv_transformed
		(transform_m_inv * s.start,
		 transform_m_inv * s.end);


	// Run through mesh and find first hit point over all triangles.
	// Store results in offhand variables for exception safety.
	vector offhand_hit_point;
	float offhand_hit_distance = 0;

	const mesh_stream_vector& vertices = m.vertices();
	const mesh_stream_triangle& triangles = m.triangles();

	bool any_hit = false;
	for (int64 i = 0; i < m.triangle_count(); ++i)
	{
		triangle tri
			(vertices.at(triangles.at(i).i),
			 vertices.at(triangles.at(i).j),
			 vertices.at(triangles.at(i).k));

		vector tri_hit_point;
		float tri_hit_distance = 0.f;

		if (geo_raycast::try_raycast
				(s_inv_transformed, tri, tri_hit_point, tri_hit_distance))
		{
			if (!any_hit || tri_hit_distance < offhand_hit_distance)
			{
				offhand_hit_point = tri_hit_point;
				offhand_hit_distance = tri_hit_distance;
				any_hit = true;
			}
		}
	}

	if (!any_hit) return false;


	// Found at least one intersection point,
	// so send this result back to caller.
	// Exception safety: NOTHROW from here.
	hit_point = offhand_hit_point;
	hit_distance = offhand_hit_distance;

	return true;
}




} /* namespace viral_core */
