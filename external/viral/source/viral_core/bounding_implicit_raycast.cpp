/**
 *************************************************************************
 *
 * @file bounding_implicit_raycast.cpp
 *
 * Raycasting for implicit bounding volume hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_implicit_raycast.hpp"


#include "geo_transform.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"
#include "geo_raycast.hpp"
#include "bounding_implicit.hpp"
#include "bounding_raycast.hpp"
#include "bounding_raycast_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_raycast_test
//
//////////////////////////////////////////////////////////////////////////


bool bounding_implicit_raycast_test::try_node_raycast
	(const segment& s, const sphere_tree& t,
	 const sphere_tree_const_node_iterator& n,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	return bounding_raycast_util::try_shape_raycast
				(s, t.shape(n), hit_distance_lower_bound, evaluation_potential);
}


bool bounding_implicit_raycast_test::try_node_raycast
	(const segment& s, const box_tree& t,
	 const box_tree_const_node_iterator& n,
	 float& hit_distance_lower_bound, float& evaluation_potential) const
{
	return bounding_raycast_util::try_shape_raycast
				(s, t.shape(n), hit_distance_lower_bound, evaluation_potential);
}


bool bounding_implicit_raycast_test::try_leaf_raycast
	(const segment& s, const sphere_tree& t,
	 const sphere_tree_const_node_iterator& n,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Determine exact hit point and hit distance.
	return geo_raycast::try_raycast_interior
				(s, t.shape(n), hit_point, hit_distance);
}


bool bounding_implicit_raycast_test::try_leaf_raycast
	(const segment& s, const box_tree& t,
	 const box_tree_const_node_iterator& n,
	 vector& hit_point, float& hit_distance,
	 bounding_raycast_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Determine exact hit point and hit distance.
	return geo_raycast::try_raycast_interior
				(s, t.shape(n), hit_point, hit_distance);
}




//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_raycast
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_raycast_result_sphere_tree
	bounding_implicit_raycast::raycast
		(const segment& s, const sphere_tree& t,
		 const si_transform& transform_t,
 		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	si_transform transform_t_inv(transform_t.invert());
	segment s_inv_transformed
		(transform_t_inv * s.start,
		 transform_t_inv * s.end);

	bounding_implicit_raycast_test test;
	bounding_raycast_segment_sphere_tree::raycast_heap heap;

	return
		bounding_raycast_segment_sphere_tree::raycast
			(s_inv_transformed, t, test, heap,
			 hit_distance_bound, hit_distance_bound_valid, hit_distance_tolerance);
}


bounding_implicit_raycast_result_box_tree
	bounding_implicit_raycast::raycast
		(const segment& s, const box_tree& t,
		 const si_transform& transform_t,
 		 float hit_distance_bound, bool hit_distance_bound_valid,
		 float hit_distance_tolerance)
{
	si_transform transform_t_inv(transform_t.invert());
	segment s_inv_transformed
		(transform_t_inv * s.start,
		 transform_t_inv * s.end);

	bounding_implicit_raycast_test test;
	bounding_raycast_segment_box_tree::raycast_heap heap;

	return
		bounding_raycast_segment_box_tree::raycast
			(s_inv_transformed, t, test, heap,
			 hit_distance_bound, hit_distance_bound_valid, hit_distance_tolerance);
}




} /* namespace viral_core */
