/**
 *************************************************************************
 * 
 * @file bounding_intersect_util.cpp
 *
 * Utilities for bounding_intersect tests, implementation.
 *
 ************************************************************************/


#include "bounding_intersect_util.hpp"


#include "geo_vector.hpp"
#include "geo_transform.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_intersect_util
//
//////////////////////////////////////////////////////////////////////////


bool bounding_intersect_util::shape_intersecting
	(const sphere& sa, const si_transform& transform_ta,
	 const sphere& sb, const si_transform& transform_tb,
	 float& evaluation_potential) NOTHROW
{
	// Do not use geo_intersect::intersecting() here,
	// no need to perform transforms twice. 
	sphere sa_transformed_bounded
		(sa.transform_and_bound(transform_ta));
	sphere sb_transformed_bounded
		(sb.transform_and_bound(transform_tb));

	if (sa_transformed_bounded.empty() ||
		sb_transformed_bounded.empty())
			return false;

	float radius_sum =
		sa_transformed_bounded.radius +
		sb_transformed_bounded.radius;


	// Probability for intersection increases
	// with shared volume of bounding shapes.
	evaluation_potential =
		sa_transformed_bounded.intersection_volume
			(sb_transformed_bounded);


	// Avoid sqrtf() of sphere::intersecting().
	return
		(sa_transformed_bounded.position -
			sb_transformed_bounded.position).square_sum() <
				radius_sum * radius_sum;
}


bool bounding_intersect_util::shape_intersecting
	(const box& ba, const si_transform& transform_ta,
	 const matrix& to_world_matrix_ta,
	 const box& bb, const si_transform& transform_tb,
	 const matrix& to_world_matrix_tb,
	 float& evaluation_potential) NOTHROW
{
	// Do not use geo_intersect::intersecting() here,
	// because tests for oriented boxes are slow. 
	box ba_transformed_bounded
		(ba.transform_and_bound(to_world_matrix_ta));
	box bb_transformed_bounded
		(bb.transform_and_bound(to_world_matrix_tb));

	if (ba_transformed_bounded.empty() ||
		bb_transformed_bounded.empty())
			return false;


	// Probability for intersection increases
	// with shared volume of bounding shapes.
	evaluation_potential =
		ba_transformed_bounded.intersection_volume
			(bb_transformed_bounded);


	return
		ba_transformed_bounded.intersecting
			(bb_transformed_bounded);
}




} /* namespace viral_core */