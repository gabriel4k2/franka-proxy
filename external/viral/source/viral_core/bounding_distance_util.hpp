/**
 *************************************************************************
 *
 * @file bounding_distance_util.hpp
 *
 * Utilities for bounding_distance metrics.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_DISTANCE_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_DISTANCE_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_distance_util
 * @ingroup bounding_module
 *
 * Utilities for implementing bounding_distance metrics
 * on common bounding shapes and geometry objects.
 *
 ************************************************************************/
class bounding_distance_util :
	private non_constructable
{

	public:

		/**
		 * Find distance bounds of bounding shapes
		 * and corresponding evaluation potential.
		 * Returns @false on empty @bref{shape}s.
		 *
		 * Compatible with try_node_distance_squared()
		 * of bounding_distance metrics.
		 */
		//@{
		static bool try_shape_distance_squared
			(const sphere& sa, const si_transform& transform_ta,
			 const matrix& to_world_matrix_ta,
			 const sphere& sb, const si_transform& transform_tb,
			 const matrix& to_world_matrix_tb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) NOTHROW;

		static bool try_shape_distance_squared
			(const box& ba, const matrix& to_world_matrix_ta,
			 const box& bb, const matrix& to_world_matrix_tb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) NOTHROW;		
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_DISTANCE_UTIL_HPP) */

