/**
 *************************************************************************
 *
 * @file geo_intersect.hpp
 *
 * Intersection tests for geometric primitives.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_INTERSECT_HPP)
#define INCLUDED__VIRAL_CORE__GEO_INTERSECT_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class geo_intersect
 * @ingroup geo_module
 *
 * Determines if geometric primitives of various types intersect.
 *
 * Usage notes:
 * - Tests may give approximate intersections on degenerate primitives
 *		(e.g. triangles with multiple identical points).
 * - Most intersection tests are biased to false positive intersection
 *		for empty and close primitives to improve numerical stability.
 *
 * Implementation notes:
 * - For convenience, some frequent intersection tests are implemented
 *		by classes outside of geo_intersect (e.g. box::intersecting()).
 *
 ************************************************************************/
class geo_intersect :
	private non_constructable
{
	public:

		/**
		 * Test whether two shapes intersect,
		 * potentially after transformations.
		 */
		//@{
		static bool intersecting
			(const triangle& ta, const triangle& tb) NOTHROW;
		static bool intersecting
			(const triangle& ta, const matrix& transform_ta,
			 const triangle& tb, const matrix& transform_tb) NOTHROW;

		static bool intersecting
			(const triangle& t, const box& b) NOTHROW;
		static bool intersecting
			(const triangle& t, const matrix& transform_t,
			 const box& b, const si_transform& transform_b) NOTHROW;

		static bool intersecting
			(const sphere& sa, const sphere& sb) NOTHROW;
		static bool intersecting
			(const sphere& sa, const si_transform& transform_sa,
			 const sphere& sb, const si_transform& transform_sb) NOTHROW;

		static bool intersecting
			(const sphere& s, const box& b) NOTHROW;

		static bool intersecting
			(const box& ba, const box& bb) NOTHROW;
		static bool intersecting
			(const box& ba, const si_transform& transform_ba,
			 const box& bb, const si_transform& transform_bb) NOTHROW;
		//@}


		/**
		 * Project points from two sets onto (non-unit) @bref{axis},
		 * then return @true if projected intervals are disjunct.
		 * Returns @false on overlap or on any errors.
		 *
		 * A degenerate (e.g. almost zero) @bref{axis} returns @true.
		 * Degenerate axes usually derive from degenerate primitives,
		 * where false-negative intersection (e.g. a @true separating
		 * axis test) is preferrable over false-positive intersection.			
		 * Clients should still avoid degenerate axes if possible.
		 *
		 * Clients can condition points by prepare_separating_axis(),
		 * which improves robustness of later separating axis tests.
		 * For instance, prepare_separating_axis() centers all points
		 * around the origin.
		 */
		//@{
		static bool is_separating_axis
			(const vector* points_a, int64 point_count_a,
			 const vector* points_b, int64 point_count_b,
			 const vector& axis) NOTHROW;
		static void prepare_separating_axis
			(vector* points_a, int64 point_count_a,
			 vector* points_b, int64 point_count_b) NOTHROW;
		//@}
		

		static const float intersect_triangle_plane_epsilon;

		static const float intersect_separating_axis_degenerate_epsilon;
		static const float intersect_separating_axis_interval_epsilon;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_INTERSECT_HPP) */
