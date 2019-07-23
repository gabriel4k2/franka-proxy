/**
 *************************************************************************
 *
 * @file sphere_tree_scene_raycast.hpp
 *
 * Raycasting through sphere_tree_scene%s.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_RAYCAST_HPP)
#define INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_RAYCAST_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "geo_volume.hpp"
#include "bounding_implicit.hpp"
#include "sphere_tree_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class sphere_tree_scene_raycast_test
 * @ingroup bounding_module
 * 
 * A bounding_raycast test for use by sphere_tree_scene_raycast.
 *
 ************************************************************************/
class sphere_tree_scene_raycast_test :
	private non_copyable
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;


	private:

		bool try_node_raycast
			(const segment& s,
			 const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;

		bool try_leaf_raycast
			(const segment& s,
			 const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_raycast_result
 * @ingroup bounding_module
 *
 * Stores whether a ray hits any object in a sphere_tree_scene,
 * as well as the first hit point and the signed distance from
 * the ray origin to the hit point.
 *
 * Usage notes:
 * - If the ray does not hit anything, then @bref{intersecting} is @false,
 *		iterators are past-end, and hit point and hit distance are zero.
 * - If there are multiple candidates for the first hit point,
 *		e.g. due to tolerance, then any of these may result.
 *
 ************************************************************************/
class sphere_tree_scene_raycast_result
{
	friend class sphere_tree_scene_raycast;


	public:

		const bool intersecting;

		const sphere_tree_scene::const_node_iterator hit_scene_node;
		const sphere_tree_scene::object_iterator hit_scene_object;

		const vector hit_point;
		const float hit_distance;


	private:

		sphere_tree_scene_raycast_result() NOTHROW
			:
			intersecting(false),
			hit_distance(0.f)
		{ }


		sphere_tree_scene_raycast_result
			(const sphere_tree_scene::const_node_iterator& hit_scene_node,
			 const sphere_tree_scene::object_iterator& hit_scene_object,
			 const vector& hit_point,
			 float hit_distance) NOTHROW
			:
			intersecting(true),
			hit_scene_node(hit_scene_node),
			hit_scene_object(hit_scene_object),
			hit_point(hit_point),
			hit_distance(hit_distance)
		{ }
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_raycast
 * @ingroup bounding_module
 *
 * Static utilities for raycasting through a sphere_tree_scene.
 *
 ************************************************************************/
class sphere_tree_scene_raycast :
	private non_constructable
{

	public:

		/**
		 * Find the first point of intersection, if any,
		 * between a @bref{s}egment and a sphere_tree_scene.
		 *
		 * Usage notes:
		 * - On any error, raycasting tests throw an exception.
		 * - See geo_raycast and bounding_raycast for generic notes,
		 *		notably for details on @bref{hit_distance_bound}
		 *		and @bref{hit_distance_tolerance}.
		 */
		static sphere_tree_scene_raycast_result raycast
			(const segment& s, const sphere_tree_scene& scene,
		 	 float hit_distance_bound, bool hit_distance_bound_valid,
			 float hit_distance_tolerance);


	private:

		typedef bounding_raycast
					<segment, sphere_tree_scene_spheres,
					 sphere_tree_scene_raycast_test,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_sphere_tree_scene;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_RAYCAST_HPP) */
