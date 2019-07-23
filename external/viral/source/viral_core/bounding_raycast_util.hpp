/**
 *************************************************************************
 *
 * @file bounding_raycast_util.hpp
 *
 * Utilities for bounding_raycast tests.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_RAYCAST_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_RAYCAST_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_raycast_util
 * @ingroup bounding_module
 *
 * Utilities for implementing bounding_raycast tests
 * on common bounding shapes and geometry objects.
 *
 ************************************************************************/
class bounding_raycast_util :
	private non_constructable
{

	public:

		/**
		 * Test if a ray segment intersects a bounding shape
		 * and find potential for further evaluation.
		 *
		 * Compatible with try_node_raycast()
		 * of bounding_raycast tests.
		 */
		//@{
		static bool try_shape_raycast
			(const segment& s, const sphere& shape,
			 float& hit_distance_lower_bound,
			 float& evaluation_potential) NOTHROW;

		static bool try_shape_raycast
			(const segment& s, const box& shape,
			 float& hit_distance_lower_bound,
			 float& evaluation_potential) NOTHROW;
		//@}


	private:

		template <typename S>
			static bool try_geo_raycast
				(const segment& s, const S& shape,
				 float& hit_distance_lower_bound,
				 float& evaluation_potential) NOTHROW;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_RAYCAST_UTIL_HPP) */