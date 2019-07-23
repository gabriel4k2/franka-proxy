/**
 *************************************************************************
 *
 * @file bounding_implicit_raycast.hpp
 *
 * Raycasting for implicit bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_RAYCAST_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_RAYCAST_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "bounding_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 * Various bounding_raycast_result%s for common bounding_implicit%s.
 * @ingroup mesh_module
 */
//@{
typedef bounding_raycast_result
			<sphere_tree, bounding_raycast_no_extended_result>
					bounding_implicit_raycast_result_sphere_tree;

typedef bounding_raycast_result
			<box_tree, bounding_raycast_no_extended_result>
					bounding_implicit_raycast_result_box_tree;
//@}




/**
 *************************************************************************
 *
 * @class bounding_implicit_raycast_test
 * @ingroup bounding_module
 * 
 * A bounding_raycast test for bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_raycast_test :
	private non_copyable
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;

	private:

		bool try_node_raycast
			(const segment& s, const sphere_tree& t,
			 const sphere_tree_const_node_iterator& n,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;
		bool try_node_raycast
			(const segment& s, const box_tree& t,
			 const box_tree_const_node_iterator& n,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;

		bool try_leaf_raycast
			(const segment& s, const sphere_tree& t,
			 const sphere_tree_const_node_iterator& n,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
		bool try_leaf_raycast
			(const segment& s, const box_tree& t,
			 const box_tree_const_node_iterator& n,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
};




/**
 ************************************************************************* 
 *
 * @class bounding_implicit_raycast
 * @ingroup bounding_module
 *
 * Static utilities that test for intersection between a ray
 * and a possibly transformed bounding_implicit.
 *
 ************************************************************************/
class bounding_implicit_raycast :
	private non_constructable
{

	public:

		/**
		 * Find the first point of intersection, if any,
		 * between a segment and a transformed bounding_implicit.
		 *
		 * Usage notes:
		 * - On any error, raycasting tests throw an exception.
		 * - See geo_raycast and bounding_raycast for generic notes,
		 *		notably for details on @bref{hit_distance_bound}
		 *		and @bref{hit_distance_tolerance}.
		 *
		 * Implementation notes:
		 * - To avoid expensive transforms per shape and object,
		 *		we instead transform the incoming @bref{s}egment
		 *		by the inverse of @bref{transform_t}.
		 */
		//@{
		static bounding_implicit_raycast_result_sphere_tree
			raycast
				(const segment& s, const sphere_tree& t,
				 const si_transform& transform_t,
		 		 float hit_distance_bound, bool hit_distance_bound_valid,
				 float hit_distance_tolerance);

		static bounding_implicit_raycast_result_box_tree
			raycast
				(const segment& s, const box_tree& t,
				 const si_transform& transform_t,
		 		 float hit_distance_bound, bool hit_distance_bound_valid,
				 float hit_distance_tolerance);
		//@}


	private:

		typedef bounding_raycast
					<segment, sphere_tree,
					 bounding_implicit_raycast_test,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_sphere_tree;

		typedef bounding_raycast
					<segment, box_tree,
					 bounding_implicit_raycast_test,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_box_tree;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_RAYCAST_HPP) */

