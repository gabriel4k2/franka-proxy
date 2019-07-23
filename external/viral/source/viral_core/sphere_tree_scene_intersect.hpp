/**
 *************************************************************************
 *
 * @file sphere_tree_scene_intersect.hpp
 *
 * Intersection tests in a sphere_tree_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_INTERSECT_HPP)
#define INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_INTERSECT_HPP


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
 * @class sphere_tree_scene_intersect_test
 * @ingroup bounding_module
 *
 * A bounding_intersect test for use in sphere_tree_scene_intersect.
 *
 ************************************************************************/
class sphere_tree_scene_intersect_test :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		sphere_tree_scene_intersect_test
			(const si_transform& t_transform,
			 const sphere_tree_scene::object_iterator& t_ignore_object);


	private:

		bool node_intersecting
			(const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 const sphere_tree& t,
			 const sphere_tree::const_node_iterator& t_node,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 const sphere_tree& t,
			 const sphere_tree::const_node_iterator& t_node,
			 bounding_intersect_no_extended_result& extended_result) const;


		const si_transform t_transform_;
		const matrix t_to_world_matrix_;

		const sphere_tree_scene::object_iterator t_ignore_object_;
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_intersect_result
 * @ingroup bounding_module
 *
 * Stores whether a sphere_tree_scene intersects with a sphere_tree
 * and provides iterators to the intersecting leaf nodes.
 *
 * Usage notes:
 * - If there is no intersection, then @bref{intersecting} is @false.
 *		In this case, all iterators are past-end.
 *
 ************************************************************************/
class sphere_tree_scene_intersect_result
{
	friend class sphere_tree_scene_intersect;


	public:

		const bool intersecting;


		/** Any intersecting node and its object in sphere_tree_scene. */
		//@{
		const sphere_tree_scene::const_node_iterator intersecting_scene_node;
		const sphere_tree_scene::object_iterator intersecting_scene_object;
		//@}


		/** Iterator to intersecting leaf node in right-hand sphere_tree. */
		const sphere_tree::const_node_iterator intersecting_sphere_tree_node;


	private:

		sphere_tree_scene_intersect_result() NOTHROW
			:
			intersecting(false)
		{ }


		sphere_tree_scene_intersect_result
			(sphere_tree_scene::const_node_iterator intersecting_scene_node,
			 sphere_tree_scene::object_iterator intersecting_scene_object,
			 sphere_tree::const_node_iterator intersecting_sphere_tree_node) NOTHROW
			:
			intersecting(true),
			intersecting_scene_node(intersecting_scene_node),
			intersecting_scene_object(intersecting_scene_object),
			intersecting_sphere_tree_node(intersecting_sphere_tree_node)
		{ }
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_intersect
 * @ingroup bounding_module
 *
 * Tests the sphere_tree%s in a sphere_tree_scene for intersection
 * against each other and against other geometric objects.
 *
 ************************************************************************/
class sphere_tree_scene_intersect :
	private non_constructable
{

	public:

		/**
		 * Test a sphere_tree_scene_intersect for intersection
		 * with a si_transform%ed sphere_tree.
		 *
		 * Usage notes:
		 * - Clients may optionally supply the @bref{t}ree and its si_transform
		 *		through an iterator to a sphere_tree_scene_object in @bref{scene}.
		 *		This approach suppresses self-collision results on @bref{t_object}.
		 * - On any error, sphere_tree_scene_intersect tests throw an exception.
		 * - See geo_intersect and bounding_intersect for generic notes.
		 */
		//@{
		static sphere_tree_scene_intersect_result intersect
			(const sphere_tree_scene& scene,
			 const sphere_tree& t, const si_transform& t_transform);

		static sphere_tree_scene_intersect_result intersect
			(const sphere_tree_scene& scene,
			 const sphere_tree_scene::object_iterator& t_object);
		//@}


	private:

		typedef bounding_intersect
					<sphere_tree_scene_spheres, sphere_tree,
					 sphere_tree_scene_intersect_test,
					 bounding_intersect_no_extended_result>
							bounding_intersect_sphere_tree_scene_sphere_tree;


		static sphere_tree_scene_intersect_result intersect_util
			(const sphere_tree_scene& scene,
			 const sphere_tree& t, const si_transform& t_transform,
			 const sphere_tree_scene::object_iterator& t_ignore_object);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_INTERSECT_HPP) */