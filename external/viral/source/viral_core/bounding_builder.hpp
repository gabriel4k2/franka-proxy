/**
 *************************************************************************
 *
 * @file bounding_builder.hpp
 *
 * Building of bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_BUILDER_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_BUILDER_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "geo_forward.hpp"
#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_builder
 * @ingroup bounding_module
 *
 * Static utilities for building various bounding volume hierarchies.
 *
 ************************************************************************/
class bounding_builder :
	private non_constructable
{

	public:

		/**
		 * Build a @ref sphere or @ref box hierarchy
		 * around the triangles within some @ref mesh.
		 */
		//@{
		static sphere_tree_index
			build_sphere_tree_index(const mesh& msh);
		static sphere_tree_triangle
			build_sphere_tree_triangle(const mesh& msh);

		static box_tree_index
			build_box_tree_index(const mesh& msh);
		static box_tree_triangle
			build_box_tree_triangle(const mesh& msh);
		//@}


		/**
		 * Build a @ref sphere or @ref box hierarchy
		 * over all @ref mesh%es of a given @ref model.
		 * As triangle indices are not unique in a model,
		 * indexed tree variants are not supported.
		 */
		//@{
		static sphere_tree_triangle
			build_sphere_tree_triangle(const model& mdl);
		static box_tree_triangle
			build_box_tree_triangle(const model& mdl);
		//@}


		/**
		 * Build an implicit sphere_tree or box_tree
		 * from tesselated triangles in a @ref mesh
		 * or a @ref model.
		 *
		 * Usage notes:
		 * - Bounding spheres or boxes from tesselation
		 *		form the (implicit) geometry objects.
		 * - See mesh_tesselate for further details,
		 *		in particular on parameter usage.
		 */
		//@{
		static sphere_tree build_sphere_tree_tesselated
			(const mesh& msh, float sphere_radius);
		static box_tree build_box_tree_tesselated
			(const mesh& msh, float box_size);

		static sphere_tree build_sphere_tree_tesselated
			(const model& mdl, float sphere_radius);
		static box_tree build_box_tree_tesselated
			(const model& mdl, float box_size);
		//@}


	private:

		template <typename S>
			static dynamic_buffer<S> build_leaf_shapes
				(const mesh& msh);
		template <typename S>
			static S to_leaf_shape(const triangle& t);

		template <typename S, typename O>
			static array<bounding_tree_node<S>>
				build_bounding_tree_nodes
					(const array<bounding_builder_span<S>>& spans);
		template <typename S>
			static array<bounding_implicit_node<S>>
				build_bounding_implicit_nodes
					(const array<bounding_builder_span<S>>& spans);

		static array<int64>
			build_objects_index(const mesh& msh);
		static array<triangle>
			build_objects_triangle(const mesh& msh);


		static const float min_bounding_sphere_radius_;
		static const float min_bounding_box_size_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_BUILDER_HPP) */