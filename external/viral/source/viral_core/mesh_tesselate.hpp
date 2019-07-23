/**
 *************************************************************************
 *
 * @file mesh_tesselate.hpp
 *
 * Mesh tesselation utilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_TESSELATE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_TESSELATE_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_tesselate
 * @ingroup mesh_module
 *
 * Static tesselation utilities for @ref mesh%es.
 *
 ************************************************************************/
class mesh_tesselate :
	private non_constructable
{

	public:

		/**
		 * Find a sphere covering for all triangles in a mesh
		 * where each sphere has at most @bref{sphere_radius}.
		 * The spheres contain triangle surfaces plus points
		 * with up to @bref{sphere_radius} tolerance.
		 */
		static dynamic_buffer<sphere> tesselate_triangles_by_spheres
			(const mesh& msh, float sphere_radius);


		/**
		 * Find a box covering for all triangles in a mesh
		 * where each box side has at most @bref{box_size}.
		 * The boxes contain triangle surfaces plus points
		 * with up to @bref{box_size} tolerance.
		 */
		static dynamic_buffer<box> tesselate_triangles_by_boxes
			(const mesh& msh, float box_size);


	private:

		static void split_major_edge_and_add_to_pending
			(const triangle& tri,
			 dynamic_buffer<triangle>& pending);


		static const float triangle_degenerate_sphere_radius_;
		static const float triangle_degenerate_box_size_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_TESSELATE_HPP) */
