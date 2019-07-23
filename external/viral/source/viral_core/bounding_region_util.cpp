/**
 *************************************************************************
 *
 * @file bounding_region_util.cpp
 *
 * Utilities for bounding region tests, implementation.
 *
 ************************************************************************/


#include "bounding_region_util.hpp"


#include "geo_transform.hpp"
#include "geo_primitive.hpp"
#include "geo_volume.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_region_util
//
//////////////////////////////////////////////////////////////////////////


bool bounding_region_util::try_shape_region
	(const sphere& shape, const si_transform& transform_shape,
	 const sphere& region) NOTHROW
{
	// Do not use geo_intersect::intersecting() here,
	// no need to perform transforms for the region. 
	sphere shape_transformed_bounded
		(shape.transform_and_bound(transform_shape));

	if (shape_transformed_bounded.empty())
		return false;

	float radius_sum =
		shape_transformed_bounded.radius +
		region.radius;


	// Avoid sqrtf() of sphere::intersecting().
	return
		(shape_transformed_bounded.position -
			region.position).square_sum() <
				radius_sum * radius_sum;
}


bool bounding_region_util::try_shape_region
	(const box& shape, const matrix& to_world_matrix_shape,
	 const box& region) NOTHROW
{
	// Do not use geo_intersect::intersecting() here,
	// because tests for oriented boxes are slow. 
	box shape_transformed_bounded
		(shape.transform_and_bound(to_world_matrix_shape));

	if (shape_transformed_bounded.empty())
		return false;

	return
		shape_transformed_bounded.intersecting(region);
}


bool bounding_region_util::try_object_region
	(const triangle& object, const matrix& to_world_matrix_object,
	 const sphere& region) NOTHROW
{
	return
		region.contains(to_world_matrix_object * object.a) &&
		region.contains(to_world_matrix_object * object.b) &&
		region.contains(to_world_matrix_object * object.c);
}


bool bounding_region_util::try_object_region
	(const triangle& object, const matrix& to_world_matrix_object,
	 const box& region) NOTHROW
{
	return
		region.contains(to_world_matrix_object * object.a) &&
		region.contains(to_world_matrix_object * object.b) &&
		region.contains(to_world_matrix_object * object.c);
}




} /* namespace viral_core */

