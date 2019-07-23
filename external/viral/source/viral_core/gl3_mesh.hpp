/**
 *************************************************************************
 *
 * @file gl3_mesh.hpp
 *
 * OpenGL 3 GPU meshes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_MESH_HPP)
#define INCLUDED__VIRAL_CORE__GL3_MESH_HPP


#include "build_config.hpp"


#include "string_forward.hpp"
#include "container_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "tree_map.hpp"
#include "gl3_vertex_buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_mesh
 * @ingroup gl3_render_module
 *
 * Stores a @ref mesh in GPU memory.
 *
 ************************************************************************/
class gl3_mesh :
	private non_copyable
{
	public:

		typedef tree_map<string, GLint> stream_location_map;


		/**
		 * Upload some @bref{m}esh to gl3_vertex array%s
		 * and gl3_vertex_buffer%s.
		 */
		gl3_mesh
			(const gl3_api& gl3, const mesh& m,
			 const stream_location_map& stream_locations,
			 bool transient_hint);


		/**
		 * Fetch vertex array with points, lines or triangles.
		 * @NULL if no points, lines or triangles exist in the group.
		 */
		//@{
		const gl3_vertex_array* point_array() const;
		const gl3_vertex_array* line_array() const;
		const gl3_vertex_array* triangle_array() const;
		//@}


		/**
		 * Render each not-@NULL vertex array of this mesh.
		 *
		 * The call assumes that required rendering states
		 * (e.g. programs, uniforms) have already been set.
		 * On return, internal vertex arrays remain bound.
		 */
		void render() const;


		const GLsizei vertex_count;
		const GLsizei point_count;
		const GLsizei line_count;
		const GLsizei triangle_count;

		const stream_location_map stream_locations;

		const bool transient_hint;


	private:

		typedef void (*stream_interleaver)
			(const mesh_stream&, dynamic_buffer<char>&, int64, int64);
		typedef tree_map<type_info_ref, stream_interleaver> stream_interleaver_map;

		typedef gl3_vertex_array_attribute::attribute_element_type stream_element_type;
		typedef tree_map<type_info_ref, stream_element_type> stream_element_type_map;
		typedef tree_map<type_info_ref, int64> stream_element_size_map;
		typedef tree_map<type_info_ref, int64> stream_element_byte_size_map;


		void validate_mesh_for_gl3(const mesh& m);
		void validate_mesh_stream_properties();


		void create_vertex_data
			(const gl3_api& gl3, const mesh& m);
		void create_indices
			(const gl3_api& gl3, const mesh& m);
		void create_vertex_arrays
			(const gl3_api& gl3, const mesh& m);

		
		static void interleave_vertex_data
			(const mesh_stream& stream,
			 dynamic_buffer<char>& interleaved_vertex_data,
			 int64 source_offset, int64 source_stride,
			 const stream_interleaver_map& interleavers);

		static void interleaver_bytewise
			(const mesh_stream& stream,
			 dynamic_buffer<char>& interleaved_vertex_data,
			 int64 source_offset, int64 source_stride);
		static void interleaver_int64_to_GLint
			(const mesh_stream& stream,
			 dynamic_buffer<char>& interleaved_vertex_data,
			 int64 source_offset, int64 source_stride);


		static void collect_vertex_attributes
			(const mesh_stream& stream, GLint location,
			 gl3_vertex_array::location_to_attribute_map& attributes,
			 const gl3_vertex_buffer& source, int64 source_offset, int64 source_stride,
			 const stream_element_type_map& element_types,
			 const stream_element_size_map& element_sizes);


		static int64 find_element_byte_size
			(const mesh_stream& stream,
			 const stream_element_byte_size_map& element_byte_sizes);

		
		static stream_interleaver_map create_interleaver_map();
		static stream_element_type_map create_element_type_map();
		static stream_element_size_map create_element_size_map();
		static stream_element_byte_size_map create_element_byte_size_map();
		

		auto_pointer<gl3_vertex_buffer> vertex_data_;

		auto_pointer<gl3_vertex_buffer> point_indices_;
		auto_pointer<gl3_vertex_buffer> line_indices_;
		auto_pointer<gl3_vertex_buffer> triangle_indices_;

		auto_pointer<gl3_vertex_array> point_array_;
		auto_pointer<gl3_vertex_array> line_array_;
		auto_pointer<gl3_vertex_array> triangle_array_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_MESH_HPP) */
