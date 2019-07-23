/**
 *************************************************************************
 *
 * @file model_util.cpp
 *
 * Utilities for models, implementation.
 *
 ************************************************************************/


#include "model_util.hpp"


#include "auto_pointer.hpp"
#include "color.hpp"
#include "mesh.hpp"
#include "model.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// model_util, implementation helpers
//
//////////////////////////////////////////////////////////////////////////


/**
 * Detail namespace with helpers for model utilities.
 * @ingroup mesh_module
 */
namespace model_util_detail
{
	/**
	 * Create a new, empty mesh_stream
	 * of the same type as @bref{src_stream},
	 * or return @NULL on unknown types.
	 */
	auto_pointer<mesh_stream>
		try_join_replicate_mesh_stream
			(const mesh_stream& src_stream)
	{
		if (typeid(src_stream) == typeid(mesh_stream_color))
			return auto_pointer<mesh_stream>(new mesh_stream_color());
		else if (typeid(src_stream) == typeid(mesh_stream_vector))
			return auto_pointer<mesh_stream>(new mesh_stream_vector());
		else if (typeid(src_stream) == typeid(mesh_stream_plane))
			return auto_pointer<mesh_stream>(new mesh_stream_plane());

		else if (typeid(src_stream) == typeid(mesh_stream_point))
			return auto_pointer<mesh_stream>(new mesh_stream_point());
		else if (typeid(src_stream) == typeid(mesh_stream_line))
			return auto_pointer<mesh_stream>(new mesh_stream_line());
		else if (typeid(src_stream) == typeid(mesh_stream_triangle))
			return auto_pointer<mesh_stream>(new mesh_stream_triangle());

		else if (typeid(src_stream) == typeid(mesh_stream_skeleton_index))
			return auto_pointer<mesh_stream>(new mesh_stream_skeleton_index());
		else if (typeid(src_stream) == typeid(mesh_stream_skeleton_weight))
			return auto_pointer<mesh_stream>(new mesh_stream_skeleton_weight());

		return auto_pointer<mesh_stream>();
	}


	/**
	 * Join a @bref{src_stream} of known type
	 * onto @bref{dest_stream} of same type.
	 */
	template <typename T>
		void join_mesh_stream_typed
			(mesh_stream_typed<T>& dest_stream,
			 const mesh_stream_typed<T>& src_stream)
	{
		dest_stream.resize
			(dest_stream.size() + src_stream.size());

		memory::copy_block_typed
			(dest_stream.data() + dest_stream.size() - src_stream.size(),
			 src_stream.data(), src_stream.size());
	}


	/**
	 * Variants of join_mesh_stream_typed()
	 * that correct vertex indices by @bref{vertex_offset}.
	 */
	//@{
	void join_mesh_stream_point
		(mesh_stream_point& dest_stream,
		 const mesh_stream_point& src_stream,
		 int64 vertex_offset)
	{
		join_mesh_stream_typed(dest_stream, src_stream);

		for (int64 i = 0; i < src_stream.size(); ++i)
			dest_stream[dest_stream.size() - 1 - i].i += vertex_offset;
	}

	void join_mesh_stream_line
		(mesh_stream_line& dest_stream,
		 const mesh_stream_line& src_stream,
		 int64 vertex_offset)
	{
		join_mesh_stream_typed(dest_stream, src_stream);

		for (int64 i = 0; i < src_stream.size(); ++i)
		{
			mesh_line& line =
				dest_stream[dest_stream.size() - 1 - i];

			line.i += vertex_offset;
			line.j += vertex_offset;
		}
	}

	void join_mesh_stream_triangle
		(mesh_stream_triangle& dest_stream,
		 const mesh_stream_triangle& src_stream,
		 int64 vertex_offset)
	{
		join_mesh_stream_typed(dest_stream, src_stream);

		for (int64 i = 0; i < src_stream.size(); ++i)
		{
			mesh_triangle& triangle =
				dest_stream[dest_stream.size() - 1 - i];

			triangle.i += vertex_offset;
			triangle.j += vertex_offset;
			triangle.k += vertex_offset;
		}
	}
	//@}


	/**
	 * Join some @bref{src_stream} onto @bref{dest_stream},
	 * or return @false on type mismatch or unknown streams.
	 * Optionally, correct any indices by @bref{vertex_offset}.
	 */
	bool try_join_mesh_stream
		(mesh_stream& dest_stream,
		 const mesh_stream& src_stream,
		 int64 vertex_offset)
	{
		if (typeid(src_stream) != typeid(dest_stream))
			return false;

		if (typeid(src_stream) == typeid(mesh_stream_color))
		{
			join_mesh_stream_typed
				(static_cast<mesh_stream_color&>(dest_stream),
				 static_cast<const mesh_stream_color&>(src_stream));
			return true;
		}
		else if (typeid(src_stream) == typeid(mesh_stream_vector))
		{
			join_mesh_stream_typed
				(static_cast<mesh_stream_vector&>(dest_stream),
				 static_cast<const mesh_stream_vector&>(src_stream));
			return true;
		}
		else if (typeid(src_stream) == typeid(mesh_stream_plane))
		{
			join_mesh_stream_typed
				(static_cast<mesh_stream_plane&>(dest_stream),
				 static_cast<const mesh_stream_plane&>(src_stream));
			return true;
		}

		else if (typeid(src_stream) == typeid(mesh_stream_point))
		{
			join_mesh_stream_point
				(static_cast<mesh_stream_point&>(dest_stream),
				 static_cast<const mesh_stream_point&>(src_stream),
				 vertex_offset);
			return true;
		}
		else if (typeid(src_stream) == typeid(mesh_stream_line))
		{
			join_mesh_stream_line
				(static_cast<mesh_stream_line&>(dest_stream),
				 static_cast<const mesh_stream_line&>(src_stream),
				 vertex_offset);
			return true;
		}
		else if (typeid(src_stream) == typeid(mesh_stream_triangle))
		{
			join_mesh_stream_triangle
				(static_cast<mesh_stream_triangle&>(dest_stream),
				 static_cast<const mesh_stream_triangle&>(src_stream),
				 vertex_offset);
			return true;
		}

		else if (typeid(src_stream) == typeid(mesh_stream_skeleton_index))
		{
			join_mesh_stream_typed
				(static_cast<mesh_stream_skeleton_index&>(dest_stream),
				 static_cast<const mesh_stream_skeleton_index&>(src_stream));
			return true;
		}
		else if (typeid(src_stream) == typeid(mesh_stream_skeleton_weight))
		{
			join_mesh_stream_typed
				(static_cast<mesh_stream_skeleton_weight&>(dest_stream),
				 static_cast<const mesh_stream_skeleton_weight&>(src_stream));
			return true;
		}

		return false;
	}
		

	/**
	 * Find a vertex or triangle stream with @bref{src_name}
	 * in @bref{src_mesh}, then copy to @bref{dest_stream}.
	 * Optionally corrects any indices by @bref{vertex_offset}.
	 * Returns @false if there is no stream of given name
	 * or if there is a type mismatch, @true on success.
	 */
	//@{
	bool try_join_vertex_stream
		(mesh_stream& dest_stream,
		 const string& src_name, const mesh& src_mesh,
		 int64 vertex_offset)
	{
		mesh::const_stream_iterator find
			(src_mesh.find_vertex_stream(src_name));

		if (!find) return false;
		if (!find->value) return false;

		mesh_stream& src_stream =
			*find->value;

		return try_join_mesh_stream
					(dest_stream, src_stream, vertex_offset);
	}

	bool try_join_triangle_stream
		(mesh_stream& dest_stream,
		 const string& src_name, const mesh& src_mesh,
		 int64 vertex_offset)
	{
		mesh::const_stream_iterator find
			(src_mesh.find_triangle_stream(src_name));

		if (!find) return false;
		if (!find->value) return false;

		mesh_stream& src_stream =
			*find->value;

		return try_join_mesh_stream
					(dest_stream, src_stream, vertex_offset);
	}		
	//@}

}




//////////////////////////////////////////////////////////////////////////
//
// model_util
//
//////////////////////////////////////////////////////////////////////////


mesh model_util::join_model_groups(const model& m)
{
	mesh result;


	// On models without model groups,
	// the merged mesh is empty.	
	if (!m.first_group())
	{
		result.validate();
		result.rebuild_boundings();

		return std::move(result);
	}


	// Join mesh streams that are always present
	// in each model group.
	{
		mesh_stream_vector joined_vertices;
		mesh_stream_point joined_points;
		mesh_stream_line joined_lines;
		mesh_stream_triangle joined_triangles;

		int64 vertex_offset = 0;
		for (model::group_iterator group_it(m.first_group());
			 group_it; ++group_it)
		{
			const mesh& src_mesh =
				(*group_it)->mesh;

			model_util_detail::join_mesh_stream_typed
				(joined_vertices, src_mesh.vertices());

			model_util_detail::join_mesh_stream_point
				(joined_points, src_mesh.points(), vertex_offset);
			model_util_detail::join_mesh_stream_line
				(joined_lines, src_mesh.lines(), vertex_offset);
			model_util_detail::join_mesh_stream_triangle
				(joined_triangles, src_mesh.triangles(), vertex_offset);

			vertex_offset +=
				(src_mesh.vertex_count());
		}

		result.set_vertices(std::move(joined_vertices));
		result.set_points(std::move(joined_points));
		result.set_lines(std::move(joined_lines));
		result.set_triangles(std::move(joined_triangles));
	}


	// Try to clone auxiliary mesh streams
	// into the merged mesh.
	const mesh& first_mesh =
		(*m.first_group())->mesh;

	for (mesh::const_stream_iterator stream_it
			(first_mesh.first_vertex_stream());
		stream_it; ++stream_it)
	{
		auto_pointer<mesh_stream> stream
			(model_util_detail::try_join_replicate_mesh_stream
				(*stream_it->value));

		if (!stream) continue;

		bool success_in_all = true;
		int64 vertex_offset = 0;
		for (model::group_iterator group_it(m.first_group());
			 group_it && success_in_all; ++group_it)
		{
			success_in_all = success_in_all &&
				model_util_detail::try_join_vertex_stream
					(*stream,
					 stream_it->key, (*group_it)->mesh,
					 vertex_offset);

			vertex_offset += (*group_it)->mesh.vertex_count();
		}

		if (success_in_all)
			result.insert_vertex_stream
				(stream_it->key, std::move(stream));
	}

	for (mesh::const_stream_iterator stream_it
			(first_mesh.first_triangle_stream());
		 stream_it; ++stream_it)
	{
		auto_pointer<mesh_stream> stream
			(model_util_detail::try_join_replicate_mesh_stream
				(*stream_it->value));

		if (!stream) continue;

		bool success_in_all = true;
		int64 vertex_offset = 0;
		for (model::group_iterator group_it(m.first_group());
			 group_it && success_in_all; ++group_it)
		{
			success_in_all = success_in_all &&
				model_util_detail::try_join_triangle_stream
					(*stream,
					 stream_it->key, (*group_it)->mesh,
					 vertex_offset);

			vertex_offset += (*group_it)->mesh.vertex_count();
		}

		if (success_in_all)
			result.insert_triangle_stream
				(stream_it->key, std::move(stream));
	}


	// Success.
	result.validate();
	result.rebuild_boundings();

	return std::move(result);
}




} /* namespace viral_core */
