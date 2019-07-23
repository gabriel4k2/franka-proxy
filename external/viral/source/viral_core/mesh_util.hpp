/**
 *************************************************************************
 *
 * @file mesh_util.hpp
 *
 * Header for mesh utility operations.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__MESH_UTIL_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_util
 * @ingroup mesh_module
 *
 * Holds various static utility methods that work on the geometry
 * stored inside a @ref mesh.
 *
 * Notes:
 * - Mesh utilities do not automatically call mesh::rebuild_boundings().
 * - Skeletons are @ref model%bound, and do not update alongside meshes.
 * - On any exception, input data is reset by mesh::clear().
 *
 ************************************************************************/
class mesh_util :
	private non_constructable
{

	public:

		/**
		 * Apply @bref{transform}ation to all @bref{m}esh vertices,
		 * then update auxiliary mesh_stream%s from mesh_stream_registry,
		 * such as vertex normals and plane equations.
		 *
		 * Optionally, derive @bref{transform} from @bref{mat}rix,
		 * as by si_transform::from_matrix().
		 */
		//@{
		static void apply_transform
			(mesh& m, const si_transform& transform);
		static void apply_transform
			(mesh& m, const matrix& mat);
		//@}


		/** Center mesh on origin (0, 0, 0) along given axes. */
		static void center_mesh
			(mesh& m, bool center_x, bool center_y, bool center_z);


		/**
		 * Build a stream with smooth per-vertex normals.
		 * Clients can split the mesh at vertices to create hard edges.
		 * This call replaces any existing vertex normal stream.
		 */
		static void build_vertex_normals(mesh& m);


		/**
		 * Add or replace per-vertex colors on a @bref{m}esh
		 * with a single @bref{c}olor for each vertex.
		 */
		static void build_vertex_colors(mesh& m, const color& c);


		/**
		 * Toggle origin of any texture coordinate stream
		 * between the lower-left (e.g. OpenGL, Collada)
		 * and upper-left (e.g. image_rgba, @ref camera).
		 *
		 * @attent This call does not automatically
		 * update any tangent or bitangent streams.
		 */
		static void flip_texcoord_origin_y(mesh &m);


		/**
		 * Builds tangents and bitangents for a given @bref{m}esh,
		 * and adds them as respective mesh_stream_registry streams.
		 * The call ignores meshes without texture coordinates.
		 * Any existing tangential basis is replaced.
		 */
		static void build_tangential_basis(mesh& m);


		/**
		 * Build triangle plane equations for a given @bref{m}esh,
		 * and add these as a mesh_stream_registry::plane_stream_name.
		 * Any existing plane equation stream is replaced.
		 */
		static void build_plane_equations(mesh& m);
}; 




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_UTIL_HPP) */

