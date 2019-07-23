/**
 *************************************************************************
 *
 * @file gl3_frame_buffer.cpp
 *
 * OpenGL3 frame buffer implementation.
 *
 ************************************************************************/


#include "gl3_frame_buffer.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "gl3_api.hpp"
#include "gl3_texture_object.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_frame_buffer
//
//////////////////////////////////////////////////////////////////////////


gl3_frame_buffer::gl3_frame_buffer
	(const gl3_api& gl3, const vector2i& size,
	 const attachment_map& attachments)
	:
	gl3_(gl3),
	size_(size),
	gl_name_(0),
	attachments_(attachments)
{
	// Validate parameters of frame buffer and attachments.
	if (size_.min() <= 0 || size_.max() > gl3_api::max_frame_buffer_size)
	{
		LOG_ERROR("Invalid frame buffer size.");
		throw invalid_parameter();
	}

	for (attachment_map::const_iterator it(attachments_.first()); it; ++it)
	{
		if (!it->value)
		{
			LOG_ERROR("Invalid NULL pointer in attachment map.");
			throw null_pointer();
		}

		if (it->key < 0 || it->key >= attachment_point_count)
		{
			LOG_ERROR("Invalid attachment point specified.");
			throw invalid_parameter();
		}

		if (it->value->attach_size() != size_)
		{
			LOG_ERROR("Sizes of frame buffer and attached object do not match.");
			throw invalid_parameter();
		}
	}


	// Create frame buffer.
	gl3_.glGenFramebuffers(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for frame buffer.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(gl_name_guard, (gl3_.glDeleteFramebuffers, 1, &gl_name_));


	// Attach frame buffer objects.
	gl3_.glBindFramebuffer(GL_FRAMEBUFFER, gl_name_);

	for (int64 i = 0; i < attachment_point_count; ++i)
		gl3_frame_buffer_attachment::unbind
			(static_cast<attachment_point>(i), gl3);

	for (attachment_map::const_iterator it
			(attachments_.first()); it; ++it)
				it->value->bind(it->key, gl3);


	// Check for success
	GLenum status =
		gl3_.glCheckFramebufferStatus(GL_FRAMEBUFFER);

	if (status != GL_FRAMEBUFFER_COMPLETE && status != 0)
		LOG_WARN("Framebuffer is not complete, OpenGL error code: " + status);

	gl_name_guard.dismiss();
}


gl3_frame_buffer::~gl3_frame_buffer() NOTHROW
{
	// gl_name_ is never 0 
	gl3_.glDeleteFramebuffers(1, &gl_name_);
}


const vector2i& gl3_frame_buffer::size() const NOTHROW
	{ return size_; }


void gl3_frame_buffer::bind(binding_target target) const
{
	if (target == binding_draw)
		gl3_.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_name_);
	else if (target == binding_read)
		gl3_.glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_name_);

	else if (target == binding_draw_read)
	{
		gl3_.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, gl_name_);
		gl3_.glBindFramebuffer(GL_READ_FRAMEBUFFER, gl_name_);
	}

	else
	{
		LOG_ERROR("Invalid target provided for binding frame buffer.");
		throw invalid_parameter();
	}
}


void gl3_frame_buffer::unbind
	(binding_target target, const gl3_api& gl3) NOTHROW
{
	if (target == binding_draw)
		gl3.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	else if (target == binding_read)
		gl3.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

	else if (target == binding_draw_read)
	{
		gl3.glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
		gl3.glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
	}

	else
	{
		LOG_ERROR("Invalid target provided for unbinding frame buffer.");
		return; // NOTHROW on purpose.
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_frame_buffer_attachment
//
//////////////////////////////////////////////////////////////////////////


gl3_frame_buffer_attachment::~gl3_frame_buffer_attachment() NOTHROW { }


void gl3_frame_buffer_attachment::unbind
	(gl3_frame_buffer::attachment_point point,
	 const gl3_api& gl3) NOTHROW
{
	if (point < 0 || point >= gl3_frame_buffer::attachment_point_count)
	{
		LOG_ERROR("Invalid attachment point for unbinding.");
		return; // NOTHROW on purpose.
	}

	gl3.glFramebufferTexture2D
		(GL_FRAMEBUFFER, gl_attachments_[point],
		 GL_TEXTURE_2D, 0, 0);
}


const GLenum gl3_frame_buffer_attachment::gl_attachments_
	[gl3_frame_buffer::attachment_point_count] =
		{
			GL_COLOR_ATTACHMENT0,
			GL_COLOR_ATTACHMENT1,
			GL_COLOR_ATTACHMENT2,
			GL_COLOR_ATTACHMENT3,

			GL_DEPTH_ATTACHMENT,
			GL_STENCIL_ATTACHMENT
		};




//////////////////////////////////////////////////////////////////////////
//
// gl3_frame_buffer_attachment_2d
//
//////////////////////////////////////////////////////////////////////////


gl3_frame_buffer_attachment_2d::gl3_frame_buffer_attachment_2d
	(const gl3_texture_object_2d& texture_object_2d)
	:
	texture_object_2d_(texture_object_2d)
{ }


auto_pointer<gl3_frame_buffer_attachment>
	gl3_frame_buffer_attachment_2d::clone() const
{
	return auto_pointer<gl3_frame_buffer_attachment>
				(new gl3_frame_buffer_attachment_2d(*this));
}


vector2i gl3_frame_buffer_attachment_2d::attach_size() const NOTHROW
	{ return texture_object_2d_.size; }


void gl3_frame_buffer_attachment_2d::bind
	(gl3_frame_buffer::attachment_point point,
	 const gl3_api& gl3) const
{
	if (point < 0 || point >= gl3_frame_buffer::attachment_point_count)
	{
		LOG_ERROR("Invalid attachment point for binding.");
		throw invalid_parameter();
	}

	gl3.glFramebufferTexture2D
		(GL_FRAMEBUFFER, gl_attachments_[point],
		 GL_TEXTURE_2D, texture_object_2d_.gl_name_, 0);
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_frame_buffer_attachment_2d_array
//
//////////////////////////////////////////////////////////////////////////


gl3_frame_buffer_attachment_2d_array::gl3_frame_buffer_attachment_2d_array
	(const gl3_texture_object_2d_array& texture_object_2d_array,
	 int64 layer)
	:
	texture_object_2d_array_(texture_object_2d_array),
	layer_(layer)
{
	if (layer_ < 0 || layer_ >= texture_object_2d_array.layer_count)
	{
		LOG_ERROR("Binding layer: " + layer_ +
				  " exceeds layer count of 2D texture array.");
		throw invalid_parameter();
	}
}


auto_pointer<gl3_frame_buffer_attachment>
	gl3_frame_buffer_attachment_2d_array::clone() const 
{
	return auto_pointer<gl3_frame_buffer_attachment>
				(new gl3_frame_buffer_attachment_2d_array(*this));
}


vector2i gl3_frame_buffer_attachment_2d_array::attach_size() const NOTHROW
	{ return texture_object_2d_array_.size; }


void gl3_frame_buffer_attachment_2d_array::bind
	(gl3_frame_buffer::attachment_point point, const gl3_api& gl3) const
{
	if (point < 0 || point >= gl3_frame_buffer::attachment_point_count)
	{
		LOG_ERROR("Invalid attachment point for binding.");
		throw invalid_parameter();
	}

	// This should not throw, see gl3_api::max_texture_2d_layer_count,
	// which is enforced through the bound texture_object_2d_array_.
	GLint layer =
		integer_operations::cast_or_throw<GLint>(layer_);

	gl3.glFramebufferTextureLayer
		(GL_FRAMEBUFFER, gl_attachments_[point],
		 texture_object_2d_array_.gl_name_, 0, layer);
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_frame_buffer_attachment_cubemap
//
//////////////////////////////////////////////////////////////////////////


gl3_frame_buffer_attachment_cubemap::gl3_frame_buffer_attachment_cubemap
	(const gl3_texture_object_cubemap& texture_object_cubemap,
	 cubemap_side side)
	:
	texture_object_cubemap_(texture_object_cubemap),
	side_(side)
{
	if (side_ < 0 || side_ >= cubemap_side_count)
	{
		LOG_ERROR("Invalid cube map side for binding.");
		throw invalid_parameter();
	}
}


auto_pointer<gl3_frame_buffer_attachment>
	gl3_frame_buffer_attachment_cubemap::clone() const
{
	return auto_pointer<gl3_frame_buffer_attachment>
				(new gl3_frame_buffer_attachment_cubemap(*this));
}


vector2i gl3_frame_buffer_attachment_cubemap::attach_size() const NOTHROW
{
	return vector2i(texture_object_cubemap_.size,
					texture_object_cubemap_.size);
}


void gl3_frame_buffer_attachment_cubemap::bind
	(gl3_frame_buffer::attachment_point point,
	 const gl3_api& gl3) const
{
	if (point < 0 || point >= gl3_frame_buffer::attachment_point_count)
	{
		LOG_ERROR("Invalid attachment point for binding.");
		throw invalid_parameter();
	}

	gl3.glFramebufferTexture2D
		(GL_FRAMEBUFFER, gl_attachments_[point],
		 gl_cubemap_side_target_[side_],
		 texture_object_cubemap_.gl_name_, 0);
}


const GLuint gl3_frame_buffer_attachment_cubemap::gl_cubemap_side_target_
	[cubemap_side_count] =
		{
			GL_TEXTURE_CUBE_MAP_POSITIVE_X,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
			GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_X,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
			GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
		};




} /* namespace viral_core */

