/**
 *************************************************************************
 *
 * @file gl3_render_target.hpp
 *
 * Rendering target for OpenGL3 rendering.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_RENDER_TARGET_HPP)
#define INCLUDED__VIRAL_CORE__GL3_RENDER_TARGET_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "geo_2d.hpp"
#include "render_resource.hpp"
#include "gl3_frame_buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_render_target
 * @ingroup gl3_render_module
 *
 * Groups gl3_frame_buffer%s and gl3_frame_buffer_object%s
 * into an offscreen render target for use by gl3_canvas.
 *
 ************************************************************************/
class gl3_render_target :
	private non_copyable
{

	public:

		~gl3_render_target() NOTHROW;


		/**
		 * Create new render @bref{target} if @NULL,
		 * or recreate target on changed parameters.
		 * Clamps @bref{size} to reasonable ranges.
		 */
		static void update
			(const gl3_api& gl3,
			 auto_pointer<gl3_render_target>& target,
			 const vector2i& size,
			 render_texture_data::wrap_mode wrap,
			 render_texture_data::filter_mode filter,
			 render_texture_data::color_space_mode color_space);


		const vector2i& size() const NOTHROW;

		const gl3_texture_object_2d& color_texture() const NOTHROW;
		const gl3_texture_object_2d& depth_stencil_texture() const NOTHROW;


		/**
		 * Apply @bref{viewport} transforms and enable scissor testing.
		 * Affects most operations, including clearing and rendering.
		 */
		void set_viewport(const rect2f& viewport);


		/**
		 * Clear depth or color buffers of this render target.
		 * Clearing automatically binds required framebuffers,
		 * but restricts to any previously set_viewport(). 
		 */
		//@{
		void clear_depth();
		void clear_colors(const color& clear_color);
		//@}


		/**
		 * Use @bref{color_target_} of this render target 
		 * for successive drawing or reading operations.
		 * Implicitly enables automated sRGB conversions.
		 */
		//@{
		void bind_color_draw() const;
		void bind_color_read() const;
		//@}


		/**
		 * Prepare render target for use as a texture.
		 * In particular, create mipmap levels.
		 */
		void prepare_texture();


		/**
		 * Copy all framebuffer contents to @bref{dest_pixels}
		 * in a render_target @bref{dest} or the window buffer.
		 * @attent Discards any previously set_viewport().
		 */
		//@{
		void copy_to_render_target
			(gl3_render_target& dest,
			 const rect2i& dest_pixels,
			 bool copy_depth) const;
		void copy_to_window
			(const rect2i& dest_pixels,
			 const vector2i& unreliable_window_size,
			 bool copy_depth) const;
		//@}


		static const int64 min_size = 32;
		static const int64 max_size = 4096;
		static const int64 block_size = 128;


	private:

		gl3_render_target
			(const gl3_api& gl3,
			 const vector2i& size,
			 render_texture_data::wrap_mode wrap,
			 render_texture_data::filter_mode filter,
			 render_texture_data::color_space_mode color_space);

		static int64 bound_size(int64 size);


		const gl3_api& gl3_;

		const vector2i size_;
		const render_texture_data::wrap_mode wrap_;
		const render_texture_data::filter_mode filter_;
		const render_texture_data::color_space_mode color_space_;

		auto_pointer<gl3_texture_object_2d> color_texture_;
		auto_pointer<gl3_texture_object_2d> depth_stencil_texture_;
		auto_pointer<gl3_frame_buffer> color_target_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_RENDER_TARGET_HPP) */
