/**
 *************************************************************************
 *
 * @file bounding_region_util.hpp
 *
 * Utilities for bounding region tests.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_REGION_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_REGION_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_region_util
 * @ingroup bounding_module
 *
 * Utilities for implementing bounding_region tests
 * on common bounding shapes and geometry objects.
 *
 ************************************************************************/
class bounding_region_util :
	private non_constructable
{

	public:

		/**
		 * Test if a si_transform%ed bounding @bref{shape}
		 * partially intersects with a non-empty @bref{region},
		 * possibly with false-positive results.
		 *
		 * Compatible with try_node_region()
		 * of bounding_region tests.
		 */
		//@{
		static bool try_shape_region
			(const sphere& shape, const si_transform& transform_shape,
			 const sphere& region) NOTHROW;
		static bool try_shape_region
			(const box& shape, const matrix& to_world_matrix_shape,
			 const box& region) NOTHROW;
		//@}


		/**
		 * Test if a si_transform%ed geometry @bref{object}
		 * is fully contained inside a non-empty @bref{region}.
		 *
		 * Compatible with try_leaf_region()
		 * of bounding_region tests.
		 */
		//@{
		static bool try_object_region
			(const triangle& object, const matrix& to_world_matrix_object,
			 const sphere& region) NOTHROW;
		static bool try_object_region
			(const triangle& object, const matrix& to_world_matrix_object,
			 const box& region) NOTHROW;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_REGION_UTIL_HPP) */
