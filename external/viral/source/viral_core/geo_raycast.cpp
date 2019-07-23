/**
 *************************************************************************
 *
 * @file geo_raycast.cpp
 *
 * Raycasting for geometric primitives, implementation.
 *
 ************************************************************************/


#include "geo_raycast.hpp"


#include <math.h>


#include "geo_vector.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// geo_raycast
//
//////////////////////////////////////////////////////////////////////////


bool geo_raycast::try_raycast
	(const line& l, const plane& p,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	// Early out:
	// Abort if our line is degenerate or doesn't face the plane.
	// Pre-normalize, otherwise the epsilons are not reliable.
	vector unitdir(l.direction.normalized());

	if (unitdir.abs().max() < intersect_line_degenerate_epsilon ||
		fabs(unitdir.dot(p.normal)) < intersect_line_parallel_epsilon)
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Line faces to or against the plane, so calculate intersection.
	// Determine how much we approach the plane when we travel into
	// direction of the line, and use this value to determine the actual
	// travel distance (against distorigin, thus the minus).
	float distorigin = p.signed_distance_to(l.origin);
	float approach_per_dir = unitdir.dot(p.normal);

	hit_distance = -(distorigin / approach_per_dir);
	hit_point = l.origin + unitdir * hit_distance;

	return true;
}


bool geo_raycast::try_raycast
	(const segment& s, const plane& p,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	// See what side of the plane our points are on.
	// If both are on the same side, we can safely abort.
	float diststart = p.signed_distance_to(s.start);
	float distend = p.signed_distance_to(s.end);

	bool both_front =
		diststart < intersect_segment_oneside_epsilon &&
		distend < intersect_segment_oneside_epsilon;

	bool both_back =
		diststart > -intersect_segment_oneside_epsilon &&
		distend > -intersect_segment_oneside_epsilon;

	if (both_front || both_back)
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Points on different sides, so we have an intersection.
	// Calculate length ratio up to this intersection,
	// then find intersection point.
	float absdiststart = fabsf(diststart);
	float absdistend = fabsf(distend);

	float ratio =
		max(0.f, min(1.f, absdiststart / (absdiststart + absdistend)));

	vector tracer((s.end - s.start) * ratio);

	hit_distance = tracer.length();
	hit_point = s.start + tracer;

	return true;
}


bool geo_raycast::try_raycast
	(const line& l, const triangle& t,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	if (!try_raycast(l, t.p, hit_point, hit_distance))
		return false; // Plane already resets hit_point and hit_distance.

	if (!t.contains_unchecked(hit_point))
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}

	return true;
}


bool geo_raycast::try_raycast
	(const segment& s, const triangle& t,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	if (!try_raycast(s, t.p, hit_point, hit_distance))
		return false; // Plane already resets hit_point and hit_distance.

	if (!t.contains_unchecked(hit_point))
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}

	return true;
}


bool geo_raycast::try_raycast_interior
	(const segment& s, const sphere& sp,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	// Empty spheres cannot have segment intersections.
	// Explicit testing avoids false-positive results
	// against a non-empty(), volume-covering sphere.
	if (sp.empty())
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Separate tests for segment origin and actual segment.
	// If the sphere does not contain the origin of the segment,
	// then the first intersection is a point on the boundary.
	if (sp.contains(s.start))
	{
		hit_point = s.start;
		hit_distance = 0.f;
		return true;
	}

	return try_raycast_boundary
				(s, sp, hit_point, hit_distance);
}


bool geo_raycast::try_raycast_boundary
	(const segment& s, const sphere& sp,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	// Empty spheres cannot have segment intersections.
	// Explicit testing avoids false-positive results
	// against a non-empty(), volume-covering sphere.
	if (sp.empty())
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Find shared point on segment line and sphere boundary by solving
	//		|| s.start + lambda * (s.start - s.end) - sp.position || = sp.radius
	// for lambda.
	// Squaring both sides and simplifying gives
	//			lambda * lambda * (s.start - s.end) dot (s.start - s.end) +
	//		2 * lambda * (s.start - s.end) dot (s.start - sp.position) +
	//			(s.start - sp.position) dot (s.start - sp.position) - sp.radius * sp.radius = 0,
    // which we we solve with the quadratic formula.
	vector to_start(s.start - sp.position);
	vector dir(s.end - s.start);

	float a = dir.square_sum();
	float b = dir.dot(to_start);
	float c = to_start.square_sum() - sp.radius * sp.radius;

	float root = b * b - 4 * a * c;
	if (root < 0)
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Build (non-normalized) distances to hit point candidates.
	// Sort candidates by distance along the segment line.
	float lambda_one = (-b + sqrtf(root)) / (4 * a * c);
	float lambda_two = (-b - sqrtf(root)) / (4 * a * c);
	float lambda_min = min(lambda_one, lambda_two);
	float lambda_max = max(lambda_one, lambda_two);


	// Use the earliest on-segment hit point.
	if (lambda_min >= 0 && lambda_min <= 1)
	{
		hit_point = dir * lambda_min + s.start;
		hit_distance = (hit_point - s.start).length();
		return true;
	}

	if (lambda_max >= 0 && lambda_max <= 1)
	{
		hit_point = dir * lambda_max + s.start;
		hit_distance = (hit_point - s.start).length();
		return true;
	}


	// Neither hit point is on the segment,
	// thus segment and sphere do not intersect.
	hit_point = vector();
	hit_distance = 0.f;
	return false;
}


bool geo_raycast::try_raycast_interior
	(const segment& s, const box& b,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	// Empty boxes cannot have segment intersections.
	// Explicit testing avoids false-positive results
	// against a non-empty(), volume-covering box.
	if (b.empty())
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Separate tests for segment origin and actual segment.
	// If the box does not contain the origin of the segment,
	// then the first intersection is a point on the boundary.
	if (b.contains(s.start, intersect_segment_epsilon))
	{
		hit_point = s.start;
		hit_distance = 0.f;
		return true;
	}

	return try_raycast_boundary
				(s, b, hit_point, hit_distance);
}


bool geo_raycast::try_raycast_boundary
	(const segment& s, const box& b,
	 vector& hit_point, float& hit_distance) NOTHROW
{
	// Empty boxes cannot have segment intersections.
	// Explicit testing avoids false-positive results
	// against a non-empty(), volume-covering box.
	if (b.empty())
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}
	

	// Find potential intersection candidates,
	// i.e. points both on a bounding plane of the box
	// and on the incoming line segment.
	float candidate_distance[6];
	int64 candidate_count = 0;

	for (int64 i = 0; i < 3; ++i)
	{
		if (s.start.c[i] < b.min.c[i] && b.min.c[i] < s.end.c[i] ||
			s.end.c[i] < b.min.c[i] && b.min.c[i] < s.start.c[i])
		{
			candidate_distance[candidate_count] =
				(fabsf(s.end.c[i] - s.start.c[i]) < intersect_segment_epsilon) ?
					0.f : ((b.min.c[i] - s.start.c[i]) / (s.end.c[i]  - s.start.c[i]));

			++candidate_count;
		}

		if (s.start.c[i] < b.max.c[i] && b.max.c[i] < s.end.c[i] ||
			s.end.c[i] < b.max.c[i] && b.max.c[i] < s.start.c[i])
		{
			candidate_distance[candidate_count] =
				(fabsf(s.end.c[i] - s.start.c[i]) < intersect_segment_epsilon) ?
					0.f : ((b.max.c[i] - s.start.c[i]) / (s.end.c[i]  - s.start.c[i]));

			++candidate_count;
		}
	}

	if (candidate_count == 0)
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Check intersection candidates for actual intersection.
	// Return nearest actual intersection as hitpoint,
	// and store distance ratio for later use.
	static const float invalid_distance = 2.f; // Never reached.
	float minimum_distance = invalid_distance; 
		
	for (int64 i = 0; i < candidate_count; ++i)
	{
		if (candidate_distance[i] >= minimum_distance)
			continue;

		vector candidate
			(s.start + (s.end - s.start) * candidate_distance[i]);

		if (b.contains(candidate, intersect_segment_epsilon))
		{
			hit_point = candidate;
			minimum_distance = candidate_distance[i];
		}			
	}

	if (minimum_distance == invalid_distance)
	{
		hit_point = vector();
		hit_distance = 0.f;
		return false;
	}


	// Found a valid hitpoint, so update
	// non-ratio based hitpoint distance.
	hit_distance =
		(hit_point - s.start).length();

	return true;
}


const float geo_raycast::intersect_segment_epsilon = 1e-5f;
const float geo_raycast::intersect_segment_oneside_epsilon = 1e-6f;
const float geo_raycast::intersect_line_degenerate_epsilon = 1e-10f;
const float geo_raycast::intersect_line_parallel_epsilon = 1e-10f;




} /* namespace viral_core */

