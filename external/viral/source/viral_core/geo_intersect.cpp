/**
 *************************************************************************
 *
 * @file geo_intersect.cpp
 *
 * Intersection tests for geometric primitives, implementation.
 *
 ************************************************************************/


#include "geo_intersect.hpp"


#include <math.h>


#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// geo_intersect
//
//////////////////////////////////////////////////////////////////////////


bool geo_intersect::intersecting
	(const triangle& ta, const triangle& tb) NOTHROW
{
	// Degenerate triangles can never intersect.
	// This avoids robustness issues on separating axes.
	if (ta.degenerate() || tb.degenerate())
		return false;


	// Test both plane normals as separating axes.
	// These already exclude many intersections
	// and are readily available.
	float signed_dist_ta_to_tbp[3] =
		{ tb.p.signed_distance_to(ta.a),
		  tb.p.signed_distance_to(ta.b),
		  tb.p.signed_distance_to(ta.c) };

	if (signed_dist_ta_to_tbp[0] > intersect_triangle_plane_epsilon && 
		signed_dist_ta_to_tbp[1] > intersect_triangle_plane_epsilon &&
		signed_dist_ta_to_tbp[2] > intersect_triangle_plane_epsilon)
			return false;

	if (signed_dist_ta_to_tbp[0] < -intersect_triangle_plane_epsilon && 
		signed_dist_ta_to_tbp[1] < -intersect_triangle_plane_epsilon &&
		signed_dist_ta_to_tbp[2] < -intersect_triangle_plane_epsilon)
			return false;

	float signed_dist_tb_to_tap[3] =
		{ ta.p.signed_distance_to(tb.a),
		  ta.p.signed_distance_to(tb.b),
		  ta.p.signed_distance_to(tb.c) };

	if (signed_dist_tb_to_tap[0] > intersect_triangle_plane_epsilon && 
		signed_dist_tb_to_tap[1] > intersect_triangle_plane_epsilon &&
		signed_dist_tb_to_tap[2] > intersect_triangle_plane_epsilon)
			return false;

	if (signed_dist_tb_to_tap[0] < -intersect_triangle_plane_epsilon && 
		signed_dist_tb_to_tap[1] < -intersect_triangle_plane_epsilon &&
		signed_dist_tb_to_tap[2] < -intersect_triangle_plane_epsilon)
			return false;


	// Test the remaining candidates for separating axes.
	// All of these are perpendicular to edge-edge pairs.
	vector ta_vertices[3] =
		{ ta.a, ta.b, ta.c };
	vector tb_vertices[3] =
		{ tb.a, tb.b, tb.c };

	prepare_separating_axis
		(ta_vertices, 3, tb_vertices, 3);

	vector ta_edges[3] =
		{ ta.a - ta.c, ta.b - ta.a, ta.c - ta.b };
	vector tb_edges[3] =
		{ tb.a - tb.c, tb.b - tb.a, tb.c - tb.b };

	for (int64 ea = 0; ea < 3; ++ea)
	for (int64 eb = 0; eb < 3; ++eb)
	{
		if (is_separating_axis
				(ta_vertices, 3, tb_vertices, 3,
				 ta_edges[ea].cross(tb_edges[eb])))
						return false;
	}


	// We have overlap on all relevant axes,
	// so there must be an intersection.
	return true;
}


bool geo_intersect::intersecting
	(const triangle& ta, const matrix& transform_ta,
	 const triangle& tb, const matrix& transform_tb) NOTHROW
{
	return intersecting
				(ta.transform(transform_ta),
				 tb.transform(transform_tb));
}


bool geo_intersect::intersecting
	(const triangle& t, const box& b) NOTHROW
{
	if (b.empty()) return false;

	if (t.degenerate())
		return b.contains((t.a + t.b + t.c) / 3.f);


	// Prepare separating axis tests:
	// - Build and condition point clouds.
	// - Determine relevant axes.
	vector t_vertices[3] =
		{ t.a, t.b, t.c };
	vector t_segments[3] =
		{ t_vertices[1] - t_vertices[0],
		  t_vertices[2] - t_vertices[1],
		  t_vertices[0] - t_vertices[2] };

	vector b_size[3] =
		{ vector(b.max.x - b.min.x, 0, 0), 
		  vector(0, b.max.y - b.min.y, 0), 
		  vector(0, 0, b.max.z - b.min.z) };
	vector b_vertices[8] =
		{ b.min,
		  b.min + b_size[0],
		  b.min + b_size[1],
		  b.min + b_size[2],
		  b.min + b_size[0] + b_size[1],
		  b.min + b_size[1] + b_size[2],
		  b.min + b_size[2] + b_size[0],
		  b.min + b_size[0] + b_size[1] + b_size[2] };
	
	prepare_separating_axis
		(t_vertices, 3, b_vertices, 8);


	// Check each axis and all axis pairs for separation.
	// Note we need only check the three major box axes.
	for (int64 i = 0; i < 3; ++i)
	{
		if (is_separating_axis
				(t_vertices, 3, b_vertices, 8, t_segments[i]))
						return false;

		if (is_separating_axis
				(t_vertices, 3, b_vertices, 8, b_size[i]))
						return false;
	}

	for (int64 i = 0; i < 3; ++i)
	for (int64 j = 0; j < 3; ++j)
	{
		if (is_separating_axis
				(t_vertices, 3, b_vertices, 8,
				 t_segments[i].cross(b_size[j])))
						return false;
	}

	return true;
}


bool geo_intersect::intersecting
	(const triangle& t, const matrix& transform_t,
	 const box& b, const si_transform& transform_b) NOTHROW
{
	if (b.empty()) return false;

	if (t.degenerate())
	{
		vector midpoint((t.a + t.b + t.c) / 3.f);
		vector midpoint_transformed
			(transform_b.invert() * (transform_t * midpoint));

		return b.contains(midpoint_transformed);
	}


	// Build transformation utilities for box.
	matrix b_rotation_matrix
		(matrix::rotate(transform_b.orientation));
	vector b_axes_in_target[3] =
		{ b_rotation_matrix.x_axis(),
		  b_rotation_matrix.y_axis(),
		  b_rotation_matrix.z_axis() };


	// Prepare separating axis tests:
	// - Build and condition point clouds.
	// - Determine relevant axes.
	vector t_vertices_in_target[3] =
		{ transform_t * t.a,
		  transform_t * t.b,
		  transform_t * t.c };
	vector t_segments_in_target[3] =
		{ t_vertices_in_target[1] - t_vertices_in_target[0],
		  t_vertices_in_target[2] - t_vertices_in_target[1],
		  t_vertices_in_target[0] - t_vertices_in_target[2] };

	vector b_min_in_target(transform_b * b.min);
	vector b_size_in_target[3] =
		{ b_axes_in_target[0] * (b.max.x - b.min.x) * transform_b.scale,
		  b_axes_in_target[1] * (b.max.y - b.min.y) * transform_b.scale,
		  b_axes_in_target[2] * (b.max.z - b.min.z) * transform_b.scale };
	vector b_vertices_in_target[8] =
		{ b_min_in_target,
		  b_min_in_target + b_size_in_target[0],
		  b_min_in_target + b_size_in_target[1],
		  b_min_in_target + b_size_in_target[2],
		  b_min_in_target + b_size_in_target[0] + b_size_in_target[1],
		  b_min_in_target + b_size_in_target[1] + b_size_in_target[2],
		  b_min_in_target + b_size_in_target[2] + b_size_in_target[0],
		  b_min_in_target + b_size_in_target[0] + b_size_in_target[1] + b_size_in_target[2] };
	
	prepare_separating_axis
		(t_vertices_in_target, 3,
		 b_vertices_in_target, 8);


	// Check each axis and all axis pairs for separation.
	// Note we need only check the three major box axes.
	for (int64 i = 0; i < 3; ++i)
	{
		if (is_separating_axis
				(t_vertices_in_target, 3,
				 b_vertices_in_target, 8,
				 t_segments_in_target[i]))
						return false;

		if (is_separating_axis
				(t_vertices_in_target, 3,
				 b_vertices_in_target, 8,
				 b_size_in_target[i]))
						return false;
	}

	for (int64 i = 0; i < 3; ++i)
	for (int64 j = 0; j < 3; ++j)
	{
		if (is_separating_axis
				(t_vertices_in_target, 3,
				 b_vertices_in_target, 8,
				 t_segments_in_target[i].cross(b_size_in_target[j])))
						return false;
	}

	return true;
}


bool geo_intersect::intersecting
	(const sphere& sa, const sphere& sb) NOTHROW
		{ return sa.intersecting(sb); }

bool geo_intersect::intersecting
	(const sphere& sa, const si_transform& transform_sa,
	 const sphere& sb, const si_transform& transform_sb) NOTHROW
{
	// Early out: Empty spheres never intersect.
	// We must handle empty() spheres explicitly
	// to avoid frequent false positive results
	// due to non-empty() transform_and_bound().
	if (sa.empty() || sb.empty())
		return false;


	// Transform non-empty() spheres to world space.
	// This will give exact, non-empty() transformed spheres.
	// Then test world-space spheres for intersection.
	return intersecting
				(sa.transform_and_bound(transform_sa),
				 sb.transform_and_bound(transform_sb));
}


bool geo_intersect::intersecting
	(const sphere& s, const box& b) NOTHROW
{
	// Early out: Empty primitives never intersect.
	if (s.empty() || b.empty())
		return false;


	// Check for intersection:
	// Closest point on box must be closer
	// to sphere than the sphere radius.
	// Avoid sqrtf() by squared distance.
	vector closest_on_b
		(s.position.min(b.max).max(b.min));

	float distance_squared =
		(s.position - closest_on_b).square_sum();
	
	return
		distance_squared <= s.radius * s.radius;
}


bool geo_intersect::intersecting
	(const box& ba, const box& bb) NOTHROW
		{ return ba.intersecting(bb); }

bool geo_intersect::intersecting
	(const box& ba, const si_transform& transform_ba,
	 const box& bb, const si_transform& transform_bb) NOTHROW
{
	// Early out: Empty boxes never intersect.
	// We must handle this condition explicitly,
	// as overlapping axis projections may still 
	// cause frequent false-positive results.
	if (ba.empty() || bb.empty())
		return false;


	// Find unit-length axes of both boxes in target space.
	// We need these multiple times, e.g. to find box vertices
	// and as candidates for separating axes.
	matrix ba_rotation_matrix
		(matrix::rotate(transform_ba.orientation));
	vector ba_axes_in_target[3] =
		{ ba_rotation_matrix.x_axis(),
		  ba_rotation_matrix.y_axis(),
		  ba_rotation_matrix.z_axis() };

	matrix bb_rotation_matrix
		(matrix::rotate(transform_bb.orientation));
	vector bb_axes_in_target[3] =
		{ bb_rotation_matrix.x_axis(),
		  bb_rotation_matrix.y_axis(),
		  bb_rotation_matrix.z_axis() };


	// Find any vertex (not necessarily the minimum)
	// on each oriented bounding box in target space.
	// Then find the span vectors from these vertices
	// and calculate all remaining box vertices.
	// Finally, precondition vertices for later tests.
	vector ba_min_in_target(transform_ba * ba.min);
	vector ba_size_in_target[3] =
		{ ba_axes_in_target[0] * (ba.max.x - ba.min.x) * transform_ba.scale,
		  ba_axes_in_target[1] * (ba.max.y - ba.min.y) * transform_ba.scale,
		  ba_axes_in_target[2] * (ba.max.z - ba.min.z) * transform_ba.scale };
	vector ba_vertices_in_target[8] =
		{ ba_min_in_target,
		  ba_min_in_target + ba_size_in_target[0],
		  ba_min_in_target + ba_size_in_target[1],
		  ba_min_in_target + ba_size_in_target[2],
		  ba_min_in_target + ba_size_in_target[0] + ba_size_in_target[1],
		  ba_min_in_target + ba_size_in_target[1] + ba_size_in_target[2],
		  ba_min_in_target + ba_size_in_target[2] +	ba_size_in_target[0],
		  ba_min_in_target + ba_size_in_target[0] + ba_size_in_target[1] + ba_size_in_target[2] };

	vector bb_min_in_target(transform_bb * bb.min);
	vector bb_size_in_target[3] =
		{ bb_axes_in_target[0] * (bb.max.x - bb.min.x) * transform_bb.scale,
		  bb_axes_in_target[1] * (bb.max.y - bb.min.y) * transform_bb.scale,
		  bb_axes_in_target[2] * (bb.max.z - bb.min.z) * transform_bb.scale };
	vector bb_vertices_in_target[8] =
		{ bb_min_in_target,
		  bb_min_in_target + bb_size_in_target[0],
		  bb_min_in_target + bb_size_in_target[1],
		  bb_min_in_target + bb_size_in_target[2],
		  bb_min_in_target + bb_size_in_target[0] + bb_size_in_target[1],
		  bb_min_in_target + bb_size_in_target[1] + bb_size_in_target[2],
		  bb_min_in_target + bb_size_in_target[2] +	bb_size_in_target[0],
		  bb_min_in_target + bb_size_in_target[0] + bb_size_in_target[1] + bb_size_in_target[2] };

	prepare_separating_axis
		(ba_vertices_in_target, 8,
		 bb_vertices_in_target, 8);
		

	// Use separating axis test to exclude intersection.
	// Use box axes first, as we already have these.
	for (int64 f = 0; f < 3; ++f)
	{
		if (is_separating_axis
				(ba_vertices_in_target, 8,
				 bb_vertices_in_target, 8,
				 ba_axes_in_target[f]))
						return false;			

		if (is_separating_axis
				(ba_vertices_in_target, 8,
				 bb_vertices_in_target, 8,
				 bb_axes_in_target[f]))
						return false;
	}


	// Test all axes that are perpendicular to pairs
	// of box edges (and thus, box axes) to exclude
	// edge-edge intersection.
	for (int64 a = 0; a < 3; ++a)
	for (int64 b = 0; b < 3; ++b)
	{
		vector axis
			(ba_axes_in_target[a].cross(bb_axes_in_target[b]));

		if (is_separating_axis
				(ba_vertices_in_target, 8,
				 bb_vertices_in_target, 8, axis))
						return false;
	}


	// We have overlap on all relevant axes,
	// so there must be an intersection.
	return true;
}


bool geo_intersect::is_separating_axis
	(const vector* points_a, int64 point_count_a,
	 const vector* points_b, int64 point_count_b,
	 const vector& axis) NOTHROW
{
	// Bias to false-positive separation on degenerate axis.
	// See method documentation for rationale.
	if (axis.absmax() < intersect_separating_axis_degenerate_epsilon)
		return true;


	// Ensure there is at least one point per set.
	if (!points_a || !points_b)
		return false;

	if (point_count_a <= 0 || point_count_b <= 0)
		return false;


	// Determine axis interval for each set:
	// Project points onto the axis candidate,
	// then track minimum and maximum offset.
	// Note that axis scaling is not relevant,
	// as we only compare resulting intervals.
	float a_min = points_a[0].dot(axis);
	float a_max = a_min;

	for (int64 i = 1; i < point_count_a; ++i)
	{
		float dot = points_a[i].dot(axis);
		a_min = min(a_min, dot);
		a_max = max(a_max, dot);
	}

	float b_min = points_b[0].dot(axis);
	float b_max = b_min;

	for (int64 i = 1; i < point_count_b; ++i)
	{
		float dot = points_b[i].dot(axis);
		b_min = min(b_min, dot);
		b_max = max(b_max, dot);
	}


	// Check whether axis intervals are separated.
	// We add an epsilon bias for false-positive intersection,
	// which corresponds to false-negative separation.
	return
		a_min - b_max > intersect_separating_axis_interval_epsilon ||
		b_min - a_max > intersect_separating_axis_interval_epsilon;
}


void geo_intersect::prepare_separating_axis
	(vector* points_a, int64 point_count_a,
	 vector* points_b, int64 point_count_b) NOTHROW
{
	if (point_count_a == 0 &&
		point_count_b == 0)
			return;

	// Center incoming points.
	vector average;
	for (int64 i = 0; i < point_count_a; ++i)
		average += points_a[i];
	for (int64 i = 0; i < point_count_b; ++i)
		average += points_b[i];

	average /=
		(static_cast<float>(point_count_a) +
		 static_cast<float>(point_count_b));

	for (int64 i = 0; i < point_count_a; ++i)
		points_a[i] -= average;
	for (int64 i = 0; i < point_count_b; ++i)
		points_b[i] -= average;
}


const float geo_intersect::intersect_triangle_plane_epsilon = 1e-5f;

const float geo_intersect::intersect_separating_axis_degenerate_epsilon = 1e-10f;
const float geo_intersect::intersect_separating_axis_interval_epsilon = 1e-5f;




} /* namespace viral_core */
