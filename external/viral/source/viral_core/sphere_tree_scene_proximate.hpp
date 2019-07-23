/**
 *************************************************************************
 *
 * @file sphere_tree_scene_proximate.hpp
 *
 * Proximity tests in a sphere_tree_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_PROXIMATE_HPP)
#define INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_PROXIMATE_HPP


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
 * @class sphere_tree_scene_proximate_test
 * @ingroup bounding_module
 *
 * A bounding_proximate test for use in sphere_tree_scene_proximate.
 *
 ************************************************************************/
class sphere_tree_scene_proximate_test :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:

		sphere_tree_scene_proximate_test
			(const si_transform& t_transform,
			 const sphere_tree_scene::object_iterator& t_ignore_object);


	private:

		bool node_proximate
			(const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 const sphere_tree& t,
			 const sphere_tree::const_node_iterator& t_node,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;

		bool leaf_proximate
			(const sphere_tree_scene_spheres& stss,
			 const sphere_tree_scene_spheres_const_node_iterator& stss_node,
			 const sphere_tree& t,
			 const sphere_tree::const_node_iterator& t_node,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;


		const si_transform t_transform_;
		const matrix t_to_world_matrix_;

		const sphere_tree_scene::object_iterator t_ignore_object_;
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_proximate_result
 * @ingroup bounding_module
 *
 * Stores proximate nodes of a sphere_tree_scene and a sphere_tree.
 *
 * Usage notes:
 * - If scene and tree are farther than @bref{distance_squared_upper_bound},
 *		then @bref{valid} is @false, and all iterators are past-end.
 * - Proximity tests may pick any of multiple candidates for close spheres,
 *		potentially causing an error of up to the distance bound.
 *
 ************************************************************************/
class sphere_tree_scene_proximate_result
{
	friend class sphere_tree_scene_proximate;


	public:

		const bool valid;


		/** Any proximate node and its object in sphere_tree_scene. */
		//@{
		const sphere_tree_scene::const_node_iterator proximate_scene_node;
		const sphere_tree_scene::object_iterator proximate_scene_object;
		//@}


		/** Iterator to proximate leaf node in right-hand sphere_tree. */
		const sphere_tree::const_node_iterator proximate_sphere_tree_node;


	private:

		sphere_tree_scene_proximate_result() NOTHROW
			:
			valid(false)
		{ }


		sphere_tree_scene_proximate_result
			(const sphere_tree_scene::const_node_iterator& proximate_scene_node,
			 const sphere_tree_scene::object_iterator& proximate_scene_object,
			 const sphere_tree::const_node_iterator& proximate_sphere_tree_node) NOTHROW
			:
			valid(true),
			proximate_scene_node(proximate_scene_node),
			proximate_scene_object(proximate_scene_object),
			proximate_sphere_tree_node(proximate_sphere_tree_node)
		{ }
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_proximate
 * @ingroup bounding_module
 *
 * Tests the sphere_tree%s in a sphere_tree_scene for proximity
 * against each other and against other sphere_tree%s.
 *
 ************************************************************************/
class sphere_tree_scene_proximate :
	private non_constructable
{

	public:

		/**
		 * Test a sphere_tree_scene for proximity
		 * with a si_transform%ed sphere_tree.
		 *
		 * Usage notes:
		 * - Clients may optionally supply the @bref{t}ree and its si_transform
		 *		with an iterator to a sphere_tree_scene_object in @bref{scene}.
		 *		This approach suppresses self-proximity results on the object.
		 * - On any error, sphere_tree_scene_proximate tests throw an exception.
		 * - See bounding_proximate documentation for generic notes on usage,
		 *		notably for @bref{distance_squared_upper_bound}
		 */
		//@{
		static sphere_tree_scene_proximate_result proximate
			(const sphere_tree_scene& scene,
			 const sphere_tree& t, const si_transform& t_transform,
			 float distance_squared_upper_bound);

		static sphere_tree_scene_proximate_result proximate
			(const sphere_tree_scene& scene,
			 const sphere_tree_scene::object_iterator& t_object,
			 float distance_squared_upper_bound);
		//@}


	private:

		typedef bounding_proximate
					<sphere_tree_scene_spheres, sphere_tree,
					 sphere_tree_scene_proximate_test,
					 bounding_proximate_no_extended_result>
							bounding_proximate_sphere_tree_scene_sphere_tree;


		static sphere_tree_scene_proximate_result proximate_util
			(const sphere_tree_scene& scene,
			 const sphere_tree& t, const si_transform& t_transform,
			 const sphere_tree_scene::object_iterator& t_ignore_object,
			 float distance_squared_upper_bound);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_PROXIMATE_HPP) */
