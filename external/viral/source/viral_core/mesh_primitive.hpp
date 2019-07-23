/**
 *************************************************************************
 *
 * @file mesh_primitive.hpp
 *
 * Meshes for primitive shapes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_PRIMITIVE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_PRIMITIVE_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_primitive
 * @ingroup mesh_module
 *
 * Builds @ref mesh%es for various primitive shapes.
 *
 * Usage notes:
 * - All building calls create a primitive in a local coordinate system,
 *		as described by @bref{origin} and @bref{orientation} parameters.
 *		2D primitives and the base shapes of 3D primitives are confined
 *		to the XY plane of this local coordinate system.
 * - By default, primitives use @bref{origin} as their minimum coordinate
 *		and extend into positive local coordinates. Clients can center
 *		meshes separately on each axis by setting respective build_flags.
 * - @attent A zero @bref{build_flags} parameter only creates mesh vertices.
 *		Resulting meshes appear empty due to missing lines and triangles.
 *		Most clients will at least want the @bref{build_triangles} flag.
 *
 * Implementation notes:
 * - Primitive building calls prefer safety over efficiency.
 *		Notably, buffer access uses at() instead of operator[].
 *
 ************************************************************************/
class mesh_primitive :
	private non_constructable
{
	public:

		enum build_flags
		{
			/** Build mesh connectivity streams. */
			//@{
			build_lines = 1,
			build_triangles = 2,
			//@}


			/**
			 * Build auxiliary mesh_stream%s from mesh_stream_registry.
			 * Note values in auxiliary streams (e.g. normals, colors)
			 * are not rigidly specified, but in general are reasonable.
			 */	
			//@{
			build_normals = 4,
			build_colors = 8,
			build_texcoords = 16,
			build_tangents = 32,
			//@}


			/** Build typical content required by rendering. */
			//@{
			build_render_lines =
				build_lines |
				build_normals | build_colors |
				build_texcoords | build_tangents,		
			build_render_triangles =
				build_triangles |
				build_normals | build_colors |
				build_texcoords | build_tangents,
			//@}


			/** Build triangles, lines, and all auxiliary streams. */
			build_all =
				build_lines | build_triangles |
				build_normals | build_colors |
				build_texcoords | build_tangents,


			/** Center resulting mesh on one or more axes. */
			//@{
			center_x = 64,
			center_y = 128,
			center_z = 256,
			center_xy = center_x | center_y,
			center_all = center_x | center_y | center_z
			//@}
		};


		/**
		 * Build 2D primitives on local XY plane.
		 * See mesh_primitive class documentation for details.
		 */
		//@{
		static mesh build_quad
			(const vector& origin, const rotation& orientation,
			 const vector2f& size, int64 build_flags);
		static mesh build_circle
			(const vector& origin, const rotation& orientation,
			 float radius, int64 segment_count, int64 build_flags);
		//@}


		/**
		 * Build 3D primitives with base shape on local XY plane.
		 * See mesh_primitive class documentation for details.
		 */
		//@{
		static mesh build_box
			(const vector& origin, const rotation& orientation,
			 const vector& size, int64 build_flags);
		static mesh build_cylinder
			(const vector& origin, const rotation& orientation,
			 float height, float radius, int64 segment_count,
			 int64 build_flags);
		static mesh build_cone
			(const vector& origin, const rotation& orientation,
			 float height, float radius, int64 segment_count,
			 int64 build_flags);
		static mesh build_sphere
			(const vector& origin, const rotation& orientation,
			 float radius, int64 segment_count, int64 build_flags);
		static mesh build_ellipsoid
			(const vector& origin, const rotation& orientation,
			 const vector& radius, int64 segment_count, int64 build_flags);
		//@}


		/**
		 * Implementation-specific maximum supported @bref{segment_count}.
		 * Avoids numerical precision issues on dense tesselation.
		 */
		static const int64 max_segment_count = 2048;


	private:

		static void build_generic_postprocess
			(mesh& msh, const vector& origin, const rotation& orientation,
			 int64 build_flags);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_PRIMITIVE_HPP) */
