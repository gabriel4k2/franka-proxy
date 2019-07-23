/**
 *************************************************************************
 *
 * @file geo_distance.hpp
 *
 * Geometry distance utilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GEO_DISTANCE_HPP)
#define INCLUDED__VIRAL_CORE__GEO_DISTANCE_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"
#include "geo_vector.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class geo_distance
 * @ingroup geo_module
 *
 * Static utilities that find approach distances and closest points
 * for various geometric primitives.
 *
 ************************************************************************/
class geo_distance :
	private non_constructable
{
	public:


		/**
		 * Determine minimal distance and closest points
		 * for various primitives.
		 *
		 * Usage notes:
		 * - Variants try_distance() and try_distance_squared()
		 *		return @false, zero distances and zero points
		 *		on empty input primitives.
		 * - Distance tests have not been optimized thoroughly.
		 *		Clients should run suitable broad-phase tests,
		 *		e.g. through bounding_tree or mesh_scene.
		 *
		 * Results:
		 * - If there are multiple point pairs of minimum distance,
		 *		any of these point pairs may result.
		 * - For transformed primitives, both resulting closest points
		 *		are given in world space.
		 * - Broken primitive transformations (e.g. negative scaling)
		 *		may cause wrong (but well-behaved) output.
		 */
		//@{		
		static float distance
			(const vector& pa, const vector& pb,
			 vector& closest_point_pa, vector& closest_point_pb) NOTHROW;
		static float distance
			(const vector& p, const line& l,
			 vector& closest_point_l) NOTHROW;			
		static float distance
			(const vector& p, const segment& s,
			 vector& closest_point_s) NOTHROW;
		static float distance
			(const vector& p, const triangle& t,
			 vector& closest_point_t) NOTHROW;

		static bool try_distance
			(const vector& p, const sphere& s,
			 vector& closest_point_s,
			 float& closest_distance) NOTHROW;
		static bool try_distance
			(const vector& p, const box& b,
			 vector& closest_point_b,
			 float& closest_distance) NOTHROW;

		static float distance
			(const line& la, const line& lb,
			 vector& closest_point_la, vector& closest_point_lb) NOTHROW;

		static float distance
			(const segment& sa, const segment& sb,
			 vector& closest_point_sa, vector& closest_point_sb) NOTHROW;
		static float distance
			(const segment& s, const triangle& t,
			 vector& closest_point_s, vector& closest_point_t) NOTHROW;

		static float distance
			(const triangle& ta, const triangle& tb,
			 vector& closest_point_ta, vector& closest_point_tb) NOTHROW;
		static float distance
			(const triangle& ta, const matrix& transform_ta,
			 const triangle& tb, const matrix& transform_tb,
			 vector& closest_point_ta, vector& closest_point_tb) NOTHROW;

		static bool try_distance
			(const triangle& t, const box& b,
			 vector& closest_point_t, vector& closest_point_b,
			 float& closest_distance) NOTHROW;
		static bool try_distance
			(const triangle& t, const matrix& transform_t,
			 const box& b, const si_transform& transform_b,
			 vector& closest_point_t, vector& closest_point_b,
			 float& closest_distance) NOTHROW;

		static bool try_distance
			(const sphere& sa, const sphere& sb,
			 vector& closest_point_sa, vector& closest_point_sb,
			 float& closest_distance) NOTHROW;
		static bool try_distance
			(const sphere& sa, const si_transform& transform_sa,
			 const sphere& sb, const si_transform& transform_sb,
			 vector& closest_point_sa, vector& closest_point_sb,
			 float& closest_distance) NOTHROW;

		static bool try_distance
			(const sphere& s, const box& b,
			 vector& closest_point_s, vector& closest_point_b,
			 float& closest_distance) NOTHROW;

		static bool try_distance
			(const box& ba, const box& bb,
			 vector& closest_point_ba, vector& closest_point_bb,
			 float& closest_distance) NOTHROW;
		static bool try_distance
			(const box& ba, const si_transform& transform_ba,
			 const box& bb, const si_transform& transform_bb,
			 vector& closest_point_ba, vector& closest_point_bb,
			 float& closest_distance) NOTHROW;
		//@}


		/**
		 * Squared distance().
		 * Possibly avoids expensive sqrt() operations.
		 */
		//@{
		static float distance_squared
			(const vector& pa, const vector& pb,
			 vector& closest_point_pa, vector& closest_point_pb) NOTHROW;
		static float distance_squared
			(const vector& p, const line& l,
			 vector& closest_point_l) NOTHROW;			
		static float distance_squared
			(const vector& p, const segment& s,
			 vector& closest_point_s) NOTHROW;
		static float distance_squared
			(const vector& p, const triangle& t,
			 vector& closest_point_t) NOTHROW;

		static bool try_distance_squared
			(const vector& p, const sphere& s,
			 vector& closest_point_s,
			 float& closest_distance_squared) NOTHROW;
		static bool try_distance_squared
			(const vector& p, const box& b,
			 vector& closest_point_b,
			 float& closest_distance_squared) NOTHROW;

		static float distance_squared
			(const line& la, const line& lb,
			 vector& closest_point_la, vector& closest_point_lb) NOTHROW;

		static float distance_squared
			(const segment& sa, const segment& sb,
			 vector& closest_point_sa, vector& closest_point_sb) NOTHROW;
		static float distance_squared
			(const segment& s, const triangle& t,
			 vector& closest_point_s, vector& closest_point_t) NOTHROW;

		static float distance_squared
			(const triangle& ta, const triangle& tb,
			 vector& closest_point_ta, vector& closest_point_tb) NOTHROW;
		static float distance_squared
			(const triangle& ta, const matrix& transform_ta,
			 const triangle& tb, const matrix& transform_tb,
			 vector& closest_point_ta, vector& closest_point_tb) NOTHROW;

		static bool try_distance_squared
			(const triangle& t, const box& b,
			 vector& closest_point_t, vector& closest_point_b,
			 float& closest_distance_squared) NOTHROW;
		static bool try_distance_squared
			(const triangle& t, const matrix& transform_t,
			 const box& b, const si_transform& transform_b,
			 vector& closest_point_t, vector& closest_point_b,
			 float& closest_distance_squared) NOTHROW;

		static bool try_distance_squared
			(const sphere& sa, const sphere& sb,
			 vector& closest_point_sa, vector& closest_point_sb,
			 float& closest_distance_squared) NOTHROW;
		static bool try_distance_squared
			(const sphere& sa, const si_transform& transform_sa,
			 const sphere& sb, const si_transform& transform_sb,
			 vector& closest_point_sa, vector& closest_point_sb,
			 float& closest_distance_squared) NOTHROW;

		static bool try_distance_squared
			(const sphere& s, const box& b,
			 vector& closest_point_s, vector& closest_point_b,
			 float& closest_distance_squared) NOTHROW;

		static bool try_distance_squared
			(const box& ba, const box& bb,
			 vector& closest_point_ba, vector& closest_point_bb,
			 float& closest_distance_squared) NOTHROW;
		static bool try_distance_squared
			(const box& ba, const si_transform& transform_ba,
			 const box& bb, const si_transform& transform_bb,
			 vector& closest_point_ba, vector& closest_point_bb,
			 float& closest_distance_squared) NOTHROW;
		//@}


		/**
		 * Update results from default distance() tests
		 * when sweeping spheres of given radii through
		 * all object points. For example, this turns
		 * segments to capsules and triangles to slabs.
		 */
		static float apply_sphere_sweep
			(float radius_a, float radius_b,
			 vector& closest_point_a, vector& closest_point_b) NOTHROW;


		static const float line_degenerate_epsilon;
		static const float lines_parallel_epsilon;
		static const float sphere_distance_degenerate_epsilon;
		static const float sphere_sweep_epsilon;

		static const float intersect_segment_oneside_epsilon;
		static const float contains_unchecked_epsilon;


	private:

		static float distance_squared
			(const vector& p, const geo_distance_line& l,
			 vector& closest_point_l) NOTHROW;			
		static float distance_squared
			(const vector& p, const geo_distance_segment& s,
			 vector& closest_point_s) NOTHROW;
		static float distance_squared
			(const vector& p, const geo_distance_triangle& t,
			 vector& closest_point_t) NOTHROW;

		static float distance_squared
			(const geo_distance_line& la, const geo_distance_line& lb,
			 vector& closest_point_la, vector& closest_point_lb) NOTHROW;

		static float distance_squared
			(const geo_distance_segment& sa, const geo_distance_segment& sb,
			 vector& closest_point_sa, vector& closest_point_sb) NOTHROW;
		static float distance_squared
			(const geo_distance_segment& s, const geo_distance_triangle& t,
			 vector& closest_point_s, vector& closest_point_t) NOTHROW;

		static float distance_squared
			(const geo_distance_triangle& ta, const geo_distance_triangle& tb,
			 vector& closest_point_ta, vector& closest_point_tb) NOTHROW;


		static bool try_intersect
			(const geo_distance_segment& s,
			 const geo_distance_triangle& t,
			 vector& hit_point);

		static vector closest_on_plane
			(const geo_distance_triangle& t, const vector& p);
		static bool contains_unchecked
			(const geo_distance_triangle& t, const vector& p);		


		static bool try_distance_squared
			(const geo_distance_segment& s, const geo_distance_box_face& bf,
			 vector& closest_point_s, vector& closest_point_bf,
			 float& closest_distance_squared);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GEO_DISTANCE_HPP) */
