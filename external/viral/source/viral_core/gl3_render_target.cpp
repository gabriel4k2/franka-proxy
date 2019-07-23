/**
 *************************************************************************
 *
 * @file gl3_render_target.cpp
 *
 * Rendering target for OpenGL3 rendering, implementation.
 *
 ************************************************************************/


#include "gl3_render_target.hpp"


#include "log.hpp"
#include "integer_operations.hpp"
#include "tree_map.hpp"
#include "color.hpp"
#include "gl3_api.hpp"
#include "gl3_texture_object.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_render_target
//
//////////////////////////////////////////////////////////////////////////


gl3_render_target::~gl3_render_target() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_render_target::update
	(const gl3_api& gl3,
	 auto_pointer<gl3_render_target>& target,
	 const vector2i& size,
	 render_texture_data::wrap_mode wrap,
	 render_texture_data::filter_mode filter,
	 render_texture_data::color_space_mode color_space)
{
	vector2i bounded_size
		(bound_size(size.x), bound_size(size.y));

	if (target &&
		bounded_size == target->size_ &&
		filter == target->filter_ &&
		color_space == target->color_space_)
			return;

	target.reset
		(new gl3_render_target
			(gl3, bounded_size,
			 wrap, filter, color_space));
}


const vector2i& gl3_render_target::size() const NOTHROW
	{ return size_; }


const gl3_texture_object_2d& gl3_render_target::color_texture() const NOTHROW
	{ return *color_texture_; }
const gl3_texture_object_2d& gl3_render_target::depth_stencil_texture() const NOTHROW
	{ return *depth_stencil_texture_; }


void gl3_render_target::set_viewport
	(const rect2f& viewport)
{
	// Transform viewport to pixel coordinates.
	// These should only throw in extreme cases.
	// We need not clamp to the render target,
	// GL does this automatically.
	GLint pos_x =
		integer_operations::cast_or_throw<GLint>
			(size_.x * viewport.pos.x);
	GLint pos_y =
		integer_operations::cast_or_throw<GLint>
			(size_.y * viewport.pos.y);
	GLsizei size_x =
		integer_operations::cast_or_throw<GLsizei>
			(size_.x * viewport.size.x);
	GLsizei size_y =
		integer_operations::cast_or_throw<GLsizei>
			(size_.y * viewport.size.y);
		

	// Restrict rendering to viewport.
	// This requires explicit scissoring.
	glViewport(pos_x, pos_y, size_x, size_y);

	glEnable(GL_SCISSOR_TEST);
	glScissor(pos_x, pos_y, size_x, size_y);
}


void gl3_render_target::clear_depth()
{
	bind_color_draw();
	glDepthMask(GL_TRUE);
	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);
}


void gl3_render_target::clear_colors(const color& clear_color)
{
	bind_color_draw();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	glClear(GL_COLOR_BUFFER_BIT);
}


void gl3_render_target::bind_color_draw() const
{
	color_target_->bind(gl3_frame_buffer::binding_draw);
	glEnable(GL_FRAMEBUFFER_SRGB);
}

void gl3_render_target::bind_color_read() const
	{ color_target_->bind(gl3_frame_buffer::binding_read); }


void gl3_render_target::prepare_texture()
	{ color_texture_->build_mipmaps(); }


void gl3_render_target::copy_to_render_target
	(gl3_render_target& dest,
	 const rect2i& dest_pixels,
	 bool copy_depth) const
{
	if (&dest == this)
	{
		LOG_ERROR("Cannot copy render target to itself.");
		throw invalid_operation();
	}


	// Convert to GL integers.
	// These should only throw in extreme cases.
	GLint src_size_x =
		integer_operations::cast_or_throw<GLint>(size_.x);
	GLint src_size_y =
		integer_operations::cast_or_throw<GLint>(size_.y);

	GLint dest_pixels_pos_x =
		integer_operations::cast_or_throw<GLint>(dest_pixels.pos.x);
	GLint dest_pixels_pos_y =
		integer_operations::cast_or_throw<GLint>(dest_pixels.pos.y);
	GLint dest_pixels_end_x =
		integer_operations::cast_or_throw<GLint>(dest_pixels.end().x);
	GLint dest_pixels_end_y =
		integer_operations::cast_or_throw<GLint>(dest_pixels.end().y);

	GLsizei dest_size_x =
		integer_operations::cast_or_throw<GLsizei>(dest_pixels.size.x);
	GLsizei dest_size_y =
		integer_operations::cast_or_throw<GLsizei>(dest_pixels.size.y);


	// Perform actual blitting.
	bind_color_read();
	dest.bind_color_draw();	

	glViewport(0, 0, dest_size_x, dest_size_y);
	glDisable(GL_SCISSOR_TEST);

	gl3_.glBlitFramebuffer
		(0, 0, src_size_x, src_size_y,
		 dest_pixels_pos_x, dest_pixels_pos_y,
		 dest_pixels_end_x, dest_pixels_end_y,
		 GL_COLOR_BUFFER_BIT, GL_LINEAR);

	if (copy_depth)
	{
		// Cannot copy GL_DEPTH_BUFFER_BIT with
		// GL_LINEAR interpolation.
		gl3_.glBlitFramebuffer
			(0, 0, src_size_x, src_size_y,
			 dest_pixels_pos_x, dest_pixels_pos_y,
			 dest_pixels_end_x, dest_pixels_end_y,
			 GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}


void gl3_render_target::copy_to_window
	(const rect2i& dest_pixels,
	 const vector2i& unreliable_window_size,
	 bool copy_depth) const
{
	/**
	 * Implementation notes:
	 * - We must flip our top-down framebuffer content to bottom-up
	 *		conventions within the OpenGL window for correct orienting.
	 *		See gl3_render_context for details.
	 * - To correctly position source contents in our window,
	 *		we must use the @bref{unreliable_window_size} cached
	 *		by gl3_render_context::render(). We still might miss
	 *		a concurrent window_context::update(), in which case
	 *		we displace our content on-screen for a short time.
	 * - For the window framebuffer, we always assume sRGB gamma.
	 *		We do not explicitly request an sRGB window buffer,
	 *		because this would complicate GL context creation.
	 */


	// Destination pixels in the window.
	rect2i flipped_dest
		(dest_pixels.pos.x,
		 unreliable_window_size.y - dest_pixels.pos.y,
		 dest_pixels.size.x, -dest_pixels.size.y);


	// Convert to GL integers.
	// These should only throw in extreme cases.
	GLsizei unreliable_window_size_x =
		integer_operations::cast_or_throw<GLsizei>(unreliable_window_size.x);
	GLsizei unreliable_window_size_y =
		integer_operations::cast_or_throw<GLsizei>(unreliable_window_size.y);

	GLint src_size_x =
		integer_operations::cast_or_throw<GLint>(size_.x);
	GLint src_size_y =
		integer_operations::cast_or_throw<GLint>(size_.y);

	GLint flipped_dest_pos_x =
		integer_operations::cast_or_throw<GLint>(flipped_dest.pos.x);
	GLint flipped_dest_pos_y =
		integer_operations::cast_or_throw<GLint>(flipped_dest.pos.y);
	GLint flipped_dest_end_x =
		integer_operations::cast_or_throw<GLint>(flipped_dest.end().x);
	GLint flipped_dest_end_y =
		integer_operations::cast_or_throw<GLint>(flipped_dest.end().y);


	// Perform actual blitting.
	bind_color_read();

	gl3_frame_buffer::unbind(gl3_frame_buffer::binding_draw, gl3_);
	glEnable(GL_FRAMEBUFFER_SRGB);

	glViewport
		(0, 0, unreliable_window_size_x, unreliable_window_size_y);
	glDisable(GL_SCISSOR_TEST);

	gl3_.glBlitFramebuffer
		(0, 0, src_size_x, src_size_y,
		 flipped_dest_pos_x, flipped_dest_pos_y,
		 flipped_dest_end_x, flipped_dest_end_y,
		 GL_COLOR_BUFFER_BIT, GL_LINEAR);

	if (copy_depth)
	{
		// Cannot copy GL_DEPTH_BUFFER_BIT with
		// GL_LINEAR interpolation.
		gl3_.glBlitFramebuffer
			(0, 0, src_size_x, src_size_y,
			 flipped_dest_pos_x, flipped_dest_pos_y,
			 flipped_dest_end_x, flipped_dest_end_y,
			 GL_DEPTH_BUFFER_BIT, GL_NEAREST);
	}
}


gl3_render_target::gl3_render_target
	(const gl3_api& gl3,
	 const vector2i& size,
	 render_texture_data::wrap_mode wrap,
	 render_texture_data::filter_mode filter,
	 render_texture_data::color_space_mode color_space)
	:
	gl3_(gl3),
	size_(size),
	wrap_(wrap),
	filter_(filter),
	color_space_(color_space)
{
	if (size_.min() < min_size || size_.max() > max_size)
	{
		LOG_ERROR("Invalid size for render target.");
		throw invalid_parameter();
	}

	if (color_space_ != render_texture_data::color_space_linear &&
		color_space_ != render_texture_data::color_space_srgb)
	{
		LOG_ERROR("Invalid color space for render target.");
		throw invalid_parameter();
	}

	color_texture_.reset
		(new gl3_texture_object_2d
			(gl3_, size_,
			 (color_space_ == render_texture_data::color_space_linear) ?
				gl3_texture_object::texture_format_rgba8 :
				gl3_texture_object::texture_format_srgba8,
			 wrap_, filter_));
	depth_stencil_texture_.reset
		(new gl3_texture_object_2d
			(gl3_, size_,
			 gl3_texture_object::texture_format_packed_depth_stencil,
			 wrap_, render_texture_data::filter_nearest));

	gl3_frame_buffer::attachment_map attachements;

	auto_pointer<gl3_frame_buffer_attachment> color_0_attachment
		(new gl3_frame_buffer_attachment_2d(*color_texture_));
	auto_pointer<gl3_frame_buffer_attachment> depth_attachment
		(new gl3_frame_buffer_attachment_2d(*depth_stencil_texture_));
	auto_pointer<gl3_frame_buffer_attachment> stencil_attachment
		(new gl3_frame_buffer_attachment_2d(*depth_stencil_texture_));

	attachements.insert
		(gl3_frame_buffer::attachment_color0,
		 std::move(color_0_attachment));
	attachements.insert
		(gl3_frame_buffer::attachment_depth,
		 std::move(depth_attachment));
	attachements.insert
		(gl3_frame_buffer::attachment_stencil,
		 std::move(stencil_attachment));

	color_target_.reset
		(new gl3_frame_buffer(gl3_, size_, attachements));
}


int64 gl3_render_target::bound_size(int64 size)
	{ return max(min(size, max_size), min_size); }




} /* namespace viral_core */
