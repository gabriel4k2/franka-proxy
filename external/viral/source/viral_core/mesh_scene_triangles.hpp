/**
 *************************************************************************
 *
 * @file mesh_scene_triangles.hpp
 *
 * Triangle-level wrapper for mesh scenes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_TRIANGLES_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_TRIANGLES_HPP


#include "build_config.hpp"


#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_volume.hpp"
#include "bounding_tree.hpp"
#include "bounding_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_scene_triangles_iterator
 * @ingroup mesh_module
 *
 * Iterates over a mesh_scene up to the triangles of mesh_scene_object%s.
 *
 ************************************************************************/
class mesh_scene_triangles_iterator :
	private bool_convert<mesh_scene_triangles_iterator>
{
	friend class mesh_scene_triangles;


	public:

		typedef mesh_scene_meshes_bounding::const_node_iterator
			meshes_bounding_const_node_iterator;
		typedef mesh_scene_mesh_bounding::const_node_iterator
			mesh_bounding_const_node_iterator;


		mesh_scene_triangles_iterator() NOTHROW;
		mesh_scene_triangles_iterator
			(const mesh_scene_triangles_iterator& other) NOTHROW;


		mesh_scene_triangles_iterator&
			operator =(const mesh_scene_triangles_iterator& other) NOTHROW;


		/**
		 * Determine if the iterator is not past-end yet.
		 * Note valid iterators may still have a past-end mesh_bounding_node().
		 */
		operator bool_conversion_type() const NOTHROW;


		void swap_with
			(mesh_scene_triangles_iterator& other) NOTHROW;


		/** Iterator comparison. */
		//@{
		bool operator ==
			(const mesh_scene_triangles_iterator& other) const NOTHROW;
		bool operator !=
			(const mesh_scene_triangles_iterator& other) const NOTHROW;
		//@}


		const meshes_bounding_const_node_iterator& meshes_bounding_node() const NOTHROW;
		const mesh_bounding_const_node_iterator& mesh_bounding_node() const NOTHROW;


	private:

		/**
		 * Wrap scene and bounding iterators into combined iterator.
		 * Private to prohibit clients from creating broken iterators.
		 */
		mesh_scene_triangles_iterator
			(const meshes_bounding_const_node_iterator& meshes_bounding_node,
			 const mesh_bounding_const_node_iterator& mesh_bounding_node) NOTHROW;


		meshes_bounding_const_node_iterator meshes_bounding_node_;
		mesh_bounding_const_node_iterator mesh_bounding_node_;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_triangles
 * @ingroup mesh_module
 *
 * A bounding volume hierarchy wrapper over a mesh_scene
 * that allows traversal up to the triangle level.
 *
 * Usage notes:
 * - Iterators over a mesh_scene_triangles remain valid only until
 *		either the mesh_scene or its instanced meshes are modified.
 *		Clients thus should only keep a mesh_scene_triangles alive
 *		over respective uses (e.g. for one bounding_distance call).
 *
 * Implementation notes:
 * - Even with overhead owing to the larger mesh_scene_triangles_iterator,
 *		triangle-level operations through mesh_scene_triangles outperform
 *		two-staged variants (e.g. bounding_distance on a mesh_scene calling
 *		bounding_distance on meshes) for more than a few thousand triangles.
 *		Notably, a mesh_scene_triangles can maintain a mean-case O(log n),
 *		while alternatives degrade to a less efficient O(log n * log n).
 *
 ************************************************************************/
class mesh_scene_triangles :
	private non_copyable
{

	public:

		typedef mesh_scene_triangles_iterator const_node_iterator;


		/**
		 * Create a triangle-level wrapper around a mesh_scene.
		 * The @bref{wrapped_scene} must outlive the mesh_scene_triangles.
		 */
		mesh_scene_triangles
			(const mesh_scene& wrapped_scene) NOTHROW;


		/**
		 * Bounding volume hierarchy interface that traverses
		 * the @bref{wrapped_scene} up to individual triangles.
		 */
		//@{
		mesh_scene_triangles_iterator root_node() const NOTHROW;

		mesh_scene_triangles_iterator parent_node
			(const mesh_scene_triangles_iterator& node) const NOTHROW;
		mesh_scene_triangles_iterator left_node
			(const mesh_scene_triangles_iterator& node) const NOTHROW;
		mesh_scene_triangles_iterator right_node
			(const mesh_scene_triangles_iterator& node) const NOTHROW;
		//@}


		const mesh_scene& wrapped_scene;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_TRIANGLES_HPP) */