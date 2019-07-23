/**
 *************************************************************************
 *
 * @file mesh_scene_intersect.hpp
 *
 * Intersection tests on mesh_scenes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_INTERSECT_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_INTERSECT_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "geo_volume.hpp"
#include "bounding_tree.hpp"
#include "mesh_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_scene_intersect_test
 * @ingroup mesh_module
 *
 * A bounding_intersect test for use in mesh_scene_intersect.
 *
 ************************************************************************/
class mesh_scene_intersect_test :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		mesh_scene_intersect_test
			(const mesh& m, const si_transform& m_transform,
			 const mesh_scene::object_iterator& m_ignore_object);


	private:

		bool node_intersecting
			(const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
			 bounding_intersect_no_extended_result& extended_result) const;


		const mesh& m_; // Don't copy, expensive.
		const si_transform m_transform_;
		const matrix m_to_world_matrix_;

		const mesh_scene::object_iterator m_ignore_object_;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_intersect_result
 * @ingroup mesh_module
 *
 * Stores whether a mesh_scene intersects with a @ref mesh
 * and provides iterators to the intersecting primitives.
 *
 * Usage notes:
 * - If there is no intersection, then @bref{intersecting} is @false.
 *		In this case, both triangle indices are @bref{invalid_index},
 *		and both scene iterators are past-end.
 *
 ************************************************************************/
class mesh_scene_intersect_result
{
	friend class mesh_scene_intersect;


	public:

		static const int64 invalid_index = -1;


		const bool intersecting;


		/** Intersecting node and object in mesh_scene. */
		//@{
		const mesh_scene::const_node_iterator intersecting_scene_node;
		const mesh_scene::object_iterator intersecting_scene_object;
		//@}

		/** Index for intersecting triangle of intersecting mesh_scene object. */
		const int64 intersecting_scene_object_triangle_index;

		/** Index for intersecting triangle in right-hand mesh. */
		const int64 intersecting_mesh_triangle_index;


	private:

		mesh_scene_intersect_result() NOTHROW
			:
			intersecting(false),
			intersecting_scene_object_triangle_index(invalid_index),
			intersecting_mesh_triangle_index(invalid_index)
		{ }


		mesh_scene_intersect_result
			(mesh_scene::const_node_iterator intersecting_scene_node,
			 mesh_scene::object_iterator intersecting_scene_object,
			 int64 intersecting_scene_object_triangle_index,
			 int64 intersecting_mesh_triangle_index) NOTHROW
			:
			intersecting(true),
			intersecting_scene_node(intersecting_scene_node),
			intersecting_scene_object(intersecting_scene_object),
			intersecting_scene_object_triangle_index(intersecting_scene_object_triangle_index),
			intersecting_mesh_triangle_index(intersecting_mesh_triangle_index)
		{ }
};




/**
 *************************************************************************
 *
 * @class mesh_scene_intersect
 * @ingroup mesh_module
 *
 * Tests the mesh%es in a mesh_scene for intersection against each other
 * and against other geometric objects.
 *
 ************************************************************************/
class mesh_scene_intersect :
	private non_constructable
{

	public:

		/**
		 * Test a mesh_scene for intersection with a si_transform%ed @ref mesh.
		 *
		 * Usage notes:
		 * - Clients may optionally supply the @bref{m}esh and its si_transform
		 *		through an iterator to a mesh_scene_object in the @bref{scene}.
		 *		This approach suppresses self-collision results on the object.
		 * - On any error, mesh_scene_intersect tests throw an exception.
		 * - See geo_intersect and bounding_intersect for generic notes.
		 */
		//@{
		static mesh_scene_intersect_result intersect
			(const mesh_scene& scene,
			 const mesh& m, const si_transform& m_transform,
			 const mesh_scene_mesh_bounding& m_bounding);

		static mesh_scene_intersect_result intersect
			(const mesh_scene& scene,
			 const mesh_scene::object_iterator& m_object);
		//@}


	private:

		typedef bounding_intersect
					<mesh_scene_triangles, mesh_scene_mesh_bounding,
					 mesh_scene_intersect_test,
					 bounding_intersect_no_extended_result>
							bounding_intersect_mesh_scene_mesh;


		static mesh_scene_intersect_result intersect_util
			(const mesh_scene& scene,
			 const mesh& m, const si_transform& m_transform,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene::object_iterator& m_ignore_object);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_INTERSECT_HPP) */
