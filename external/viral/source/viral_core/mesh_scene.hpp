/**
 *************************************************************************
 *
 * @file mesh_scene.hpp
 *
 * Mesh-based bounding volume hierarchy.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_HPP


#include "build_config.hpp"


#include "mesh_forward.hpp"


#include "util.hpp"
#include "shared_pointer.hpp"
#include "geo_transform.hpp"
#include "bounding_scene.hpp"


namespace viral_core
{


/**
 * A bounding volume hierarchy over the triangles
 * of a @ref mesh instance in a mesh_scene.
 * @ingroup mesh_module
 */
typedef box_tree_index mesh_scene_mesh_bounding;


/**
 * A bounding_scene over the @ref mesh%es in a mesh_scene.
 * @ingroup mesh_module
 */
typedef bounding_scene<box, mesh_scene_object, mesh_scene_builder>
			mesh_scene_meshes_bounding;




/**
 *************************************************************************
 *
 * @class mesh_scene_object
 * @ingroup mesh_module
 *
 * An instance of a @ref mesh in the hierarchy of a mesh_scene.
 *
 * Usage notes:
 * - Clients must change mesh_scene_object%s through mesh_scene::update()
 *		to ensure an up-to-date bounding volume hierarchy.
 *
 * Implementation notes:
 * - To prohibit clients from changing mesh_scene_object%s through
 *		bounding_const_node_iterator and bounding_scene_node::object(),
 *		the public interface of mesh_scene_object is read-only.
 *		See bounding_scene for further details.
 *
 ************************************************************************/
class mesh_scene_object
{
	friend class mesh_scene;
	friend class mesh_scene_builder;


	public:

		const shared_instance<::viral_core::mesh>&
			mesh() const NOTHROW;
		const shared_instance<mesh_scene_mesh_bounding>&
			mesh_bounding() const NOTHROW;

		const si_transform& transform() const NOTHROW;
		const matrix& to_world_matrix() const NOTHROW;


	private:

		mesh_scene_object
			(const shared_instance<::viral_core::mesh>& mesh,
			 const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding,
			 const si_transform& transform);


		void update_transform
			(const si_transform& transform) NOTHROW;


		const shared_instance<::viral_core::mesh> mesh_;
		const shared_instance<mesh_scene_mesh_bounding> mesh_bounding_;
		si_transform transform_;
		matrix to_world_matrix_;
};




/**
 ************************************************************************* 
 *
 * @class mesh_scene_builder
 * @ingroup mesh_module
 *
 * Utility functions for building a bounding_scene
 * over the mesh_scene_object%s of a mesh_scene.
 *
 ************************************************************************/
class mesh_scene_builder :
	private non_copyable
{

	public:

		box bounding_shape
			(const mesh_scene_object& object) const NOTHROW;

		box combine_shapes
			(const box& sa, const box& sb) const NOTHROW;

		float score_insert
 			(const box& shape,
 			 const box& left_shape,
			 const box& right_shape) const NOTHROW;
};




/**
 *************************************************************************
 *
 * @class mesh_scene
 * @ingroup mesh_module
 *
 * A bounding volume hierarchy over si_transform%ed @ref mesh objects.
 *
 * Usage notes:
 * - Iterators over the mesh_scene bounding volume hierarchy traverse
 *		only up to the mesh level. Clients must wrap the mesh_scene
 *		into a mesh_scene_triangles for triangle-level traversal.
 *
 * Implementation notes:
 * - Clients must provide meshes and associated indexed bounding trees.
 *		Using indexed bounding trees (as opposed to the explicit variant)
 *		allows clients to associate additional (e.g. per-triangle) data
 *		to the index results of later queries (e.g. mesh_scene_distance)
 *		on a mesh_scene_triangles wrapper.
 *
 ************************************************************************/
class mesh_scene :
	private non_copyable
{

	public:

		typedef mesh_scene_meshes_bounding::shape_type shape_type;
		typedef mesh_scene_meshes_bounding::object_type object_type;

		typedef mesh_scene_meshes_bounding::node_type node_type;
		typedef mesh_scene_meshes_bounding::const_node_iterator const_node_iterator;

		typedef mesh_scene_meshes_bounding::object_iterator object_iterator;
		typedef mesh_scene_meshes_bounding::const_object_iterator const_object_iterator;


		mesh_scene();


		/**
		 * Insert a new mesh object with given @bref{mesh},
		 * associated @bref{mesh_bounding} volume hierarchy,
		 * and initial similarity @bref{transform}ation.
		 */
		object_iterator insert
			(const shared_instance<::viral_core::mesh>& mesh,
			 const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding,
			 const si_transform& transform);


		/**
		 * Change the @bref{transform}ation of a mesh object
		 * through the given @bref{it}erator.
		 */
		void update
			(const object_iterator& it,
			 const si_transform& transform) NOTHROW;


		/** Remove mesh object for some @bref{it}erator. */
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


		/** Access to per-mesh hierarchies on leaf nodes. */
		const mesh_scene_mesh_bounding& detail_hierarchy
			(const mesh_scene_triangles_const_node_iterator& node)
				const NOTHROW;


		/** Access to the encapsulated object list. */
		//@{
		object_iterator first_object() NOTHROW;
		object_iterator last_object() NOTHROW;

		const_object_iterator first_object() const NOTHROW;
		const_object_iterator last_object() const NOTHROW;
		//@}


		/** Clear the entire mesh scene. */
		void clear() NOTHROW;


		/** Fetch the internal bounding_scene. */
		const mesh_scene_meshes_bounding&
			meshes_bounding() const NOTHROW;


	private:
		
		mesh_scene_builder builder_;
		mesh_scene_meshes_bounding meshes_bounding_;
};


/**
 * A bounding volume hierarchy wrapper over a mesh_scene
 * that allows traversal up to the triangle level.
 *
 * @ingroup mesh_module
 */
//@{
typedef bounding_combiner<mesh_scene, mesh_scene_mesh_bounding>
			mesh_scene_triangles;
typedef bounding_combiner_iterator<mesh_scene, mesh_scene_mesh_bounding>
			mesh_scene_triangles_const_node_iterator;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_HPP) */
