/**
 *************************************************************************
 *
 * @file bounding_raycast_util.cpp
 *
 * Utilities for bounding_raycast tests, implementation.
 *
 ************************************************************************/


#include "bounding_raycast_util.hpp"


#include "geo_vector.hpp"
#include "geo_raycast.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// bounding_raycast_util
//
//////////////////////////////////////////////////////////////////////////


bool bounding_raycast_util::try_shape_raycast
	(const segment& s, const sphere& shape,
	 float& hit_distance_lower_bound,
	 float& evaluation_potential) NOTHROW
{
	return try_geo_raycast
				(s, shape,
				 hit_distance_lower_bound,
				 evaluation_potential);
}


bool bounding_raycast_util::try_shape_raycast
	(const segment& s, const box& shape,
	 float& hit_distance_lower_bound,
	 float& evaluation_potential) NOTHROW
{
	return try_geo_raycast
				(s, shape,
				 hit_distance_lower_bound,
				 evaluation_potential);
}


template <typename S>
	bool bounding_raycast_util::try_geo_raycast
		(const segment& s, const S& shape,
		 float& hit_distance_lower_bound,
		 float& evaluation_potential) NOTHROW
{
	vector hit_point;
	float hit_distance = 0.f;

	bool intersecting =
		geo_raycast::try_raycast_interior
			(s, shape, hit_point, hit_distance);

	// Pass through intersection results,
	// prefer to evaluate nearer nodes.
	// No need to check for intersecting,
	// output is ignored on false return.
	hit_distance_lower_bound = hit_distance;
	evaluation_potential = -hit_distance;

	return intersecting;
}




} /* namespace viral_core */
