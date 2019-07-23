/**
 *************************************************************************
 *
 * @file gl3_vertex_buffer.hpp
 *
 * Header for OpenGL3 vertex buffers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_VERTEX_BUFFER_HPP)
#define INCLUDED__VIRAL_CORE__GL3_VERTEX_BUFFER_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "gl3_forward.hpp"


#include "util.hpp"
#include "tree_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_vertex_buffer
 * @ingroup gl3_render_module
 *
 * Stores unspecified data for standalone use or for later binding
 * to a gl3_vertex_array.
 *
 ************************************************************************/
class gl3_vertex_buffer :
	private non_copyable
{
	public:

		enum vertex_buffer_type
			{ vertex_data, index_data, vertex_buffer_type_count };
		enum vertex_buffer_usage
			{ static_usage, stream_usage, vertex_buffer_usage_count };


		/**		
		 * Create buffer with indicated @bref{type} and @bref{usage},
		 * and an initial data of @bref{size} bytes from @bref{source}.
		 * Preallocates memory for later set() if @bref{source} is @NULL.
		 * This call always changes the bound vertex buffer.
		 */
		gl3_vertex_buffer
			(const gl3_api& gl3,
			 vertex_buffer_type type, vertex_buffer_usage usage,
			 const void *source, int64 size);


		~gl3_vertex_buffer() NOTHROW;


		/**
		 * Replace current buffer data, as in gl3_vertex_buffer().
		 * Potentially better performance if the buffer size does not change.
		 * @attent Unbinds any current gl3_vertex_array.
		 */
		void set(const void *source, int64 size);


		vertex_buffer_type type() const NOTHROW;
		GLsizei size() const NOTHROW;


		/**
		 * Bind or unbind some gl3_vertex_buffer.
		 * @attent If a gl3_vertex_array is currently bound,
		 * its binding points will be modified as well.
		 */
		//@{
		void bind() const;
		static void unbind
			(vertex_buffer_type type, const gl3_api& gl3) NOTHROW;
		//@}


		static const GLsizei max_size = 1073741824;


	private:

		const gl3_api& gl3_;

		GLuint gl_name_;
		vertex_buffer_type type_;
		vertex_buffer_usage usage_;
		GLsizei size_;

		static const GLenum gl_buffer_target_[vertex_buffer_type_count];
		static const GLenum gl_buffer_usage_[vertex_buffer_usage_count];
};




/**
 *************************************************************************
 *
 * @class gl3_vertex_array_attribute
 * @ingroup gl3_render_module
 *
 * Maps a gl3_vertex_buffer to some per-vertex attribute stream
 * within a gl3_vertex_array.
 *
 ************************************************************************/
class gl3_vertex_array_attribute
{
	public:

		enum attribute_element_type
			{ float_elements, int_elements, attribute_element_type_count };


		gl3_vertex_array_attribute
			(const gl3_vertex_buffer& source,
			 int64 in_source_offset, int64 in_source_stride,
			 attribute_element_type element_type, int64 in_element_size);


		const gl3_vertex_buffer& source;
		const GLvoid* source_offset;
		const GLsizei source_stride;

		const attribute_element_type element_type;
		const GLint element_size;
};




/**
 *************************************************************************
 *
 * @class gl3_vertex_array
 * @ingroup gl3_render_module
 *
 * Binds together a series of gl3_vertex_buffer%s that define
 * all per-vertex attributes as well as primitive indices for use
 * in a single rendering call.
 *
 * Usage notes:
 * - A map of gl3_vertex_array_attribute%s binds gl3_vertex_buffer%s
 *		to unique attribute locations and provides attribute layout
 *		within these vertex buffers.
 * - For performance reasons, it is encouraged to use but few buffers,
 *		yet with interleaved vertex attributes.
 * - Attached gl3_vertex_buffer%s are not owned, and must not be deleted
 *		before the vertex array.
 *
 ************************************************************************/
class gl3_vertex_array :
	private non_copyable
{
	public:
	
		typedef tree_map<GLint, gl3_vertex_array_attribute>
			location_to_attribute_map;


		/**
		 * Create a vertex array that combines various vertex buffers.
		 *
		 * The @bref{location_to_attribute} map names GLSL locations,
		 * vertex buffers to bind to these locations, and the layout
		 * of vertex attributes within the vertex buffers.
		 *
		 * The @bref{indices} buffer holds primitive indices
		 * into the vertex attributes of bound vertex buffers.
		 *
		 * This call always changes currently bound vertex buffers
		 * and vertex array objects.
		 */
		gl3_vertex_array
			(const gl3_api& gl3,
			 const location_to_attribute_map& location_to_attribute,
			 const gl3_vertex_buffer& indices);


		~gl3_vertex_array() NOTHROW;


		location_to_attribute_map::const_iterator
			first_attribute() const NOTHROW;
		location_to_attribute_map::const_iterator
			find_attribute(GLint location) const NOTHROW;


		void bind() const;
		static void unbind(const gl3_api& gl3) NOTHROW;


	private:

		const gl3_api& gl3_;
		GLuint gl_name_;
		location_to_attribute_map location_to_attribute_;
		const gl3_vertex_buffer& indices_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_VERTEX_BUFFER_HPP) */
