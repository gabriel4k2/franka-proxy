/**
 *************************************************************************
 *
 * @file bounding_k_nearest_util.cpp
 *
 * Utilities for nearest neighbor queries, implementation.
 *
 ************************************************************************/


#include "bounding_k_nearest_util.hpp"


#include "geo_distance.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_k_nearest_util
//
//////////////////////////////////////////////////////////////////////////


bool bounding_k_nearest_util::try_shape_distance_squared
	(const sphere& shape, const vector& point,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) NOTHROW
{
	vector closest_point_shape;
	if (!geo_distance::try_distance_squared
			(point, shape,
			 closest_point_shape,
			 min_distance_squared_lower_bound))
	{
		return false;
	}

	// Prefer to evaluate close-by shapes.
	evaluation_potential =
		-min_distance_squared_lower_bound;

	return true;
}


bool bounding_k_nearest_util::try_shape_distance_squared
	(const box& shape, const vector& point,
	 float& min_distance_squared_lower_bound,
	 float& evaluation_potential) NOTHROW
{
	vector closest_point_shape;
	if (!geo_distance::try_distance_squared
			(point, shape,
			 closest_point_shape,
			 min_distance_squared_lower_bound))
	{
		return false;
	}

	// Prefer to evaluate close-by shapes.
	evaluation_potential =
		-min_distance_squared_lower_bound;

	return true;
}




} /* namespace viral_core */
