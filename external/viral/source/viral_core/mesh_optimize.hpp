/**
 *************************************************************************
 *
 * @file mesh_optimize.hpp
 *
 * Optimization for meshes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_OPTIMIZE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_OPTIMIZE_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_optimize
 * @ingroup mesh_module
 *
 * Static utilities that optimize the geometry of a @ref mesh.
 *
 * Notes:
 * - Optimization does not automatically call mesh::rebuild_boundings().
 * - On any exception, input data is reset by mesh::clear().
 *
 ************************************************************************/
class mesh_optimize :
	private non_constructable
{

	public:

		/**
		 * Remove any vertices within a @bref{m}esh that are
		 * not referenced by any point, line, or triangle.
		 *
		 * Notes:
		 * - Per-vertex streams are processed appropriately.
		 * - Vertex order is maintained.
		 */
		static void remove_unused_vertices(mesh& m);


		/**
		 * Merge all vertices inside a @bref{m}esh that match
		 * in vertex coordinates and in all per-vertex streams
		 * within some small epsilon.
		 *
		 * Notes:
		 * - @attent Vertex order is not maintained.
		 * - @attent Triangle streams (e.g. plane equations)
		 *		are not updated to accommodate for vertex merges,
		 *		and might suffer slight rounding-related errors.
		 */
		static void merge_duplicate_vertices(mesh& m);


		/**
		 * Remove all lines or triangles in a @bref{m}esh that
		 * do not consist of fully unique indices.
		 *
		 * Notes:
		 * - Assumes that there are no duplicate vertices (i.e. that
		 *		merge_duplicate_vertices() has already been invoked).
		 * - Per-triangle streams are processed appropriately.
		 * - Line and triangle order is maintained.
		 */
		//@{
		static void remove_degenerate_lines(mesh& m);
		static void remove_degenerate_triangles(mesh& m);
		//@}


		/**
		 * Minimum distance for vertex merging per attribute,
		 * and flag to indicate that no merge is possible.
		 */
		//@{
		static const float merge_vertex_epsilon;
		static const float merge_vertex_prohibited;
		//@}


	private:

		static void remove_vertices_helper
			(mesh& m, const dynamic_buffer<bool>& keep_vertex_flags);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_OPTIMIZE_HPP) */