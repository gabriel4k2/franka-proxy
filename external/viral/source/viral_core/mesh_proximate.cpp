/**
 *************************************************************************
 *
 * @file mesh_proximate.cpp
 *
 * Determine proximity of two meshes, implementation.
 *
 ************************************************************************/


#include "mesh_proximate.hpp"


#include "geo_distance.hpp"
#include "bounding_tree.hpp"
#include "bounding_proximate.hpp"
#include "bounding_proximate_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_proximate_test_triangle
//
//////////////////////////////////////////////////////////////////////////


mesh_proximate_test_triangle::mesh_proximate_test_triangle
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb.build_local_to_world())
{ }


bool mesh_proximate_test_triangle::node_proximate
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const sphere_tree_triangle& tb,
	 const sphere_tree_triangle_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	return bounding_proximate_util::shape_proximate
		(ta.shape(na), transform_ta_, to_world_matrix_ta_,
		 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
		 distance_squared_upper_bound,
		 evaluation_potential);
}


bool mesh_proximate_test_triangle::node_proximate
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const box_tree_triangle& tb,
	 const box_tree_triangle_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	return bounding_proximate_util::shape_proximate
		(ta.shape(na), to_world_matrix_ta_,
		 tb.shape(nb), to_world_matrix_tb_,
		 distance_squared_upper_bound,
		 evaluation_potential);
}


bool mesh_proximate_test_triangle::leaf_proximate
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const sphere_tree_triangle& tb,
	 const sphere_tree_triangle_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	// We must find a correct (expensive) distance
	// to determine proximate geometry objects.
	vector closest_point_ta;
	vector closest_point_tb;
	float closest_distance_squared =
		geo_distance::distance_squared
			(ta.object(na), to_world_matrix_ta_,
			 tb.object(nb), to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);


	// We return an exact distance_squared_upper_bound
	// as proximate, consistent to bounding_proximate_util.
	return closest_distance_squared <= distance_squared_upper_bound;
}


bool mesh_proximate_test_triangle::leaf_proximate
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const box_tree_triangle& tb,
	 const box_tree_triangle_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	// We must find a correct (expensive) distance
	// to determine proximate geometry objects.
	vector closest_point_ta;
	vector closest_point_tb;
	float closest_distance_squared =
		geo_distance::distance_squared
			(ta.object(na), to_world_matrix_ta_,
			 tb.object(nb), to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);


	// We return an exact distance_squared_upper_bound
	// as proximate, consistent to bounding_proximate_util.
	return closest_distance_squared <= distance_squared_upper_bound;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_proximate_test_index
//
//////////////////////////////////////////////////////////////////////////


mesh_proximate_test_index::mesh_proximate_test_index
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


bool mesh_proximate_test_index::node_proximate
	(const sphere_tree_index& ta,
	 const sphere_tree_index_const_node_iterator& na,
	 const sphere_tree_index& tb,
	 const sphere_tree_index_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	return bounding_proximate_util::shape_proximate
		(ta.shape(na), transform_ta_, to_world_matrix_ta_,
		 tb.shape(nb), transform_tb_, to_world_matrix_tb_,
		 distance_squared_upper_bound,
		 evaluation_potential);
}


bool mesh_proximate_test_index::node_proximate
	(const box_tree_index& ta,
	 const box_tree_index_const_node_iterator& na,
	 const box_tree_index& tb,
	 const box_tree_index_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential) const
{
	return bounding_proximate_util::shape_proximate
		(ta.shape(na), to_world_matrix_ta_,
		 tb.shape(nb), to_world_matrix_tb_,
		 distance_squared_upper_bound,
		 evaluation_potential);
}


bool mesh_proximate_test_index::leaf_proximate
	(const sphere_tree_index& ta,
	 const sphere_tree_index_const_node_iterator& na,
	 const sphere_tree_index& tb,
	 const sphere_tree_index_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	int64 triangle_index_ta = ta.object(na);
	int64 triangle_index_tb = tb.object(nb);

	const mesh_stream_vector& v_ta = mesh_ta_.vertices();
	const mesh_stream_triangle& t_ta = mesh_ta_.triangles();
	const mesh_stream_vector& v_tb = mesh_tb_.vertices();
	const mesh_stream_triangle& t_tb = mesh_tb_.triangles();


	// Checked access to ensure valid bounding_tree indices.
	// Later triangle distance anyways is much more expensive.
	triangle triangle_ta
		(v_ta.at(t_ta.at(triangle_index_ta).i),
		 v_ta.at(t_ta.at(triangle_index_ta).j),
		 v_ta.at(t_ta.at(triangle_index_ta).k));
	triangle triangle_tb
		(v_tb.at(t_tb.at(triangle_index_tb).i),
		 v_tb.at(t_tb.at(triangle_index_tb).j),
		 v_tb.at(t_tb.at(triangle_index_tb).k));


	// We must find a correct (expensive) distance
	// to determine proximate geometry objects.
	vector closest_point_ta;
	vector closest_point_tb;
	float closest_distance_squared =
		geo_distance::distance_squared
			(triangle_ta, to_world_matrix_ta_,
			 triangle_tb, to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);


	// We return an exact distance_squared_upper_bound
	// as proximate, consistent to bounding_proximate_util.
	return closest_distance_squared <= distance_squared_upper_bound;
}


bool mesh_proximate_test_index::leaf_proximate
	(const box_tree_index& ta,
	 const box_tree_index_const_node_iterator& na,
	 const box_tree_index& tb,
	 const box_tree_index_const_node_iterator& nb,
	 float distance_squared_upper_bound,
	 bounding_proximate_no_extended_result& extended_result) const
{
	int64 triangle_index_ta = ta.object(na);
	int64 triangle_index_tb = tb.object(nb);

	const mesh_stream_vector& v_ta = mesh_ta_.vertices();
	const mesh_stream_triangle& t_ta = mesh_ta_.triangles();
	const mesh_stream_vector& v_tb = mesh_tb_.vertices();
	const mesh_stream_triangle& t_tb = mesh_tb_.triangles();


	// Checked access to ensure valid bounding_tree indices.
	// Later triangle distance anyways is much more expensive.
	triangle triangle_ta
		(v_ta.at(t_ta.at(triangle_index_ta).i),
		 v_ta.at(t_ta.at(triangle_index_ta).j),
		 v_ta.at(t_ta.at(triangle_index_ta).k));
	triangle triangle_tb
		(v_tb.at(t_tb.at(triangle_index_tb).i),
		 v_tb.at(t_tb.at(triangle_index_tb).j),
		 v_tb.at(t_tb.at(triangle_index_tb).k));


	// We must find a correct (expensive) distance
	// to determine proximate geometry objects.
	vector closest_point_ta;
	vector closest_point_tb;
	float closest_distance_squared =
		geo_distance::distance_squared
			(triangle_ta, to_world_matrix_ta_,
			 triangle_tb, to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);


	// We return an exact distance_squared_upper_bound
	// as proximate, consistent to bounding_proximate_util.
	return closest_distance_squared <= distance_squared_upper_bound;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_proximate_test
//
//////////////////////////////////////////////////////////////////////////


mesh_proximate_result_sphere_tree_triangle
	mesh_proximate::proximate
		(const sphere_tree_triangle& ta, const si_transform& transform_ta,
		 const sphere_tree_triangle& tb, const si_transform& transform_tb,
		 float distance_squared_upper_bound)
{
	mesh_proximate_test_triangle test
		(transform_ta, transform_tb);

	bounding_proximate_sphere_triangle::proximate_heap heap;

	return
		bounding_proximate_sphere_triangle::proximate
			(ta, tb, test, heap,
			 distance_squared_upper_bound);
}


mesh_proximate_result_box_tree_triangle
	mesh_proximate::proximate
		(const box_tree_triangle& ta, const si_transform& transform_ta,
		 const box_tree_triangle& tb, const si_transform& transform_tb,
		 float distance_squared_upper_bound)
{
	mesh_proximate_test_triangle test
		(transform_ta, transform_tb);

	bounding_proximate_box_triangle::proximate_heap heap;

	return
		bounding_proximate_box_triangle::proximate
			(ta, tb, test, heap,
			 distance_squared_upper_bound);
}


mesh_proximate_result_sphere_tree_index
	mesh_proximate::proximate
		(const sphere_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const sphere_tree_index& tb, const mesh& mesh_tb,
		 const si_transform& transform_tb,			
		 float distance_squared_upper_bound)
{
	mesh_proximate_test_index test
		(mesh_ta, transform_ta, mesh_tb, transform_tb);

	bounding_proximate_sphere_index::proximate_heap heap;

	return
		bounding_proximate_sphere_index::proximate
			(ta, tb, test, heap,
			 distance_squared_upper_bound);
}


mesh_proximate_result_box_tree_index
	mesh_proximate::proximate
		(const box_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const box_tree_index& tb, const mesh& mesh_tb,
		 const si_transform& transform_tb,
		 float distance_squared_upper_bound)
{
	mesh_proximate_test_index test
		(mesh_ta, transform_ta, mesh_tb, transform_tb);

	bounding_proximate_box_index::proximate_heap heap;

	return
		bounding_proximate_box_index::proximate
			(ta, tb, test, heap,
			 distance_squared_upper_bound);
}


bool mesh_proximate::proximate_slow
	(const mesh& ma, const si_transform& transform_ma,
	 const mesh& mb, const si_transform& transform_mb,
	 float distance_squared_upper_bound)
{
	// Run through both meshes, transform triangles to world,
	// finally check for proximity over all triangle pairs.
	matrix matrix_ma(transform_ma.build_local_to_world());
	matrix matrix_mb(transform_mb.build_local_to_world());

	const mesh_stream_vector& vertices_ma = ma.vertices();
	const mesh_stream_vector& vertices_mb = mb.vertices();
	const mesh_stream_triangle& triangles_ma = ma.triangles();
	const mesh_stream_triangle& triangles_mb = mb.triangles();

	for (int64 a = 0; a < ma.triangle_count(); ++a)
	{
		triangle ta(matrix_ma * vertices_ma.at(triangles_ma.at(a).i),
					matrix_ma * vertices_ma.at(triangles_ma.at(a).j),
					matrix_ma * vertices_ma.at(triangles_ma.at(a).k));
		sphere sa(ta.build_bounding_sphere());

		for (int64 b = 0; b < mb.triangle_count(); ++b)
		{
			triangle tb(matrix_mb * vertices_mb.at(triangles_mb.at(b).i),
						matrix_mb * vertices_mb.at(triangles_mb.at(b).j),
						matrix_mb * vertices_mb.at(triangles_mb.at(b).k));
			sphere sb(tb.build_bounding_sphere());


			// Minor optimization:
			// Discard distant triangles with bounding sphere pre-test.
			// We need a compare against zero for later squared tests.
			float min_sphere_distance =
				(sa.position - sb.position).length() - (sa.radius + sb.radius);
			if (min_sphere_distance > 0 &&
				min_sphere_distance * min_sphere_distance > distance_squared_upper_bound)
					continue;


			// Check transformed triangles
			// for exact closest-point distance.
			vector tri_closest_point_a, tri_closest_point_b;
			float tri_closest_distance =
				geo_distance::distance
					(ta, tb, tri_closest_point_a, tri_closest_point_b);

			if (tri_closest_distance * tri_closest_distance > distance_squared_upper_bound)
				continue;

	
			// Found proximate points below distance_squared_upper_bound.
			return true;			
		}
	}


	// There are no proximate triangles
	// below distance_squared_upper_bound.
	return false;
}




} /* namespace viral_core */
