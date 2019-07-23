/**
 *************************************************************************
 *
 * @file bounding_implicit_intersect.cpp
 *
 * Intersection of implicit bounding volume hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_implicit_intersect.hpp"


#include "geo_volume.hpp"
#include "geo_intersect.hpp"
#include "bounding_implicit.hpp"
#include "bounding_intersect.hpp"
#include "bounding_intersect_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_intersect_test
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_intersect_test::bounding_implicit_intersect_test
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb.build_local_to_world())
{ }


bool bounding_implicit_intersect_test::node_intersecting
	(const sphere_tree& ta,
	 const sphere_tree_const_node_iterator& na,
	 const sphere_tree& tb,
	 const sphere_tree_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return bounding_intersect_util::shape_intersecting
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_,
				 evaluation_potential);
}


bool bounding_implicit_intersect_test::node_intersecting
	(const box_tree& ta,
	 const box_tree_const_node_iterator& na,
	 const box_tree& tb,
	 const box_tree_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return bounding_intersect_util::shape_intersecting
				(ta.shape(na), transform_ta_, to_world_matrix_ta_,
				 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
				 evaluation_potential);
}


bool bounding_implicit_intersect_test::leaf_intersecting
	(const sphere_tree& ta,
	 const sphere_tree_const_node_iterator& na,
	 const sphere_tree& tb,
	 const sphere_tree_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Check for exact intersection here, no false positives.
	return geo_intersect::intersecting
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_);
}


bool bounding_implicit_intersect_test::leaf_intersecting
	(const box_tree& ta,
	 const box_tree_const_node_iterator& na,
	 const box_tree& tb,
	 const box_tree_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	// Leaf shapes are geometry objects of bounding_implicit.
	// Check for exact intersection here, no false positives.
	return geo_intersect::intersecting
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_);
}




//////////////////////////////////////////////////////////////////////////
//
// bounding_implicit_intersect
//
//////////////////////////////////////////////////////////////////////////


bounding_implicit_intersect_result_sphere_tree
	bounding_implicit_intersect::intersect
		(const sphere_tree& ta, const si_transform& transform_ta,
		 const sphere_tree& tb, const si_transform& transform_tb)
{
	bounding_implicit_intersect_test test
		(transform_ta, transform_tb);

	bounding_intersect_sphere_tree::intersect_heap heap;

	return
		bounding_intersect_sphere_tree::intersect
			(ta, tb, test, heap);
}


bounding_implicit_intersect_result_box_tree
	bounding_implicit_intersect::intersect
		(const box_tree& ta, const si_transform& transform_ta,
		 const box_tree& tb, const si_transform& transform_tb)
{
	bounding_implicit_intersect_test test
		(transform_ta, transform_tb);

	bounding_intersect_box_tree::intersect_heap heap;

	return
		bounding_intersect_box_tree::intersect
			(ta, tb, test, heap);
}




} /* namespace viral_core */
