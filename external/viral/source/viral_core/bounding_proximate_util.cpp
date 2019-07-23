/**
 *************************************************************************
 *
 * @file bounding_proximate_util.cpp
 *
 * Utilities for proximity tests, implementation.
 *
 ************************************************************************/


#include "bounding_proximate_util.hpp"


#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "geo_volume.hpp"
#include "geo_distance.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_proximate_util
//
//////////////////////////////////////////////////////////////////////////


bool bounding_proximate_util::shape_proximate
	(const sphere& sa, const si_transform& transform_ta,
	 const matrix& to_world_matrix_ta,
	 const sphere& sb, const si_transform& transform_tb,
	 const matrix& to_world_matrix_tb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential)
{
	// Check for empty spheres after transforms.
	float world_radius_sa = 
		sa.radius * transform_ta.scale;
	float world_radius_sb = 
		sb.radius * transform_tb.scale;

	if (world_radius_sa < 0 ||
		world_radius_sb < 0)
			return false;


	// Transform spheres to world coordinates.
	vector world_sa(to_world_matrix_ta * sa.position);
	vector world_sb(to_world_matrix_tb * sb.position);


	// Determine exact closest distance in world space.
	// No geo_distance, no need for expensive closest points.
	float center_distance = (world_sb - world_sa).length();
	float min_sphere_distance =
		max(center_distance -
				(world_radius_sa + world_radius_sb), 0.f);

	if (min_sphere_distance * min_sphere_distance > distance_squared_upper_bound)
		return false;


	// Assume that proximate points are near to sphere centers,
	// thus first evaluate spheres with minimum center distance.
	evaluation_potential =
		-(center_distance * center_distance);


	return true;
}


bool bounding_proximate_util::shape_proximate
	(const sphere& s, const si_transform& transform_s,
	 const matrix& to_world_matrix_s,
	 const box& b, const si_transform& transform_b,
	 float distance_squared_upper_bound,
	 float& evaluation_potential)
{
	// Transform sphere position to box coordinates
	// and find overhang of position over box.
	vector spb
		(transform_b.invert() * (to_world_matrix_s * s.position));

	vector overhang
		(spb.x > b.max.x ? (spb.x - b.max.x) :
		 spb.x < b.min.x ? (spb.x - b.min.x) : 0,
		 spb.y > b.max.y ? (spb.y - b.max.y) :
		 spb.y < b.min.y ? (spb.y - b.min.y) : 0,
		 spb.z > b.max.z ? (spb.z - b.max.z) :
		 spb.z < b.min.z ? (spb.z - b.min.z) : 0);		 


	// Use maximum metric to find lower bound.
	// Transform lower bound to target space,
	// matching distance_squared_upper_bound
	// and sphere radius.
	float distance_lower_bound =
		max(0.f,
			overhang.absmax() * max(0.f, transform_b.scale) -
				s.radius * max(0.f, transform_s.scale));

	float distance_squared_lower_bound =
		distance_lower_bound * distance_lower_bound;


	// Prefer to evaluate close-by nodes next.
	evaluation_potential =
		-distance_squared_lower_bound;


	// This test creates false-positives only:
	// - If the lower bound is greater than the incoming upper bound,
	//		box and sphere are at least apart by the upper bound.
	// - If the lower bound is smaller than the incoming upper bound,
	//		distance may, but need not, be closer than upper bound.
	return
		distance_squared_lower_bound < distance_squared_upper_bound;
}


bool bounding_proximate_util::shape_proximate
	(const box& ba, const matrix& to_world_matrix_ta,
	 const box& bb, const matrix& to_world_matrix_tb,
	 float distance_squared_upper_bound,
	 float& evaluation_potential)
{
	// Build world-space bounding boxes
	// over transformed, oriented boxes.
	// May cause false-positive proximity,
	// as accepted by bounding_proximate.
	box ba_world(ba.transform_and_bound(to_world_matrix_ta));
	box bb_world(bb.transform_and_bound(to_world_matrix_tb));

	if (ba_world.empty() || bb_world.empty())
		return false;


	// Closest distance between two axis aligned boxes
	// occurs per-component from a maximum to a minimum.
	// We check each component individually to reject
	// candidates that cannot possibly be proximate.
	// This again may cause false-positive proximity.
	vector closest_delta
		(vector().max(bb_world.min - ba_world.max)
				 .max(ba_world.min - bb_world.max));

	if (closest_delta.x * closest_delta.x > distance_squared_upper_bound ||
		closest_delta.y * closest_delta.y > distance_squared_upper_bound ||
		closest_delta.z * closest_delta.z > distance_squared_upper_bound)
			return false;


	// Assume that closest points are near to box centers,
	// thus first evaluate boxes with minimum center distance.
	evaluation_potential =
		-((bb_world.min + bb_world.max) * 0.5f -
		  (ba_world.min + ba_world.max) * 0.5f).square_sum();

	
	return true;
}




} /* namespace viral_core */
