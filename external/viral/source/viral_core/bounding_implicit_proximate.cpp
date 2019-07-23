/**
 *************************************************************************
 *
 * @file bounding_implicit_proximate.cpp
 *
 * Proximity for implicit bounding volume hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_implicit_proximate.hpp"


#include "geo_volume.hpp"
#include "geo_distance.hpp"
#include "bounding_implicit.hpp"
#include "bounding_proximate.hpp"
#include "bounding_proximate_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_proximate_test
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_proximate_test::bounding_implicit_proximate_test
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb.build_local_to_world())
{ }


bool bounding_implicit_proximate_test::node_proximate
	(const sphere_tree& ta,
	 const sphere_tree_const_node_iterator& na,
	 const sphere_tree& tb,
	 const sphere_tree_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	return bounding_proximate_util::shape_proximate
		(ta.shape(na), transform_ta_, to_world_matrix_ta_,
		 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
		 distance_squared_upper_bound,
		 evaluation_potential);
}


bool bounding_implicit_proximate_test::node_proximate
	(const box_tree& ta,
	 const box_tree_const_node_iterator& na,
	 const box_tree& tb,
	 const box_tree_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	return bounding_proximate_util::shape_proximate
		(ta.shape(na), to_world_matrix_ta_,
		 tb.shape(nb), to_world_matrix_tb_,
		 distance_squared_upper_bound,
		 evaluation_potential);
}


bool bounding_implicit_proximate_test::leaf_proximate
	(const sphere_tree& ta,
	 const sphere_tree_const_node_iterator& na,
	 const sphere_tree& tb,
	 const sphere_tree_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Determine exact proximity instead of false positives.
	vector closest_point_ta;
	vector closest_point_tb;
	float closest_distance_squared = 0.f;
	if (!geo_distance::try_distance_squared
			(ta.shape(na), transform_ta_,
			 tb.shape(nb), transform_tb_,
			 closest_point_ta, closest_point_tb,
			 closest_distance_squared))
					return false;
			 
	return closest_distance_squared <= distance_squared_upper_bound;
}


bool bounding_implicit_proximate_test::leaf_proximate
	(const box_tree& ta,
	 const box_tree_const_node_iterator& na,
	 const box_tree& tb,
	 const box_tree_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Determine exact proximity instead of false positives.
	vector closest_point_ta;
	vector closest_point_tb;
	float closest_distance_squared = 0.f;
	if (!geo_distance::try_distance_squared
			(ta.shape(na), transform_ta_,
			 tb.shape(nb), transform_tb_,
			 closest_point_ta, closest_point_tb,
			 closest_distance_squared))
					return false;

	return closest_distance_squared <= distance_squared_upper_bound;
}




//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_proximate
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_proximate_result_sphere_tree
	bounding_implicit_proximate::proximate
		(const sphere_tree& ta, const si_transform& transform_ta,
		 const sphere_tree& tb, const si_transform& transform_tb,
		 float distance_squared_upper_bound)
{
	bounding_implicit_proximate_test test
		(transform_ta, transform_tb);

	bounding_proximate_sphere_tree::proximate_heap heap;

	return
		bounding_proximate_sphere_tree::proximate
			(ta, tb, test, heap,
			 distance_squared_upper_bound);
}


bounding_implicit_proximate_result_box_tree
	bounding_implicit_proximate::proximate
		(const box_tree& ta, const si_transform& transform_ta,
		 const box_tree& tb, const si_transform& transform_tb,
		 float distance_squared_upper_bound)
{
	bounding_implicit_proximate_test test
		(transform_ta, transform_tb);

	bounding_proximate_box_tree::proximate_heap heap;

	return
		bounding_proximate_box_tree::proximate
			(ta, tb, test, heap,
			 distance_squared_upper_bound);
}




} /* namespace viral_core */
