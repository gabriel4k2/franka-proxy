/**
 *************************************************************************
 *
 * @file geo_volume.cpp
 *
 * Volumetric primitives, implementation.
 *
 ************************************************************************/


#include "geo_volume.hpp"


#include <math.h>


#include "util.hpp"
#include "geo_util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// sphere
//
//////////////////////////////////////////////////////////////////////////


sphere::sphere() NOTHROW : radius(0) { }

sphere::sphere(const vector& position, float radius) NOTHROW
	: position(position), radius(radius) { }


bool sphere::operator ==(const sphere& other) const NOTHROW
{
	return position == other.position &&
		   radius == other.radius;
}

bool sphere::operator !=(const sphere& other) const NOTHROW
{
	return position != other.position ||
		   radius != other.radius;
}


void sphere::swap_with(sphere& other) NOTHROW
{
	swap(position, other.position);
	swap(radius, other.radius);
}


float sphere::volume() const NOTHROW
{
	return (radius > 0) ?
				(geo_constants::pi * radius * radius * radius * 4.f / 3.f) : 0;
}


bool sphere::empty() const NOTHROW
	{ return radius < 0; }


bool sphere::intersecting(const sphere& other) const NOTHROW
{
	if (radius < 0 || other.radius < 0)
		return false;

	float center_diff_len =
		(other.position - position).length();

	// Keep these conditions in sync
	// with intersect() below.
	return 
		center_diff_len < intersect_epsilon ||
		center_diff_len <= radius + other.radius;
}


bool sphere::try_intersection_center
	(const sphere& other, vector &intersection_center) const NOTHROW
{
	// Empty spheres never intersect.
	if (empty() || other.empty())
		return false;


	// Abort if spheres do not intersect.
	vector center_diff(other.position - position);
	float center_diff_len = center_diff.length();

	if (center_diff_len > radius + other.radius)
		return false;


	// Handle degenerate cases, e.g. where spheres
	// are very close or where one sphere contains
	// the center of the other sphere.
	if (center_diff_len < intersect_epsilon)
	{
		intersection_center = (position + other.position) * 0.5f;
		return true;	
	}
	else if (center_diff_len < radius &&
			 center_diff_len < other.radius)
	{
		intersection_center = (position + other.position) * 0.5f;
		return true;
	}
	else if (center_diff_len < radius)
	{
		intersection_center = other.position;
		return true;
	}
	else if (center_diff_len < other.radius)
	{
		intersection_center = position;
		return true;
	}
	

	// Spheres intersect partially,
	//		ra > d and rb > d,	
	// and the result is a two-sided lens shape.
	// We now find the center of the lens circle:
	//		ra * ra = da * da + h * h,
	//		rb * rb = db * db + h * h,
	// with db = (d - da) and equating, this gives
	//		ra * ra - da * da = rb * rb - (d - da) * (d - da),
	// and solving for da yields
	//		da = (ra * ra - rb * rb + d * d) / (2 * d).
	// A final min/max clamping of the result distance
	// protects against numerical issues.

	float inv_center_diff_len =
		1.f / center_diff_len;

	float dist_to_intersect_mid =
		max(0.f, min(center_diff_len,
			0.5f * inv_center_diff_len *
				(radius * radius - other.radius * other.radius +
				 center_diff_len * center_diff_len)));

	intersection_center =
		position +
			center_diff * inv_center_diff_len * dist_to_intersect_mid;
	return true;
}


float sphere::intersection_volume(const sphere& other) const NOTHROW
{
	if (radius <= 0 || other.radius <= 0) // Zero-radius spheres have no volume.
		return 0;

	float r1 = radius;
	float r2 = other.radius;


	// Avoid degenerate case in later formula
	// for close-by spheres.
	float d = (position - other.position).length();
	if (d < intersection_volume_distance_epsilon)
		return (r1 < r2) ? volume() : other.volume();


	// Avoid degenerate case in later formula
	// where one sphere fully contains the other.
	if (r1 >= r2)
	{
		if (d < r1 - r2 + intersection_volume_radius_epsilon)
			return other.volume();
	}
	else
	{
		if (d < r2 - r1 + intersection_volume_radius_epsilon)
			return volume();
	}


	// Calculate sum of volumes for both lenses
	// within the intersection volume.
	return geo_constants::pi *
			(r1 + r2 - d) * (r1 + r2 - d) *
			(d * d + 6 * r1 * r2 +
			 2 * (d * r1 + d * r2) +
			 -3 * (r1 * r1 + r2 * r2)) / (12 * d);
}


void sphere::grow_radius_to(const vector &point) NOTHROW
{
	float point_distance = (point - position).length();
	radius = max(radius, point_distance);	
}


void sphere::grow_radius_to(const sphere &s) NOTHROW
{
	if (s.radius < 0) return;
	float center_distance = (s.position - position).length();
	radius = max(radius, center_distance + s.radius);
}


void sphere::enclose(const vector& point) NOTHROW
{
	if (radius < 0)
	{
		position = point;
		radius = 0;
		return;
	}

	vector to_center(position - point);
	float center_distance = to_center.length();

	if (center_distance < enclose_epsilon)
	{
		// Avoid precision issues:
		// Don't enclose if positions are very close,
		// but grow radius instead.
		radius = max(radius, center_distance);
		return;
	}

	float extremal_distance = center_distance + radius;
	radius = extremal_distance * 0.5f;
	position = point + to_center * radius / center_distance;
}


void sphere::enclose(const sphere& s) NOTHROW
{
	if (s.radius < 0) return;

	if (radius < 0)
	{
		// Target sphere s contains at least one point,
		// while we do not contain any points.
		position = s.position;
		radius = s.radius;
		return;
	}

	vector to_other(s.position - position);
	float center_distance = to_other.length();

	if (center_distance < enclose_epsilon)
	{
		// Avoid precision issues:
		// Don't enclose if positions are very close,
		// but grow radius instead.
		radius = max(radius, center_distance + s.radius);
		return;
	}

	// Determine overhang on either side of the to_other vector.
	// This gives us two opposing points on the enclosing sphere.
	// From these, determine new radius and find suitable midpoint.
	float overhang_this = max(radius, s.radius - center_distance);
	float overhang_other = max(s.radius, radius - center_distance);
	float extremal_distance =
		center_distance + overhang_this + overhang_other;

	to_other *= 1.f / center_distance;
	radius = extremal_distance * 0.5f;
	position += to_other * (-overhang_this + radius);	
}


bool sphere::contains(float x, float y, float z) const NOTHROW
{
	if (radius < 0) return false; // Necessary for squared radius test.

	float dist_x = x - position.x;
	float dist_y = y - position.y;
	float dist_z = z - position.z;

	float dist_sqr = dist_x * dist_x + dist_y * dist_y + dist_z * dist_z;
	return dist_sqr <= radius * radius;
}

bool sphere::contains(const vector& point) const NOTHROW
	{ return contains(point.x, point.y, point.z); }


bool sphere::contains(float x, float y, float z, float epsilon) const NOTHROW
{
	float radius_epsilon = radius + epsilon;
	if (radius_epsilon < 0) return false;  // Necessary for squared radius test.

	float dist_x = x - position.x;
	float dist_y = y - position.y;
	float dist_z = z - position.z;

	float dist_sqr = dist_x * dist_x + dist_y * dist_y + dist_z * dist_z;
	return dist_sqr <= radius_epsilon * radius_epsilon;
}

bool sphere::contains(const vector& point, float epsilon) const NOTHROW
	{ return contains(point.x, point.y, point.z, epsilon); }


box sphere::build_bounding_box() const NOTHROW
	{ return box(position - vector(radius, radius, radius),
				 position + vector(radius, radius, radius)); }


sphere sphere::transform_and_bound(const si_transform& t) const NOTHROW
{
	return sphere(t.position + t.orientation * (position * t.scale),
				  fabsf(t.scale * radius)); // Build non-empty sphere.
}

sphere sphere::transform_and_bound(const matrix& t) const NOTHROW
{
	return sphere(t * position,
				  fabsf(t.x_axis().length() * radius)); // Build non-empty sphere.
}


const float sphere::intersect_epsilon = 1e-5f;
const float sphere::intersection_volume_distance_epsilon = 1e-5f;
const float sphere::intersection_volume_radius_epsilon = 1e-5f;
const float sphere::enclose_epsilon = 1e-10f;




//////////////////////////////////////////////////////////////////////////
//
// box
//
//////////////////////////////////////////////////////////////////////////


box::box() NOTHROW { }

box::box(const vector& min, const vector& max) NOTHROW
	: min(min), max(max) { }


bool box::operator ==(const box& other) const NOTHROW
	{ return min == other.min && max == other.max; }
bool box::operator !=(const box& other) const NOTHROW
	{ return min != other.min || max != other.max; }


void box::swap_with(box& other) NOTHROW
{
	swap(min, other.min);
	swap(max, other.max);
}


vector box::mid() const NOTHROW { return (max + min) * 0.5f; }


vector box::size() const NOTHROW { return max - min; }
float box::size_x() const NOTHROW { return max.x - min.x; }
float box::size_y() const NOTHROW { return max.y - min.y; }
float box::size_z() const NOTHROW { return max.z - min.z; }


float box::volume() const NOTHROW
{
	return viral_core::max(size_x(), 0.f) *
		   viral_core::max(size_y(), 0.f) *
		   viral_core::max(size_z(), 0.f);
}


bool box::empty() const NOTHROW
{
	return min.x > max.x ||
		   min.y > max.y ||
		   min.z > max.z;
}


bool box::intersecting(const box& other) const NOTHROW
{
	if (empty() || other.empty()) return false;

	// We now know that min <= max for both boxes.
	// No intersection if one box is completely
	// on one side of the other.
	return !(max.x < other.min.x || other.max.x < min.x ||
			 max.y < other.min.y || other.max.y < min.y ||
			 max.z < other.min.z || other.max.z < min.z);			
}


bool box::try_intersect
	(const box& other, box& target) const NOTHROW
{
	if (!intersecting(other))
		return false;


	// We know the boxes intersect.
	// The intersection area then is defined by the middle
	// two points on each axis. Find these by sorting.
	float sort_x[4] =
		{ min.x, max.x, other.min.x, other.max.x };
	float sort_y[4] =
		{ min.y, max.y, other.min.y, other.max.y };
	float sort_z[4] =
		{ min.z, max.z, other.min.z, other.max.z };

	for (int64 i = 0; i < 4; ++i)
	for (int64 j = i + 1; j < 4; ++j)
	{
		if (sort_x[i] > sort_x[j]) swap(sort_x[i], sort_x[j]);
		if (sort_y[i] > sort_y[j]) swap(sort_y[i], sort_y[j]);
		if (sort_z[i] > sort_z[j]) swap(sort_z[i], sort_z[j]);
	}

	target.min = vector(sort_x[1], sort_y[1], sort_z[1]);
	target.max = vector(sort_x[2], sort_y[2], sort_z[2]);
	return true;
}


float box::intersection_volume(const box& other) const NOTHROW
{
	if (empty() || other.empty())
		return 0;

	box intersect_box;
	if (!try_intersect(other, intersect_box))
		return 0;

	return intersect_box.volume();
}


void box::grow_to(const vector &point) NOTHROW
{
	if (empty())
	{
		min = point;
		max = point;
		return;
	}

	min = min.min(point);
	max = max.max(point);
}


void box::grow_to(const box& b) NOTHROW
{
	if (b.empty()) return;
	grow_to(b.min);
	grow_to(b.max);
}


bool box::contains(float x, float y, float z) const NOTHROW
{
	return
		x >= min.x && x <= max.x &&
		y >= min.y && y <= max.y &&
		z >= min.z && z <= max.z;
}

bool box::contains(const vector& point) const NOTHROW
	{ return contains(point.x, point.y, point.z); }


bool box::contains(float x, float y, float z, float epsilon) const NOTHROW
{
	return
		x >= min.x - epsilon && x <= max.x + epsilon &&
		y >= min.y - epsilon && y <= max.y + epsilon &&
		z >= min.z - epsilon && z <= max.z + epsilon;
}

bool box::contains(const vector& point, float epsilon) const NOTHROW
	{ return contains(point.x, point.y, point.z, epsilon); }


sphere box::build_bounding_sphere() const NOTHROW
{
	vector halfsize((max - min) * 0.5f);
	return sphere(min + halfsize,
				  empty() ? -halfsize.length() :
							 halfsize.length());
}


box box::transform_and_bound(const si_transform& t) const NOTHROW
	{ return transform_and_bound(t.build_local_to_world()); }

box box::transform_and_bound(const matrix& t) const NOTHROW
{
	// Find any vertex (not necessarily the minimum)
	// on the oriented bounding box in target space,
	// then find the span vectors for that vertex.
	vector min_in_target(t * min);
	vector size_target[3] =
		{ t.x_axis() * (max.x - min.x),
		  t.y_axis() * (max.y - min.y),
		  t.z_axis() * (max.z - min.z) };


	// Potentially follow span vectors.
	// Per component, choose only span vectors
	// that lead to either a minimum or a maximum.
	box result(min_in_target, min_in_target);
	for (int64 i = 0; i < 3; ++i)
	for (int64 c = 0; c < 3; ++c)
	{
		if (size_target[i][c] < 0) result.min[c] += size_target[i][c];
			else result.max[c] += size_target[i][c]; 
	}

	return result;
}




} /* namespace viral_core */
