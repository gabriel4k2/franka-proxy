/**
 *************************************************************************
 *
 * @file mesh_distance.cpp
 *
 * Distance between meshes, implementation.
 *
 ************************************************************************/


#include "mesh_distance.hpp"


#include "geo_distance.hpp"
#include "bounding_tree.hpp"
#include "bounding_distance.hpp"
#include "bounding_distance_util.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_distance_metric_triangle
//
//////////////////////////////////////////////////////////////////////////


mesh_distance_metric_triangle::mesh_distance_metric_triangle
	(const si_transform& transform_ta,
	 const si_transform& transform_tb) NOTHROW
	:
	transform_ta_(transform_ta),
	to_world_matrix_ta_(transform_ta.build_local_to_world()),
	transform_tb_(transform_tb),
	to_world_matrix_tb_(transform_tb.build_local_to_world())
{ }


bool mesh_distance_metric_triangle::try_node_distance_squared
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const sphere_tree_triangle& tb,
	 const sphere_tree_triangle_const_node_iterator& nb,
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


bool mesh_distance_metric_triangle::try_node_distance_squared
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const box_tree_triangle& tb,
	 const box_tree_triangle_const_node_iterator& nb,
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


bool mesh_distance_metric_triangle::try_leaf_distance_squared
	(const sphere_tree_triangle& ta,
	 const sphere_tree_triangle_const_node_iterator& na,
	 const sphere_tree_triangle& tb,
	 const sphere_tree_triangle_const_node_iterator& nb,
	 vector& closest_point_ta, vector& closest_point_tb,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
{
	closest_distance_squared =
		geo_distance::distance_squared
			(ta.object(na), to_world_matrix_ta_,
			 tb.object(nb), to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);

	return true;
}


bool mesh_distance_metric_triangle::try_leaf_distance_squared
	(const box_tree_triangle& ta,
	 const box_tree_triangle_const_node_iterator& na,
	 const box_tree_triangle& tb,
	 const box_tree_triangle_const_node_iterator& nb,
	 vector& closest_point_ta, vector& closest_point_tb,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
{
	closest_distance_squared =
		geo_distance::distance_squared
			(ta.object(na), to_world_matrix_ta_,
			 tb.object(nb), to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_distance_metric_index
//
//////////////////////////////////////////////////////////////////////////
	

mesh_distance_metric_index::mesh_distance_metric_index
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


bool mesh_distance_metric_index::try_node_distance_squared
	(const sphere_tree_index& ta,
	 const sphere_tree_index_const_node_iterator& na,
	 const sphere_tree_index& tb,
	 const sphere_tree_index_const_node_iterator& nb,
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


bool mesh_distance_metric_index::try_node_distance_squared
	(const box_tree_index& ta,
	 const box_tree_index_const_node_iterator& na,
	 const box_tree_index& tb,
	 const box_tree_index_const_node_iterator& nb,
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


bool mesh_distance_metric_index::try_leaf_distance_squared
	(const sphere_tree_index& ta,
	 const sphere_tree_index_const_node_iterator& na,
	 const sphere_tree_index& tb,
	 const sphere_tree_index_const_node_iterator& nb,
	 vector& closest_point_ta, vector& closest_point_tb,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
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

	closest_distance_squared =
		geo_distance::distance_squared
			(triangle_ta, to_world_matrix_ta_,
			 triangle_tb, to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);

	return true;
}


bool mesh_distance_metric_index::try_leaf_distance_squared
	(const box_tree_index& ta,
	 const box_tree_index_const_node_iterator& na,
	 const box_tree_index& tb,
	 const box_tree_index_const_node_iterator& nb,
	 vector& closest_point_ta, vector& closest_point_tb,
	 float& closest_distance_squared,
	 bounding_distance_no_extended_result& extended_result) const
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

	closest_distance_squared =
		geo_distance::distance_squared
			(triangle_ta, to_world_matrix_ta_,
			 triangle_tb, to_world_matrix_tb_,
			 closest_point_ta, closest_point_tb);

	return true;
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_distance
//
//////////////////////////////////////////////////////////////////////////


mesh_distance_result_sphere_tree_triangle
	mesh_distance::distance
		(const sphere_tree_triangle& ta, const si_transform& transform_ta,
		 const sphere_tree_triangle& tb, const si_transform& transform_tb,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	mesh_distance_metric_triangle metric
		(transform_ta, transform_tb);

	bounding_distance_sphere_triangle::distance_heap heap;

	return
		bounding_distance_sphere_triangle::distance
			(ta, tb, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


mesh_distance_result_box_tree_triangle
	mesh_distance::distance
		(const box_tree_triangle& ta, const si_transform& transform_ta,
		 const box_tree_triangle& tb, const si_transform& transform_tb,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	mesh_distance_metric_triangle metric
		(transform_ta, transform_tb);

	bounding_distance_box_triangle::distance_heap heap;

	return
		bounding_distance_box_triangle::distance
			(ta, tb, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


mesh_distance_result_sphere_tree_index
	mesh_distance::distance
		(const sphere_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const sphere_tree_index& tb, const mesh& mesh_tb,
		 const si_transform& transform_tb,			
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	mesh_distance_metric_index metric
		(mesh_ta, transform_ta, mesh_tb, transform_tb);

	bounding_distance_sphere_index::distance_heap heap;

	return
		bounding_distance_sphere_index::distance
			(ta, tb, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


mesh_distance_result_box_tree_index
	mesh_distance::distance
		(const box_tree_index& ta, const mesh& mesh_ta,
		 const si_transform& transform_ta,
		 const box_tree_index& tb, const mesh& mesh_tb,
		 const si_transform& transform_tb,
		 float distance_squared_bound,
		 bool distance_squared_bound_valid,
		 float distance_squared_tolerance)
{
	mesh_distance_metric_index metric
		(mesh_ta, transform_ta, mesh_tb, transform_tb);

	bounding_distance_box_index::distance_heap heap;

	return
		bounding_distance_box_index::distance
			(ta, tb, metric, heap,
			 distance_squared_bound,
			 distance_squared_bound_valid,
			 distance_squared_tolerance);
}


bool mesh_distance::try_distance_slow
	(const mesh& ma, const si_transform& transform_ma,
	 const mesh& mb, const si_transform& transform_mb,
	 vector& closest_point_ma, vector& closest_point_mb,
	 float& closest_distance)
{
	// Reset in-out parameters just in case
	// we should early out or throw.
	closest_point_ma = vector();
	closest_point_mb = vector();
	closest_distance = 0;

	if (ma.triangle_count() == 0 ||
		mb.triangle_count() == 0)
			return false;


	// Run through both meshes, transform triangles to world,
	// finally check for closest points over all triangle pairs.
	// Store results in offhand variables for exception safety.
	vector offhand_closest_point_ma;
	vector offhand_closest_point_mb;
	float offhand_closest_distance = 0;

	matrix matrix_ma(transform_ma.build_local_to_world());
	matrix matrix_mb(transform_mb.build_local_to_world());

	const mesh_stream_vector& vertices_ma = ma.vertices();
	const mesh_stream_vector& vertices_mb = mb.vertices();
	const mesh_stream_triangle& triangles_ma = ma.triangles();
	const mesh_stream_triangle& triangles_mb = mb.triangles();

	bool first_tri = true;
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
			// Discard out-of-reach triangles with bounding sphere pre-test.
			if (!first_tri)
			{
				float min_sphere_distance =
					(sa.position - sb.position).length() - (sa.radius + sb.radius);
				if (min_sphere_distance > offhand_closest_distance)
					continue;
			}
		

			// Check transformed triangles
			// for closest-point distance.
			vector tri_closest_point_ma, tri_closest_point_mb;
			float tri_closest_distance =
				geo_distance::distance
					(ta, tb, tri_closest_point_ma, tri_closest_point_mb);

			if (first_tri || tri_closest_distance < offhand_closest_distance)
			{
				offhand_closest_distance = tri_closest_distance;
				offhand_closest_point_ma = tri_closest_point_ma;
				offhand_closest_point_mb = tri_closest_point_mb;
				first_tri = false;
			}
		}
	}


	// Found at least one closest-point pair,
	// so send this result back to caller.
	// Exception safety: NOTHROW from here.
	closest_distance = offhand_closest_distance;
	closest_point_ma = offhand_closest_point_ma;
	closest_point_mb = offhand_closest_point_mb;

	return true;
}




} /* namespace viral_core */
