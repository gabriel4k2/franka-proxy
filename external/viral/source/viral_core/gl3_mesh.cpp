/**
 *************************************************************************
 *
 * @file gl3_mesh.cpp
 *
 * OpenGL 3 GPU meshes, implementation.
 *
 ************************************************************************/


#include "gl3_mesh.hpp"


#include "log.hpp"
#include "integer_checks.hpp"
#include "integer_operations.hpp"
#include "buffer.hpp"
#include "color.hpp"
#include "skeleton.hpp"
#include "mesh.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_mesh
//
//////////////////////////////////////////////////////////////////////////


gl3_mesh::gl3_mesh
	(const gl3_api& gl3, const mesh& m,
	 const stream_location_map& stream_locations,
	 bool transient_hint)
	:
	vertex_count
		(integer_operations::cast_or_clamp<GLsizei>
			(m.vertex_count())),
	point_count
		(integer_operations::cast_or_clamp<GLsizei>
			(m.point_count())),
	line_count
		(integer_operations::cast_or_clamp<GLsizei>
			(m.line_count())),
	triangle_count
		(integer_operations::cast_or_clamp<GLsizei>
			(m.triangle_count())),

	stream_locations(stream_locations),
	transient_hint(transient_hint)
{
	validate_mesh_for_gl3(m);
	validate_mesh_stream_properties();

	create_vertex_data(gl3, m);
	create_indices(gl3, m);
	create_vertex_arrays(gl3, m);
}


const gl3_vertex_array* gl3_mesh::point_array() const
	{ return point_array_.get(); }
const gl3_vertex_array* gl3_mesh::line_array() const
	{ return line_array_.get(); }
const gl3_vertex_array* gl3_mesh::triangle_array() const
	{ return triangle_array_.get(); }


void gl3_mesh::render() const
{
	if (point_array_ && point_count > 0)
	{
		point_array_->bind();
		glDrawElements
			(GL_POINTS, point_count,
			 GL_UNSIGNED_INT, NULL);
	}

	if (line_array_ && line_count > 0)
	{
		line_array_->bind();
		glDrawElements
			(GL_LINES, line_count * 2,
			 GL_UNSIGNED_INT, NULL);	
	}

	if (triangle_array_ && triangle_count > 0)
	{
		triangle_array_->bind();
		glDrawElements
			(GL_TRIANGLES, triangle_count * 3,
			 GL_UNSIGNED_INT, NULL);	
	}
}


void gl3_mesh::validate_mesh_for_gl3(const mesh& m)
{
	/**
	 * Check if a @bref{m}esh is suitable
	 * for rendering with OpenGL 3.
	 *
	 * Some GL drivers (e.g. Intel HD) have limits
	 * to the number of vertex indices or primitives,
	 * but we can neither query nor check these.
	 */


	// Mesh must be valid on its own.
	// Interleaving anyways has the same complexity.
	m.validate();


	// Mesh must fit into GLsizei ranges.
	// Clamping in gl3_mesh::gl3_mesh()
	// must not have changed counts.
	if (vertex_count != m.vertex_count())
	{
		LOG_ERROR("Vertex count exceeds GLsizei range.");
		throw bounds_exceeded();
	}

	if (point_count != m.point_count())
	{
		LOG_ERROR("Point count exceeds GLsizei range.");
		throw bounds_exceeded();
	}

	if (line_count != m.line_count())
	{
		LOG_ERROR("Line count exceeds GLsizei range.");
		throw bounds_exceeded();
	}

	if (triangle_count != m.triangle_count())
	{
		LOG_ERROR("Triangle count exceeds GLsizei range.");
		throw bounds_exceeded();
	}


	// Indices and bytes of OpenGL elements must not overflow.
	// See gl3_mesh::render() for reference.
	if (!integer_checks::is_mult_safe(line_count, 2) ||
		!integer_checks::is_mult_safe(triangle_count, 3))
	{
		LOG_ERROR("Overflow on line or triangle index count.");
		throw bounds_exceeded();
	}

	if (!integer_checks::is_mult_safe
			(point_count, size_of<GLsizei>()) ||
		!integer_checks::is_mult_safe
			(line_count * 2, size_of<GLsizei>()) ||
		!integer_checks::is_mult_safe
			(triangle_count * 3, size_of<GLsizei>()))
	{
		LOG_ERROR("Overflow on point, line, or triangle bytes.");
		throw bounds_exceeded();
	}
}


void gl3_mesh::validate_mesh_stream_properties()
{
	/**
	 * Ensure that properties of mesh_stream%s
	 * match with those of interleaving utilities
	 * (e.g. create_element_byte_size_map()).
	 *
	 * Implementation notes:
	 * - The below tests detect changes in assumed formats
	 *		(e.g. skeleton_index::max_joints_per_vertex).
	 * - On any exception, accordingly update interleavers,
	 *		interleaving utilities, and these tests.
	 */

	if (size_of<color>() != size_of<float>() * 4)
	{
		LOG_ERROR("Format of color class not supported by interleaving.");
		throw unsupported_feature();
	}

	if (size_of<vector>() != size_of<float>() * 3)
	{
		LOG_ERROR("Format of vector class not supported by interleaving.");
		throw unsupported_feature();
	}


	if (size_of<skeleton_index>() != size_of<int64>() * 4)
	{
		LOG_ERROR("Format of skeleton_index class not supported by interleaving.");
		throw unsupported_feature();
	}

	if (size_of<skeleton_weight>() != size_of<float>() * 4)
	{
		LOG_ERROR("Format of skeleton_weight class not supported by interleaving.");
		throw unsupported_feature();
	}
}


void gl3_mesh::create_vertex_data
	(const gl3_api& gl3, const mesh& m)
{
	/**
	 * Build a single, interleaved data stream
	 * with all the per-vertex attribute data,
	 * then wrap into a gl3_vertex_buffer.
	 */


	// Initialize utility maps.
	stream_interleaver_map interleavers
		(create_interleaver_map());
	stream_element_byte_size_map element_byte_sizes
		(create_element_byte_size_map());


	// Resize host memory buffer to required storage.
	dynamic_buffer<char> interleaved_vertex_data;

	int64 bytes_per_vertex =
		find_element_byte_size
			(m.vertices(), element_byte_sizes);

	for (mesh::const_stream_iterator it(m.first_vertex_stream()); it; ++it)
		bytes_per_vertex =
			integer_operations::add_or_throw
				(bytes_per_vertex,
				 find_element_byte_size(*it->value, element_byte_sizes));

	interleaved_vertex_data.resize
		(integer_operations::mult_or_throw
			(bytes_per_vertex, m.vertex_count()));


	// Copy per-attribute data to host memory buffer
	// via suitable interleaver utility.
	interleave_vertex_data
		(m.vertices(), interleaved_vertex_data,
		 0, bytes_per_vertex, interleavers);

	int64 source_offset =
		find_element_byte_size
			(m.vertices(), element_byte_sizes);

	for (mesh::const_stream_iterator it(m.first_vertex_stream()); it; ++it)
	{
		interleave_vertex_data
			(*it->value, interleaved_vertex_data,
			 source_offset, bytes_per_vertex, interleavers);

		source_offset =
			integer_operations::add_or_throw
				(source_offset,
				 find_element_byte_size(*it->value, element_byte_sizes));
	}


	// Create OpenGL vertex buffer with interleaved
	// vertex data from host memory buffer.
	vertex_data_.reset
		(new gl3_vertex_buffer
			(gl3, gl3_vertex_buffer::vertex_data,
			 transient_hint ? gl3_vertex_buffer::stream_usage :
							  gl3_vertex_buffer::static_usage,
			 interleaved_vertex_data.data(),
			 interleaved_vertex_data.size()));
}


void gl3_mesh::create_indices
	(const gl3_api& gl3, const mesh& m)
{
	/**
	 * Create vertex buffers with indices
	 * for points, lines, and triangles.
	 *
	 * Implementation notes:
	 * - We need not check for overflow on conversions or sizes,
	 *		as validate_mesh_for_gl3() has limited all streams
	 *		to suitable sizes for use in OpenGL.
	 */


	// Fetch indices from mesh.
	const mesh_stream_point& points = m.points();
	const mesh_stream_line& lines = m.lines();
	const mesh_stream_triangle& triangles = m.triangles();


	// Convert points to GLuint and store in vertex buffer.
	if (points.size() != 0)
	{
		dynamic_buffer<GLuint> gl_points;
		gl_points.resize(points.size());

		const int64* VIRAL__RESTRICT_PTR source =
			reinterpret_cast<const int64*>(points.data());
		GLuint* VIRAL__RESTRICT_PTR target =
			gl_points.data();

		for (int64 i = 0; i < points.size(); ++i)
		{
			*target = static_cast<GLuint>(*source);
			++target; ++source;
		}

		point_indices_.reset
			(new gl3_vertex_buffer
				(gl3, gl3_vertex_buffer::index_data,
				 transient_hint ? gl3_vertex_buffer::stream_usage :
								  gl3_vertex_buffer::static_usage,
				 gl_points.data(),
				 gl_points.size() * size_of<GLuint>()));
	}


	// Convert lines to GLuint and store in vertex buffer.
	if (lines.size() != 0)
	{
		dynamic_buffer<GLuint> gl_lines;
		gl_lines.resize(lines.size() * 2);

		const int64* VIRAL__RESTRICT_PTR source =
			reinterpret_cast<const int64*>(lines.data());
		GLuint* VIRAL__RESTRICT_PTR target =
			gl_lines.data();

		for (int64 i = 0; i < lines.size(); ++i)
		for (int64 c = 0; c < 2; ++c)
		{
			*target = static_cast<GLuint>(*source);
			++target; ++source;
		}

		line_indices_.reset
			(new gl3_vertex_buffer
				(gl3, gl3_vertex_buffer::index_data,
				 transient_hint ? gl3_vertex_buffer::stream_usage :
								  gl3_vertex_buffer::static_usage,
				 gl_lines.data(),
				 gl_lines.size() * size_of<GLuint>()));
	}
	

	// Convert triangles to GLuint and store in vertex buffer.
	if (triangles.size() != 0)
	{
		dynamic_buffer<GLuint> gl_triangles;
		gl_triangles.resize(triangles.size() * 3);

		const int64* VIRAL__RESTRICT_PTR source =
			reinterpret_cast<const int64*>(triangles.data());
		GLuint* VIRAL__RESTRICT_PTR target =
			gl_triangles.data();

		for (int64 i = 0; i < triangles.size(); ++i)
		for (int64 c = 0; c < 3; ++c)
		{
			*target = static_cast<GLuint>(*source);
			++target; ++source;
		}

		triangle_indices_.reset
			(new gl3_vertex_buffer
				(gl3, gl3_vertex_buffer::index_data,
				 transient_hint ? gl3_vertex_buffer::stream_usage :
								  gl3_vertex_buffer::static_usage,
				 gl_triangles.data(),
				 gl_triangles.size() * size_of<GLuint>()));
	}
}


void gl3_mesh::create_vertex_arrays
	(const gl3_api& gl3, const mesh& m)
{
	/**
	 * Collect per-vertex attribute information
	 * and create vertex array objects that bind
	 * indices, attributes, and data together.
	 *
	 * @attent Vertex coordinates must always map
	 * to location 0 on NVIDIA hardware.
	 */


	// Initialize utility maps.
	stream_element_type_map element_types
		(create_element_type_map());
	stream_element_size_map element_sizes
		(create_element_size_map());
	stream_element_byte_size_map element_byte_sizes
		(create_element_byte_size_map());


	// Determine total number of bytes per vertex.
	int64 bytes_per_vertex =
		find_element_byte_size
			(m.vertices(), element_byte_sizes);

	for (mesh::const_stream_iterator it(m.first_vertex_stream()); it; ++it)
		bytes_per_vertex =
			integer_operations::add_or_throw
				(bytes_per_vertex,
				 find_element_byte_size(*it->value, element_byte_sizes));


	// Collect attribute information in temporary list.
	gl3_vertex_array::location_to_attribute_map location_to_attribute;

	collect_vertex_attributes
		(m.vertices(), 0, location_to_attribute,
		 *vertex_data_, 0, bytes_per_vertex,
		 element_types, element_sizes);

	int64 source_offset =
		find_element_byte_size
			(m.vertices(), element_byte_sizes);

	for (mesh::const_stream_iterator it(m.first_vertex_stream()); it; ++it)
	{
		stream_location_map::const_iterator find_location
			(stream_locations.find(it->key));

		if (!find_location)
		{
			LOG_ERROR("Location binding not found for stream: " + it->key);
			throw invalid_parameter();
		}

		collect_vertex_attributes
			(*it->value, find_location->value, location_to_attribute,
			 *vertex_data_, source_offset, bytes_per_vertex,
			 element_types, element_sizes);

		source_offset =
			integer_operations::add_or_throw
				(source_offset,
				 find_element_byte_size(*it->value, element_byte_sizes));
	}


	// Create separate vertex array objects
	// for points, lines, and triangles.
	if (point_indices_)
		point_array_.reset
			(new gl3_vertex_array
				(gl3, location_to_attribute, *point_indices_));

	if (line_indices_)
		line_array_.reset
			(new gl3_vertex_array
				(gl3, location_to_attribute, *line_indices_));

	if (triangle_indices_)
		triangle_array_.reset
			(new gl3_vertex_array
				(gl3, location_to_attribute, *triangle_indices_));
}


void gl3_mesh::interleave_vertex_data
	(const mesh_stream& stream,
	 dynamic_buffer<char>& interleaved_vertex_data,
	 int64 source_offset, int64 source_stride,
	 const stream_interleaver_map& interleavers)
{
	/**
	 * Copy all data from incoming mesh @bref{stream}
	 * into an @bref{interleaved_vertex_data} buffer
	 * by an interleaver that suits the stream type.
	 */

	stream_interleaver_map::const_iterator find_interleaver
		(interleavers.find(typeid(stream)));
	
	if (!find_interleaver)
	{
		LOG_ERROR("Vertex stream within mesh contains unknown elements.");
		throw invalid_parameter();
	}

	(*find_interleaver->value)
		(stream, interleaved_vertex_data, source_offset, source_stride);
}


void gl3_mesh::interleaver_bytewise
	(const mesh_stream& stream,
	 dynamic_buffer<char>& interleaved_vertex_data,
	 int64 source_offset, int64 source_stride)
{
	/**
	 * Bytewise copy of data from source stream
	 * to target interleaved buffer.
	 *	
	 * Implementation notes:
	 * - @attent Only works for streams with POD content.
	 * - Has low-level optimizations for improved throughput.	 
	 */

	const char* VIRAL__RESTRICT_PTR source =
		reinterpret_cast<const char*>(stream.data_untyped());
	char* VIRAL__RESTRICT_PTR target =
		interleaved_vertex_data.data() + source_offset;

	int64 stream_size = stream.size();
	int64 stream_bytes_per_entry = stream.bytes_per_entry();

	for (int64 i = 0; i < stream_size; ++i)
	{
		for (int64 j = 0; j < stream_bytes_per_entry; ++j)
			target[j] = source[j];
		
		source += stream_bytes_per_entry;
		target += source_stride;
	}
}


void gl3_mesh::interleaver_int64_to_GLint
	(const mesh_stream& stream,
	 dynamic_buffer<char>& interleaved_vertex_data,
	 int64 source_offset, int64 source_stride)
{
	/**
	 * Convert int64 from source stream to GLint
	 * in target interleaved buffer.
	 *
	 * Implementation notes:
	 * - Conversion throws on overflow.
	 * - @attent Only works for streams with int64 content.
	 * - Has low-level optimizations for improved throughput.	 
	 * - @attent The @bref{source_stride} is a size in bytes
	 *		and need not be a multiple of sizeof(GLint).
	 */

	const int64* VIRAL__RESTRICT_PTR source =
		reinterpret_cast<const int64*>(stream.data_untyped());
	char* VIRAL__RESTRICT_PTR target_untyped =
		reinterpret_cast<char*>
			(interleaved_vertex_data.data() + source_offset);

	int64 stream_size = stream.size();
	int64 stream_components_per_entry =
		stream.bytes_per_entry() / size_of<int64>();

	for (int64 i = 0; i < stream_size; ++i)
	{
		GLint* VIRAL__RESTRICT_PTR target_typed =
			reinterpret_cast<GLint*>(target_untyped);

		for (int64 j = 0; j < stream_components_per_entry; ++j)
			target_typed[j] =
				integer_operations::cast_or_throw<GLint>
					(source[j]);
		
		source += stream_components_per_entry;
		target_untyped += source_stride;
	}
}


void gl3_mesh::collect_vertex_attributes
	(const mesh_stream& stream, GLint location,
	 gl3_vertex_array::location_to_attribute_map& location_to_attribute,
	 const gl3_vertex_buffer& source, int64 source_offset, int64 source_stride,
	 const stream_element_type_map& element_types,
	 const stream_element_size_map& element_sizes)
{
	/**
	 * Add vertex attribute information for some mesh @bref{stream}
	 * to the vertex @bref{attributes} list.
	 *
	 * In particular, indicate where attribute data is located
	 * in the @bref{vertex_data_} buffer after interleaving.
	 */

	stream_element_type_map::const_iterator find_element_type
		(element_types.find(typeid(stream)));
	stream_element_size_map::const_iterator find_element_size
		(element_sizes.find(typeid(stream)));

	if (!find_element_type || !find_element_size)
	{
		LOG_ERROR("Vertex stream within mesh contains unknown elements.");
		throw invalid_parameter();
	}

	if (location_to_attribute.contains(location))
	{
		LOG_ERROR("Duplicate vertex stream for attribute location: " + location);
		throw invalid_parameter();
	}

	location_to_attribute.try_insert
		(location, gl3_vertex_array_attribute
			(source, source_offset, source_stride,
			 find_element_type->value, find_element_size->value));
}


int64 gl3_mesh::find_element_byte_size
	(const mesh_stream& stream,
	 const stream_element_byte_size_map& element_byte_sizes)
{
	/**
	 * Determine the number of bytes required in OpenGL
	 * for an element of the indicated @bref{stream}.
	 * This may differ from mesh_stream::bytes_per_entry(),
	 * for instance due to int64 to GLint conversion.
	 */

	stream_element_byte_size_map::const_iterator
		find_element_byte_size
			(element_byte_sizes.find(typeid(stream)));

	if (!find_element_byte_size)
	{
		LOG_ERROR("Vertex stream within mesh contains unknown elements.");
		throw invalid_parameter();
	}

	return find_element_byte_size->value;
}


gl3_mesh::stream_interleaver_map
	gl3_mesh::create_interleaver_map()
{
	/**
	 * Create a map with functions that interleave data
	 * of mesh_stream types that occur as vertex streams.
	 */

	stream_interleaver_map result;

	result.insert(typeid(mesh_stream_int32),
				  &interleaver_bytewise);
	result.insert(typeid(mesh_stream_uint32),
				  &interleaver_bytewise);
	result.insert(typeid(mesh_stream_float),
				  &interleaver_bytewise);

	result.insert(typeid(mesh_stream_color),
				  &interleaver_bytewise);
	result.insert(typeid(mesh_stream_vector),
				  &interleaver_bytewise);

	result.insert(typeid(mesh_stream_skeleton_index),
				  &interleaver_int64_to_GLint);
	result.insert(typeid(mesh_stream_skeleton_weight),
				  &interleaver_bytewise);

	return std::move(result);
}


gl3_mesh::stream_element_type_map
	gl3_mesh::create_element_type_map()
{
	/**
	 * Create a map with the OpenGL stream_element_type
	 * for mesh_stream types that occur as vertex streams.
	 */

	stream_element_type_map result;

	result.insert(typeid(mesh_stream_int32),
				  stream_element_type::int_elements);
	result.insert(typeid(mesh_stream_uint32),
				  stream_element_type::int_elements);
	result.insert(typeid(mesh_stream_float),
				  stream_element_type::float_elements);

	result.insert(typeid(mesh_stream_color),
				  stream_element_type::float_elements);
	result.insert(typeid(mesh_stream_vector),
				  stream_element_type::float_elements);

	result.insert(typeid(mesh_stream_skeleton_index),
				  stream_element_type::int_elements);
	result.insert(typeid(mesh_stream_skeleton_weight),
				  stream_element_type::float_elements);

	return std::move(result);
}


gl3_mesh::stream_element_size_map
	gl3_mesh::create_element_size_map()
{
	/**
	 * Create a map with the number of components per vertex
	 * (i.e. size parameter of a OpenGL vertex array attribute)
	 * for mesh_stream types that occur as vertex streams.
	 */

	stream_element_size_map result;

	result.insert(typeid(mesh_stream_int32), 1);
	result.insert(typeid(mesh_stream_uint32), 1);
	result.insert(typeid(mesh_stream_float), 1);

	result.insert(typeid(mesh_stream_color), 4);
	result.insert(typeid(mesh_stream_vector), 3);

	result.insert(typeid(mesh_stream_skeleton_index), 4);
	result.insert(typeid(mesh_stream_skeleton_weight), 4);

	return std::move(result);
}


gl3_mesh::stream_element_byte_size_map
	gl3_mesh::create_element_byte_size_map()
{
	/**
	 * Create a map with the number of bytes per vertex
	 * for mesh_stream types that occur as vertex streams.
	 */

	stream_element_byte_size_map result;

	result.insert(typeid(mesh_stream_int32),
				  sizeof(GLint));
	result.insert(typeid(mesh_stream_uint32),
				  sizeof(GLint));
	result.insert(typeid(mesh_stream_float),
				  sizeof(float));

	result.insert(typeid(mesh_stream_color),
				  sizeof(float) * 4);
	result.insert(typeid(mesh_stream_vector),
				  sizeof(float) * 3);

	result.insert(typeid(mesh_stream_skeleton_index),
				  sizeof(GLint) * 4);
	result.insert(typeid(mesh_stream_skeleton_weight),
				  sizeof(float) * 4);

	return std::move(result);
}




} /* namespace viral_core */
