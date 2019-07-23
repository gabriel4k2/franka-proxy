/**
 *************************************************************************
 *
 * @file bounding_implicit_region.cpp
 *
 * Region tests for implicit bounding hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_implicit_region.hpp"


#include "geo_volume.hpp"
#include "geo_intersect.hpp"
#include "bounding_implicit.hpp"
#include "bounding_region.hpp"
#include "bounding_region_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_region_test
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_region_test::bounding_implicit_region_test
	(const si_transform& transform_t) NOTHROW
	:
	transform_t_(transform_t),
	to_world_matrix_t_(transform_t_.build_local_to_world())
{ }


bool bounding_implicit_region_test::try_node_region
	(const sphere_tree& t,
	 const sphere_tree_const_node_iterator& n,
	 const sphere& s) const
{
	return bounding_region_util::try_shape_region
				(t.shape(n), transform_t_, s);
}


bool bounding_implicit_region_test::try_node_region
	(const box_tree& t,
	 const box_tree_const_node_iterator& n,
	 const box& b) const
{
	return bounding_region_util::try_shape_region
				(t.shape(n), to_world_matrix_t_, b);
}


bool bounding_implicit_region_test::try_leaf_region
	(const sphere_tree& t,
	 const sphere_tree_const_node_iterator& n,
	 const sphere& s,
	 bounding_region_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Perform exact intersection here, no false positives.
	return geo_intersect::intersecting
				(t.shape(n), transform_t_, s, si_transform());
}


bool bounding_implicit_region_test::try_leaf_region
	(const box_tree& t,
	 const box_tree_const_node_iterator& n,
	 const box& b,
	 bounding_region_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Perform exact intersection here, no false positives.
	return geo_intersect::intersecting
				(t.shape(n), transform_t_, b, si_transform());
}




//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_region
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_region_result_sphere_tree
	bounding_implicit_region::region
		(const sphere_tree& t,
		 const si_transform& transform_t,
		 const sphere& s)
{
	bounding_implicit_region_test test(transform_t);

	bounding_region_sphere_tree_sphere::region_array node_array;

	return
		bounding_region_sphere_tree_sphere::region
			(t, s, test, node_array);
}


bounding_implicit_region_result_box_tree
	bounding_implicit_region::region
		(const box_tree& t, 
		 const si_transform& transform_t,
		 const box& b)
{
	bounding_implicit_region_test test(transform_t);

	bounding_region_box_tree_box::region_array node_array;

	return
		bounding_region_box_tree_box::region
			(t, b, test, node_array);
}




} /* namespace viral_core */
