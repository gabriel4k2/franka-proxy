/**
 *************************************************************************
 *
 * @file mesh_scene_raycast.hpp
 *
 * Raycasting through a mesh_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_RAYCAST_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_RAYCAST_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_vector.hpp"
#include "mesh_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_scene_raycast_test
 * @ingroup mesh_module
 * 
 * A bounding_raycast test for use by mesh_scene_raycast.
 *
 ************************************************************************/
class mesh_scene_raycast_test :
	private non_copyable
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;


	private:

		bool try_node_raycast
			(const segment& s,
			 const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;

		bool try_leaf_raycast
			(const segment& s,
			 const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_raycast_result
 * @ingroup mesh_module
 *
 * Stores whether a ray hits any object in a mesh_scene, as well as
 * the first hit point and the signed distance from the ray origin
 * to the hit point.
 *
 * Usage notes:
 * - If the ray does not hit any mesh, then @bref{intersecting} is @false,
 *		iterators are invalid, the triangle index is @bref{invalid_index},
 *		and both the hit point and the hit distance are zero.
 * - If there are multiple candidates for the first hit point,
 *		e.g. due to tolerance, then any of these may result.
 *
 ************************************************************************/
class mesh_scene_raycast_result
{
	friend class mesh_scene_raycast;


	public:

		static const int64 invalid_index = -1;

		const bool intersecting;

		const mesh_scene::const_node_iterator hit_scene_node;
		const mesh_scene::object_iterator hit_scene_object;
		const int64 hit_scene_object_triangle_index;

		const vector hit_point;
		const float hit_distance;


	private:

		mesh_scene_raycast_result() NOTHROW
			:
			intersecting(false),
			hit_scene_object_triangle_index(invalid_index),
			hit_distance(0.f)
		{ }


		mesh_scene_raycast_result
			(const mesh_scene::const_node_iterator& hit_scene_node,
			 const mesh_scene::object_iterator& hit_scene_object,
			 int64 hit_scene_object_triangle_index,
			 const vector& hit_point,
			 float hit_distance) NOTHROW
			:
			intersecting(true),
			hit_scene_node(hit_scene_node),
			hit_scene_object(hit_scene_object),
			hit_scene_object_triangle_index(hit_scene_object_triangle_index),
			hit_point(hit_point),
			hit_distance(hit_distance)
		{ }
};




/**
 *************************************************************************
 *
 * @class mesh_scene_raycast
 * @ingroup mesh_module
 *
 * Static utilities for raycasting through a mesh_scene.
 *
 * Implementation notes:
 * - Current raycasting utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_scene_raycast :
	private non_constructable
{

	public:

		/**
		 * Find the first point of intersection, if any,
		 * between a @bref{s}egment and a mesh_scene.
		 *
		 * Usage notes:
		 * - On any error, mesh_scene_raycast tests throw an exception.
		 * - See geo_raycast and bounding_raycast for generic notes,
		 *		notably for details on @bref{hit_distance_bound}
		 *		and @bref{hit_distance_tolerance}.
		 */
		static mesh_scene_raycast_result raycast
			(const segment& s, const mesh_scene& scene,
		 	 float hit_distance_bound, bool hit_distance_bound_valid,
			 float hit_distance_tolerance);


	private:

		typedef bounding_raycast
					<segment, mesh_scene_triangles,
					 mesh_scene_raycast_test,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_mesh_scene;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_RAYCAST_HPP) */
