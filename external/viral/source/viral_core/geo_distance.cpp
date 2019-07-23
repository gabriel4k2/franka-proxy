/**
 *************************************************************************
 *
 * @file geo_distance.cpp
 *
 * Geometry distance utilities, implementation.
 *
 ************************************************************************/


#include "geo_distance.hpp"


#include <math.h>
#include <memory>


#include "geo_transform.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class geo_distance_line
 * @ingroup geo_module
 *
 * Stores a pre-normalized line for use with geo_distance.
 *
 ************************************************************************/
class geo_distance_line
{
	friend class geo_distance;
	friend class geo_distance_segment;
	friend class geo_distance_triangle;


	public:

		geo_distance_line
			(const vector& origin, const vector& normalized_direction) NOTHROW
			:
			origin_(origin),
			normalized_direction_(normalized_direction)
		{ }


		explicit geo_distance_line(const line& other) NOTHROW
			:
			origin_(other.origin),
			normalized_direction_(other.direction.normalized())
		{ }


	private:

		vector origin_;
		vector normalized_direction_;
};




/**
 *************************************************************************
 *
 * @class geo_distance_segment
 * @ingroup geo_module
 *
 * Stores a pre-normalized line segment for use within geo_distance.
 *
 ************************************************************************/
class geo_distance_segment
{
	friend class geo_distance;
	friend class geo_distance_triangle;


	public:

		geo_distance_segment
			(const vector& start, const vector& end) NOTHROW
			:
			start_to_end_(end - start),
			length_squared_(start_to_end_.x * start_to_end_.x +
						    start_to_end_.y * start_to_end_.y +
						    start_to_end_.z * start_to_end_.z),
			length_(sqrtf(length_squared_)),
			end_(end),
			line_(start,
				  length_ > vector::normalize_epsilon ?
						start_to_end_ * (1.f / length_) : vector())
		{ }


		explicit geo_distance_segment(const segment& other) NOTHROW
			:
			start_to_end_(other.end - other.start),
			length_squared_(start_to_end_.x * start_to_end_.x +
						    start_to_end_.y * start_to_end_.y +
						    start_to_end_.z * start_to_end_.z),
			length_(sqrtf(length_squared_)),
			end_(other.end),
			line_(other.start,
				  length_ > vector::normalize_epsilon ?
						start_to_end_ * (1.f / length_) : vector())
		{ }


	private:

		vector start_to_end_;
		float length_squared_;
		float length_;
		vector end_;

		geo_distance_line line_;
};




/**
 *************************************************************************
 *
 * @class geo_distance_triangle
 * @ingroup geo_module
 *
 * Stores additional data for optimization of geo_distance tests.
 *
 ************************************************************************/
class geo_distance_triangle
{
	friend class geo_distance;


	public:

		explicit geo_distance_triangle(const triangle& t) NOTHROW
			:
			a_(t.a), b_(t.b), c_(t.c),
			segment_ba_(a_, b_),
			segment_cb_(b_, c_),
			segment_ac_(c_, a_),
			normalized_plane_normal_
				(segment_ba_.start_to_end_.cross
					(-segment_ac_.start_to_end_).normalized()),
			plane_distance_(normalized_plane_normal_.dot(a_)),
			min_(a_.min(b_).min(c_)),
			max_(a_.max(b_).max(c_))
		{ }


	private:

		vector a_, b_, c_;

		geo_distance_segment segment_ba_;
		geo_distance_segment segment_cb_;
		geo_distance_segment segment_ac_;

		vector normalized_plane_normal_;
		float plane_distance_;

		vector min_;
		vector max_;
};




/**
 *************************************************************************
 *
 * @class geo_distance_box_face
 * @ingroup geo_module
 *
 * Stores a face of a @ref box for geo_distance tests.
 *
 ************************************************************************/
class geo_distance_box_face
{
	friend class geo_distance;


	public:

		geo_distance_box_face
			(const vector& world_space_face_normal,
			 const vector& world_space_face_u,
			 const vector& world_space_face_v,
			 vector face_origin,
			 float face_size_u,
			 float face_size_v) NOTHROW
			:
			world_space_face_normal_(world_space_face_normal),
			world_space_face_u_(world_space_face_u),
			world_space_face_v_(world_space_face_v),
			face_origin_(face_origin),
			face_size_u_(face_size_u),
			face_size_v_(face_size_v)
		{ }


	private:

		const vector world_space_face_normal_;
		const vector world_space_face_u_;
		const vector world_space_face_v_;

		const vector face_origin_;
		const float face_size_u_;
		const float face_size_v_;
};




//////////////////////////////////////////////////////////////////////////
//
// geo_distance
//
//////////////////////////////////////////////////////////////////////////


float geo_distance::distance
	(const vector& pa, const vector& pb,
	 vector& closest_point_pa, vector& closest_point_pb) NOTHROW
		{ return sqrtf(distance_squared(pa, pb, closest_point_pa, closest_point_pb)); }


float geo_distance::distance
	(const vector& p, const line& l,
	 vector& closest_point_l) NOTHROW
		{ return sqrtf(distance_squared(p, l, closest_point_l)); }


float geo_distance::distance
	(const vector& p, const segment& s,
	 vector& closest_point_s) NOTHROW
		{ return sqrtf(distance_squared(p, s, closest_point_s)); }


float geo_distance::distance
	(const vector& p, const triangle& t,
	 vector& closest_point_t) NOTHROW
		{ return sqrtf(distance_squared(p, t, closest_point_t)); }


bool geo_distance::try_distance
	(const vector& p, const sphere& s,
	 vector& closest_point_s,
	 float& closest_distance) NOTHROW
{
	if (s.empty())
	{
		closest_point_s = vector();
		closest_distance = 0.f;
		return false;
	}


	vector s_to_p(p - s.position);
	float distance_center = s_to_p.length();


	// If the point is in the sphere, distance is zero
	// and the point itself is the closest point.
	if (distance_center <= s.radius)
	{
		closest_point_s = p;
		closest_distance = 0.f;
		return true;
	}


	// If sphere and point are very close, assume they coincide,
	// as we cannot reliably normalize the in-between vector.
	if (distance_center < sphere_distance_degenerate_epsilon)
	{
		closest_point_s = s.position;
		closest_distance = 0.f;
		return true;
	}


	// Point is outside of sphere,
	// so find distance and closest point on sphere.
	vector s_to_p_normalized
		(s_to_p * (1.f / distance_center));

	closest_point_s = 
		s_to_p_normalized * s.radius;
	closest_distance =
		distance_center - s.radius;

	return true;
}


bool geo_distance::try_distance
	(const vector& p, const box& b,
	 vector& closest_point_b,
	 float& closest_distance) NOTHROW
{
	float closest_distance_squared = 0.f;
	bool success =
		try_distance_squared
			(p, b,
			 closest_point_b,
			 closest_distance_squared);

	closest_distance =
		sqrtf(closest_distance_squared);

	return success;
}


float geo_distance::distance
	(const line& la, const line& lb,
	 vector& closest_point_la, vector& closest_point_lb) NOTHROW
		{ return sqrtf(distance_squared(la, lb, closest_point_la, closest_point_lb)); }


float geo_distance::distance
	(const segment& sa, const segment& sb,
	 vector& closest_point_sa, vector& closest_point_sb) NOTHROW
		{ return sqrtf(distance_squared(sa, sb, closest_point_sa, closest_point_sb)); }


float geo_distance::distance
	(const segment& s, const triangle& t,
	 vector& closest_point_s, vector& closest_point_t) NOTHROW
		{ return sqrtf(distance_squared(s, t, closest_point_s, closest_point_t)); }


float geo_distance::distance
	(const triangle& ta, const triangle& tb,
	 vector& closest_point_ta, vector& closest_point_tb) NOTHROW
		{ return sqrtf(distance_squared(ta, tb, closest_point_ta, closest_point_tb)); }


float geo_distance::distance
	(const triangle& ta, const matrix& transform_ta,
	 const triangle& tb, const matrix& transform_tb,
	 vector& closest_point_ta, vector& closest_point_tb) NOTHROW
{
	return sqrtf(distance_squared
					(ta, transform_ta, tb, transform_tb,
					 closest_point_ta, closest_point_tb));
}


bool geo_distance::try_distance
	(const triangle& t, const box& b,
	 vector& closest_point_t, vector& closest_point_b,
	 float& closest_distance) NOTHROW
{
	float closest_distance_squared = 0.f;
	bool success =
		try_distance_squared
			(t, b,
			 closest_point_t, closest_point_b,
			 closest_distance_squared);

	closest_distance =
		sqrtf(closest_distance_squared);

	return success;
}


bool geo_distance::try_distance
	(const triangle& t, const matrix& transform_t,
	 const box& b, const si_transform& transform_b,
	 vector& closest_point_t, vector& closest_point_b,
	 float& closest_distance) NOTHROW
{
	float closest_distance_squared = 0.f;
	bool success =
		try_distance_squared
			(t, transform_t, b, transform_b,
			 closest_point_t, closest_point_b,
			 closest_distance_squared);

	closest_distance =
		sqrtf(closest_distance_squared);

	return success;
}


bool geo_distance::try_distance
	(const sphere& sa, const sphere& sb,
	 vector& closest_point_sa, vector& closest_point_sb,
	 float& closest_distance) NOTHROW
{
	if (sa.empty() || sb.empty())
	{
		closest_point_sa = vector();
		closest_point_sb = vector();
		closest_distance = 0.f;
		return false;
	}


	vector sa_to_sb(sb.position - sa.position);
	float distance_centers = sa_to_sb.length();
	

	// If spheres are very close, assume they share centers,
	// as we cannot reliably normalize the in-between vector.
	if (distance_centers < sphere_distance_degenerate_epsilon)
	{
		closest_point_sa = sa.position;
		closest_point_sb = sb.position;
		closest_distance = 0.f;
		return true;
	}

	vector sa_to_sb_normalized
		(sa_to_sb * (1.f / distance_centers));


	// On any intersection, return a shared point.
	// Use either sphere center, if possible, otherwise
	// find a shared point in the intersecting volume.
	if (distance_centers < sa.radius + sb.radius)
	{	
		if (distance_centers < sa.radius)
		{
			// Thus, sa contains sb.position.
			closest_point_sa = sb.position;
			closest_point_sb = sb.position;
		}
		else if (distance_centers < sb.radius)
		{
			// Thus, sb contains sa.position.
			closest_point_sa = sa.position;
			closest_point_sb = sa.position;
		}
		else
		{
			float midpoint_distance =
				0.5f * sa.radius +
				0.5f * (distance_centers - sb.radius);

			vector midpoint
				(sa.position + sa_to_sb_normalized * midpoint_distance);

			closest_point_sa = midpoint;
			closest_point_sb = midpoint;
		}

		closest_distance = 0.f;
		return true;
	}


	// No intersection, so just use points
	// on sphere surfaces.
	closest_point_sa =
		sa.position + sa_to_sb_normalized * sa.radius;
	closest_point_sb =
		sb.position - sa_to_sb_normalized * sb.radius;	
	closest_distance =
		distance_centers - sa.radius - sb.radius;

	return true;
}


bool geo_distance::try_distance
	(const sphere& sa, const si_transform& transform_sa,
	 const sphere& sb, const si_transform& transform_sb,
	 vector& closest_point_sa, vector& closest_point_sb,
	 float& closest_distance) NOTHROW
{
	// We do not use transform_and_bound(),
	// so we correctly handle empty spheres.
	sphere sa_transformed
		(transform_sa.orientation * sa.position + transform_sa.position,
		 transform_sa.scale * sa.radius);
	sphere sb_transformed
		(transform_sb.orientation * sb.position + transform_sb.position,
		 transform_sb.scale * sb.radius);

	return try_distance
				(sa_transformed, sb_transformed,
				 closest_point_sa, closest_point_sb,
				 closest_distance);
}


bool geo_distance::try_distance
	(const sphere& s, const box& b, 
	 vector& closest_point_s, vector& closest_point_b,
	 float& closest_distance) NOTHROW
{
	if (s.empty() || b.empty())
	{
		closest_point_s = vector();
		closest_point_b = vector();
		closest_distance = 0.f;
		return false;
	}


	// Closest point to sphere position in the box,
	// connecting vector between these, and distance.
	closest_point_b =
		s.position.min(b.max).max(b.min);

	vector closest_point_b_to_s_position
		(s.position - closest_point_b);
	float distance_to_s_position =
		closest_point_b_to_s_position.length();


	// Early out if sphere position is in the box,
	// or very close to the box for robustness.
	if (distance_to_s_position < sphere_distance_degenerate_epsilon)
	{
		closest_point_s = closest_point_b;
		closest_distance = 0.f;
		return true;
	}


	// Determine distance to closest point on sphere,
	// then find that closest point.
	float distance_to_s_closest =
		max(distance_to_s_position - s.radius, 0.f);
	
	closest_point_s =
		closest_point_b +
		closest_point_b_to_s_position *
			distance_to_s_closest / distance_to_s_position;

	closest_distance = distance_to_s_closest;
	return true;
}


bool geo_distance::try_distance
	(const box& ba, const box& bb,
	 vector& closest_point_ba, vector& closest_point_bb,
	 float& closest_distance) NOTHROW
{
	float closest_distance_squared = 0.f;
	bool success =
		try_distance_squared
			(ba, bb,
			 closest_point_ba, closest_point_bb,
			 closest_distance_squared);

	closest_distance =
		sqrtf(closest_distance_squared);

	return success;
}


bool geo_distance::try_distance
	(const box& ba, const si_transform& transform_ba,
	 const box& bb, const si_transform& transform_bb,
	 vector& closest_point_ba, vector& closest_point_bb,
	 float& closest_distance) NOTHROW
{
	float closest_distance_squared = 0.f;
	bool success =
		try_distance_squared
			(ba, transform_ba,
			 bb, transform_bb,
			 closest_point_ba, closest_point_bb,
			 closest_distance_squared);

	closest_distance =
		sqrtf(closest_distance_squared);

	return success;
}


float geo_distance::distance_squared
	(const vector& pa, const vector& pb,
	 vector& closest_point_pa, vector& closest_point_pb) NOTHROW
{
	closest_point_pa = pa;
	closest_point_pb = pb;
	return (pa - pb).square_sum();
}


float geo_distance::distance_squared
	(const vector& p, const line& l,
	 vector& closest_point_l) NOTHROW
{
	return distance_squared(p, geo_distance_line(l), closest_point_l);
}


float geo_distance::distance_squared
	(const vector& p, const segment& s,
	 vector& closest_point_s) NOTHROW
{
	return distance_squared(p, geo_distance_segment(s), closest_point_s);
}


float geo_distance::distance_squared
	(const vector& p, const triangle& t,
	 vector& closest_point_t) NOTHROW
{
	return distance_squared(p, geo_distance_triangle(t), closest_point_t);
}


bool geo_distance::try_distance_squared
	(const vector& p, const sphere& s,
	 vector& closest_point_s,
	 float& closest_distance_squared) NOTHROW
{
	float closest_distance = 0.f;

	// We anyways need a non-squared distance
	// to find a closest point on the sphere,
	// so no need for an extra squared variant.
	bool success =
		try_distance(p, s,
					 closest_point_s,
					 closest_distance);

	closest_distance_squared =
		closest_distance * closest_distance;

	return success;
}


bool geo_distance::try_distance_squared
	(const vector& p, const box& b,
	 vector& closest_point_b,
	 float& closest_distance_squared) NOTHROW
{
	if (b.empty())
	{
		closest_point_b = vector();
		closest_distance_squared = 0.f;
		return false;
	}

	vector overhang
		(p.x > b.max.x ? (p.x - b.max.x) :
		 p.x < b.min.x ? (p.x - b.min.x) : 0,
		 p.y > b.max.y ? (p.y - b.max.y) :
		 p.y < b.min.y ? (p.y - b.min.y) : 0,
		 p.z > b.max.z ? (p.z - b.max.z) :
		 p.z < b.min.z ? (p.z - b.min.z) : 0);

	closest_point_b = p - overhang;
	closest_distance_squared = overhang.square_sum();
	return true;
}


float geo_distance::distance_squared
	(const line& la, const line& lb,
	 vector& closest_point_la, vector& closest_point_lb) NOTHROW
{
	return distance_squared(geo_distance_line(la), geo_distance_line(lb),
							closest_point_la, closest_point_lb);
}


float geo_distance::distance_squared
	(const segment& sa, const segment& sb,
	 vector& closest_point_sa, vector& closest_point_sb) NOTHROW
{
	return distance_squared(geo_distance_segment(sa), geo_distance_segment(sb),
							closest_point_sa, closest_point_sb);
}


float geo_distance::distance_squared
	(const segment& s, const triangle& t,
	 vector& closest_point_s, vector& closest_point_t) NOTHROW
{
	return distance_squared(geo_distance_segment(s), geo_distance_triangle(t),
							closest_point_s, closest_point_t);
}


float geo_distance::distance_squared
	(const triangle& ta, const triangle& tb,
	 vector& closest_point_ta, vector& closest_point_tb) NOTHROW
{
	return distance_squared(geo_distance_triangle(ta), geo_distance_triangle(tb),
							closest_point_ta, closest_point_tb);
}


float geo_distance::distance_squared
	(const triangle& ta, const matrix& transform_ta,
	 const triangle& tb, const matrix& transform_tb,
	 vector& closest_point_ta, vector& closest_point_tb) NOTHROW
{
	triangle ta_transformed
		(transform_ta * ta.a, transform_ta * ta.b, transform_ta * ta.c);
	triangle tb_transformed
		(transform_tb * tb.a, transform_tb * tb.b, transform_tb * tb.c);

	return distance_squared
				(ta_transformed, tb_transformed,
				 closest_point_ta, closest_point_tb);
}


bool geo_distance::try_distance_squared
	(const triangle& t, const box& b,
	 vector& closest_point_t, vector& closest_point_b,
	 float& closest_distance_squared) NOTHROW
{
	return try_distance_squared
				(t, matrix(),
				 b, si_transform(),
				 closest_point_t, closest_point_b,
				 closest_distance_squared);
}


bool geo_distance::try_distance_squared
	(const triangle& t, const matrix& transform_t,
	 const box& b, const si_transform& transform_b,
	 vector& closest_point_t, vector& closest_point_b,
	 float& closest_distance_squared) NOTHROW
{
	// Check if box becomes empty through transform.
	vector b_size_scaled
		((b.max - b.min) * transform_b.scale);

	if (b_size_scaled.min() < 0)
	{
		closest_point_t = vector();
		closest_point_b = vector();
		closest_distance_squared = 0.f;
		return false;
	}


	// Find triangle in target space
	// and prepare for distance tests.
	geo_distance_triangle t_transformed
		(triangle
			(transform_t * t.a,
			 transform_t * t.b,
			 transform_t * t.c));


	// Early out if box contains triangle points.
	// Otherwise, triangle and box must intersect
	// or be separated.
	si_transform transform_b_inv
		(transform_b.invert());

	vector t_transformed_points[3] =
		{ t_transformed.a_, t_transformed.b_, t_transformed.c_ };

	for (int i = 0; i < 3; ++i)
	{
		if (b.contains(transform_b_inv * t_transformed_points[i]))
		{
			closest_point_t = t_transformed_points[i];
			closest_point_b = t_transformed_points[i];
			closest_distance_squared = 0.f;
			return true;
		}
	}


	// Find unit-length axes of box in target space.
	matrix b_rotation_matrix
		(matrix::rotate(transform_b.orientation));
	vector b_axes_in_target[3] =
		{ b_rotation_matrix.x_axis(),
		  b_rotation_matrix.y_axis(),
		  b_rotation_matrix.z_axis() };


	// Find any vertex (not necessarily the minimum)
	// on each oriented bounding box in target space.
	// Then find the span vectors from these vertices
	// and calculate all remaining box vertices.
	vector b_min_in_target(transform_b * b.min);
	vector b_size_in_target[3] =
		{ b_axes_in_target[0] * b_size_scaled.x,
		  b_axes_in_target[1] * b_size_scaled.y,
		  b_axes_in_target[2] * b_size_scaled.z };
	vector b_vertices_in_target[8] =
		{ b_min_in_target,
		  b_min_in_target + b_size_in_target[0],
		  b_min_in_target + b_size_in_target[0] + b_size_in_target[1],
		  b_min_in_target + b_size_in_target[1],
		  b_min_in_target + b_size_in_target[2],
		  b_min_in_target + b_size_in_target[2] + b_size_in_target[0],
		  b_min_in_target + b_size_in_target[2] + b_size_in_target[0] + b_size_in_target[1],
		  b_min_in_target + b_size_in_target[2] + b_size_in_target[1] };

	
	// Edge lookup table.
	static const int64 edge_indices[12][2] =
		{
			{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, 
			{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, 
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
		};


	// Test each box edge against the triangle.
	bool first_distance = true;

	for (int i = 0; i < 12; ++i)
	{
		vector candidate_closest_point_eb;
		vector candidate_closest_point_t;
		float candidate_closest_distance_squared =
			distance_squared
				(geo_distance_segment
					(b_vertices_in_target[edge_indices[i][0]],
					 b_vertices_in_target[edge_indices[i][1]]),
				 t_transformed,
				 candidate_closest_point_eb,
				 candidate_closest_point_t);

		if (first_distance ||
			candidate_closest_distance_squared < closest_distance_squared)
		{
			first_distance = false;
			closest_point_t = candidate_closest_point_t;
			closest_point_b = candidate_closest_point_eb;
			closest_distance_squared = candidate_closest_distance_squared;
		}
	}


	// Precalculate box faces.
	// Note faces are not correctly oriented,
	// so we need not flip normals and sizes.
	geo_distance_box_face b_faces[6] =
		{
			geo_distance_box_face
				(b_axes_in_target[0], b_axes_in_target[1], b_axes_in_target[2],
				 b_vertices_in_target[0], b_size_scaled[1], b_size_scaled[2]),
			geo_distance_box_face
				(b_axes_in_target[0], b_axes_in_target[1], b_axes_in_target[2],
				 b_vertices_in_target[1], b_size_scaled[1], b_size_scaled[2]),
			geo_distance_box_face
				(b_axes_in_target[1], b_axes_in_target[2], b_axes_in_target[0],
				 b_vertices_in_target[0], b_size_scaled[2], b_size_scaled[0]),
			geo_distance_box_face
				(b_axes_in_target[1], b_axes_in_target[2], b_axes_in_target[0],
				 b_vertices_in_target[3], b_size_scaled[2], b_size_scaled[0]),
			geo_distance_box_face
				(b_axes_in_target[2], b_axes_in_target[0], b_axes_in_target[1],
				 b_vertices_in_target[0], b_size_scaled[0], b_size_scaled[1]),
			geo_distance_box_face
				(b_axes_in_target[2], b_axes_in_target[0], b_axes_in_target[1],
				 b_vertices_in_target[4], b_size_scaled[0], b_size_scaled[1]),
		};


	// Test each triangle edge against each box face.
	geo_distance_segment t_transformed_segments[3] =
		{
			t_transformed.segment_ac_,
			t_transformed.segment_ba_,
			t_transformed.segment_cb_
		};

	for (int i = 0; i < 3; ++i)
	for (int j = 0; j < 6; ++j)
	{
		vector candidate_closest_point_et;
		vector candidate_closest_point_bf;
		float candidate_closest_distance_squared;

		bool success =
			try_distance_squared
				(t_transformed_segments[i],
				 b_faces[j],
				 candidate_closest_point_et,
				 candidate_closest_point_bf,
				 candidate_closest_distance_squared);
	
		if (!success)
			continue;

		if (first_distance ||
			candidate_closest_distance_squared < closest_distance_squared)
		{
			first_distance = false;
			closest_point_t = candidate_closest_point_et;
			closest_point_b = candidate_closest_point_bf;
			closest_distance_squared = candidate_closest_distance_squared;
		}
	}


	// Success.
	// Note triangle-to-edge tests always have a result,
	// so first_distance is always false and we always
	// return valid closest distance and points here.
	return true;
}


bool geo_distance::try_distance_squared
	(const sphere& sa, const sphere& sb,
	 vector& closest_point_sa, vector& closest_point_sb,
	 float& closest_distance_squared) NOTHROW
{
	float closest_distance = 0.f;

	// We anyways need a non-squared distance
	// to find closest points on both spheres,
	// so no need for an extra squared variant.
	bool success =
		try_distance(sa, sb,
					 closest_point_sa, closest_point_sb,
					 closest_distance);

	closest_distance_squared =
		closest_distance * closest_distance;

	return success;
}


bool geo_distance::try_distance_squared
	(const sphere& sa, const si_transform& transform_sa,
	 const sphere& sb, const si_transform& transform_sb,
	 vector& closest_point_sa, vector& closest_point_sb,
	 float& closest_distance_squared) NOTHROW
{
	// We do not use transform_and_bound(),
	// so we correctly handle empty spheres.
	sphere sa_transformed
		(transform_sa.orientation * sa.position + transform_sa.position,
		 transform_sa.scale * sa.radius);
	sphere sb_transformed
		(transform_sb.orientation * sb.position + transform_sb.position,
		 transform_sb.scale * sb.radius);

	return try_distance_squared
				(sa_transformed, sb_transformed,
				 closest_point_sa, closest_point_sb,
				 closest_distance_squared);
}


bool geo_distance::try_distance_squared
	(const sphere& s, const box& b,
	 vector& closest_point_s, vector& closest_point_b,
	 float& closest_distance_squared) NOTHROW
{
	float closest_distance = 0.f;

	// We anyways need a non-squared distance
	// to find the closest point on the sphere,
	// so no need for an extra squared variant.
	bool success =
		try_distance(s, b,
					 closest_point_s, closest_point_b,
					 closest_distance);

	closest_distance_squared =
		closest_distance * closest_distance;

	return success;	
}


bool geo_distance::try_distance_squared
	(const box& ba, const box& bb,
	 vector& closest_point_ba, vector& closest_point_bb,
	 float& closest_distance_squared) NOTHROW
{
	if (ba.empty() || bb.empty())
	{
		closest_point_ba = vector();
		closest_point_bb = vector();
		closest_distance_squared = 0.f;
		return false;
	}


	// Idea: Separate problem over base axes.
	// Closest points on non-empty axis aligned boxes
	// are also closest in each base axis projection.
	for (int64 c = 0; c < 3; ++c)
	{

		// Test if component of closest point is a min-max pair.
		// In this case, axis projections do not intersect.
		if (ba.max.c[c] < bb.min.c[c])
		{
			closest_point_ba.c[c] = ba.max.c[c];
			closest_point_bb.c[c] = bb.min.c[c];
		}
		else if (bb.max[c] < ba.min.c[c])
		{
			closest_point_ba.c[c] = ba.min.c[c];
			closest_point_bb.c[c] = bb.max.c[c];
		}


		// Component is not a min-max pair.
		// Box projections thus must intersect,
		// and at least one projected box vertex 
		// is in the projection of the other box.
		else if (ba.max.c[c] <= bb.max.c[c] &&
				 ba.max.c[c] >= bb.min.c[c])
		{
			closest_point_ba.c[c] = ba.max.c[c];
			closest_point_bb.c[c] = ba.max.c[c];
		}
		else if (ba.min.c[c] <= bb.max.c[c] &&
				 ba.min.c[c] >= bb.min.c[c])
		{
			closest_point_ba.c[c] = ba.min.c[c];
			closest_point_bb.c[c] = ba.min.c[c];
		}
		else
		{
			// Intersection, but no projected vertex of ba falls in bb.
			// Therefore, both projected vertices of bb must fall in ba,
			// i.e. bb.max.c[c] <= ba.max.c[c] &&
			//		bb.max.c[c] >= ba.min.c[c] &&
			//		bb.min.c[c] <= ba.max.c[c] &&
			//		bb.min.c[c] >= ba.min.c[c],
			// so we can use the component of either min or max of bb.
			closest_point_ba.c[c] = bb.max.c[c];
			closest_point_bb.c[c] = bb.max.c[c];
		}
	}

	closest_distance_squared =
		(closest_point_bb - closest_point_ba).square_sum();
	return true;
}


bool geo_distance::try_distance_squared
	(const box& ba, const si_transform& transform_ba,
	 const box& bb, const si_transform& transform_bb,
	 vector& closest_point_ba, vector& closest_point_bb,
	 float& closest_distance_squared) NOTHROW
{
	/**
	 * Implementation notes:
	 * - A closest point on at least one oriented box
	 *		is on a box edge. Thus we need only test
	 *		edges against each other and against faces.
	 * - Rationale: Closest points interior to both boxes
	 *		imply intersecting or parallel oriented boxes.
	 *		We can then find additional closest points
	 *		along a suitably chosen line on either box
	 *		until we arrive at an edge of one box.
	 */
	

	// Check if boxes become empty through transform.
	vector ba_size_scaled
		((ba.max - ba.min) * transform_ba.scale);
	vector bb_size_scaled
		((bb.max - bb.min) * transform_bb.scale);

	if (ba_size_scaled.min() < 0 ||
		bb_size_scaled.min() < 0)
	{
		closest_point_ba = vector();
		closest_point_bb = vector();
		closest_distance_squared = 0.f;
		return false;
	}


	// Find unit-length axes of both boxes in target space.
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
	vector ba_min_in_target(transform_ba * ba.min);
	vector ba_size_in_target[3] =
		{ ba_axes_in_target[0] * ba_size_scaled.x,
		  ba_axes_in_target[1] * ba_size_scaled.y,
		  ba_axes_in_target[2] * ba_size_scaled.z };
	vector ba_vertices_in_target[8] =
		{ ba_min_in_target,
		  ba_min_in_target + ba_size_in_target[0],
		  ba_min_in_target + ba_size_in_target[0] + ba_size_in_target[1],
		  ba_min_in_target + ba_size_in_target[1],
		  ba_min_in_target + ba_size_in_target[2],
		  ba_min_in_target + ba_size_in_target[2] +	ba_size_in_target[0],
		  ba_min_in_target + ba_size_in_target[2] + ba_size_in_target[0] + ba_size_in_target[1],
		  ba_min_in_target + ba_size_in_target[2] + ba_size_in_target[1] };

	vector bb_min_in_target(transform_bb * bb.min);
	vector bb_size_in_target[3] =
		{ bb_axes_in_target[0] * bb_size_scaled.x,
		  bb_axes_in_target[1] * bb_size_scaled.y,
		  bb_axes_in_target[2] * bb_size_scaled.z };
	vector bb_vertices_in_target[8] =
		{ bb_min_in_target,
		  bb_min_in_target + bb_size_in_target[0],
		  bb_min_in_target + bb_size_in_target[0] + bb_size_in_target[1],
		  bb_min_in_target + bb_size_in_target[1],
		  bb_min_in_target + bb_size_in_target[2],
		  bb_min_in_target + bb_size_in_target[2] +	bb_size_in_target[0],
		  bb_min_in_target + bb_size_in_target[2] + bb_size_in_target[0] + bb_size_in_target[1],
		  bb_min_in_target + bb_size_in_target[2] + bb_size_in_target[1] };

	
	// Edge lookup table.
	// Shared for both boxes.
	static const int64 edge_indices[12][2] =
		{
			{ 0, 1 }, { 1, 2 }, { 2, 3 }, { 3, 0 }, 
			{ 4, 5 }, { 5, 6 }, { 6, 7 }, { 7, 4 }, 
			{ 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 }
		};


	// Precalculate box edges.
	// Use placement new to avoid code clutter.
	unsigned char ba_edges_memory[sizeof(geo_distance_segment) * 12];
	geo_distance_segment* ba_edges =
		reinterpret_cast<geo_distance_segment*>(ba_edges_memory);

	for (int64 i = 0; i < 12; ++i)
		new(ba_edges + i)
			geo_distance_segment
				(ba_vertices_in_target[edge_indices[i][0]],
				 ba_vertices_in_target[edge_indices[i][1]]);

	unsigned char bb_edges_memory[sizeof(geo_distance_segment) * 12];
	geo_distance_segment* bb_edges =
		reinterpret_cast<geo_distance_segment*>(bb_edges_memory);

	for (int64 i = 0; i < 12; ++i)
		new(bb_edges + i)
			geo_distance_segment
				(bb_vertices_in_target[edge_indices[i][0]],
				 bb_vertices_in_target[edge_indices[i][1]]);


	// Precalculate box faces.
	// Note faces are not correctly oriented,
	// so we need not flip normals and sizes.
	geo_distance_box_face ba_faces[6] =
		{
			geo_distance_box_face
				(ba_axes_in_target[0], ba_axes_in_target[1], ba_axes_in_target[2],
				 ba_vertices_in_target[0], ba_size_scaled[1], ba_size_scaled[2]),
			geo_distance_box_face
				(ba_axes_in_target[0], ba_axes_in_target[1], ba_axes_in_target[2],
				 ba_vertices_in_target[1], ba_size_scaled[1], ba_size_scaled[2]),
			geo_distance_box_face
				(ba_axes_in_target[1], ba_axes_in_target[2], ba_axes_in_target[0],
				 ba_vertices_in_target[0], ba_size_scaled[2], ba_size_scaled[0]),
			geo_distance_box_face
				(ba_axes_in_target[1], ba_axes_in_target[2], ba_axes_in_target[0],
				 ba_vertices_in_target[3], ba_size_scaled[2], ba_size_scaled[0]),
			geo_distance_box_face
				(ba_axes_in_target[2], ba_axes_in_target[0], ba_axes_in_target[1],
				 ba_vertices_in_target[0], ba_size_scaled[0], ba_size_scaled[1]),
			geo_distance_box_face
				(ba_axes_in_target[2], ba_axes_in_target[0], ba_axes_in_target[1],
				 ba_vertices_in_target[4], ba_size_scaled[0], ba_size_scaled[1]),
		};

	geo_distance_box_face bb_faces[6] =
		{
			geo_distance_box_face
				(bb_axes_in_target[0], bb_axes_in_target[1], bb_axes_in_target[2],
				 bb_vertices_in_target[0], bb_size_scaled[1], bb_size_scaled[2]),
			geo_distance_box_face
				(bb_axes_in_target[0], bb_axes_in_target[1], bb_axes_in_target[2],
				 bb_vertices_in_target[1], bb_size_scaled[1], bb_size_scaled[2]),
			geo_distance_box_face
				(bb_axes_in_target[1], bb_axes_in_target[2], bb_axes_in_target[0],
				 bb_vertices_in_target[0], bb_size_scaled[2], bb_size_scaled[0]),
			geo_distance_box_face
				(bb_axes_in_target[1], bb_axes_in_target[2], bb_axes_in_target[0],
				 bb_vertices_in_target[3], bb_size_scaled[2], bb_size_scaled[0]),
			geo_distance_box_face
				(bb_axes_in_target[2], bb_axes_in_target[0], bb_axes_in_target[1],
				 bb_vertices_in_target[0], bb_size_scaled[0], bb_size_scaled[1]),
			geo_distance_box_face
				(bb_axes_in_target[2], bb_axes_in_target[0], bb_axes_in_target[1],
				 bb_vertices_in_target[4], bb_size_scaled[0], bb_size_scaled[1]),
		};


	// Test each edge against each face of the other box.
	// Border cases (e.g. closest points on a face edge)
	// are already handled through edge-to-edge tests,
	// so testing closest on-plane points is sufficient.
	bool first_distance = true;

	for (int64 ea = 0; ea < 12; ++ea)
	for (int64 fb = 0; fb < 6; ++fb)
	{
		vector closest_point_ea;
		vector closest_point_fb;
		float closest_distance_squared_ea_fb = 0.f;

		if (!try_distance_squared
				(ba_edges[ea], bb_faces[fb],
				 closest_point_ea, closest_point_fb,
				 closest_distance_squared_ea_fb))
						continue;

		if (first_distance ||
			closest_distance_squared_ea_fb < closest_distance_squared)
		{
			first_distance = false;
			closest_point_ba = closest_point_ea;
			closest_point_bb = closest_point_fb;
			closest_distance_squared = closest_distance_squared_ea_fb;
		}
	}

	for (int64 eb = 0; eb < 12; ++eb)
	for (int64 fa = 0; fa < 6; ++fa)
	{
		vector closest_point_fa;
		vector closest_point_eb;
		float closest_distance_squared_fa_eb = 0.f;

		if (!try_distance_squared
				(bb_edges[eb], ba_faces[fa],
				 closest_point_eb, closest_point_fa,
				 closest_distance_squared_fa_eb))
						continue;

		if (first_distance ||
			closest_distance_squared_fa_eb < closest_distance_squared)
		{
			first_distance = false;
			closest_point_ba = closest_point_fa;
			closest_point_bb = closest_point_eb;
			closest_distance_squared = closest_distance_squared_fa_eb;
		}
	}


	// Test each edge against each edge of the other box.
	for (int64 ea = 0; ea < 12; ++ea)
	for (int64 eb = 0; eb < 12; ++eb)
	{
		vector closest_point_ea;	
		vector closest_point_eb;		
		float closest_distance_squared_ea_eb =
			distance_squared
				(ba_edges[ea], bb_edges[eb],
				 closest_point_ea, closest_point_eb);

		if (first_distance ||
			closest_distance_squared_ea_eb < closest_distance_squared)
		{
			first_distance = false;
			closest_point_ba = closest_point_ea;
			closest_point_bb = closest_point_eb;
			closest_distance_squared = closest_distance_squared_ea_eb;
		}
	}


	// Success.
	// Note edge-to-edge tests always produce a result,
	// so first_distance is always false and we always
	// return valid closest distance and points here.
	return true;
}


float geo_distance::apply_sphere_sweep
	(float radius_a, float radius_b,
	 vector& closest_point_a, vector& closest_point_b) NOTHROW
{
	vector connect(closest_point_b - closest_point_a);
	float distance = connect.length();

	if (distance < radius_a + radius_b ||
		distance < sphere_sweep_epsilon)
	{
		// Points are too close to reliably
		// consider sweep spheres.
		closest_point_a = closest_point_b =
			(closest_point_a + closest_point_b) * 0.5f;
		return 0.f;
	}

	closest_point_a += connect * radius_a / distance;
	closest_point_b -= connect * radius_b / distance; 
	return max(0.f, distance - radius_a - radius_b);
}


/**
 * Epsilons of 1e-10 give us leeway to coordinates in ~1e10 range
 * without division overflow to INF or NAN.
 */
//@{
const float geo_distance::line_degenerate_epsilon = 1e-10f;
const float geo_distance::lines_parallel_epsilon = 1e-10f;
const float geo_distance::sphere_distance_degenerate_epsilon = 1e-10f;
const float geo_distance::sphere_sweep_epsilon = 1e-10f;
//@}


const float geo_distance::intersect_segment_oneside_epsilon = 1e-6f;
const float geo_distance::contains_unchecked_epsilon = 1e-5f;


float geo_distance::distance_squared
	(const vector& p, const geo_distance_line& l,
	 vector& closest_point_l) NOTHROW
{
	/** Orthogonally project point onto line. */
	float line_dir_distance =
		l.normalized_direction_.dot(p - l.origin_);

	closest_point_l =
		l.origin_ + l.normalized_direction_ * line_dir_distance;

	return (closest_point_l - p).square_sum();
}


float geo_distance::distance_squared
	(const vector& p, const geo_distance_segment& s,
	 vector& closest_point_s) NOTHROW
{
	/**
	 * Implementation notes:
	 * - Find the closest point on the corresponding line,
	 *		then clamp to valid segment range.
	 * - Rationale: Starting from the closest on-line point,
	 *		distance increases monotonically over the line.
	 * - We provide special handling for segment end points,
	 *		because both cases occur rather frequently
	 *		and allow us to skip all vector arithmetics.
	 */

	vector to_point(p - s.line_.origin_);
	float distance_along_segment =
		to_point.dot(s.line_.normalized_direction_);

	if (distance_along_segment <= 0)
		{ closest_point_s = s.line_.origin_; }
	else if (distance_along_segment >= s.length_)
		{ closest_point_s = s.end_; }
	else
	{
		closest_point_s =
			s.line_.origin_ +
			s.line_.normalized_direction_ * distance_along_segment;
	}

	return (closest_point_s - p).square_sum();
}


float geo_distance::distance_squared
	(const vector& p, const geo_distance_triangle& t,
	 vector& closest_point_t) NOTHROW
{
	// Check projection of point to triangle plane.
	// If contained in the triangle, the projection
	// is always the closest point.
	vector closest_on_plane(closest_on_plane(t, p));
	if (contains_unchecked(t, closest_on_plane))
	{
		closest_point_t = closest_on_plane;
		return (p - closest_point_t).square_sum();
	}
		

	// Check point against each triangle segment.
	float candidate_dist[3];
	vector candidates_t[3];

	candidate_dist[0] =
		distance_squared(p, t.segment_ba_, candidates_t[0]);
	candidate_dist[1] =
		distance_squared(p, t.segment_cb_, candidates_t[1]);
	candidate_dist[2] =
		distance_squared(p, t.segment_ac_, candidates_t[2]);

	closest_point_t =
		(candidate_dist[0] > candidate_dist[1] &&
		 candidate_dist[0] > candidate_dist[2]) ?
				candidates_t[0] :
		(candidate_dist[1] > candidate_dist[2]) ?
				candidates_t[1] :
				candidates_t[2];

	return (p - closest_point_t).square_sum();
}


float geo_distance::distance_squared
	(const geo_distance_line& la, const geo_distance_line& lb,
	 vector& closest_point_la, vector& closest_point_lb) NOTHROW
{
	/**
	 * Implementation notes:
	 * - The difference vector d between two closest points
	 *		is zero for intersecting lines and perpendicular
	 *		to both directions da, db in all other cases.
	 *		Hence, the dot product of the difference vector
	 *		with both direction vectors always is zero:
	 *		da * d = 0 and db * d = 0.
	 * - Parametrization of the closest point difference d
	 *		for origins oa, ob and directions da, db yields:
	 *		d = (oa + s * da) - (ob + t * db).
	 * - Substituting the latter equation into the former ones
	 *		gives a linear equation system, namely:
	 *		(da * da) * s + (-da * db) * t = (da * ob - da * oa)
	 *		(db * da) * s + (-db * db) * t = (db * ob - db * oa),
	 *		and with some shorthand:
	 *		u * s + v * t = w
	 *		x * s + y * t = z.
	 * - Solving this system for s, t with determinants gives:
	 *		s = (w * y - z * v) / (u * y - x * v)
	 *		t = (u * z - x * w) / (u * y - x * v).
	 * - A near-zero denominator indicates parallel lines.
	 *		In this case, choose either s or t as zero,
	 *		then solve the system for the other parameter:
	 *		s = w / u or t = z / y.
	 */

	float u = la.normalized_direction_.dot(la.normalized_direction_);
	float v = -la.normalized_direction_.dot(lb.normalized_direction_);
	float w = la.normalized_direction_.dot(lb.origin_) -
			  la.normalized_direction_.dot(la.origin_);

	float x = -v; // == lb.normalized_direction_.dot(la.normalized_direction_);
	float y = -lb.normalized_direction_.dot(lb.normalized_direction_);
	float z = lb.normalized_direction_.dot(lb.origin_) -
			  lb.normalized_direction_.dot(la.origin_);

	float denom = u * y - x * v;

	float s, t;

	if (fabsf(denom) > lines_parallel_epsilon)
	{
		float denom_inv = (1.f / denom);
		s = (w * y - z * v) * denom_inv;
		t = (u * z - x * w) * denom_inv;
	}
	else if (fabsf(u) > line_degenerate_epsilon)
	{
		s = w / u; // Lines are parallel, line a is not degenerate.
		t = 0;
	}
	else if (fabsf(y) > line_degenerate_epsilon)
	{		
		s = 0; // Lines are parallel, line a is degenerate, line b is not.
		t = z / y;
	}
	else
	{		
		s = 0; // Both lines are degenerate.
		t = 0;
	}

	closest_point_la = la.origin_ + la.normalized_direction_ * s;
	closest_point_lb = lb.origin_ + lb.normalized_direction_ * t;

	return (closest_point_lb - closest_point_la).square_sum();
}


float geo_distance::distance_squared
	(const geo_distance_segment& sa, const geo_distance_segment& sb,
	 vector& closest_point_sa, vector& closest_point_sb) NOTHROW
{
	/**
	 * Implementation notes:
	 * - Let pa, pb be closest points on respective lines,
	 *		as per distance_line_line().
	 * - If segments sa, sb respectively contain pa, pb,
	 *		closest points match and we can stop already.
	 *		Otherwise, one of the closest segment points
	 *		is a delimiting point on either segment.
	 * - We need not consider all delimiter combinations,
	 *		but only delimiters closest to points pa, pb,
	 *		because distance grows monotonically over lines
	 *		when moving away from closest line points.
	 */


	// Determine closest points on respective lines.
	vector line_line_closest_a, line_line_closest_b;		
	float line_line_distance =
		distance_squared
			(sa.line_, sb.line_,
			 line_line_closest_a, line_line_closest_b);


	// Determine which delimiter points to consider.
	// If closest line points are on segments,
	// we have finished already.
	vector to_line_line_closest_a
		(line_line_closest_a - sa.line_.origin_);
	float distance_line_line_closest_a =
		to_line_line_closest_a.dot(sa.line_.normalized_direction_);
	bool line_line_on_sa =
		distance_line_line_closest_a >= 0 &&
		distance_line_line_closest_a <= sa.length_;

	vector to_line_line_closest_b
		(line_line_closest_b - sb.line_.origin_);
	float distance_line_line_closest_b =
		to_line_line_closest_b.dot(sb.line_.normalized_direction_);
	bool line_line_on_sb =
		distance_line_line_closest_b >= 0 &&
		distance_line_line_closest_b <= sb.length_;

	if (line_line_on_sa && line_line_on_sb)
	{
		closest_point_sa = line_line_closest_a;
		closest_point_sb = line_line_closest_b;
		return line_line_distance;
	}


	// If closest point is past segment on one line only,
	// move to closest on-segment delimiter point.
	// This point must be closest segment-segment point,
	// we only need to find its counterpart.
	if (line_line_on_sa)
	{
		closest_point_sb =
			(distance_line_line_closest_b >= sb.length_) ?
				sb.end_ : sb.line_.origin_;

		return distance_squared(closest_point_sb, sa, closest_point_sa);
	}

	if (line_line_on_sb)
	{
		closest_point_sa =
			(distance_line_line_closest_a >= sa.length_) ?
				sa.end_ : sa.line_.origin_;

		return distance_squared(closest_point_sa, sb, closest_point_sb);
	}


	// Closest points are past segments on both lines,
	// so we have two delimiter points as candidates.
	// Find opposed closest point for each candidate,
	// then choose the closer point pair as result.
	vector try_closest_point_b
		((distance_line_line_closest_b >= sb.length_) ?
		 sb.end_ : sb.line_.origin_);
	vector try_closest_b_on_a;
	float try_closest_b_distance =
		distance_squared(try_closest_point_b, sa, try_closest_b_on_a);

	vector try_closest_point_a
		((distance_line_line_closest_a >= sa.length_) ?
		 sa.end_ : sa.line_.origin_);
	vector try_closest_a_on_b;
	float try_closest_a_distance =
		distance_squared(try_closest_point_a, sb, try_closest_a_on_b);

	if (try_closest_a_distance < try_closest_b_distance)
	{
		closest_point_sa = try_closest_point_a;
		closest_point_sb = try_closest_a_on_b;	
		return try_closest_a_distance;
	}
	
	closest_point_sa = try_closest_b_on_a;
	closest_point_sb = try_closest_point_b;
	return try_closest_b_distance;
}


float geo_distance::distance_squared
	(const geo_distance_segment& s, const geo_distance_triangle& t,
	 vector& closest_point_s, vector& closest_point_t) NOTHROW
{
	/**
	 * Implementation notes:
	 * - Candidate points on the triangle are only:
	 *		- Points on any edge.
	 *		- Any interior points closest to segment end points.
	 *		- An interior point where segment and triangle intersect.
	 * - Rationale: Assume a closest point on the interior of the triangle.
	 *		If distance is zero, this point is a segment-plane intersection.
	 *		Otherwise, the point is a projection of a segment end point
	 *		or the segment is parallel to triangle. In the latter case,
	 *		follow the segment to a segment end point or a triangle edge.
	 */


	// Check line-triangle intersection.
	// Early out because zero-distance intersection is common.
	vector tp_hit_point;
	if (try_intersect(s, t, tp_hit_point))
	{
		closest_point_s = tp_hit_point;
		closest_point_t = tp_hit_point;
		return 0.f;
	}


	// Check segment against each triangle segment.
	float candidate_dist[5];
	vector candidates_s[5];
	vector candidates_t[5];
	int64 num_candidates = 3;

	candidate_dist[0] =
		distance_squared(s, t.segment_ba_, candidates_s[0], candidates_t[0]);
	candidate_dist[1] =
		distance_squared(s, t.segment_cb_, candidates_s[1], candidates_t[1]);
	candidate_dist[2] =
		distance_squared(s, t.segment_ac_, candidates_s[2], candidates_t[2]);


	// Check line endpoints.
	vector closest_start(closest_on_plane(t, s.line_.origin_));	
	if (contains_unchecked(t, closest_start))
	{
		candidate_dist[num_candidates] = (closest_start - s.line_.origin_).square_sum();
		candidates_s[num_candidates] = s.line_.origin_;
		candidates_t[num_candidates] = closest_start;
		++num_candidates;
	}
	
	vector closest_end(closest_on_plane(t, s.end_));	
	if (contains_unchecked(t, closest_end))
	{
		candidate_dist[num_candidates] = (closest_end - s.end_).square_sum();
		candidates_s[num_candidates] = s.end_;
		candidates_t[num_candidates] = closest_end;
		++num_candidates;
	}


	// Find and return minimum of our closest point candidates.
	int64 min_index = 0;
	for (int64 i = 1; i < num_candidates; ++i)
	{
		if (candidate_dist[min_index] > candidate_dist[i])
			min_index = i;
	}

	closest_point_s = candidates_s[min_index];
	closest_point_t = candidates_t[min_index];
	return candidate_dist[min_index];
}


float geo_distance::distance_squared
	(const geo_distance_triangle& ta, const geo_distance_triangle& tb,
	 vector& closest_point_a, vector& closest_point_b) NOTHROW
{
	/**
	 * Implementation notes:
	 * - The closest point on at least one triangle
	 *		is on a triangle edge.
	 * - Rationale: Closest points interior to both triangles
	 *		imply intersecting triangles or parallel triangles.
	 *		We can then find additional closest points
	 *		along an arbitrary on-triangle line (former case)
	 *		or along the intersection line (latter case)
	 *		until we arrive at an edge on at least one triangle.
	 * - We do not separately check all segments against triangles.
	 *		Instead, we directly integrate segment-triangle distance()
	 *		to avoid duplicate segment-segment and point-plane tests.
	 *		See segment-triangle distance() for test rationale.
	 */


	// Early-out on any segment-triangle intersection.
	// Short-circuit logics avoid overwriting tp_hit_point.
	// Box pretests reduce overhead for distant triangles.
	vector tp_hit_point;
	if (ta.max_.x >= tb.min_.x && tb.max_.x >= ta.min_.x &&
		ta.max_.y >= tb.min_.y && tb.max_.y >= ta.min_.y &&
		ta.max_.z >= tb.min_.z && tb.max_.z >= ta.min_.z)
	{
		if (try_intersect(ta.segment_ba_, tb, tp_hit_point) ||
			try_intersect(ta.segment_cb_, tb, tp_hit_point) ||
			try_intersect(ta.segment_ac_, tb, tp_hit_point) ||
			try_intersect(tb.segment_ba_, ta, tp_hit_point) ||
			try_intersect(tb.segment_cb_, ta, tp_hit_point) ||
			try_intersect(tb.segment_ac_, ta, tp_hit_point))
		{
			closest_point_a = tp_hit_point;
			closest_point_b = tp_hit_point;
			return 0.f;
		}
	}


	// Check each segment against opposing segments.
	// We don't use segment-triangle tests to avoid
	// a lot of duplicate segment-segment cases.
	float candidate_dist[15];
	vector candidates_ta[15];
	vector candidates_tb[15];
	int64 num_candidates = 9;

	candidate_dist[0] =
		distance_squared(ta.segment_ba_, tb.segment_ba_, candidates_ta[0], candidates_tb[0]);
	candidate_dist[1] =
		distance_squared(ta.segment_cb_, tb.segment_ba_, candidates_ta[1], candidates_tb[1]);
	candidate_dist[2] =
		distance_squared(ta.segment_ac_, tb.segment_ba_, candidates_ta[2], candidates_tb[2]);

	candidate_dist[3] =
		distance_squared(ta.segment_ba_, tb.segment_cb_, candidates_ta[3], candidates_tb[3]);
	candidate_dist[4] =
		distance_squared(ta.segment_cb_, tb.segment_cb_, candidates_ta[4], candidates_tb[4]);
	candidate_dist[5] =
		distance_squared(ta.segment_ac_, tb.segment_cb_, candidates_ta[5], candidates_tb[5]);

	candidate_dist[6] =
		distance_squared(ta.segment_ba_, tb.segment_ac_, candidates_ta[6], candidates_tb[6]);
	candidate_dist[7] =
		distance_squared(ta.segment_cb_, tb.segment_ac_, candidates_ta[7], candidates_tb[7]);
	candidate_dist[8] =
		distance_squared(ta.segment_ac_, tb.segment_ac_, candidates_ta[8], candidates_tb[8]);


	// Check all segment end points against opposing triangle.
	// Again, no segment-triangle tests to avoid redundant work.
	const vector* opposing_points[2][3] =
		{ { &ta.a_, &ta.b_, &ta.c_ } ,
		  { &tb.a_, &tb.b_, &tb.c_ } };

	for (int64 i = 0; i < 3; ++i)
	{
		vector closest_on_tb(closest_on_plane(tb, *opposing_points[0][i]));
		if (contains_unchecked(tb, closest_on_tb))
		{
			candidates_ta[num_candidates] = *opposing_points[0][i];
			candidates_tb[num_candidates] = closest_on_tb;
			candidate_dist[num_candidates] =
				(closest_on_tb - *opposing_points[0][i]).square_sum();
			++num_candidates;
		}

		vector closest_on_ta(closest_on_plane(ta, *opposing_points[1][i]));
		if (contains_unchecked(ta, closest_on_ta))
		{
			candidates_ta[num_candidates] = closest_on_ta;
			candidates_tb[num_candidates] = *opposing_points[1][i];
			candidate_dist[num_candidates] =
				(closest_on_ta - *opposing_points[1][i]).square_sum();
			++num_candidates;
		}
	}
	

	// Find and return minimum of our closest point candidates.
	int64 min_index = 0;
	for (int64 i = 1; i < num_candidates; ++i)
	{
		if (candidate_dist[min_index] > candidate_dist[i])
			min_index = i;
	}

	closest_point_a = candidates_ta[min_index];
	closest_point_b = candidates_tb[min_index];
	return candidate_dist[min_index];
}


bool geo_distance::try_intersect
	(const geo_distance_segment& s, const geo_distance_triangle& t, vector& hit_point)
{
	/** Optimized variant of geo_intersect::try_intersect() for segment-triangle pairs. */
	
	float diststart = s.line_.origin_.dot(t.normalized_plane_normal_) - t.plane_distance_;
	float distend = s.end_.dot(t.normalized_plane_normal_) - t.plane_distance_;

	bool both_front =
		diststart < intersect_segment_oneside_epsilon &&
		distend < intersect_segment_oneside_epsilon;

	bool both_back =
		diststart > -intersect_segment_oneside_epsilon &&
		distend > -intersect_segment_oneside_epsilon;

	if (both_front || both_back)
		return false; // No need to reset hit_point for internal use.

	float absdiststart = fabsf(diststart);
	float absdistend = fabsf(distend);

	float ratio =
		max(0.f, min(1.f, absdiststart / (absdiststart + absdistend)));

	hit_point =
		s.line_.origin_ + s.start_to_end_ * ratio;

	return contains_unchecked(t, hit_point);
}


vector geo_distance::closest_on_plane
	(const geo_distance_triangle& t, const vector& p)
{
	/**
	 * Variant of plane::closest_on_plane() for geo_distance_triangle&s.
	 * Avoids rewrapping of triangle data.
	 */

	float signed_distance = p.dot(t.normalized_plane_normal_) - t.plane_distance_;
	return p - t.normalized_plane_normal_ * signed_distance;
}


bool geo_distance::contains_unchecked
	(const geo_distance_triangle& t, const vector& p)
{
	/**
	 * Alternative to triangle::contains_unchecked() for geo_distance_triangle%s.	 
	 * Avoids rewrapping of triangle data, and uses a faster, less robust approach:
	 * We check if all sub-triangles through @bref{p} face into triangle direction.
	 *
	 * Unlike triangle::contains_unchecked(), we bias to false negatives here.
	 * In particular, cross products for sub-triangle facing become almost zero
	 * for degenerate triangles, hence these triangles test as @false.
	 *
	 * The bias for false-negatives triggers segment-segment tests when in doubt
	 * (e.g. for degenerate triangles or near triangle edges). This more reliably
	 * constrains closest points to actual triangles.
	 */

	if (p.x < t.min_.x || p.y < t.min_.y || p.z < t.min_.z ||
		p.x > t.max_.x || p.y > t.max_.y || p.z > t.max_.z)
			return false; // Early out on bounding box for increased robustness.

	vector pa(t.a_ - p);
	vector pb(t.b_ - p);
	vector pc(t.c_ - p);

	vector normal_pab(pa.cross(pb));
	vector normal_pbc(pb.cross(pc));
	vector normal_pca(pc.cross(pa));

	return normal_pab.dot(t.normalized_plane_normal_) > contains_unchecked_epsilon &&
		   normal_pbc.dot(t.normalized_plane_normal_) > contains_unchecked_epsilon &&
		   normal_pca.dot(t.normalized_plane_normal_) > contains_unchecked_epsilon;
}


bool geo_distance::try_distance_squared
	(const geo_distance_segment& s, const geo_distance_box_face& bf,
	 vector& closest_point_s, vector& closest_point_bf,
	 float& closest_distance_squared)
{
	/**
	 * Find closest point on interior of face @bref{bf}
	 * to segment @bref{s}.
	 *
	 * Implementation notes:
	 * - There are three candidates for closest points:
	 *		Either segment end-point and its projection
	 *		onto the face plane, and any shared point
	 *		between segment and face plane.
	 * - Discard any candidates that are not in the face, 
	 *		then pick the closest remaining candidate.
	 * - We need not check for closest points on face edges,
	 *		as we test face edges separately.
	 */
	

	// Check for intersection point first,
	// as this allows for an early out.
	float offset =
		bf.face_origin_.dot(bf.world_space_face_normal_);
	float diststart =
		s.line_.origin_.dot(bf.world_space_face_normal_) - offset;
	float distend =
		s.end_.dot(bf.world_space_face_normal_) - offset;

	bool both_front =
		diststart < intersect_segment_oneside_epsilon &&
		distend < intersect_segment_oneside_epsilon;

	bool both_back =
		diststart > -intersect_segment_oneside_epsilon &&
		distend > -intersect_segment_oneside_epsilon;

	if (!both_front && !both_back)
	{
		float absdiststart = fabsf(diststart);
		float absdistend = fabsf(distend);

		float ratio =
			max(0.f, min(1.f, absdiststart / (absdiststart + absdistend)));

		vector hit_point
			(s.line_.origin_ + s.start_to_end_ * ratio);

		vector hit_point_relative_origin
			(s.end_ - bf.face_origin_);
		float hit_point_relative_u =
			hit_point_relative_origin.dot(bf.world_space_face_u_);
		float hit_point_relative_v =
			hit_point_relative_origin.dot(bf.world_space_face_v_);

		if (hit_point_relative_u > 0 && hit_point_relative_u < bf.face_size_u_ &&
			hit_point_relative_v > 0 && hit_point_relative_v < bf.face_size_v_)
		{
			closest_point_s = hit_point;
			closest_point_bf = hit_point;
			closest_distance_squared = 0.f;
			return true;
		}
	}


	// Check for projection of line start
	// and line end onto plane.
	bool any_valid = false;
	const vector *line_points[2] =
		{ &s.line_.origin_, &s.end_ };

	for (int64 i = 0; i < 2; ++i)
	{
		vector line_point_relative_origin
			(*line_points[i] - bf.face_origin_);
		float line_point_relative_u =
			line_point_relative_origin.dot(bf.world_space_face_u_);
		float line_point_relative_v =
			line_point_relative_origin.dot(bf.world_space_face_v_);
	
		if (line_point_relative_u > 0 && line_point_relative_u < bf.face_size_u_ &&
			line_point_relative_v > 0 && line_point_relative_v < bf.face_size_v_)
		{
			vector closest_point_bf_line_point
				(bf.face_origin_ +
				 bf.world_space_face_u_ * line_point_relative_u +
				 bf.world_space_face_v_ * line_point_relative_v);

			float closest_distance_squared_line_point =
				(closest_point_bf_line_point - *line_points[i]).square_sum();

			if (!any_valid ||
				closest_distance_squared_line_point < closest_distance_squared)
			{
				any_valid = true;
				closest_point_s = *line_points[i];
				closest_point_bf = closest_point_bf_line_point;
				closest_distance_squared = closest_distance_squared_line_point;
			}
		}
	}


	// No need to reset hit_point for internal use
	// on a false return.
	return any_valid;
}




} /* namespace viral_core */
