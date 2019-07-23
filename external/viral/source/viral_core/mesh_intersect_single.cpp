/**
 *************************************************************************
 *
 * @file mesh_intersect_single.cpp
 *
 * Intersection between a mesh and a single shape, implementation.
 *
 ************************************************************************/


#include "mesh_intersect_single.hpp"


#include "geo_intersect.hpp"
#include "bounding_single.hpp"
#include "bounding_tree.hpp"
#include "bounding_intersect.hpp"
#include "bounding_intersect_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_intersect_single_test_triangle
//
//////////////////////////////////////////////////////////////////////////


mesh_intersect_single_test_triangle::mesh_intersect_single_test_triangle
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta_.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb_.build_local_to_world())
{ }


bool mesh_intersect_single_test_triangle::node_intersecting
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const bounding_single_sphere& tb,
	 const bounding_single_sphere_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return
		bounding_intersect_util::shape_intersecting
			(ta.shape(na), transform_ta_,
			 tb.shape(nb), transform_tb_,
			 evaluation_potential);
}


bool mesh_intersect_single_test_triangle::node_intersecting
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const bounding_single_box& tb,
	 const bounding_single_box_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return
		bounding_intersect_util::shape_intersecting
			(ta.shape(na), transform_ta_, to_world_matrix_ta_,
			 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
			 evaluation_potential);
}


bool mesh_intersect_single_test_triangle::node_intersecting
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const bounding_single_sphere_triangle& tb,
	 const bounding_single_sphere_triangle_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	throw unsupported_feature();
}


bool mesh_intersect_single_test_triangle::node_intersecting
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const bounding_single_box_triangle& tb,
	 const bounding_single_box_triangle_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	throw unsupported_feature();
}


bool mesh_intersect_single_test_triangle::leaf_intersecting
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const bounding_single_sphere& tb,
	 const bounding_single_sphere_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}


bool mesh_intersect_single_test_triangle::leaf_intersecting
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const bounding_single_box& tb,
	 const bounding_single_box_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}


bool mesh_intersect_single_test_triangle::leaf_intersecting
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const bounding_single_sphere_triangle& tb,
	 const bounding_single_sphere_triangle_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}


bool mesh_intersect_single_test_triangle::leaf_intersecting
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const bounding_single_box_triangle& tb,
	 const bounding_single_box_triangle_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	throw unsupported_feature();
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_intersect_single_test_index
//
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
//
// mesh_intersect_single
//
//////////////////////////////////////////////////////////////////////////


mesh_intersect_single_result_sphere_tree_triangle_sphere
	mesh_intersect_single::intersect
		(const sphere_tree_triangle& ta, const si_transform& transform_ta,
		 const bounding_single_sphere& tb, const si_transform& transform_tb)
{
	mesh_intersect_single_test_triangle test
		(transform_ta, transform_tb);

	bounding_intersect_sphere_triangle_sphere::intersect_heap heap;

	return
		bounding_intersect_sphere_triangle_sphere::intersect
			(ta, tb, test, heap);
}


mesh_intersect_single_result_box_tree_triangle_box
	mesh_intersect_single::intersect
		(const box_tree_triangle& ta, const si_transform& transform_ta,
		 const bounding_single_box& tb, const si_transform& transform_tb)
{
	mesh_intersect_single_test_triangle test
		(transform_ta, transform_tb);

	bounding_intersect_box_triangle_box::intersect_heap heap;

	return
		bounding_intersect_box_triangle_box::intersect
			(ta, tb, test, heap);
}


mesh_intersect_single_result_box_tree_triangle_box_triangle
	mesh_intersect_single::intersect
		(const box_tree_triangle& ta, const si_transform& transform_ta,
		 const bounding_single_box_triangle& tb, const si_transform& transform_tb)
{
	mesh_intersect_single_test_triangle test
		(transform_ta, transform_tb);

	bounding_intersect_box_triangle_box_triangle::intersect_heap heap;

	return
		bounding_intersect_box_triangle_box_triangle::intersect
			(ta, tb, test, heap);
}


mesh_intersect_single_result_sphere_tree_index_sphere
	mesh_intersect_single::intersect
		(const sphere_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const bounding_single_sphere& tb,
		 const si_transform& transform_tb)
{
	mesh_intersect_single_test_index test
		(mesh_ta, transform_ta, transform_tb);

	bounding_intersect_sphere_index_sphere::intersect_heap heap;

	return
		bounding_intersect_sphere_index_sphere::intersect
			(ta, tb, test, heap);
}


mesh_intersect_single_result_box_tree_index_box
	mesh_intersect_single::intersect
		(const box_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const bounding_single_box& tb,
		 const si_transform& transform_tb)
{
	mesh_intersect_single_test_index test
		(mesh_ta, transform_ta, transform_tb);

	bounding_intersect_box_index_box::intersect_heap heap;

	return
		bounding_intersect_box_index_box::intersect
			(ta, tb, test, heap);
}


mesh_intersect_single_result_box_tree_index_box_triangle
	mesh_intersect_single::intersect
		(const box_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const bounding_single_box_triangle& tb,
		 const si_transform& transform_tb)
{
	mesh_intersect_single_test_index test
		(mesh_ta, transform_ta, transform_tb);

	bounding_intersect_box_index_box_triangle::intersect_heap heap;

	return
		bounding_intersect_box_index_box_triangle::intersect
			(ta, tb, test, heap);
}




} /* namespace viral_core */
