/**
 *************************************************************************
 *
 * @file gl3_frame_buffer.hpp
 *
 * Header for OpenGL3 frame buffer management.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_FRAME_BUFFER_HPP)
#define INCLUDED__VIRAL_CORE__GL3_FRAME_BUFFER_HPP


#include "build_config.hpp"
#include "gl3_config.hpp"


#include "container_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "geo_2d.hpp"
#include "owning_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_frame_buffer
 * @ingroup gl3_render_module
 *
 * Encapsules an OpenGL frame buffer.
 *
 * Usage notes:
 * - Any gl3_texture_object%s linked by gl3_frame_buffer_attachment%s
 *		are not owned and must outlive the gl3_frame_buffer.
 *
 ************************************************************************/
class gl3_frame_buffer :
	private non_copyable
{
	public:

		enum binding_target
			{ binding_draw, binding_read, binding_draw_read };

		enum attachment_point
		{
			attachment_color0, attachment_color1,
			attachment_color2, attachment_color3,
			attachment_depth, attachment_stencil,
			attachment_point_count
		};


		typedef owning_map
			<attachment_point, gl3_frame_buffer_attachment>
				attachment_map;


		/**
		 * Create a frame buffer with given dimensions
		 * and suitable @bref{attachments}.
		 */
		gl3_frame_buffer
			(const gl3_api& gl3, const vector2i& size,
			 const attachment_map& attachments);

	
		~gl3_frame_buffer() NOTHROW;


		const vector2i& size() const NOTHROW;


		void bind(binding_target target) const;
		static void unbind
			(binding_target target, const gl3_api& gl3) NOTHROW;


	private:

		const gl3_api& gl3_;
		const vector2i size_;

		GLuint gl_name_;
		const attachment_map attachments_;
};




/**
 *************************************************************************
 *
 * @class gl3_frame_buffer_attachment
 * @ingroup gl3_render_module
 *
 * Attaches some gl3_texture_object to a gl3_frame_buffer.
 *
 ************************************************************************/
class gl3_frame_buffer_attachment
{
	public:

		virtual ~gl3_frame_buffer_attachment() NOTHROW;	

		virtual auto_pointer<gl3_frame_buffer_attachment>
			clone() const = 0;

		virtual vector2i attach_size() const NOTHROW = 0;

		virtual void bind
			(gl3_frame_buffer::attachment_point point,
			 const gl3_api& gl3) const = 0;
		static void unbind
			(gl3_frame_buffer::attachment_point point,
			 const gl3_api& gl3) NOTHROW;


	protected:

		static const GLenum gl_attachments_
			[gl3_frame_buffer::attachment_point_count];
};




/**
 *************************************************************************
 *
 * @class gl3_frame_buffer_attachment_2d
 * @ingroup gl3_render_module
 *
 * Attaches a gl3_texture_object_2d to a gl3_frame_buffer.
 *
 ************************************************************************/
class gl3_frame_buffer_attachment_2d :
	public gl3_frame_buffer_attachment
{
	public:

		gl3_frame_buffer_attachment_2d
			(const gl3_texture_object_2d& texture_object_2d);

		auto_pointer<gl3_frame_buffer_attachment>
			clone() const override;

		vector2i attach_size() const NOTHROW override;

		void bind
			(gl3_frame_buffer::attachment_point point,
			 const gl3_api& gl3) const override;


	private:

		const gl3_texture_object_2d& texture_object_2d_;
};




/**
 *************************************************************************
 *
 * @class gl3_frame_buffer_attachment_2d_array
 * @ingroup gl3_render_module
 *
 * Attaches a specific @bref{layer} of a gl3_texture_object_2d_array
 * to a gl3_frame_buffer.
 *
 ************************************************************************/
class gl3_frame_buffer_attachment_2d_array :
	public gl3_frame_buffer_attachment
{
	public:

		gl3_frame_buffer_attachment_2d_array
			(const gl3_texture_object_2d_array& texture_object_2d_array,
			 int64 layer);

		auto_pointer<gl3_frame_buffer_attachment>
			clone() const override;

		vector2i attach_size() const NOTHROW override;

		void bind
			(gl3_frame_buffer::attachment_point point,
			 const gl3_api& gl3) const override;


	private:

		const gl3_texture_object_2d_array& texture_object_2d_array_;
		const int64 layer_;
};




/**
 *************************************************************************
 *
 * @class gl3_frame_buffer_attachment_cubemap
 * @ingroup gl3_render_module
 *
 * Attaches a specific @bref{side} of a gl3_texture_object_2d_array
 * to a gl3_frame_buffer.
 *
 ************************************************************************/
class gl3_frame_buffer_attachment_cubemap :
	public gl3_frame_buffer_attachment
{
	public:

		enum cubemap_side
			{ cubemap_side_xpos, cubemap_side_ypos, cubemap_side_zpos,
			  cubemap_side_xneg, cubemap_side_yneg, cubemap_side_zneg,
			  cubemap_side_count };

		gl3_frame_buffer_attachment_cubemap
			(const gl3_texture_object_cubemap& texture_object_cubemap,
			 cubemap_side side);

		auto_pointer<gl3_frame_buffer_attachment>
			clone() const override;

		vector2i attach_size() const NOTHROW override;

		void bind
			(gl3_frame_buffer::attachment_point point,
			 const gl3_api& gl3) const override;


	private:

		static const GLuint gl_cubemap_side_target_[cubemap_side_count];

		const gl3_texture_object_cubemap& texture_object_cubemap_;
		const cubemap_side side_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_FRAME_BUFFER_HPP) */
