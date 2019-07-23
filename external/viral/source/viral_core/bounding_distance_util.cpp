/**
 *************************************************************************
 *
 * @file bounding_distance_util.cpp
 *
 * Utilities for bounding_distance metrics, implementation.
 *
 ************************************************************************/


#include "bounding_distance_util.hpp"


#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_distance_util
//
//////////////////////////////////////////////////////////////////////////


bool bounding_distance_util::try_shape_distance_squared
	(const sphere& sa, const si_transform& transform_ta,
	 const matrix& to_world_matrix_ta,
	 const sphere& sb, const si_transform& transform_tb,
	 const matrix& to_world_matrix_tb,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) NOTHROW
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
	float min_distance_lower_bound =
		max(center_distance -
				(world_radius_sa + world_radius_sb), 0.f);

	min_distance_squared_lower_bound =
		min_distance_lower_bound * min_distance_lower_bound;


	// Assume that closest points are near to sphere centers,
	// thus first evaluate spheres with minimum center distance.
	evaluation_potential =
		-(center_distance * center_distance);


	return true;
}


bool bounding_distance_util::try_shape_distance_squared
	(const box& ba, const matrix& to_world_matrix_ta,
	 const box& bb, const matrix& to_world_matrix_tb,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) NOTHROW
{
	// Build world-space bounding boxes
	// over transformed, oriented boxes.
	box ba_world(ba.transform_and_bound(to_world_matrix_ta));
	box bb_world(bb.transform_and_bound(to_world_matrix_tb));

	if (ba_world.empty() || bb_world.empty())
		return false;


	// Closest distance between two axis aligned boxes
	// occurs per-component from a maximum to a minimum.
	min_distance_squared_lower_bound =
		vector().max(bb_world.min - ba_world.max)
				.max(ba_world.min - bb_world.max)
				.square_sum();


	// Assume that closest points are near to box centers,
	// thus first evaluate boxes with minimum center distance.
	evaluation_potential =
		-((bb_world.min + bb_world.max) * 0.5f -
		  (ba_world.min + ba_world.max) * 0.5f).square_sum();


	return true;
}




} /* namespace viral_core */
