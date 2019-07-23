/**
 *************************************************************************
 *
 * @file sphere_tree_scene.hpp
 *
 * Bounding scenes over implicit sphere trees.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_HPP)
#define INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"


#include "util.hpp"
#include "shared_pointer.hpp"
#include "geo_transform.hpp"
#include "bounding_scene.hpp"


namespace viral_core
{


/**
 * A bounding_scene over the @ref sphere_tree%s in a sphere_tree_scene.
 * @ingroup bounding_module
 */
typedef bounding_scene<sphere, sphere_tree_scene_object,
					   sphere_tree_scene_builder>
							sphere_tree_scene_trees_bounding;




/**
 *************************************************************************
 *
 * @class sphere_tree_scene_object
 * @ingroup bounding_module
 *
 * An instance of a @ref sphere_tree in the hierarchy
 * of a sphere_tree_scene.
 *
 * Usage notes:
 * - Clients must use sphere_tree_scene::update() to change
 *		sphere_tree_scene_object%s in a sphere_tree_scene.
 *		This ensures an up-to-date bounding volume hierarchy.
 *
 * Implementation notes:
 * - To prohibit clients from changing sphere_tree_scene_object%s with
 *		bounding_const_node_iterator and bounding_scene_node::object(),
 *		the public interface of sphere_tree_scene_object is read-only.
 *		See bounding_scene for further details.
 *
 ************************************************************************/
class sphere_tree_scene_object
{
	friend class sphere_tree_scene;
	friend class sphere_tree_scene_builder;


	public:

		const shared_instance<::viral_core::sphere_tree>&
			sphere_tree() const NOTHROW;

		const si_transform& transform() const NOTHROW;
		const matrix& to_world_matrix() const NOTHROW;


	private:

		sphere_tree_scene_object
			(const shared_instance<::viral_core::sphere_tree>& sphere_tree,
			 const si_transform& transform);

		void update_transform
			(const si_transform& transform) NOTHROW;


		const shared_instance<::viral_core::sphere_tree> sphere_tree_;
		si_transform transform_;
		matrix to_world_matrix_;
};




/**
 ************************************************************************* 
 *
 * @class sphere_tree_scene_builder
 * @ingroup bounding_module
 *
 * Utility functions for building a bounding_scene
 * on sphere_tree_scene_object%s of a sphere_tree_scene.
 *
 ************************************************************************/
class sphere_tree_scene_builder :
	private non_copyable
{

	public:

		sphere bounding_shape
			(const sphere_tree_scene_object& object) const NOTHROW;

		sphere combine_shapes
			(const sphere& sa, const sphere& sb) const NOTHROW;

		float score_insert
 			(const sphere& shape,
 			 const sphere& left_shape,
			 const sphere& right_shape) const NOTHROW;
};




/**
 *************************************************************************
 *
 * @class sphere_tree_scene
 * @ingroup bounding_module
 *
 * A bounding volume hierarchy over si_transform%ed sphere_tree%s.
 *
 ************************************************************************/
class sphere_tree_scene :
	private non_copyable
{

	public:

		typedef sphere_tree_scene_trees_bounding::shape_type shape_type;
		typedef sphere_tree_scene_trees_bounding::object_type object_type;

		typedef sphere_tree_scene_trees_bounding::node_type node_type;
		typedef sphere_tree_scene_trees_bounding::const_node_iterator const_node_iterator;

		typedef sphere_tree_scene_trees_bounding::object_iterator object_iterator;
		typedef sphere_tree_scene_trees_bounding::const_object_iterator const_object_iterator;


		sphere_tree_scene();


		/**
		 * Insert a new sphere_tree object into the scene
		 * at an initial similarity @bref{transform}ation.
		 */
		object_iterator insert
			(const shared_instance<::viral_core::sphere_tree>& sphere_tree,
			 const si_transform& transform);


		/**
		 * Change the @bref{transform}ation of a sphere_tree object
		 * through the given @bref{it}erator.
		 */
		void update
			(const object_iterator& it,
			 const si_transform& transform) NOTHROW;


		/** Remove sphere_tree object for some @bref{it}erator. */
		void remove
			(const object_iterator& it) NOTHROW;


		/** Bounding volume hierarchy traversal. */
		//@{
		const_node_iterator root_node() const NOTHROW;

		const_node_iterator parent_node
			(const const_node_iterator& node) const NOTHROW;
		const_node_iterator left_node
			(const const_node_iterator& node) const NOTHROW;
		const_node_iterator right_node
			(const const_node_iterator& node) const NOTHROW;
		//@}


		/** Access to individual sphere_tree hierarchies on leaf nodes. */
		const sphere_tree& detail_hierarchy
			(const sphere_tree_scene_spheres_const_node_iterator& node)
				const NOTHROW;


		/** Access to the encapsulated object list. */
		//@{
		object_iterator first_object() NOTHROW;
		object_iterator last_object() NOTHROW;

		const_object_iterator first_object() const NOTHROW;
		const_object_iterator last_object() const NOTHROW;
		//@}


		/** Clear the entire sphere_tree_scene. */
		void clear() NOTHROW;


		/** Fetch the internal bounding_scene. */
		const sphere_tree_scene_trees_bounding&
			trees_bounding() const NOTHROW;


	private:
		
		sphere_tree_scene_builder builder_;
		sphere_tree_scene_trees_bounding trees_bounding_;
};


/**
 * A bounding volume hierarchy wrapper over a sphere_tree_scene
 * that allows traversal up to the sphere level.
 *
 * @ingroup bounding_module
 */
//@{
typedef bounding_combiner<sphere_tree_scene, sphere_tree>
			sphere_tree_scene_spheres;
typedef bounding_combiner_iterator<sphere_tree_scene, sphere_tree>
			sphere_tree_scene_spheres_const_node_iterator;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SPHERE_TREE_SCENE_HPP) */
