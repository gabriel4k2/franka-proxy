/**
 *************************************************************************
 * 
 * @file bounding_intersect_util.hpp
 *
 * Utilities for bounding_intersect tests.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_INTERSECT_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_INTERSECT_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_intersect_util
 * @ingroup bounding_module
 *
 * Utilities for implementing bounding_intersect tests
 * on common bounding shapes and geometry objects.
 *
 ************************************************************************/
class bounding_intersect_util :
	private non_constructable
{

	public:

		/**
		 * Test if two bounding shapes intersect
		 * and find potential for further evaluation,
		 * possibly with false-positive results.
		 *
		 * Compatible with node_intersecting()
		 * of bounding_intersect tests.
		 */
		//@{
		static bool shape_intersecting
			(const sphere& sa, const si_transform& transform_ta,
			 const sphere& sb, const si_transform& transform_tb,
			 float& evaluation_potential) NOTHROW;

		static bool shape_intersecting
			(const box& ba, const si_transform& transform_ta,
			 const matrix& to_world_matrix_ta,
			 const box& bb, const si_transform& transform_tb,
			 const matrix& to_world_matrix_tb,
			 float& evaluation_potential) NOTHROW;	
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_INTERSECT_UTIL_HPP) */