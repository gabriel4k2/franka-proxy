/**
 *************************************************************************
 *
 * @file mesh_scene_proximate.hpp
 * 
 * Proximity tests on meshes in a mesh_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_PROXIMATE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_PROXIMATE_HPP


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
 * @class mesh_scene_proximate_test
 * @ingroup mesh_module
 *
 * A bounding_proximate test for use in mesh_scene_proximate.
 *
 ************************************************************************/
class mesh_scene_proximate_test :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:

		mesh_scene_proximate_test
			(const mesh& m, const si_transform& m_transform,
			 const mesh_scene::object_iterator& m_ignore_object);


	private:

		bool node_proximate
			(const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;

		bool leaf_proximate
			(const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;


		const mesh& m_; // Don't copy, expensive.
		const si_transform m_transform_;
		const matrix m_to_world_matrix_;

		const mesh_scene::object_iterator m_ignore_object_;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_proximate_result
 * @ingroup mesh_module
 *
 * Stores proximate nodes of a mesh_scene and a separate @ref mesh.
 *
 * Usage notes:
 * - If scene and mesh are farther than @bref{distance_squared_upper_bound},
 *		then @bref{valid} is @false, tri indices are @bref{invalid_index},
 *		and both scene iterators are past-end.
 * - Proximity tests may pick any of multiple candidates for close triangles,
 *		potentially causing an error of up to the distance bound.
 *
 ************************************************************************/
class mesh_scene_proximate_result
{
	friend class mesh_scene_proximate;


	public:

		static const int64 invalid_index = -1;


		const bool valid;


		/** Any proximate node and its object in mesh_scene. */
		//@{
		const mesh_scene::const_node_iterator proximate_scene_node;
		const mesh_scene::object_iterator proximate_scene_object;
		//@}


		/**
		 * Index for any proximate triangle
		 * in @bref{proximate_scene_object}.
		 */
		const int64 proximate_scene_object_triangle_index;


		/** Index for any proximate triangle in right-hand mesh. */
		const int64 proximate_mesh_triangle_index;


	private:

		mesh_scene_proximate_result() NOTHROW
			:
			valid(false),
			proximate_scene_object_triangle_index(invalid_index),
			proximate_mesh_triangle_index(invalid_index)
		{ }


		mesh_scene_proximate_result
			(mesh_scene::const_node_iterator proximate_scene_node,
			 mesh_scene::object_iterator proximate_scene_object,
			 int64 proximate_scene_object_triangle_index,
			 int64 proximate_mesh_triangle_index) NOTHROW
			:
			valid(true),
			proximate_scene_node(proximate_scene_node),
			proximate_scene_object(proximate_scene_object),
			proximate_scene_object_triangle_index(proximate_scene_object_triangle_index),
			proximate_mesh_triangle_index(proximate_mesh_triangle_index)
		{ }
};




/**
 *************************************************************************
 *
 * @class mesh_scene_proximate
 * @ingroup mesh_module
 *
 * Tests the mesh%es in a mesh_scene for proximity against each other
 * and against other geometric objects.
 *
 ************************************************************************/
class mesh_scene_proximate :
	private non_constructable
{

	public:

		/**
		 * Test a mesh_scene for proximity with a si_transform%ed @ref mesh.
		 *
		 * Usage notes:
		 * - Clients may optionally supply the @bref{m}esh and its si_transform
		 *		through an iterator to a mesh_scene_object in the @bref{scene}.
		 *		This approach suppresses self-proximity results on the object.
		 * - On any error, mesh_scene_proximate tests throw an exception.
		 * - See bounding_proximate documentation for generic notes on usage,
		 *		notably for @bref{distance_squared_upper_bound}
		 */
		//@{
		static mesh_scene_proximate_result proximate
			(const mesh_scene& scene,
			 const mesh& m, const si_transform& m_transform,
			 const mesh_scene_mesh_bounding& m_bounding,
			 float distance_squared_upper_bound);

		static mesh_scene_proximate_result proximate
			(const mesh_scene& scene,
			 const mesh_scene::object_iterator& m_object,
			 float distance_squared_upper_bound);
		//@}


	private:

		typedef bounding_proximate
					<mesh_scene_triangles, mesh_scene_mesh_bounding,
					 mesh_scene_proximate_test,
					 bounding_proximate_no_extended_result>
							bounding_proximate_mesh_scene_mesh;


		static mesh_scene_proximate_result proximate_util
			(const mesh_scene& scene,
			 const mesh& m, const si_transform& m_transform,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene::object_iterator& m_ignore_object,
			 float distance_squared_upper_bound);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_PROXIMATE_HPP) */
