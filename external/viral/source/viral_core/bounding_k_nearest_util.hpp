/**
 *************************************************************************
 *
 * @file bounding_k_nearest_util.hpp
 *
 * Utilities for nearest neighbor queries.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_K_NEAREST_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_K_NEAREST_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_k_nearest_util
 * @ingroup bounding_module
 *
 * Utilities for implementing bounding_k_nearest metrics
 * on common bounding shapes and geometry objects.
 *
 ************************************************************************/
class bounding_k_nearest_util :
	private non_constructable
{

	public:

		/**
		 * Find distance bounds of bounding shapes
		 * and corresponding evaluation potential.
		 * Returns @false on empty @bref{shape}s.
		 *
		 * Compatible with try_node_distance_squared()
		 * of bounding_k_nearest metrics.
		 */
		//@{
		static bool try_shape_distance_squared
			(const sphere& shape, const vector& point,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) NOTHROW;

		static bool try_shape_distance_squared
			(const box& shape, const vector& point,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) NOTHROW;		
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_K_NEAREST_UTIL_HPP) */
