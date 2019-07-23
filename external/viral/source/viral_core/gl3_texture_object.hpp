/**
 *************************************************************************
 *
 * @file gl3_texture_object.hpp
 *
 * Wrapper classes for OpenGL3 texture object identifiers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_TEXTURE_OBJECT_HPP)
#define INCLUDED__VIRAL_CORE__GL3_TEXTURE_OBJECT_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "gl3_forward.hpp"


#include "util.hpp"
#include "render_resource.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_texture_object
 * @ingroup gl3_render_module
 *
 * Wrapper for any OpenGL3 object that can be bound to a texture unit.
 *
 ************************************************************************/
class gl3_texture_object :
	private non_copyable
{

	public:

		enum texture_format
		{
			texture_format_srgba8,
			texture_format_rgba8,
			texture_format_srgba8_compressed,
			texture_format_rgba8_compressed,
			texture_format_rgba32f,
			texture_format_depth16,
			texture_format_depth24,
			texture_format_depth32,
			texture_format_packed_depth_stencil,
			texture_format_count
		};


		virtual ~gl3_texture_object() NOTHROW { }


		/**
		 * Rebuild the mipmap chain of this texture
		 * in accordance to its texture filter mode.
		 * Only needed after changing texture contents,
		 * such as after render-to-texture operations.		 
		 *
		 * The texture bind()s to texture unit zero
		 * during mipmap rebuilds.
		 */
		virtual void build_mipmaps() const = 0;
		

		virtual void bind(GLenum texture_unit_index) const = 0;
		static void unbind
			(GLenum texture_unit_index, const gl3_api& gl3) NOTHROW;


	protected:

		gl3_texture_object(const gl3_api& gl3);


		static texture_format to_texture_format
			(render_texture_data::color_space_mode color_space,
			 render_texture_data::compression_mode compression);

		static void apply_wrap
			(GLenum target, render_texture_data::wrap_mode wrap);

		static void apply_filter_and_mipmaps
			(const gl3_api& gl3, GLenum target,
			 render_texture_data::filter_mode filter,
			 bool create_mipmaps);


		static const GLint gl_internal_formats_[texture_format_count];
		static const GLenum gl_base_formats_[texture_format_count];
		static const GLenum gl_format_types_[texture_format_count];

		const gl3_api& gl3_;
};




/**
 *************************************************************************
 *
 * @class gl3_texture_object_2d
 * @ingroup gl3_render_module
 *
 * Wrapper for simple 2D texture objects.
 *
 ************************************************************************/
class gl3_texture_object_2d :
	public gl3_texture_object
{
	friend class gl3_frame_buffer_attachment_2d;


	public:

		/** Create texture from @bref{texture} image. */
		gl3_texture_object_2d
			(const gl3_api& gl3,
			 const image_rgba& texture,
			 render_texture_data::wrap_mode wrap,
			 render_texture_data::filter_mode filter,
			 render_texture_data::color_space_mode color_space,
			 render_texture_data::compression_mode compression);


		/**
		 * Create empty texture of specified @bref{format},
		 * usually for use as a render target.
		 */
		gl3_texture_object_2d
			(const gl3_api& gl3,
			 const vector2i& size,
			 texture_format format,
			 render_texture_data::wrap_mode wrap,
			 render_texture_data::filter_mode filter);


		~gl3_texture_object_2d() NOTHROW override;

		void build_mipmaps() const override;

		void bind(GLenum texture_unit_index) const override;


		const vector2i size;
		const texture_format format;
		const render_texture_data::wrap_mode wrap;
		const render_texture_data::filter_mode filter;


	private:

		GLuint gl_name_;
};




/**
 *************************************************************************
 *
 * @class gl3_texture_object_2d_array
 * @ingroup gl3_render_module
 *
 * Wrapper for an array of 2D textures.
 *
 ************************************************************************/
class gl3_texture_object_2d_array :
	public gl3_texture_object
{
	friend class gl3_frame_buffer_attachment_2d_array;


	public:

		/**
		 * Create empty texture array of specified @bref{format},
		 * usually for use as a render target.
		 */
		gl3_texture_object_2d_array
			(const gl3_api& gl3,
			 const vector2i& size,
			 int64 layer_count,
			 texture_format format,
			 render_texture_data::wrap_mode wrap,
			 render_texture_data::filter_mode filter);

		~gl3_texture_object_2d_array() NOTHROW override;

		void build_mipmaps() const override;

		void bind(GLenum texture_unit_index) const override;


		const vector2i size;
		const int64 layer_count;
		const texture_format format;
		const render_texture_data::wrap_mode wrap;
		const render_texture_data::filter_mode filter;


	private:

		GLuint gl_name_;
};




/**
 *************************************************************************
 *
 * @class gl3_texture_object_cubemap
 * @ingroup gl3_render_module
 *
 * Wrapper for cube maps.
 *
 ************************************************************************/
class gl3_texture_object_cubemap :
	public gl3_texture_object
{
	friend class gl3_frame_buffer_attachment_cubemap;


	public:

		/**
		 * Create texture and initialize with image
		 * for each face of the cube.
		 */
		gl3_texture_object_cubemap
			(const gl3_api& gl3,
			 const image_rgba& x_pos, const image_rgba& y_pos, const image_rgba& z_pos,
			 const image_rgba& x_neg, const image_rgba& y_neg, const image_rgba& z_neg,
			 render_texture_data::filter_mode filter,
			 render_texture_data::color_space_mode color_space,
			 render_texture_data::compression_mode compression);


		/**
		 * Create empty texture of specified @bref{format},
		 * usually for use as a render target.
		 */
		gl3_texture_object_cubemap
			(const gl3_api& gl3,
			 int64 size,
			 texture_format format,
			 render_texture_data::filter_mode filter);


		~gl3_texture_object_cubemap() NOTHROW override;

		void build_mipmaps() const override;

		void bind(GLenum texture_unit_index) const override;


		const int64 size;
		const texture_format format;
		const render_texture_data::filter_mode filter;


	private:

		GLuint gl_name_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_TEXTURE_OBJECT_HPP) */
