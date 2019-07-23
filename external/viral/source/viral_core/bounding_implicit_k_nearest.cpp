/**
 *************************************************************************
 *
 * @file bounding_implicit_k_nearest.cpp
 *
 * Nearest neighbors in implicit bounding hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_implicit_k_nearest.hpp"


#include "geo_transform.hpp"
#include "geo_volume.hpp"
#include "geo_distance.hpp"
#include "bounding_implicit.hpp"
#include "bounding_k_nearest.hpp"
#include "bounding_k_nearest_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_k_nearest_metric
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_k_nearest_metric::
	bounding_implicit_k_nearest_metric() NOTHROW { }


bool bounding_implicit_k_nearest_metric::try_node_distance_squared
	(const sphere_tree& t,
	 const sphere_tree_const_node_iterator& n,
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


bool bounding_implicit_k_nearest_metric::try_node_distance_squared
	(const box_tree& t,
	 const box_tree_const_node_iterator& n,
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


bool bounding_implicit_k_nearest_metric::try_leaf_distance_squared
	(const sphere_tree& t,
	 const sphere_tree_const_node_iterator& n,
	 const vector& o,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Fetch exact distance instead of bounds here.
	vector closest_point_t;
	return geo_distance::try_distance_squared
				(o, t.shape(n), closest_point_t,
				 closest_distance_squared);
}


bool bounding_implicit_k_nearest_metric::try_leaf_distance_squared
	(const box_tree& t,
	 const box_tree_const_node_iterator& n,
	 const vector& o,
	 float& closest_distance_squared,
	 bounding_k_nearest_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Fetch exact distance instead of bounds here.
	vector closest_point_t;
	return geo_distance::try_distance_squared
				(o, t.shape(n), closest_point_t,
				 closest_distance_squared);
}




//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_k_nearest
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_k_nearest_result_sphere_tree
	bounding_implicit_k_nearest::k_nearest
		(const sphere_tree& t, const si_transform& transform_t,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	vector point_inv_transformed
		(transform_t.invert() * point);

	bounding_implicit_k_nearest_metric metric;

	bounding_k_nearest_sphere_tree::k_nearest_node_heap node_heap;
	bounding_k_nearest_sphere_tree::k_nearest_neighbor_heap neighbor_heap;

	return
		bounding_k_nearest_sphere_tree::k_nearest
			(t, point_inv_transformed, k,
			 metric, node_heap, neighbor_heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


bounding_implicit_k_nearest_result_box_tree
	bounding_implicit_k_nearest::k_nearest
		(const box_tree& t, const si_transform& transform_t,
		 const vector& point,
		 int64 k,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	vector point_inv_transformed
		(transform_t.invert() * point);

	bounding_implicit_k_nearest_metric metric;

	bounding_k_nearest_box_tree::k_nearest_node_heap node_heap;
	bounding_k_nearest_box_tree::k_nearest_neighbor_heap neighbor_heap;

	return
		bounding_k_nearest_box_tree::k_nearest
			(t, point_inv_transformed, k,
			 metric, node_heap, neighbor_heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}




} /* namespace viral_core */
