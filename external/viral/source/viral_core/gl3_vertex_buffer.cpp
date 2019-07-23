/**
 *************************************************************************
 *
 * @file gl3_vertex_buffer.cpp
 *
 * OpenGL3 vertex buffer implementation.
 *
 ************************************************************************/


#include "gl3_vertex_buffer.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "gl3_api.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_vertex_buffer
//
//////////////////////////////////////////////////////////////////////////


gl3_vertex_buffer::gl3_vertex_buffer
	(const gl3_api& gl3,
	 vertex_buffer_type type, vertex_buffer_usage usage,
	 const void *source, int64 size)
	:
	gl3_(gl3),
	gl_name_(0),
	type_(type), usage_(usage),
	size_(0)
{
	// Validate incoming parameters.
	// No validation for size and source,
	// as this is done in set() later on.
	if (type_ < 0 || type_ >= vertex_buffer_type_count)
	{
		LOG_ERROR("Invalid type for vertex buffer.");
		throw invalid_parameter();
	}

	if (usage_ < 0 || usage_ >= vertex_buffer_usage_count)
	{
		LOG_ERROR("Invalid usage for vertex buffer.");
		throw invalid_parameter();
	}


	// Create the vertex buffer
	// and fill it with data.
	gl3_.glGenBuffers(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for vertex buffer.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(gl_name_guard, (gl3_.glDeleteBuffers, 1, &gl_name_));

	set(source, size);

	gl_name_guard.dismiss();
}


gl3_vertex_buffer::~gl3_vertex_buffer() NOTHROW
{
	// gl_name_ is never 0 here.
	gl3_.glDeleteBuffers(1, &gl_name_);
}


void gl3_vertex_buffer::set
	(const void *source, int64 size)
{
	// Validate incoming buffer.
	if (size < 0 || size > max_size)
	{
		LOG_ERROR("Invalid size for vertex buffer data.");
		throw invalid_parameter();
	}

	if (size > 0 && !source)
	{
		LOG_ERROR("Invalid pointer for vertex buffer data.");
		throw null_pointer();
	}


	// Provide buffer data to OpenGL.
	// Possibly reuse any existing vertex buffer,
	// but don't modify any currently bound array.
	gl3_.glBindVertexArray(0); 
	gl3_.glBindBuffer(gl_buffer_target_[type_], gl_name_);

	if (size != size_)
	{
		size_ =
			integer_operations::cast_or_throw<GLsizei>(size);

		// Exception safety: NOTHROW from here.
		gl3_.glBufferData
			(gl_buffer_target_[type_],
			 size_, source, gl_buffer_usage_[usage_]);
	}
	else
	{
		// Performance optimization: Reuse existing buffer.
		gl3_.glBufferSubData
			(gl_buffer_target_[type_], 0, size_, source);
	}
}


gl3_vertex_buffer::vertex_buffer_type gl3_vertex_buffer::type() const NOTHROW
	{ return type_; }
GLsizei gl3_vertex_buffer::size() const NOTHROW
	{ return size_; }


void gl3_vertex_buffer::bind() const
{
	if (type_ == vertex_data)
		gl3_.glBindBuffer(GL_ARRAY_BUFFER, gl_name_);
	else if (type_ == index_data)
		gl3_.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gl_name_);
	else
	{
		LOG_ERROR("Invalid buffer type provided for binding vertex buffer.");
		throw invalid_parameter();
	}
}


void gl3_vertex_buffer::unbind
	(vertex_buffer_type type, const gl3_api& gl3) NOTHROW
{
	if (type == vertex_data)
		gl3.glBindBuffer(GL_ARRAY_BUFFER, 0);
	else if (type == index_data)
		gl3.glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	else
	{
		LOG_ERROR("Invalid buffer type provided for unbinding vertex buffer.");
		return; // NOTHROW on purpose
	}
}


const GLenum gl3_vertex_buffer::gl_buffer_target_[vertex_buffer_type_count] =
	{ GL_ARRAY_BUFFER, GL_ELEMENT_ARRAY_BUFFER };

const GLenum gl3_vertex_buffer::gl_buffer_usage_[vertex_buffer_type_count] =
	{ GL_STATIC_DRAW, GL_STREAM_DRAW };




//////////////////////////////////////////////////////////////////////////
//
// gl3_vertex_array_attribute
//
//////////////////////////////////////////////////////////////////////////


gl3_vertex_array_attribute::gl3_vertex_array_attribute
	(const gl3_vertex_buffer& source,
	 int64 in_source_offset, int64 in_source_stride,
	 attribute_element_type element_type, int64 in_element_size)
	:
	source(source),
	source_offset
		(reinterpret_cast<GLvoid*>(in_source_offset)),
	source_stride
		(integer_operations::cast_or_throw<GLsizei>(in_source_stride)),		

	element_type(element_type),
	element_size
		(integer_operations::cast_or_throw<GLint>(in_element_size))
{
	// We don't have a checked cast from int64 to GLvoid*,
	// so instead check for information loss after round-trip.
	if (reinterpret_cast<int64>(source_offset) != in_source_offset)
	{
		LOG_ERROR("Overflow on vertex array offset.");
		throw integer_overflow();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_vertex_array
//
//////////////////////////////////////////////////////////////////////////


gl3_vertex_array::gl3_vertex_array
	(const gl3_api& gl3,
	 const location_to_attribute_map& location_to_attribute,
	 const gl3_vertex_buffer& indices)
	:
	gl3_(gl3),
	gl_name_(0),
	location_to_attribute_(location_to_attribute),
	indices_(indices)
{
	// Create vertex array.
	gl3_.glGenVertexArrays(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for vertex array.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(gl_name_guard, (gl3_.glDeleteVertexArrays, 1, &gl_name_));

	gl3_.glBindVertexArray(gl_name_);


	// Apply incoming attributes.
	for (location_to_attribute_map::iterator it
			(location_to_attribute_.first()); it; ++it)
	{
		GLint location = it->key;
		GLuint checked_location =
			integer_operations::cast_or_throw<GLuint>(location);

		const gl3_vertex_array_attribute& attribute = it->value;

		if (attribute.source.type() != gl3_vertex_buffer::vertex_data)
		{
			LOG_WARN("Ignoring attached buffer that carries no vertex data.");
			continue;
		}

		if (attribute.element_type == gl3_vertex_array_attribute::float_elements)
		{
			attribute.source.bind();

			gl3_.glVertexAttribPointer
				(checked_location,
				 attribute.element_size,
				 GL_FLOAT, GL_FALSE,
				 attribute.source_stride,
				 attribute.source_offset);

			gl3_.glEnableVertexAttribArray(checked_location);
		}
		else if (attribute.element_type == gl3_vertex_array_attribute::int_elements)
		{
			attribute.source.bind();

			// This variant uses an internal GL_INT representation
			// and correctly maps integral GLSL types (e.g. uvec4).
			// In contrast, glVertexAttribPointer() implicitly casts to float,
			// which causes GLSL integers to end up with the float bit pattern.
			gl3_.glVertexAttribIPointer
				(checked_location,
				 attribute.element_size,
				 GL_INT, 
				 attribute.source_stride,
				 attribute.source_offset);

			gl3_.glEnableVertexAttribArray(checked_location);
		}
		else
		{
			LOG_WARN("Ignoring attached attribute with invalid element type.");
			continue;
		}
	}


	// Apply indices buffer.
	if (indices_.type() != gl3_vertex_buffer::index_data)
	{
		LOG_ERROR("Attached index buffer does not hold index data.");
		throw invalid_parameter();
	}

	indices_.bind();


	gl_name_guard.dismiss();
}


gl3_vertex_array::~gl3_vertex_array() NOTHROW
{
	// gl_name never is 0 here.
	gl3_.glDeleteVertexArrays(1, &gl_name_);
}


gl3_vertex_array::location_to_attribute_map::const_iterator
	gl3_vertex_array::first_attribute() const NOTHROW
		{ return location_to_attribute_.first(); }

gl3_vertex_array::location_to_attribute_map::const_iterator
	gl3_vertex_array::find_attribute(GLint location) const NOTHROW
		{ return location_to_attribute_.find(location); }


void gl3_vertex_array::bind() const
	{ gl3_.glBindVertexArray(gl_name_); }

void gl3_vertex_array::unbind
	(const gl3_api& gl3) NOTHROW
		{ gl3.glBindVertexArray(0); }




} /* namespace viral_core */

