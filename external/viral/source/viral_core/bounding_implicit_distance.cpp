/**
 *************************************************************************
 *
 * @file bounding_implicit_distance.cpp
 *
 * Distances between implicit bounding volume hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_implicit_distance.hpp"


#include "geo_volume.hpp"
#include "geo_distance.hpp"
#include "bounding_implicit.hpp"
#include "bounding_distance.hpp"
#include "bounding_distance_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_distance_metric
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_distance_metric::bounding_implicit_distance_metric
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb.build_local_to_world())
{ }


bool bounding_implicit_distance_metric::try_node_distance_squared
	(const sphere_tree& ta,
	 const sphere_tree_const_node_iterator& na,
	 const sphere_tree& tb,
	 const sphere_tree_const_node_iterator& nb,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	return
		bounding_distance_util::try_shape_distance_squared
			(ta.shape(na), transform_ta_, to_world_matrix_ta_,
			 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
			 min_distance_squared_lower_bound,
			 evaluation_potential);
}


bool bounding_implicit_distance_metric::try_node_distance_squared
	(const box_tree& ta,
	 const box_tree_const_node_iterator& na,
	 const box_tree& tb,
	 const box_tree_const_node_iterator& nb,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) const
{
	return
		bounding_distance_util::try_shape_distance_squared
			(ta.shape(na), to_world_matrix_ta_,
			 tb.shape(nb), to_world_matrix_tb_,
			 min_distance_squared_lower_bound,
			 evaluation_potential);
}


bool bounding_implicit_distance_metric::try_leaf_distance_squared
	(const sphere_tree& ta,
	 const sphere_tree_const_node_iterator& na,
	 const sphere_tree& tb,
	 const sphere_tree_const_node_iterator& nb,
	 vector& closest_point_ta, vector& closest_point_tb,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Additionally return closest points and distance here.
	return geo_distance::try_distance_squared
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_,
				 closest_point_ta, closest_point_tb,
				 closest_distance_squared);
}


bool bounding_implicit_distance_metric::try_leaf_distance_squared
	(const box_tree& ta,
	 const box_tree_const_node_iterator& na,
	 const box_tree& tb,
	 const box_tree_const_node_iterator& nb,
	 vector& closest_point_ta, vector& closest_point_tb,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Additionally return closest points and distance here.
	return geo_distance::try_distance_squared
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_,
				 closest_point_ta, closest_point_tb,
				 closest_distance_squared);
}




//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_distance
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_distance_result_sphere_tree
	bounding_implicit_distance::distance
		(const sphere_tree& ta, const si_transform& transform_ta,
		 const sphere_tree& tb, const si_transform& transform_tb,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	bounding_implicit_distance_metric metric
		(transform_ta, transform_tb);

	bounding_distance_sphere_tree::distance_heap heap;

	return
		bounding_distance_sphere_tree::distance
			(ta, tb, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


bounding_implicit_distance_result_box_tree
	bounding_implicit_distance::distance
		(const box_tree& ta, const si_transform& transform_ta,
		 const box_tree& tb, const si_transform& transform_tb,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	bounding_implicit_distance_metric metric
		(transform_ta, transform_tb);

	bounding_distance_box_tree::distance_heap heap;

	return
		bounding_distance_box_tree::distance
			(ta, tb, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}




} /* namespace viral_core */
