/**
 *************************************************************************
 *
 * @file geo_raycast.hpp
 *
 * Raycasting for geometric primitives.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_RAYCAST_HPP)
#define INCLUDED__VIRAL_CORE__GEO_RAYCAST_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class geo_raycast
 * @ingroup geo_module
 *
 * Static utilities for raycasting against geometric primitives.
 *
 * Usage notes:
 * - Raycasting may give approximate results on degenerate primitives
 *		(e.g. triangles with multiple identical points).
 * - Most raycasting tests are biased to false positive intersection
 *		for empty and close primitives to improve numerical stability.
 * - Some raycasting tests come in boundary and interior versions.
 *		The former consider only primitive surfaces, while the latter
 *		check against entire volumes.
 *
 ************************************************************************/
class geo_raycast :
	private non_constructable
{

	public:

		/**
		 * Cast a @bref{l}ine or a line @bref{s}egment
		 * against various geometric primitives. 
		 *
		 * Raycasting returns @true if there is an intersection
		 * between the primitive and the line or the line segment.
		 * The call then stores the first point of intersection
		 * along the line or segment direction as @bref{hit_point},
		 * and calculates the signed @bref{hit_distance}, in units,
		 * from the line or segment origin to the hitpoint.
		 *
		 * Raycasting returns @false if there is no intersection.
		 * In this case, @bref{hit_point} and @bref{hit_distance}
		 * both revert to zero.
		 *
		 * Note that (infinite) @ref line%s prefer any hitpoints
		 * with negative distance (e.g. in front of the origin)
		 * over hit points with positive hit distance.
		 */
		//@{
		static bool try_raycast
			(const line& l, const plane& p,
			 vector& hit_point, float& hit_distance) NOTHROW;
		static bool try_raycast
			(const segment& s, const plane& p,
			 vector& hit_point, float& hit_distance) NOTHROW;

		static bool try_raycast
			(const line& l, const triangle& t,
			 vector& hit_point, float& hit_distance) NOTHROW;
		static bool try_raycast
			(const segment& s, const triangle& t,
			 vector& hit_point, float& hit_distance) NOTHROW;

		static bool try_raycast_interior
			(const segment& s, const sphere& sp,
			 vector& hit_point, float& hit_distance) NOTHROW;
		static bool try_raycast_boundary
			(const segment& s, const sphere& sp,
			 vector& hit_point, float& hit_distance) NOTHROW;

		static bool try_raycast_interior
			(const segment& s, const box& b,
			 vector& hit_point, float& hit_distance) NOTHROW;
		static bool try_raycast_boundary
			(const segment& s, const box& b,
			 vector& hit_point, float& hit_distance) NOTHROW;
		//@}


		static const float intersect_segment_epsilon;
		static const float intersect_segment_oneside_epsilon;
		static const float intersect_line_degenerate_epsilon;
		static const float intersect_line_parallel_epsilon;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_RAYCAST_HPP) */
