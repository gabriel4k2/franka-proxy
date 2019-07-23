/**
 *************************************************************************
 *
 * @file mesh_optimize.cpp
 *
 * Optimization for meshes, implementation.
 *
 ************************************************************************/


#include "mesh_optimize.hpp"


#include <algorithm>


#include "log.hpp"
#include "scope_guard.hpp"
#include "buffer.hpp"
#include "color.hpp"
#include "mesh.hpp"
#include "skeleton.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_optimize, implementation helpers
//
//////////////////////////////////////////////////////////////////////////


/**
 * Detail namespace with mesh optimization helpers.
 * @ingroup mesh_module
 */
namespace mesh_optimize_detail
{

	/**
	 * Utility structure that helps in sorting vertices
	 * for merging during mesh optimization.
	 */
	struct merge_vertex_sort_entry
		{ vector vertex; int64 original_index; };


	/** Compare vertices for sorting along component @bref{c}. */
	template <int64 c>
		bool compare_vertices_for_merge
			(const merge_vertex_sort_entry& a,
			 const merge_vertex_sort_entry& b)
				{ return a.vertex.c[c] < b.vertex.c[c]; }


	/**
	 * Determine the distance in-between two valid mesh_stream
	 * entries at @bref{from_index} and @bref{to_index}.
	 */
	class stream_entry_distance_functor
	{
		public:

			stream_entry_distance_functor()
			{
				distance_functions_.insert
					(typeid(mesh_stream_vector),
					 &distance_mesh_stream_vector);
				distance_functions_.insert
					(typeid(mesh_stream_color),
					 &distance_mesh_stream_color);

				distance_functions_.insert
					(typeid(mesh_stream_skeleton_index),
					 &distance_mesh_stream_skeleton_index);
				distance_functions_.insert
					(typeid(mesh_stream_skeleton_weight),
					 &distance_mesh_stream_skeleton_weight);
			}


			float operator()
				(const mesh_stream& stream,
				 int64 from_index, int64 to_index) const
			{
				// Dispatch to appropriate handler for stream type
				distance_function_map::const_iterator function_it
					(distance_functions_.find(typeid(stream)));

				if (!function_it)
				{
					LOG_ERROR("Not implemented for some mesh_stream.");
					throw invalid_state();
				}

				return (*function_it->value)(stream, from_index, to_index);
			}


		private:

			static float distance_mesh_stream_vector
				(const mesh_stream& stream, int64 from_index, int64 to_index)
			{
				const mesh_stream_vector& typed_stream =
					static_cast<const mesh_stream_vector&>(stream);

				vector dif(typed_stream[from_index] -
						   typed_stream[to_index]); 
				return dif.abs().max();
			}


			static float distance_mesh_stream_color
				(const mesh_stream& stream, int64 from_index, int64 to_index)
			{
				const mesh_stream_color& typed_stream =
					static_cast<const mesh_stream_color&>(stream);

				color dif(typed_stream[from_index] -
						  typed_stream[to_index]); 
				return dif.abs().max();
			}


			static float distance_mesh_stream_skeleton_index
				(const mesh_stream& stream, int64 from_index, int64 to_index)
			{
				const mesh_stream_skeleton_index& typed_stream =
					static_cast<const mesh_stream_skeleton_index&>(stream);

				return
					(typed_stream[from_index] == typed_stream[to_index]) ?
						0 : mesh_optimize::merge_vertex_prohibited;
			}


			static float distance_mesh_stream_skeleton_weight
				(const mesh_stream& stream, int64 from_index, int64 to_index)
			{
				const mesh_stream_skeleton_weight& typed_stream =
					static_cast<const mesh_stream_skeleton_weight&>(stream);

				float absmaxdif =
					fabsf(typed_stream[from_index].C[0] -
						  typed_stream[to_index].C[0]);

				for (int64 i = 1; i < skeleton_weight::max_weights_per_vertex; ++i)
					absmaxdif = max(fabsf(typed_stream[from_index].C[i] -
										  typed_stream[to_index].C[i]), absmaxdif);

				return absmaxdif;
			}


			typedef float (*distance_function)(const mesh_stream&, int64, int64);
			typedef tree_map<type_info_ref, distance_function> distance_function_map;
			distance_function_map distance_functions_;
	};


	/**
	 * Removes all indices from a mesh @bref{stream}
	 * except for those marked in @bref{keep_flags}.
	 *
	 * Notes:
	 * - To avoid complicated index updates, we must maintain order.
	 *		To avoid O(n * r) swapping, we maintain order by pushing
	 *		any remaining entries into a new stream instead of deletion
	 *		in the original stream.
	 * - The client must guarantee that deleted entries are not
	 *		in use anymore.
	 */
	class remove_stream_indices_functor
	{
		public:

			remove_stream_indices_functor()
			{
				remove_functions_.insert
					(typeid(mesh_stream_vector),
					 &remove_stream_typed<vector>);
				remove_functions_.insert
					(typeid(mesh_stream_color),
					 &remove_stream_typed<color>);

				remove_functions_.insert
					(typeid(mesh_stream_skeleton_index),
					 &remove_stream_typed<skeleton_index>);
				remove_functions_.insert
					(typeid(mesh_stream_skeleton_weight),
					 &remove_stream_typed<skeleton_weight>);

				remove_functions_.insert
					(typeid(mesh_stream_plane),
					 &remove_stream_typed<plane>);

				remove_functions_.insert
					(typeid(mesh_stream_point),
					 &remove_stream_typed<mesh_point>);
				remove_functions_.insert
					(typeid(mesh_stream_line),
					 &remove_stream_typed<mesh_line>);
				remove_functions_.insert
					(typeid(mesh_stream_triangle),
					 &remove_stream_typed<mesh_triangle>);
			}


			void operator()
				(mesh_stream& stream,
				 const dynamic_buffer<bool>& keep_flags) const
			{
				// Dispatch to appropriate handler for stream type
				remove_function_map::const_iterator function_it
					(remove_functions_.find(typeid(stream)));

				if (!function_it)
				{
					LOG_ERROR("Not implemented for some mesh_stream.");
					throw invalid_state();
				}

				(*function_it->value)(stream, keep_flags);
			}


		private:

			template <typename T>
				static void remove_stream_typed
					(mesh_stream& stream,
					 const dynamic_buffer<bool>& keep_flags)
			{
				mesh_stream_typed<T>& typed_stream =
					static_cast<mesh_stream_typed<T>&>(stream);

				if (typed_stream.size() != keep_flags.size())
				{
					LOG_ERROR("Incoming stream and keep flags array have different size.");
					throw assert_failed();
				}

				int64 size = typed_stream.size();

				int64 removed_count = 0;
				for (int64 i = 0; i < size; ++i)
					if (!keep_flags[i]) ++removed_count;

				if (removed_count == 0) return;

				mesh_stream_typed<T> new_stream;
				new_stream.reserve(size - removed_count);

				for (int64 i = 0; i < size; ++i)
					if (keep_flags[i])
						new_stream.push_back(typed_stream[i]);

				swap(new_stream, typed_stream);
			}


			typedef void (*remove_function)(mesh_stream&, const dynamic_buffer<bool>&);
			typedef tree_map<type_info_ref, remove_function> remove_function_map;
			remove_function_map remove_functions_;
	};
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_optimize, actual implementation
//
//////////////////////////////////////////////////////////////////////////


void mesh_optimize::remove_unused_vertices(mesh& m)
{
	SCOPE_GUARD
		(clear_guard, (m, &mesh::clear));


	// Prepare buffer that records which
	// vertices are unused.
	int64 vcount = m.vertex_count();

	dynamic_buffer<bool> keep_vertex_flags;
	keep_vertex_flags.resize(vcount);

	memory::fill_with_zero_typed
		(keep_vertex_flags.data(),
		 keep_vertex_flags.size());


	// Update buffer with vertices in use
	// by points, lines or triangles.
	{
		const mesh_stream_point& points = m.points();
		int64 pcount = m.point_count();

		for (int64 i = 0; i < pcount; ++i)
			keep_vertex_flags[points[i].i] = true;
	}
	{
		const mesh_stream_line& lines = m.lines();
		int64 lcount = m.line_count();

		for (int64 i = 0; i < lcount; ++i)
			for (int64 c = 0; c < 2; ++c)
				keep_vertex_flags[lines[i].C[c]] = true;
	}

	{
		const mesh_stream_triangle& triangles = m.triangles();
		int64 tcount = m.triangle_count();

		for (int64 i = 0; i < tcount; ++i)
			for (int64 c = 0; c < 3; ++c)
				keep_vertex_flags[triangles[i].C[c]] = true;
	}


	// Finally, remove actual vertices and return.
	remove_vertices_helper(m, keep_vertex_flags);
	clear_guard.dismiss();
}


void mesh_optimize::merge_duplicate_vertices(mesh& m)
{
	/**
	 * Average-case O(n log n) algorithm idea:
	 *
	 * 1. Build vertex clusters that are separated from all other
	 *		clusters along X, Y or Z by at least epsilon.
	 *
	 *		- Initially, one large cluster with all vertices.
	 *		- For X, Y, Z:
	 *			- For each cluster:
	 *				- Sort contained vertices along axis.
	 *				- Split at neighbor vertices with distance > epsilon.
	 *
	 * 2. Within every final cluster, test all vertices for merging
	 *		against each other, based on vertex coordinates and
	 *		any additional vertex streams.
	 *
	 *		- No gain from further sorting, since large clusters
	 *			in X, Y, and Z are rare.
	 *		- Test vertex coordinates as well, since a single cluster
	 *			may still contain vertices with a distance > epsilon.
	 *
	 * 3. Build a merge map which associates each initial vertex
	 *		index with a target vertex index that it is merged onto.
	 *
	 *		- Chain merges A->B->C (and thus cycles) are suppressed
	 *			to achieve clean merging results.
	 *		- @attent Since we temporarily resort the vertices,
	 *			we do not process them in order. Thus the result
	 *			map is random, not monotonic.
	 *
	 * 4. Remap vertex indices within all points, lines, and triangles
	 *		according to merge map.
	 *
	 * 5. Remove all vertices that have been merged onto another vertex.
	 */

	SCOPE_GUARD
		(clear_guard, (m, &mesh::clear));


	mesh_optimize_detail::stream_entry_distance_functor entry_distance;

	const mesh_stream_vector& vertices = m.vertices();
	int64 vcount = m.vertex_count();

	mesh_stream_point& points = m.points();
	int64 pcount = m.point_count();
	mesh_stream_line& lines = m.lines();
	int64 lcount = m.line_count();
	mesh_stream_triangle& triangles = m.triangles();
	int64 tcount = m.triangle_count();

	
	// Initialize temporaries for sorting and result storage.
	// original_index keeps track of original vertex index
	// throughout sorting operations.
	dynamic_buffer<int64> merge_map;
	merge_map.resize(vcount);
	for (int64 i = 0; i < vcount; ++i)
		merge_map[i] = i;

	dynamic_buffer<mesh_optimize_detail::merge_vertex_sort_entry> sorting;
	sorting.resize(vcount);
	for (int64 i = 0; i < vcount; ++i)
	{
		sorting[i].vertex = vertices[i];
		sorting[i].original_index = i;
	}


	// Build clusters.
	// Clusters are formed and processed in-situ within the
	// temporary sorting array.
	std::sort
		(sorting.data(),
		 sorting.data() + vcount,
		 mesh_optimize_detail::compare_vertices_for_merge<0>);

	int64 start_x = 0;

	for (int64 next_x = 1; next_x <= vcount; ++next_x)
	{
		float xdif =
			fabsf(sorting[next_x].vertex.x - sorting[next_x - 1].vertex.x);

		if (next_x != vcount && xdif < merge_vertex_epsilon)
			continue;

		std::sort
			(sorting.data() + start_x,
			 sorting.data() + next_x,
			 mesh_optimize_detail::compare_vertices_for_merge<1>);

		int64 start_y = start_x;

		for (int64 next_y = start_y + 1; next_y <= next_x; ++next_y)
		{
			float ydif =
				fabsf(sorting[next_y].vertex.y - sorting[next_y - 1].vertex.y);

			if (next_y != next_x && ydif < merge_vertex_epsilon)
				continue;

			std::sort
				(sorting.data() + start_y,
				 sorting.data() + next_y,
				 mesh_optimize_detail::compare_vertices_for_merge<2>);

			int64 start_z = start_y;

			for (int64 next_z = start_z + 1; next_z <= next_y; ++next_z)
			{
				float zdif =
					fabsf(sorting[next_z].vertex.z - sorting[next_z - 1].vertex.z);

				if (next_z != next_y && zdif < merge_vertex_epsilon)
					continue;


				// Indices start_z to next_z - 1 are cluster in x, y, and z.
				// Thus, check for merging inside cluster.
				// This includes suppressing chain / cycle merges (only merge
				// onto unmerged vertex), and vertex stream tests.
				for (int64 i = start_z; i < next_z; ++i)
				for (int64 j = start_z; j < i; ++j)
				{
					int64 from = sorting[i].original_index;
					int64 to = sorting[j].original_index;

					if (merge_map[to] != to) continue; 

					bool merge_vertices = true;

					float coorddist =
						entry_distance(m.vertices(), from, to);

					if (coorddist == merge_vertex_prohibited ||
						coorddist > merge_vertex_epsilon)
							merge_vertices = false;
	
					for (mesh::stream_iterator it(m.first_vertex_stream()); it; ++it)
					{
						float streamdist =
							entry_distance(*it->value, from, to);

						if (streamdist == merge_vertex_prohibited ||
							streamdist > merge_vertex_epsilon)
								merge_vertices = false;
					}

					if (!merge_vertices) continue;


					// Merging target found, so store in result map,
					// and directly start with next unmapped vertex.
					merge_map[from] = to;
					break;
				}

				start_z = next_z;
			}			

			start_y = next_y;
		}
	
		start_x = next_x;
	}


	// Apply mapping to point, line and triangle indices.
	// We could actually have some bit of a slip in plane
	// equations here, but we can't correct this without
	// making definite assumptions about stream contents.
	for (int64 i = 0; i < pcount; ++i)
		points[i] = merge_map[points[i].i];

	for (int64 i = 0; i < lcount; ++i)
		for (int64 c = 0; c < 2; ++c)
			lines[i].C[c] = merge_map[lines[i].C[c]];

	for (int64 i = 0; i < tcount; ++i)
		for (int64 c = 0; c < 3; ++c)
			triangles[i].C[c] = merge_map[triangles[i].C[c]];


	// Finally, remove all vertices that have been remapped.
	// Keep all vertices that have been mapped onto themselves.
	dynamic_buffer<bool> keep_vertex_flags;
	keep_vertex_flags.resize(vcount);

	for (int64 i = 0; i < vcount; ++i)
		keep_vertex_flags[i] = (merge_map[i] == i);

	remove_vertices_helper(m, keep_vertex_flags);	


	clear_guard.dismiss();
}


void mesh_optimize::remove_degenerate_lines(mesh& m)
{
	SCOPE_GUARD
		(clear_guard, (m, &mesh::clear));


	// Determine which lines are not degenerate
	// and should be kept.
	const mesh_stream_line& lines = m.lines();
	int64 lcount = m.line_count();

	dynamic_buffer<bool> keep_line_flags;
	keep_line_flags.resize(lcount);

	for (int64 i = 0; i < lcount; ++i)
		keep_line_flags[i] =
			lines[i].i != lines[i].j;


	// Remove all degenerate lines.
	mesh_optimize_detail::remove_stream_indices_functor remove_indices;
	remove_indices(m.lines(), keep_line_flags);


	clear_guard.dismiss();
}


void mesh_optimize::remove_degenerate_triangles(mesh& m)
{
	SCOPE_GUARD
		(clear_guard, (m, &mesh::clear));


	// Determine which triangles are not degenerate
	// and should be kept.
	const mesh_stream_triangle& triangles = m.triangles();
	int64 tcount = m.triangle_count();

	dynamic_buffer<bool> keep_triangle_flags;
	keep_triangle_flags.resize(tcount);

	for (int64 i = 0; i < tcount; ++i)
		keep_triangle_flags[i] =
			triangles[i].i != triangles[i].j &&
			triangles[i].i != triangles[i].k &&
			triangles[i].j != triangles[i].k;


	// Remove all degenerate triangles.
	mesh_optimize_detail::remove_stream_indices_functor remove_indices;

	remove_indices(m.triangles(), keep_triangle_flags);

	for (mesh::stream_iterator it(m.first_triangle_stream()); it; ++it)
		remove_indices(*it->value, keep_triangle_flags);


	clear_guard.dismiss();
}


const float mesh_optimize::merge_vertex_epsilon = 1e-6f;
const float mesh_optimize::merge_vertex_prohibited = -1.f;


void mesh_optimize::remove_vertices_helper
	(mesh& m, const dynamic_buffer<bool>& keep_vertex_flags)
{
	/**
	 * Remove all vertices except for those in @bref{keep_vertex_flags},
	 * adapt per-vertex streams and vertex indices accordingly.
	 *
	 * Notes:
	 * - The caller must see that deleted vertices are not in use anymore.
	 * - Vertex order is maintained throughout removal.
	 */


	// Determine number of vertices removed below
	// each original vertex index.
	dynamic_buffer<int64> removed_below;
	int64 vcount = m.vertex_count();
	removed_below.resize(vcount);

	int64 current_removed = 0;

	for (int64 i = 0; i < vcount; ++i)
	{
		removed_below[i] = current_removed;

		if (!keep_vertex_flags[i])
			++current_removed;
	}


	// Actually remove vertices.
	mesh_optimize_detail::remove_stream_indices_functor remove_indices;

	remove_indices(m.vertices(), keep_vertex_flags);

	for (mesh::stream_iterator it(m.first_vertex_stream()); it; ++it)
			remove_indices(*it->value, keep_vertex_flags);


	// Adapt point, line, and triangle indices.
	// Since removal maintains order, each index shifts
	// to the left by the count of vertices removed below
	// that index. Per-triangle streams need not be touched.
	{
		mesh_stream_point& points = m.points();
		int64 pcount = m.point_count();

		for (int64 i = 0; i < pcount; ++i)
			points[i].i -= removed_below[points[i].i];
	}
	{
		mesh_stream_line& lines = m.lines();
		int64 lcount = m.line_count();

		for (int64 i = 0; i < lcount; ++i)
			for (int64 c = 0; c < 2; ++c)
				lines[i].C[c] -= removed_below[lines[i].C[c]];
	}

	{
		mesh_stream_triangle& triangles = m.triangles();
		int64 tcount = m.triangle_count();

		for (int64 i = 0; i < tcount; ++i)
			for (int64 c = 0; c < 3; ++c)
				triangles[i].C[c] -= removed_below[triangles[i].C[c]];
	}
}




} /* namespace viral_core */
