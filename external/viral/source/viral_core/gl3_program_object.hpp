/**
 *************************************************************************
 *
 * @file gl3_program_object.hpp
 *
 * Header with shader utilities for OpenGL3.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_PROGRAM_OBJECT_HPP)
#define INCLUDED__VIRAL_CORE__GL3_PROGRAM_OBJECT_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "gl3_forward.hpp"


#include "util.hpp"
#include "string.hpp"
#include "auto_pointer.hpp"
#include "list.hpp"
#include "gl3_vertex_buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_shader_object
 * @ingroup gl3_render_module
 *
 * Wrapper class for OpenGL3 shader objects, used by gl3_program_object.
 *
 ************************************************************************/
class gl3_shader_object :
	private non_copyable
{
	friend class gl3_program_object;


	public:

		enum shader_type
		{
			vertex_shader,
			geometry_shader,
			fragment_shader,
			shader_type_count
		};


		gl3_shader_object
			(const gl3_api& gl3, const string& source_code,
			 shader_type type,
			 const string& virtual_path_hint,
			 const string& absolute_path_hint);

		~gl3_shader_object() NOTHROW;


	private:

		void log_trace_on_error();
		void log_compile_errors();


		const gl3_api& gl3_;

		GLuint gl_name_;

		const string source_code_;
		const shader_type type_;

		const string virtual_path_hint_;
		const string absolute_path_hint_;

		static const GLsizei max_shader_size_;
		static const GLenum gl_shader_types_[shader_type_count];
};




/**
 *************************************************************************
 *
 * @class gl3_program_vertex_attribute
 * @ingroup gl3_render_module
 *
 * Stores information about a named vertex attribute
 * that is required as input by a gl3_program_object.
 *
 ************************************************************************/
class gl3_program_vertex_attribute
{

	public:

		gl3_program_vertex_attribute
			(const string& name, GLsizei size,
			 GLenum type, GLint location)
			:
			name(name), size(size),
			type(type), location(location),
			element_type(to_element_type(type)),
			element_size(to_element_size(type))
		{ }
		

		const string name;
		const GLsizei size;
		const GLenum type;
		const GLint location;		


		/**
		 * Type and size of data within a compatible gl3_vertex_array_attribute.
		 * Used to check for matching attribute streams over meshes and shaders.
		 */
		//@{
		const gl3_vertex_array_attribute::attribute_element_type element_type;
		const GLint element_size;
		//@}


	private:

		static gl3_vertex_array_attribute::attribute_element_type
			to_element_type(GLenum type);
		static GLint
			to_element_size(GLenum type);
};




/**
 *************************************************************************
 *
 * @class gl3_program_object
 * @ingroup gl3_render_module
 *
 * Wrapper class for shader program objects within OpenGL3.
 *
 * Usage notes:
 * - Clients should prefer gl3_shader_workshop and gl3_shader_bricks
 *		over entirely hand-written shaders.
 * - GLSL code must explicitly use layout(location) statements
 *		to apply locations to all vertex @bref{attributes_}.
 * - On rendering, shader attribute locations must match with
 *		corresponding @bref{gl3_model_data::stream_locations}.
 *		See gl3_model_data for rationale.
 *
 ************************************************************************/
class gl3_program_object :
	private non_copyable
{

	public:

		typedef list<gl3_program_vertex_attribute> attribute_list;
		typedef list<gl3_uniform> uniform_list;


		/**
		 * Create an OpenGL program object from given shaders.
		 *
		 * Usage notes:
		 * - The @bref{vertex_shader} and @bref{fragment_shader}
		 *		are mandatory and must not be @NULL.
		 * - The optional @bref{geometry_shader} may be @NULL,
		 *		which disables all custom geometry processing.
		 *
		 * Implementation notes:
		 * - As geometry shaders restrict rendering to one type
		 *		of primitive (i.e. points, lines, or triangles),
		 *		we cannot make geometry shaders mandatory.
		 */
		gl3_program_object
			(const gl3_api& gl3,
			 auto_pointer<gl3_shader_object> vertex_shader,
			 auto_pointer<gl3_shader_object> geometry_shader,
			 auto_pointer<gl3_shader_object> fragment_shader);


		~gl3_program_object() NOTHROW;


		attribute_list::const_iterator first_attribute() const;
		uniform_list::const_iterator first_uniform() const;


		/**
		 * Ensure that a given mesh provides suitable streams
		 * for the attributes required by this shader program.
		 * Otherwise, log and throw an exception.
		 */
		void check_matching_locations(const gl3_mesh& m) const;


		/**
		 * Apply uniform values from a gl3_uniform_value_stack
		 * to all uniforms of this shader.
		 *
		 * The call throws unless the stack assigns a value
		 * of matching type to each uniform. This enables
		 * rapid debugging of shader code.
		 *
		 * @attent Runtime scales linearly with stack size,
		 * so avoid stacks with too many entries.
		 */
		void apply_uniform_values
			(const gl3_uniform_value_stack& uniform_value_stack) const;


		void bind() const;
		static void unbind(const gl3_api& gl3) NOTHROW;


	private:

		void log_link_errors();

		void init_query_attributes();
		void init_query_uniforms();


		const gl3_api& gl3_;

		auto_pointer<gl3_shader_object> vertex_shader_;
		auto_pointer<gl3_shader_object> geometry_shader_;
		auto_pointer<gl3_shader_object> fragment_shader_;

		attribute_list attributes_;
		uniform_list uniforms_;

		GLuint gl_name_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_PROGRAM_OBJECT_HPP) */
