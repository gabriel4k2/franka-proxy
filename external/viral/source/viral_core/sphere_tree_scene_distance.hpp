/**
 *************************************************************************
 *
 * @file sphere_tree_scene_distance.hpp
 *
 * Distances in a sphere_tree_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_DISTANCE_HPP)
#define INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_DISTANCE_HPP


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
 * @class sphere_tree_scene_distance_metric
 * @ingroup bounding_module
 *
 * A bounding_distance metric for use in sphere_tree_scene_distance.
 *
 ************************************************************************/
class sphere_tree_scene_distance_metric :
	private non_copyable
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:

		sphere_tree_scene_distance_metric
			(const si_transform& t_transform,
			 const sphere_tree_scene::object_iterator& t_ignore_object);


	private:

		bool try_node_distance_squared
			(const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 const sphere_tree& t,
			 const sphere_tree::const_node_iterator& t_node,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 const sphere_tree& t,
			 const sphere_tree::const_node_iterator& t_node,
			 vector& stss_closest_point, vector& t_closest_point,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;


		const si_transform t_transform_;
		const matrix t_to_world_matrix_;

		const sphere_tree_scene::object_iterator t_ignore_object_;
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_distance_result
 * @ingroup bounding_module
 *
 * Stores sphere_tree_scene_distance results in form of closest points,
 * the squared distance of these points, and associated node iterators.
 *
 * Usage notes:
 * - If there are no closest points (e.g. for empty scenes or trees),
 *		then @bref{valid} is @false, all node iterators are past-end,
 *		and closest points as well as squared distance remain zero.
 * - Distance tests may pick any of multiple candidates for closest points,
 *		potentially causing an error of up to the distance tolerance.
 *
 ************************************************************************/
class sphere_tree_scene_distance_result
{
	friend class sphere_tree_scene_distance;


	public:

		const bool valid;


		/** Any closest node and its object in sphere_tree_scene. */
		//@{
		const sphere_tree_scene::const_node_iterator closest_scene_node;
		const sphere_tree_scene::object_iterator closest_scene_object;
		//@}


		/** Iterator to closest leaf node in right-hand sphere_tree. */
		const sphere_tree::const_node_iterator closest_sphere_tree_node;


		const vector closest_scene_point;
		const vector closest_sphere_tree_point;

		const float closest_distance_squared;


	private:

		sphere_tree_scene_distance_result() NOTHROW
			:
			valid(false),
			closest_distance_squared(0.f)
		{ }


		sphere_tree_scene_distance_result
			(const sphere_tree_scene::const_node_iterator& closest_scene_node,
			 const sphere_tree_scene::object_iterator& closest_scene_object,
			 const sphere_tree::const_node_iterator& closest_sphere_tree_node,
			 const vector& closest_scene_point,
			 const vector& closest_sphere_tree_point,
			 float closest_distance_squared) NOTHROW
			:
			valid(true),
			closest_scene_node(closest_scene_node),
			closest_scene_object(closest_scene_object),
			closest_sphere_tree_node(closest_sphere_tree_node),
			closest_scene_point(closest_scene_point),
			closest_sphere_tree_point(closest_sphere_tree_point),
			closest_distance_squared(closest_distance_squared)
		{ }
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_distance
 * @ingroup bounding_module
 *
 * Calculates distances on the sphere_tree%s in a sphere_tree_scene.
 *
 ************************************************************************/
class sphere_tree_scene_distance :
	private non_constructable
{

	public:

		/**
		 * Find the closest distance between a sphere_tree_scene
		 * and another, possibly si_transform%ed sphere_tree.
		 *
		 * Usage notes:
		 * - Clients may optionally supply the @bref{t}ree and its si_transform
		 *		with an iterator to a sphere_tree_scene_object in @bref{scene}.
		 *		This approach suppresses self-distance results on @bref{t_object}.
		 * - On any error, sphere_tree_scene_distance tests throw an exception.
		 * - See geo_distance and bounding_distance for generic notes,
		 *		notably for details on @bref{distance_squared_bound}
		 *		and @bref{distance_squared_tolerance}.
		 */
		//@{
		static sphere_tree_scene_distance_result distance
			(const sphere_tree_scene& scene,
			 const sphere_tree& t, const si_transform& t_transform,
			 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);

		static sphere_tree_scene_distance_result distance
			(const sphere_tree_scene& scene,
			 const sphere_tree_scene::object_iterator& t_object,
			 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);
		//@}


	private:

		typedef bounding_distance
					<sphere_tree_scene_spheres, sphere_tree,
					 sphere_tree_scene_distance_metric,
					 bounding_distance_no_extended_result>
							bounding_distance_sphere_tree_scene_sphere_tree;


		static sphere_tree_scene_distance_result distance_util
			(const sphere_tree_scene& scene,
			 const sphere_tree& t, const si_transform& t_transform,
			 const sphere_tree_scene::object_iterator& t_ignore_object,
		 	 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_DISTANCE_HPP) */
