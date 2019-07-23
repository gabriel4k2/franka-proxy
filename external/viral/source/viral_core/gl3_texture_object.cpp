/**
 *************************************************************************
 *
 * @file gl3_texture_object.cpp
 *
 * OpenGL3 texture object identifiers, implementation.
 *
 ************************************************************************/


#include "gl3_texture_object.hpp"


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "image_raw.hpp"
#include "gl3_api.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_texture_object
//
//////////////////////////////////////////////////////////////////////////


void gl3_texture_object::unbind
	(GLenum texture_unit_index, const gl3_api& gl3) NOTHROW
{
	if (texture_unit_index < 0 ||
		texture_unit_index >= gl3.texture_image_units)
	{
		LOG_ERROR("Invalid texture unit provided for unbinding texture.");
		return; // NOTHROW on purpose
	}

	gl3.glActiveTexture
		(GL_TEXTURE0 + texture_unit_index);

	// We don't know which texture type currently
	// is bound on the slot, so just deactivate all.
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}


gl3_texture_object::gl3_texture_object
	(const gl3_api& gl3)
	:	
	gl3_(gl3)
{ }


gl3_texture_object::texture_format
	gl3_texture_object::to_texture_format
		(render_texture_data::color_space_mode color_space,
		 render_texture_data::compression_mode compression)
{
	if (color_space == render_texture_data::color_space_srgb)
	{
		if (compression == render_texture_data::compression_lossy)
			return texture_format_srgba8_compressed;
		else if (compression == render_texture_data::compression_none)
			return texture_format_srgba8;
	}
	else if (color_space == render_texture_data::color_space_linear)
	{
		if (compression == render_texture_data::compression_lossy)
			return texture_format_rgba8_compressed;
		else if (compression == render_texture_data::compression_none)
			return texture_format_rgba8;
	}

	LOG_ERROR("Unsupported color space or compression mode provided.");
	throw unsupported_feature();
}


void gl3_texture_object::apply_wrap
	(GLenum target, render_texture_data::wrap_mode wrap)
{
	if (wrap == render_texture_data::wrap_none)
	{
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else if (wrap == render_texture_data::wrap_both)
	{
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else if (wrap == render_texture_data::wrap_horizontal)
	{
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else if (wrap == render_texture_data::wrap_vertical)
	{
		glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_REPEAT);
	}
	else
	{
		LOG_ERROR("Unsupported wrap mode provided.");
		throw unsupported_feature();
	}
}


void gl3_texture_object::apply_filter_and_mipmaps
	(const gl3_api& gl3, GLenum target,
	 render_texture_data::filter_mode filter,
	 bool create_mipmaps)
{
	if (filter == render_texture_data::filter_trilinear)
	{
		glTexParameteri
			(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri
			(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		if (create_mipmaps)
			gl3.glGenerateMipmap(target);
	}
	else if (filter == render_texture_data::filter_linear)
	{
		glTexParameteri
			(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri
			(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	}
	else if (filter == render_texture_data::filter_nearest)
	{
		glTexParameteri
			(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri
			(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);		
	}
	else
	{
		LOG_ERROR("Unsupported filtering mode provided.");
		throw unsupported_feature();
	}
}


const GLint gl3_texture_object::gl_internal_formats_
	[gl3_texture_object::texture_format_count] =
		{
			GL_SRGB8_ALPHA8,
			GL_RGBA8,
			GL_COMPRESSED_SRGB_ALPHA,
			GL_COMPRESSED_RGBA,
			GL_RGBA32F,
			GL_DEPTH_COMPONENT16,
			GL_DEPTH_COMPONENT24,
			GL_DEPTH_COMPONENT32,
			GL_DEPTH24_STENCIL8
		};

const GLenum gl3_texture_object::gl_base_formats_
	[gl3_texture_object::texture_format_count] =
		{
			GL_RGBA,
			GL_RGBA,
			GL_RGBA,
			GL_RGBA,
			GL_RGBA,
			GL_DEPTH_COMPONENT,
			GL_DEPTH_COMPONENT,
			GL_DEPTH_COMPONENT,
			GL_DEPTH_STENCIL	
		};

const GLenum gl3_texture_object::gl_format_types_
	[gl3_texture_object::texture_format_count] =
		{
			GL_UNSIGNED_BYTE,
			GL_UNSIGNED_BYTE,
			GL_UNSIGNED_BYTE,
			GL_UNSIGNED_BYTE,
			GL_FLOAT,
			GL_FLOAT,
			GL_FLOAT,
			GL_FLOAT,
			GL_UNSIGNED_INT_24_8
		};




//////////////////////////////////////////////////////////////////////////
//
// gl3_texture_object_2d
//
//////////////////////////////////////////////////////////////////////////


gl3_texture_object_2d::gl3_texture_object_2d
	(const gl3_api& gl3,
	 const image_rgba& texture,
	 render_texture_data::wrap_mode wrap,
	 render_texture_data::filter_mode filter,
	 render_texture_data::color_space_mode color_space,
	 render_texture_data::compression_mode compression)
	:
	gl3_texture_object(gl3),
	size(texture.size()),
	format(to_texture_format(color_space, compression)),
	wrap(wrap),
	filter(filter),
	gl_name_(0)
{
	// Check parameters.
	if (size.min() <= 0 || size.max() > gl3_api::max_texture_2d_size)
	{
		LOG_ERROR("Invalid texture size.");
		throw invalid_parameter();
	}

	if (format < 0 || format >= texture_format_count)
	{
		LOG_ERROR("Invalid texture format.");
		throw invalid_parameter();
	}

	GLsizei gl_size_x =
		integer_operations::cast_or_throw<GLsizei>(texture.size().x);
	GLsizei gl_size_y =
		integer_operations::cast_or_throw<GLsizei>(texture.size().y);


	// Generate OpenGL texture name.
	glGenTextures(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for texture.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(guard_gl_name, (&glDeleteTextures, 1, &gl_name_));


	// Apply texture data to OpenGL.
	// We store OpenGL texture data top-down by providing
	// a pointer to the top instead of the bottom row.
	// See gl3_render_context for rationale.
	gl3_.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl_name_);
	glTexImage2D(GL_TEXTURE_2D, 0,
				 gl_internal_formats_[format],
				 gl_size_x, gl_size_y, 0,
				 gl_base_formats_[format],
				 gl_format_types_[format],
				 texture.data());


	// Apply texture parameters.
	apply_wrap
		(GL_TEXTURE_2D, wrap);
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_2D, filter, true);


	guard_gl_name.dismiss();
}


gl3_texture_object_2d::gl3_texture_object_2d
	(const gl3_api& gl3,
	 const vector2i& size,
	 texture_format format,
	 render_texture_data::wrap_mode wrap,
	 render_texture_data::filter_mode filter)
	:
	gl3_texture_object(gl3),
	size(size),
	format(format),
	wrap(wrap),
	filter(filter),
	gl_name_(0)
{
	// Check parameters.
	if (size.min() <= 0 || size.max() > gl3_api::max_texture_2d_size)
	{
		LOG_ERROR("Invalid texture size.");
		throw invalid_parameter();
	}

	if (format < 0 || format >= texture_format_count)
	{
		LOG_ERROR("Invalid texture format.");
		throw invalid_parameter();
	}

	GLsizei gl_size_x =
		integer_operations::cast_or_throw<GLsizei>(size.x);
	GLsizei gl_size_y =
		integer_operations::cast_or_throw<GLsizei>(size.y);


	// Generate OpenGL texture name.
	glGenTextures(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for texture.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(guard_gl_name, (&glDeleteTextures, 1, &gl_name_));


	// Set GL format and reserve storage space.
	gl3_.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, gl_name_);
	glTexImage2D(GL_TEXTURE_2D, 0,
				 gl_internal_formats_[format],
				 gl_size_x, gl_size_y, 0,
				 gl_base_formats_[format],
				 gl_format_types_[format],
				 NULL);


	// Apply texture parameters.
	apply_wrap
		(GL_TEXTURE_2D, wrap);
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_2D, filter, false);


	guard_gl_name.dismiss();
}


gl3_texture_object_2d::~gl3_texture_object_2d() NOTHROW
{
	glDeleteTextures(1, &gl_name_); // never 0
}


void gl3_texture_object_2d::build_mipmaps() const
{
	bind(0);
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_2D, filter, true);
}


void gl3_texture_object_2d::bind
	(GLenum texture_unit_index) const
{
	if (texture_unit_index < 0 ||
		texture_unit_index >= gl3_.texture_image_units)
	{
		LOG_ERROR("Invalid texture unit provided for binding texture.");
		throw invalid_parameter();
	}

	gl3_.glActiveTexture
		(GL_TEXTURE0 + texture_unit_index);

	glBindTexture(GL_TEXTURE_2D, gl_name_);

	// Potentially, a texture of some other type
	// could be bound, so deactivate all other types
	// to achieve a consistent binding state.
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_texture_object_2d_array
//
//////////////////////////////////////////////////////////////////////////


gl3_texture_object_2d_array::gl3_texture_object_2d_array
	(const gl3_api& gl3,
	 const vector2i& size,
	 int64 layer_count,
	 texture_format format,
	 render_texture_data::wrap_mode wrap,
	 render_texture_data::filter_mode filter)
	:
	gl3_texture_object(gl3),
	size(size),
	layer_count(layer_count),
	format(format),
	wrap(wrap),
	filter(filter),
	gl_name_(0)
{
	// Check parameters.
	if (size.min() <= 0 || size.max() > gl3_api::max_texture_2d_size)
	{
		LOG_ERROR("Texture size invalid or too large.");
		throw invalid_parameter();
	}

	if (layer_count <= 0 || layer_count > gl3_api::max_texture_2d_layer_count)
	{
		LOG_ERROR("Texture layer count invalid or too large.");
		throw invalid_parameter();
	}

	GLsizei gl_size_x =
		integer_operations::cast_or_throw<GLsizei>(size.x);
	GLsizei gl_size_y =
		integer_operations::cast_or_throw<GLsizei>(size.y);
	GLsizei gl_depth =
		integer_operations::cast_or_throw<GLsizei>(layer_count);


	// Generate OpenGL texture name.
	glGenTextures(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for texture.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(guard_gl_name, (&glDeleteTextures, 1, &gl_name_));


	// Apply format and request texture storage.
	gl3_.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, gl_name_);
	gl3_.glTexImage3D(GL_TEXTURE_2D_ARRAY, 0,
					  gl_internal_formats_[format],
					  gl_size_x, gl_size_y, gl_depth, 0,
					  gl_base_formats_[format],
					  gl_format_types_[format],
					  NULL);


	// Apply texture parameters.
	apply_wrap
		(GL_TEXTURE_2D_ARRAY, wrap);
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_2D_ARRAY, filter, false);


	guard_gl_name.dismiss();
}


gl3_texture_object_2d_array::~gl3_texture_object_2d_array() NOTHROW
{
	glDeleteTextures(1, &gl_name_); // never 0
}


void gl3_texture_object_2d_array::build_mipmaps() const
{
	bind(0);
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_2D_ARRAY, filter, true);
}


void gl3_texture_object_2d_array::bind
	(GLenum texture_unit_index) const
{
	if (texture_unit_index < 0 ||
		texture_unit_index >= gl3_.texture_image_units)
	{
		LOG_ERROR("Invalid texture unit provided for binding texture.");
		throw invalid_parameter();
	}

	gl3_.glActiveTexture
		(GL_TEXTURE0 + texture_unit_index);

	glBindTexture(GL_TEXTURE_2D_ARRAY, gl_name_);

	// Potentially, a texture of some other type
	// could be bound, so deactivate all other types
	// to achieve a consistent binding state.
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_texture_object_cubemap
//
//////////////////////////////////////////////////////////////////////////


gl3_texture_object_cubemap::gl3_texture_object_cubemap
	(const gl3_api& gl3,
	 const image_rgba& x_pos, const image_rgba& y_pos, const image_rgba& z_pos,
	 const image_rgba& x_neg, const image_rgba& y_neg, const image_rgba& z_neg,
	 render_texture_data::filter_mode filter,
	 render_texture_data::color_space_mode color_space,
	 render_texture_data::compression_mode compression)
	:
	gl3_texture_object(gl3),
	size(x_pos.size().x),
	format(to_texture_format(color_space, compression)),
	filter(filter),
	gl_name_(0)
{
	// Validate size of input images.
	const image_rgba* images[6] =
		{ &x_pos, &y_pos, &z_pos, &x_neg, &y_neg, &z_neg };

	for (int64 i = 0; i < 6; ++i)
	{
		if (images[i]->size().min() <= 0 ||
			images[i]->size().max() > gl3_api::max_texture_cube_map_size)
		{
			LOG_ERROR("Invalid size for cube map.");
			throw invalid_parameter();
		}

		if (images[i]->size().x != images[i]->size().y)
		{
			LOG_ERROR("Cannot create cube map from non-square images.");
			throw invalid_parameter();
		}

		if (images[i]->size() != images[0]->size())
		{
			LOG_ERROR("Cannot create cube map from inconsistent image sizes.");
			throw invalid_parameter();
		}
	}


	// Generate OpenGL texture name.
	glGenTextures(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for texture.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(guard_gl_name, (&glDeleteTextures, 1, &gl_name_));


	// Apply texture data to OpenGL.
	// We store OpenGL texture data top-down by providing
	// a pointer to the top instead of the bottom row.
	// See gl3_render_context for rationale.
	gl3_.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, gl_name_);

	const GLenum targets[6] =
		{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		  GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		  GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

	for (int64 i = 0; i < 6; ++i)
	{
		GLsizei gl_size_x =
			integer_operations::cast_or_throw<GLsizei>(images[i]->size().x);
		GLsizei gl_size_y =
			integer_operations::cast_or_throw<GLsizei>(images[i]->size().y);

		glTexImage2D(targets[i], 0,
					 gl_internal_formats_[format],
					 gl_size_x, gl_size_y, 0,
					 gl_base_formats_[format],
					 gl_format_types_[format],
					 images[i]->data());
	}


	// Apply texture parameters.
	// No wrapping for cube maps.
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_CUBE_MAP, filter, true);


	guard_gl_name.dismiss();
}


gl3_texture_object_cubemap::gl3_texture_object_cubemap
	(const gl3_api& gl3,
	 int64 size,
	 texture_format format,
	 render_texture_data::filter_mode filter)
	:
	gl3_texture_object(gl3),
	size(size),
	format(format),
	filter(filter),
	gl_name_(0)
{
	// Check parameters.
	if (size <= 0 || size > gl3_api::max_texture_cube_map_size)
	{
		LOG_ERROR("Cube map texture size invalid or too large.");
		throw invalid_parameter();
	}

	GLsizei gl_size =
		integer_operations::cast_or_throw<GLsizei>(size);


	// Generate OpenGL texture name.
	glGenTextures(1, &gl_name_);

	if (gl_name_ == 0)
	{
		LOG_ERROR("Failed to create OpenGL name for texture.");
		throw api_call_failed();
	}

	SCOPE_GUARD
		(guard_gl_name, (&glDeleteTextures, 1, &gl_name_));


	// Apply format and reserve texture storage.
	gl3_.glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, gl_name_);

	const GLenum targets[6] =
		{ GL_TEXTURE_CUBE_MAP_POSITIVE_X, GL_TEXTURE_CUBE_MAP_POSITIVE_Y,
		  GL_TEXTURE_CUBE_MAP_POSITIVE_Z,
		  GL_TEXTURE_CUBE_MAP_NEGATIVE_X, GL_TEXTURE_CUBE_MAP_NEGATIVE_Y,
		  GL_TEXTURE_CUBE_MAP_NEGATIVE_Z };

	for (int64 i = 0; i < 6; ++i)
		glTexImage2D(targets[i], 0,
					 gl_internal_formats_[format],
					 gl_size, gl_size, 0,
					 gl_base_formats_[format],
					 gl_format_types_[format],
					 NULL);


	// Apply texture parameters.
	// No wrapping for cube maps.
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_CUBE_MAP, filter, false);


	guard_gl_name.dismiss();
}


gl3_texture_object_cubemap::~gl3_texture_object_cubemap() NOTHROW
{
	glDeleteTextures(1, &gl_name_); // never 0
}


void gl3_texture_object_cubemap::build_mipmaps() const
{
	bind(0);
	apply_filter_and_mipmaps
		(gl3_, GL_TEXTURE_CUBE_MAP, filter, true);
}


void gl3_texture_object_cubemap::bind
	(GLenum texture_unit_index) const
{
	if (texture_unit_index < 0 ||
		texture_unit_index >= gl3_.texture_image_units)
	{
		LOG_ERROR("Invalid texture unit provided for binding texture.");
		throw invalid_parameter();
	}

	gl3_.glActiveTexture
		(GL_TEXTURE0 + texture_unit_index);

	glBindTexture(GL_TEXTURE_CUBE_MAP, gl_name_);


	// Enable seamless cubemap sampling.
	// This is reasonable for all cubemaps, as opposed
	// to no wrapping or per-face filtering artifacts.
	// Usually, this should be the per-cubemap state,
	// but legacy hardware only supports global state.
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);


	// Potentially, a texture of some other type
	// could be bound, so deactivate all other types
	// to achieve a consistent binding state.
	glBindTexture(GL_TEXTURE_2D, 0);
	glBindTexture(GL_TEXTURE_2D_ARRAY, 0);
}




} /* namespace viral_core */
