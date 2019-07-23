/**
 *************************************************************************
 *
 * @file mesh_intersect.cpp
 *
 * Determine if two meshes intersect, implementation.
 *
 ************************************************************************/


#include "mesh_intersect.hpp"


#include "geo_intersect.hpp"
#include "bounding_tree.hpp"
#include "bounding_intersect.hpp"
#include "bounding_intersect_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_intersect_test_triangle
//
//////////////////////////////////////////////////////////////////////////


mesh_intersect_test_triangle::mesh_intersect_test_triangle
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb.build_local_to_world())
{ }


bool mesh_intersect_test_triangle::node_intersecting
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const sphere_tree_triangle& tb,
	 const sphere_tree_triangle_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return bounding_intersect_util::shape_intersecting
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_,
				 evaluation_potential);
}


bool mesh_intersect_test_triangle::node_intersecting
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const box_tree_triangle& tb,
	 const box_tree_triangle_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return bounding_intersect_util::shape_intersecting
				(ta.shape(na), transform_ta_, to_world_matrix_ta_,
				 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
				 evaluation_potential);
}


bool mesh_intersect_test_triangle::leaf_intersecting
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const sphere_tree_triangle& tb,
	 const sphere_tree_triangle_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	return
		geo_intersect::intersecting
			(ta.object(na), to_world_matrix_ta_,
			 tb.object(nb), to_world_matrix_tb_);
}


bool mesh_intersect_test_triangle::leaf_intersecting
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const box_tree_triangle& tb,
	 const box_tree_triangle_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	return
		geo_intersect::intersecting
			(ta.object(na), to_world_matrix_ta_,
			 tb.object(nb), to_world_matrix_tb_);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_intersect_test_index
//
//////////////////////////////////////////////////////////////////////////


mesh_intersect_test_index::mesh_intersect_test_index
	(const mesh& mesh_ta,
	 const si_transform& transform_ta,
	 const mesh& mesh_tb,
	 const si_transform& transform_tb) NOTHROW
	:
	mesh_ta_(mesh_ta), transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta_.build_local_to_world()),
	mesh_tb_(mesh_tb), transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb_.build_local_to_world())
{ }


bool mesh_intersect_test_index::node_intersecting
	(const sphere_tree_index& ta,
	 const sphere_tree_index_const_node_iterator& na,
	 const sphere_tree_index& tb,
	 const sphere_tree_index_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return bounding_intersect_util::shape_intersecting
				(ta.shape(na), transform_ta_,
				 tb.shape(nb), transform_tb_,
				 evaluation_potential);
}


bool mesh_intersect_test_index::node_intersecting
	(const box_tree_index& ta,
	 const box_tree_index_const_node_iterator& na,
	 const box_tree_index& tb,
	 const box_tree_index_const_node_iterator& nb,
	 float& evaluation_potential) const
{
	return bounding_intersect_util::shape_intersecting
				(ta.shape(na), transform_ta_, to_world_matrix_ta_,
				 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
				 evaluation_potential);
}


bool mesh_intersect_test_index::leaf_intersecting
	(const sphere_tree_index& ta,
	 const sphere_tree_index_const_node_iterator& na,
	 const sphere_tree_index& tb,
	 const sphere_tree_index_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	int64 triangle_index_ta = ta.object(na);
	int64 triangle_index_tb = tb.object(nb);

	const mesh_stream_vector& v_ta = mesh_ta_.vertices();
	const mesh_stream_triangle& t_ta = mesh_ta_.triangles();
	const mesh_stream_vector& v_tb = mesh_tb_.vertices();
	const mesh_stream_triangle& t_tb = mesh_tb_.triangles();

	// Checked access to ensure valid bounding_tree indices.
	// Later triangle intersection anyways is much more expensive.
	triangle triangle_ta
		(v_ta.at(t_ta.at(triangle_index_ta).i),
		 v_ta.at(t_ta.at(triangle_index_ta).j),
		 v_ta.at(t_ta.at(triangle_index_ta).k));
	triangle triangle_tb
		(v_tb.at(t_tb.at(triangle_index_tb).i),
		 v_tb.at(t_tb.at(triangle_index_tb).j),
		 v_tb.at(t_tb.at(triangle_index_tb).k));

	return
		geo_intersect::intersecting
			(triangle_ta, to_world_matrix_ta_,
			 triangle_tb, to_world_matrix_tb_);
}


bool mesh_intersect_test_index::leaf_intersecting
	(const box_tree_index& ta,
	 const box_tree_index_const_node_iterator& na,
	 const box_tree_index& tb,
	 const box_tree_index_const_node_iterator& nb,
	 bounding_intersect_no_extended_result& extended_result) const
{
	int64 triangle_index_ta = ta.object(na);
	int64 triangle_index_tb = tb.object(nb);

	const mesh_stream_vector& v_ta = mesh_ta_.vertices();
	const mesh_stream_triangle& t_ta = mesh_ta_.triangles();
	const mesh_stream_vector& v_tb = mesh_tb_.vertices();
	const mesh_stream_triangle& t_tb = mesh_tb_.triangles();

	// Checked access to ensure valid bounding_tree indices.
	// Later triangle intersection anyways is much more expensive.
	triangle triangle_ta
		(v_ta.at(t_ta.at(triangle_index_ta).i),
		 v_ta.at(t_ta.at(triangle_index_ta).j),
		 v_ta.at(t_ta.at(triangle_index_ta).k));
	triangle triangle_tb
		(v_tb.at(t_tb.at(triangle_index_tb).i),
		 v_tb.at(t_tb.at(triangle_index_tb).j),
		 v_tb.at(t_tb.at(triangle_index_tb).k));

	return
		geo_intersect::intersecting
			(triangle_ta, to_world_matrix_ta_,
			 triangle_tb, to_world_matrix_tb_);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_intersect
//
//////////////////////////////////////////////////////////////////////////


mesh_intersect_result_sphere_tree_triangle
	mesh_intersect::intersect
		(const sphere_tree_triangle& ta, const si_transform& transform_ta,
		 const sphere_tree_triangle& tb, const si_transform& transform_tb)
{
	mesh_intersect_test_triangle test
		(transform_ta, transform_tb);

	bounding_intersect_sphere_triangle::intersect_heap heap;

	return
		bounding_intersect_sphere_triangle::intersect
			(ta, tb, test, heap);
}


mesh_intersect_result_box_tree_triangle
	mesh_intersect::intersect
		(const box_tree_triangle& ta, const si_transform& transform_ta,
		 const box_tree_triangle& tb, const si_transform& transform_tb)
{ 
	mesh_intersect_test_triangle test
		(transform_ta, transform_tb);

	bounding_intersect_box_triangle::intersect_heap heap;

	return
		bounding_intersect_box_triangle::intersect
			(ta, tb, test, heap);
}


mesh_intersect_result_sphere_tree_index
	mesh_intersect::intersect
		(const sphere_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const sphere_tree_index& tb, const mesh& mesh_tb,
		 const si_transform& transform_tb)
{
	mesh_intersect_test_index test
		(mesh_ta, transform_ta,
		 mesh_tb, transform_tb);

	bounding_intersect_sphere_index::intersect_heap heap;

	return
		bounding_intersect_sphere_index::intersect
			(ta, tb, test, heap);
}


mesh_intersect_result_box_tree_index
	mesh_intersect::intersect
		(const box_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const box_tree_index& tb, const mesh& mesh_tb,
		 const si_transform& transform_tb)
{
	mesh_intersect_test_index test
		(mesh_ta, transform_ta,
		 mesh_tb, transform_tb);

	bounding_intersect_box_index::intersect_heap heap;

	return
		bounding_intersect_box_index::intersect
			(ta, tb, test, heap);
}


bool mesh_intersect::intersecting_slow
	(const mesh& ma, const si_transform& transform_ma,
	 const mesh& mb, const si_transform& transform_mb)
{
	// Transform each triangle into world coordinates,
	// and test each and every triangle pair for intersection.
	// Early out on the first intersection.
	matrix matrix_ma(transform_ma.build_local_to_world());
	matrix matrix_mb(transform_mb.build_local_to_world());

	const mesh_stream_vector& vertices_ma = ma.vertices();
	const mesh_stream_vector& vertices_mb = mb.vertices();
	const mesh_stream_triangle& triangles_ma = ma.triangles();
	const mesh_stream_triangle& triangles_mb = mb.triangles();

	for (int64 a = 0; a < ma.triangle_count(); ++a)
	{
		triangle ta(vertices_ma.at(triangles_ma.at(a).i),
					vertices_ma.at(triangles_ma.at(a).j),
					vertices_ma.at(triangles_ma.at(a).k));
		sphere sa(ta.build_bounding_sphere());

		for (int64 b = 0; b < mb.triangle_count(); ++b)
		{
			triangle tb(vertices_mb.at(triangles_mb.at(b).i),
						vertices_mb.at(triangles_mb.at(b).j),
						vertices_mb.at(triangles_mb.at(b).k));
			sphere sb(tb.build_bounding_sphere());

			if (geo_intersect::intersecting
					(ta, matrix_ma, tb, matrix_mb))
						return true;
		}
	}


	// No intersecting triangle pair found.
	return false;
}




} /* namespace viral_core */
