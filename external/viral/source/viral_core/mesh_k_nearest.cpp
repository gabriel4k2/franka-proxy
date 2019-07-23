/**
 *************************************************************************
 *
 * @file mesh_k_nearest.cpp
 *
 * Find nearest neighbor triangles in a mesh, implementation.
 *
 ************************************************************************/


#include "mesh_k_nearest.hpp"


#include "geo_transform.hpp"
#include "geo_distance.hpp"
#include "bounding_tree.hpp"
#include "bounding_k_nearest.hpp"
#include "bounding_k_nearest_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_k_nearest_metric_triangle
//
//////////////////////////////////////////////////////////////////////////


mesh_k_nearest_metric_triangle::mesh_k_nearest_metric_triangle() NOTHROW
	{ }


bool mesh_k_nearest_metric_triangle::try_node_distance_squared
	(const sphere_tree_triangle& t,
	 const sphere_tree_triangle_const_node_iterator& n,
	 const vector& o,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	return
		bounding_k_nearest_util::try_shape_distance_squared
			(t.shape(n), o,
			 min_distance_squared_lower_bound,
			 evaluation_potential);
}


bool mesh_k_nearest_metric_triangle::try_node_distance_squared
	(const box_tree_triangle& t,
	 const box_tree_triangle_const_node_iterator& n,
	 const vector& o,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	return
		bounding_k_nearest_util::try_shape_distance_squared
			(t.shape(n), o,
			 min_distance_squared_lower_bound,
			 evaluation_potential);
}


bool mesh_k_nearest_metric_triangle::try_leaf_distance_squared
	(const sphere_tree_triangle& t,
	 const sphere_tree_triangle_const_node_iterator& n,
	 const vector& o,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	vector closest_point_t;
	closest_distance_squared =
		geo_distance::distance_squared
			(o, t.object(n), closest_point_t);

	return true;
}


bool mesh_k_nearest_metric_triangle::try_leaf_distance_squared
	(const box_tree_triangle& t,
	 const box_tree_triangle_const_node_iterator& n,
	 const vector& o,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	vector closest_point_t;
	closest_distance_squared =
		geo_distance::distance_squared
			(o, t.object(n), closest_point_t);

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_k_nearest_metric_index
//
//////////////////////////////////////////////////////////////////////////


mesh_k_nearest_metric_index::mesh_k_nearest_metric_index
	(const mesh& mesh_t) NOTHROW
	:
	mesh_t_(mesh_t)
{ }


bool mesh_k_nearest_metric_index::try_node_distance_squared
	(const sphere_tree_index& t,
	 const sphere_tree_index_const_node_iterator& n,
	 const vector& o,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	return
		bounding_k_nearest_util::try_shape_distance_squared
			(t.shape(n), o,
			 min_distance_squared_lower_bound,
			 evaluation_potential);
}


bool mesh_k_nearest_metric_index::try_node_distance_squared
	(const box_tree_index& t,
	 const box_tree_index_const_node_iterator& n,
	 const vector& o,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	return
		bounding_k_nearest_util::try_shape_distance_squared
			(t.shape(n), o,
			 min_distance_squared_lower_bound,
			 evaluation_potential);
}


bool mesh_k_nearest_metric_index::try_leaf_distance_squared
	(const sphere_tree_index& t,
	 const sphere_tree_index_const_node_iterator& n,
	 const vector& o,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	int64 triangle_index_t = t.object(n);

	const mesh_stream_vector& v_t = mesh_t_.vertices();
	const mesh_stream_triangle& t_t = mesh_t_.triangles();

	// Checked access to ensure valid bounding_tree indices.
	// Later triangle distance anyways is much more expensive.
	triangle triangle_t
		(v_t.at(t_t.at(triangle_index_t).i),
		 v_t.at(t_t.at(triangle_index_t).j),
		 v_t.at(t_t.at(triangle_index_t).k));

	vector closest_point_t;
	closest_distance_squared =
		geo_distance::distance_squared
			(o, triangle_t, closest_point_t);

	return true;
}


bool mesh_k_nearest_metric_index::try_leaf_distance_squared
	(const box_tree_index& t,
	 const box_tree_index_const_node_iterator& n,
	 const vector& o,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	int64 triangle_index_t = t.object(n);

	const mesh_stream_vector& v_t = mesh_t_.vertices();
	const mesh_stream_triangle& t_t = mesh_t_.triangles();

	// Checked access to ensure valid bounding_tree indices.
	// Later triangle distance anyways is much more expensive.
	triangle triangle_t
		(v_t.at(t_t.at(triangle_index_t).i),
		 v_t.at(t_t.at(triangle_index_t).j),
		 v_t.at(t_t.at(triangle_index_t).k));

	vector closest_point_t;
	closest_distance_squared =
		geo_distance::distance_squared
			(o, triangle_t, closest_point_t);

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_k_nearest
//
//////////////////////////////////////////////////////////////////////////


mesh_k_nearest_result_sphere_tree_triangle
	mesh_k_nearest::k_nearest
		(const sphere_tree_triangle& t, const si_transform& transform_t,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	vector point_inv_transformed
		(transform_t.invert() * point);

	mesh_k_nearest_metric_triangle metric;

	bounding_k_nearest_sphere_triangle::k_nearest_node_heap node_heap;
	bounding_k_nearest_sphere_triangle::k_nearest_neighbor_heap neighbor_heap;

	return
		bounding_k_nearest_sphere_triangle::k_nearest
			(t, point_inv_transformed, k,
			 metric, node_heap, neighbor_heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


mesh_k_nearest_result_box_tree_triangle
	mesh_k_nearest::k_nearest
		(const box_tree_triangle& t, const si_transform& transform_t,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	vector point_inv_transformed
		(transform_t.invert() * point);

	mesh_k_nearest_metric_triangle metric;

	bounding_k_nearest_box_triangle::k_nearest_node_heap node_heap;
	bounding_k_nearest_box_triangle::k_nearest_neighbor_heap neighbor_heap;

	return
		bounding_k_nearest_box_triangle::k_nearest
			(t, point_inv_transformed, k,
			 metric, node_heap, neighbor_heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


mesh_k_nearest_result_sphere_tree_index
	mesh_k_nearest::k_nearest
		(const sphere_tree_index& t, const mesh& mesh_t,
		 const si_transform& transform_t,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	vector point_inv_transformed
		(transform_t.invert() * point);

	mesh_k_nearest_metric_index metric(mesh_t);

	bounding_k_nearest_sphere_index::k_nearest_node_heap node_heap;
	bounding_k_nearest_sphere_index::k_nearest_neighbor_heap neighbor_heap;

	return
		bounding_k_nearest_sphere_index::k_nearest
			(t, point_inv_transformed, k,
			 metric, node_heap, neighbor_heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


mesh_k_nearest_result_box_tree_index
	mesh_k_nearest::k_nearest
		(const box_tree_index& t, const mesh& mesh_t,
		 const si_transform& transform_t,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	vector point_inv_transformed
		(transform_t.invert() * point);

	mesh_k_nearest_metric_index metric(mesh_t);

	bounding_k_nearest_box_index::k_nearest_node_heap node_heap;
	bounding_k_nearest_box_index::k_nearest_neighbor_heap neighbor_heap;

	return
		bounding_k_nearest_box_index::k_nearest
			(t, point_inv_transformed, k,
			 metric, node_heap, neighbor_heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


dynamic_buffer<int64>
	mesh_k_nearest::try_k_nearest_slow
		(const mesh& m, const si_transform& transform_m,
		 const vector& point, int64 k)
{
	dynamic_buffer<int64> result;
	result.reserve(k);

	dynamic_buffer<float> distances_squared;
	distances_squared.reserve(k);

	vector point_inv_transformed
		(transform_m.invert() * point);
	
	const mesh_stream_vector& vertices_m = m.vertices();
	const mesh_stream_triangle& triangles_m = m.triangles();

	for (int64 i = 0; i < m.triangle_count(); ++i)
	{
		triangle t
			(vertices_m.at(triangles_m.at(i).i),
			 vertices_m.at(triangles_m.at(i).j),
			 vertices_m.at(triangles_m.at(i).k));

		vector closest_point_t;
		float closest_distance_squared =
			geo_distance::distance_squared
				(point_inv_transformed, t, closest_point_t);


		// Add first k neighbors without further consideration,
		// then replace most distant neighbor with closer candidates.
		if (result.size() < k)
		{
			result.push_back(i);
			distances_squared.push_back(closest_distance_squared);
		}
		else
		{
			int64 max_candidate = 0;
			for (int64 j = 1; j < k; ++j)
			{
				if (distances_squared[j] > distances_squared[max_candidate])
					max_candidate = j;
			}

			if (distances_squared[max_candidate] > closest_distance_squared)
			{
				result[max_candidate] = i;
				distances_squared[max_candidate] = closest_distance_squared;
			}
		}
	}


	// Sort result indices by increasing distance.
	for (int64 i = 0; i < result.size(); ++i)
	for (int64 j = i + 1; j < result.size(); ++j)
	{
		if (distances_squared[i] > distances_squared[j])
		{
			swap(result[i], result[j]);
			swap(distances_squared[i], distances_squared[j]);
		}
	}


	return std::move(result);
}




} /* namespace viral_core */
