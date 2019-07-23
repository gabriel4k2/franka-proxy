/**
 *************************************************************************
 *
 * @file mesh_region.cpp
 *
 * Query for mesh triangles in a region, implementation.
 *
 ************************************************************************/


#include "mesh_region.hpp"


#include "bounding_tree.hpp"
#include "bounding_region.hpp"
#include "bounding_region_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_region_test_triangle
//
//////////////////////////////////////////////////////////////////////////


mesh_region_test_triangle::mesh_region_test_triangle
	(const si_transform& transform_t) NOTHROW
	:
	transform_t_(transform_t),
	to_world_matrix_t_(transform_t_.build_local_to_world())
{ }


bool mesh_region_test_triangle::try_node_region
	(const sphere_tree_triangle& t,
	 const sphere_tree_triangle_const_node_iterator& n,
	 const sphere& s) const
{
	return bounding_region_util::try_shape_region
				(t.shape(n), transform_t_, s);
}


bool mesh_region_test_triangle::try_node_region
	(const box_tree_triangle& t,
	 const box_tree_triangle_const_node_iterator& n,
	 const box& b) const
{
	return bounding_region_util::try_shape_region
				(t.shape(n), to_world_matrix_t_, b);
}


bool mesh_region_test_triangle::try_leaf_region
	(const sphere_tree_triangle& t,
	 const sphere_tree_triangle_const_node_iterator& n,
	 const sphere& s,
	 bounding_region_no_extended_result& extended_result) const
{
	return bounding_region_util::try_object_region
				(t.object(n), to_world_matrix_t_, s);
}


bool mesh_region_test_triangle::try_leaf_region
	(const box_tree_triangle& t,
	 const box_tree_triangle_const_node_iterator& n,
	 const box& b,
	 bounding_region_no_extended_result& extended_result) const
{
	return bounding_region_util::try_object_region
				(t.object(n), to_world_matrix_t_, b);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_region_test_index
//
//////////////////////////////////////////////////////////////////////////


mesh_region_test_index::mesh_region_test_index
	(const mesh& mesh_t,
	 const si_transform& transform_t) NOTHROW
	:
	mesh_t_(mesh_t),
	transform_t_(transform_t),
	to_world_matrix_t_(transform_t_.build_local_to_world())
{ }


bool mesh_region_test_index::try_node_region
	(const sphere_tree_index& t,
	 const sphere_tree_index_const_node_iterator& n,
	 const sphere& s) const
{
	return bounding_region_util::try_shape_region
				(t.shape(n), transform_t_, s);
}


bool mesh_region_test_index::try_node_region
	(const box_tree_index& t,
	 const box_tree_index_const_node_iterator& n,
	 const box& b) const
{
	return bounding_region_util::try_shape_region
				(t.shape(n), to_world_matrix_t_, b);
}


bool mesh_region_test_index::try_leaf_region
	(const sphere_tree_index& t,
	 const sphere_tree_index_const_node_iterator& n,
	 const sphere& s,
	 bounding_region_no_extended_result& extended_result) const
{
	int64 triangle_index_t = t.object(n);

	const mesh_stream_vector& v_t = mesh_t_.vertices();
	const mesh_stream_triangle& t_t = mesh_t_.triangles();

	// Checked access to ensure valid bounding_tree indices.
	// Later triangle test anyways is much more expensive.
	triangle triangle_t
		(v_t.at(t_t.at(triangle_index_t).i),
		 v_t.at(t_t.at(triangle_index_t).j),
		 v_t.at(t_t.at(triangle_index_t).k));

	return bounding_region_util::try_object_region
				(triangle_t, to_world_matrix_t_, s);
}


bool mesh_region_test_index::try_leaf_region
	(const box_tree_index& t,
	 const box_tree_index_const_node_iterator& n,
	 const box& b,
	 bounding_region_no_extended_result& extended_result) const
{
	int64 triangle_index_t = t.object(n);

	const mesh_stream_vector& v_t = mesh_t_.vertices();
	const mesh_stream_triangle& t_t = mesh_t_.triangles();

	// Checked access to ensure valid bounding_tree indices.
	// Later triangle test anyways is much more expensive.
	triangle triangle_t
		(v_t.at(t_t.at(triangle_index_t).i),
		 v_t.at(t_t.at(triangle_index_t).j),
		 v_t.at(t_t.at(triangle_index_t).k));

	return bounding_region_util::try_object_region
				(triangle_t, to_world_matrix_t_, b);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_region
//
//////////////////////////////////////////////////////////////////////////


mesh_region_result_sphere_tree_triangle
	mesh_region::region
		(const sphere_tree_triangle& t,
		 const si_transform& transform_t,
		 const sphere& s)
{
	mesh_region_test_triangle test(transform_t);

	bounding_region_sphere_triangle_sphere::region_array node_array;

	return
		bounding_region_sphere_triangle_sphere::region
			(t, s, test, node_array);
}


mesh_region_result_box_tree_triangle
	mesh_region::region
		(const box_tree_triangle& t, 
		 const si_transform& transform_t,
		 const box& b)
{
	mesh_region_test_triangle test(transform_t);

	bounding_region_box_triangle_box::region_array node_array;

	return
		bounding_region_box_triangle_box::region
			(t, b, test, node_array);
}


mesh_region_result_sphere_tree_index
	mesh_region::region
		(const sphere_tree_index& t, const mesh& mesh_t,
		 const si_transform& transform_t,
		 const sphere& s)
{
	mesh_region_test_index test(mesh_t, transform_t);

	bounding_region_sphere_index_sphere::region_array node_array;

	return
		bounding_region_sphere_index_sphere::region
			(t, s, test, node_array);
}


mesh_region_result_box_tree_index
	mesh_region::region
		(const box_tree_index& t, const mesh& mesh_t,
		 const si_transform& transform_t,
		 const box& b)
{
	mesh_region_test_index test(mesh_t, transform_t);

	bounding_region_box_index_box::region_array node_array;

	return
		bounding_region_box_index_box::region
			(t, b, test, node_array);
}


dynamic_buffer<int64>
	mesh_region::region_slow
		(const mesh& m, const si_transform& transform_m,
		 const sphere& s)
{
	// Transform each triangle into world coordinates,
	// and test each and every triangle against region.
	dynamic_buffer<int64> result;
	
	matrix matrix_m(transform_m.build_local_to_world());

	const mesh_stream_vector& vertices_m = m.vertices();
	const mesh_stream_triangle& triangles_m = m.triangles();

	for (int64 i = 0; i < m.triangle_count(); ++i)
	{
		if (s.contains(matrix_m * vertices_m.at(triangles_m.at(i).i)) &&
			s.contains(matrix_m * vertices_m.at(triangles_m.at(i).j)) &&
			s.contains(matrix_m * vertices_m.at(triangles_m.at(i).k)))
				result.push_back(i);
	}

	return std::move(result);
}


dynamic_buffer<int64>
	mesh_region::region_slow
		(const mesh& m, const si_transform& transform_m,
		 const box& b)
{
	// Transform each triangle into world coordinates,
	// and test each and every triangle against region.
	dynamic_buffer<int64> result;
	
	matrix matrix_m(transform_m.build_local_to_world());

	const mesh_stream_vector& vertices_m = m.vertices();
	const mesh_stream_triangle& triangles_m = m.triangles();

	for (int64 i = 0; i < m.triangle_count(); ++i)
	{
		if (b.contains(matrix_m * vertices_m.at(triangles_m.at(i).i)) &&
			b.contains(matrix_m * vertices_m.at(triangles_m.at(i).j)) &&
			b.contains(matrix_m * vertices_m.at(triangles_m.at(i).k)))
				result.push_back(i);
	}

	return std::move(result);
}




} /* namespace viral_core */
