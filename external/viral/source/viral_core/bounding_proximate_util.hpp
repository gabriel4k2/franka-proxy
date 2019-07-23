/**
 *************************************************************************
 *
 * @file bounding_proximate_util.hpp
 *
 * Utilities for proximity tests.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_PROXIMATE_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_PROXIMATE_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_proximate_util
 * @ingroup bounding_module
 *
 * Utilities for implementing bounding_proximate tests
 * on common bounding shapes and geometry objects. 
 *
 ************************************************************************/
class bounding_proximate_util :
	private non_constructable
{

	public:

		/**
		 * Test if two bounding shapes are proximate
		 * and find potential for further evaluation,
		 * possibly with false-positive results.
		 *
		 * Compatible with node_proximate()
		 * of bounding_proximate tests.
		 */
		//@{
		static bool shape_proximate
			(const sphere& sa, const si_transform& transform_ta,
			 const matrix& to_world_matrix_ta,
			 const sphere& sb, const si_transform& transform_tb,
			 const matrix& to_world_matrix_tb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential);

		static bool shape_proximate
			(const sphere& s, const si_transform& transform_s,
			 const matrix& to_world_matrix_s,
			 const box& b, const si_transform& transform_b,
			 float distance_squared_upper_bound,
			 float& evaluation_potential);

		static bool shape_proximate
			(const box& ba, const matrix& to_world_matrix_ta,
			 const box& bb, const matrix& to_world_matrix_tb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential);
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_PROXIMATE_UTIL_HPP) */
