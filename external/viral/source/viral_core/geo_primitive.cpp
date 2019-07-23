/**
 *************************************************************************
 *
 * @file geo_primitive.cpp
 *
 * Geometric primitives, implementation.
 *
 ************************************************************************/


#include "geo_primitive.hpp"


#include <math.h>


#include "util.hpp"
#include "geo_transform.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// line
//
//////////////////////////////////////////////////////////////////////////


line::line() NOTHROW { }

line::line(const vector& origin, const vector& direction) NOTHROW
	: origin(origin), direction(direction) { }


bool line::operator ==(const line& other) const NOTHROW
	{ return origin == other.origin && direction == other.direction; }
bool line::operator !=(const line& other) const NOTHROW
	{ return origin != other.origin || direction != other.direction; }


void line::swap_with(line& other) NOTHROW
{
	swap(origin, other.origin);
	swap(direction, other.direction);
}




//////////////////////////////////////////////////////////////////////////
//
// segment
//
//////////////////////////////////////////////////////////////////////////


segment::segment() NOTHROW { }

segment::segment(const vector& start, const vector& end) NOTHROW
	: start(start), end(end) { }


bool segment::operator ==(const segment& other) const NOTHROW
	{ return start == other.start && end == other.end; }
bool segment::operator !=(const segment& other) const NOTHROW
	{ return start != other.start || end != other.end; }


void segment::swap_with(segment& other) NOTHROW
{
	swap(start, other.start);
	swap(end, other.end);
}


float segment::length() const NOTHROW
	{ return (end - start).length(); }


void segment::set(const vector& new_start, const vector& new_end) NOTHROW
	{ start = new_start; end = new_end; }


bool segment::contains(const vector& point) const NOTHROW
{
	return contains(point,
					default_segment_degenerate_epsilon,
					default_contains_epsilon_constant,
					default_contains_epsilon_linear);
}

bool segment::contains
	(const vector& point,
	 float segment_degenerate_epsilon,
	 float contains_epsilon_constant,
	 float contains_epsilon_linear) const NOTHROW
{
	vector closest_point(closest_on_segment(point, segment_degenerate_epsilon));
	return closest_point.nearly_equal
				(point, contains_epsilon_constant, contains_epsilon_linear);
}


vector segment::closest_on_segment(const vector& point) const NOTHROW
	{ return closest_on_segment(point, default_segment_degenerate_epsilon); }

vector segment::closest_on_segment
	(const vector& point, float segment_degenerate_epsilon) const NOTHROW
{
	// Handle degenerate segments.
	vector to_end(end - start);
	float segment_length = to_end.length();
	if (segment_length < segment_degenerate_epsilon)
		return start;

	// Project point onto segment line,
	// then clamp to segment range.
	float inv_segment_length = 1.f / segment_length;
	float length_to_projection =
		((point - start).dot(to_end)) * inv_segment_length;

	float length_to_closest =
		max(0.f, min(segment_length, length_to_projection));

	return 
		start + to_end * length_to_closest * inv_segment_length;
}


/* Only used for normalization divide, where a headroom of 1e-10f is sufficient. */
const float segment::default_segment_degenerate_epsilon = 1e-10f;


/**
 * Should be larger than segment::segment_degenerate_epsilon
 * so that tests on degenerate segments work reliably.
 * See viral_core::nearly_equal() and vector::nearly_equal().
 */
//@{
const float segment::default_contains_epsilon_constant = 1e-5f;
const float segment::default_contains_epsilon_linear = 1e-5f;
//@}




//////////////////////////////////////////////////////////////////////////
//
// plane
//
//////////////////////////////////////////////////////////////////////////


plane::plane() NOTHROW
	: distance(0) { }

plane::plane(const vector& normal, float distance) NOTHROW
	: normal(normal), distance(distance) { }

plane::plane(float nx, float ny, float nz, float distance) NOTHROW
	: normal(nx, ny, nz), distance(distance) { }

plane::plane
	(const vector& a, const vector& b, const vector& c) NOTHROW
		{ set(a, b, c); }

plane::plane(const triangle& t) NOTHROW { set(t); }

plane::plane(const vector& normal, const vector& point) NOTHROW
	: normal(normal), distance(normal.dot(point)) { }


bool plane::operator ==(const plane& other) const NOTHROW
	{ return normal == other.normal && distance == other.distance; }
bool plane::operator !=(const plane& other) const NOTHROW
	{ return normal != other.normal || distance != other.distance; }


void plane::swap_with(plane& other) NOTHROW
{
	swap(normal, other.normal);
	swap(distance, other.distance);
}


void plane::set
	(const vector& new_normal, float new_distance) NOTHROW
{
	normal = new_normal;
	distance = new_distance;
}

void plane::set
	(float nx, float ny, float nz, float new_distance) NOTHROW
{
	normal = vector(nx, ny, nz);
	distance = new_distance;
}

void plane::set
	(const vector& a, const vector& b, const vector& c) NOTHROW
{
	normal = ((b - a).cross(c - a)).normalized();
	distance = normal.dot(a);
}

void plane::set(const triangle& t) NOTHROW
	{ set(t.a, t.b, t.c); }

void plane::set
	(const vector& new_normal, const vector& point) NOTHROW
{
	normal = new_normal;
	distance = new_normal.dot(point);
}


void plane::flip() NOTHROW
	{ normal *= - 1; distance *= -1; }
plane plane::flipped() const NOTHROW
	{ return plane(normal * -1, distance * -1); }


bool plane::contains(const vector& point) const NOTHROW
{
	return contains(point, default_contains_epsilon_constant,
						   default_contains_epsilon_linear);
}

bool plane::contains
	(const vector& point,
	 float contains_epsilon_constant,
	 float contains_epsilon_linear) const NOTHROW
{
	float epsilon =
		max(point.absmax() * contains_epsilon_linear,
			contains_epsilon_constant);

	return fabsf(signed_distance_to(point)) < epsilon;
}


float plane::signed_distance_to(const vector& point) const NOTHROW
	{ return point.dot(normal) - distance; }


vector plane::closest_on_plane(const vector& point) const NOTHROW
	{ return point - normal * signed_distance_to(point); }


/**
 * Distance tolerance for points on a plane.
 * Both constant and linear in absolute maximum point component.
 * See viral_core::nearly_equal().
 */
//@{
const float plane::default_contains_epsilon_constant = 1e-10f;
const float plane::default_contains_epsilon_linear = 1e-5f;
//@}




//////////////////////////////////////////////////////////////////////////
//
// triangle
//
//////////////////////////////////////////////////////////////////////////


triangle::triangle() NOTHROW { }

triangle::triangle(const triangle& other) NOTHROW
	: a(other.a), b(other.b), c(other.c), p(other.p) { }
	
triangle::triangle
	(const vector& a, const vector& b, const vector& c) NOTHROW :
		a(a), b(b), c(c), p(a, b, c) { }


bool triangle::operator ==(const triangle& other) const NOTHROW
	{ return a == other.a && b == other.b && c == other.c && p == other.p; }
bool triangle::operator !=(const triangle& other) const NOTHROW
	{ return a != other.a || b != other.b || c != other.c && p == other.p; }


void triangle::swap_with(triangle& other) NOTHROW
{
	swap(a, other.a);
	swap(b, other.b);
	swap(c, other.c);
	swap(p, other.p);
}


bool triangle::degenerate() const NOTHROW
	{ return degenerate(default_degenerate_epsilon); }

bool triangle::degenerate(float degenerate_epsilon) const NOTHROW
{
	/**
	 * Implementation notes:
	 * - The length of the cross product between triangle edges
	 *		is proportional to triangle area and thus disappears
	 *		for degenerate triangles.
	 * - We use the absolute maximum component of the cross product
	 *		instead of its actual length for performance reasons.
	 */

	return (b - a).cross(c - a).absmax() < degenerate_epsilon;
}


vector triangle::to_barycentric(const vector& cartesian) const NOTHROW
{
	/**
	 * Implementation notes:
	 * - For improved numerical robustness, we use triangle areas
	 *		instead of linear equations to find barycentric coordinates.
	 * - Split the original triangle along all three point-to-vertex segments.
	 *		The barycentric coordinate of a point in respect to a triangle vertex
	 *		then is proportional to the area of the opposing sub-triangle.
	 * - On a degenerate triangle, one or more triangle areas may be zero.
	 *		In this case, normalization may fail, and we do not normalize.
	 */
	
	vector pa(a - cartesian);
	vector pb(b - cartesian);
	vector pc(c - cartesian);

	vector area_pab(pa.cross(pb));
	vector area_pbc(pb.cross(pc));
	vector area_pca(pc.cross(pa));

	float sign_pab = (area_pab.dot(p.normal) >= 0) ? 1.f : -1.f;
	float sign_pbc = (area_pbc.dot(p.normal) >= 0) ? 1.f : -1.f;
	float sign_pca = (area_pca.dot(p.normal) >= 0) ? 1.f : -1.f;

	vector result
		(area_pbc.length() * sign_pbc,
		 area_pca.length() * sign_pca,
		 area_pab.length() * sign_pab);

	float normalize =
		(result.x + result.y + result.z);

	if (fabs(normalize) > barycentric_epsilon)
		result /= normalize;

	return result;
}


vector triangle::from_barycentric(const vector& barycentric) const NOTHROW
	{ return a * barycentric.x + b * barycentric.y + c * barycentric.z; }


bool triangle::contains(const vector& point) const NOTHROW
{
	if (!p.contains(point)) return false;
	return contains_unchecked(point);
}

bool triangle::contains
	(const vector& point,
	 float plane_epsilon_constant,
	 float plane_epsilon_linear,
	 float contains_unchecked_epsilon) const NOTHROW
{
	if (!p.contains(point, plane_epsilon_constant,
						   plane_epsilon_linear))
	{
		return false;
	}

	return contains_unchecked
				(point, contains_unchecked_epsilon);
}


bool triangle::contains_unchecked(const vector& point) const NOTHROW
	{ return contains_unchecked(point, default_contains_unchecked_epsilon); }

bool triangle::contains_unchecked
	(const vector& point, float contains_unchecked_epsilon) const NOTHROW
{
	// Bounding box pretest.
	// Avoids far-off false positives for degenerate triangles,
	// e.g. where two of the later planes are nearly identical.
	// Also an early out to avoid the more expensive later test.
	vector min(a.min(b).min(c));
	vector max(a.max(b).max(c));

	vector tolerance
		(contains_unchecked_epsilon,
		 contains_unchecked_epsilon,
		 contains_unchecked_epsilon);

	min = min - min.abs() * tolerance - tolerance;
	max = max + max.abs() * tolerance + tolerance;

	if (point.x < min.x || point.y < min.y || point.z < min.z ||
		point.x > max.x || point.y > max.y || point.z > max.z)
			return false;


	// Find planes perpendicular to the triangle plane through
	// all triangle edges. The specific cross product order
	// makes all planes face inward. Later biasing requires
	// approximately normalized plane normals for consistency.
	vector ba((b - a).normalized_absmax());
	vector cb((c - b).normalized_absmax());
	vector ac((a - c).normalized_absmax());

	vector ba_normal(p.normal.cross(ba));
	vector cb_normal(p.normal.cross(cb));
	vector ac_normal(p.normal.cross(ac));

	vector pa((point - a).normalized_absmax());
	vector pb((point - b).normalized_absmax());
	vector pc((point - c).normalized_absmax());


	// Actual point-triangle test.
	// The input point must be on the positive side of all edge planes.
	// We offset the planes by epsilon for a false-positive bias.
	// Checking each plane twice with different vectors increases
	// bias robustness on large triangles.
	float pa_ba_dist = pa.dot(ba_normal);
	float pb_ba_dist = pb.dot(ba_normal);
	float pb_cb_dist = pb.dot(cb_normal);
	float pc_cb_dist = pc.dot(cb_normal);
	float pc_ac_dist = pc.dot(ac_normal);
	float pa_ac_dist = pa.dot(ac_normal);

	return
		(pa_ba_dist >= -contains_unchecked_epsilon ||
		 pb_ba_dist >= -contains_unchecked_epsilon) &&
		(pb_cb_dist >= -contains_unchecked_epsilon ||
		 pc_cb_dist >= -contains_unchecked_epsilon) &&
		(pc_ac_dist >= -contains_unchecked_epsilon ||
		 pa_ac_dist >= -contains_unchecked_epsilon);
}


sphere triangle::build_bounding_sphere() const NOTHROW
{
	// There are two midpoint candidates for minimal bounding circles:
	// The circumcircle midpoint and the midpoint of the longest side.
	// The former point gives minimum area if contained in the triangle.
	// Otherwise, the latter point gives minimum area.

	float ba_squared = (b - a).square_sum();
	float cb_squared = (c - b).square_sum();
	float ac_squared = (a - c).square_sum();

	vector barycentric_denorm
		(cb_squared * (ac_squared + ba_squared - cb_squared),
		 ac_squared * (ba_squared + cb_squared - ac_squared),
		 ba_squared * (cb_squared + ac_squared - ba_squared));

	float normalize =
		barycentric_denorm.x +
		barycentric_denorm.y +
		barycentric_denorm.z;


	// Check whether triangle is degenerate.
	// If so, use center of mass and a loose fit instead.
	if (fabsf(normalize) < barycentric_epsilon)
	{
		vector midpoint_com
			((a + b + c) * (1.f / 3.f));

		float radius_squared =
			max((a - midpoint_com).square_sum(),
				max((b - midpoint_com).square_sum(),
					(c - midpoint_com).square_sum()));
		
		return sphere(midpoint_com,
					  sqrtf(radius_squared));		
	}


	// Check if circumcircle midpoint is in triangle.
	// If so, use this midpoint as midpoint of bounding sphere.
	if (barycentric_denorm.x > 0 &&
		barycentric_denorm.y > 0 &&
		barycentric_denorm.z > 0)
	{
		vector barycentric_normalized
			(barycentric_denorm * (1.f / normalize));
		vector midpoint_cc
			(from_barycentric(barycentric_normalized));

		float radius_squared =
			max((a - midpoint_cc).square_sum(),
				max((b - midpoint_cc).square_sum(),
					(c - midpoint_cc).square_sum()));
		
		return sphere(midpoint_cc,
					  sqrtf(radius_squared));		
	}


	// Circumcircle midpoint is not in triangle.
	// The midpoint of the longest side gives a smaller sphere.
	vector midpoint_ls
		((ba_squared > cb_squared && ba_squared > ac_squared) ?
			(b + a) * 0.5f :
		 (cb_squared > ac_squared) ?
			(c + b) * 0.5f :
			(a + c) * 0.5f);

	float radius_squared =
		max((a - midpoint_ls).square_sum(),
			max((b - midpoint_ls).square_sum(),
				(c - midpoint_ls).square_sum()));
		
	return sphere(midpoint_ls,
				  sqrtf(radius_squared));	
}

box triangle::build_bounding_box() const NOTHROW
{
	return box(a.min(b).min(c),
			   a.max(b).max(c));
}


triangle triangle::transform(const si_transform& t) const NOTHROW
	{ return transform(t.build_local_to_world()); }
triangle triangle::transform(const matrix& t) const NOTHROW
	{ return triangle(t * a, t * b, t * c); }


/** Quite approximate triangle area below which a triangle is degenerate(). */
const float triangle::default_degenerate_epsilon = 1e-5f;

/** Only used for normalization divide, where a headroom of 1e-10f is sufficient. */
const float triangle::barycentric_epsilon = 1e-10f;

/** Distance tolerance for points on the frontside of a triangle edge plane. */
const float triangle::default_contains_unchecked_epsilon = 1e-5f;




} /* namespace viral_core */
