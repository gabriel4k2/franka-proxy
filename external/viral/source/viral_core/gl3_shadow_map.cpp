/**
 *************************************************************************
 *
 * @file gl3_shadow_map.cpp
 *
 * Shadow map targets for OpenGL3 rendering, implementation.
 *
 ************************************************************************/


#include "gl3_shadow_map.hpp"


#include "log.hpp"
#include "integer_operations.hpp"
#include "geo_camera.hpp"
#include "gl3_texture_object.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_shadow_map_point
//
//////////////////////////////////////////////////////////////////////////


gl3_shadow_map_point::gl3_shadow_map_point
	(const gl3_api& gl3,
	 const int64 size)
	:
	gl3_(gl3),
	size_(size),
	z_factor_(0),
	z_offset_(0)
{
	if (size_ < min_size || size_ > max_size)
	{
		LOG_ERROR("Invalid size for shadow map.");
		throw invalid_parameter();
	}

	depth_texture_.reset
		(new gl3_texture_object_cubemap
			(gl3_, size_,
			 gl3_texture_object::texture_format_depth32,
			 render_texture_data::filter_linear));

	for (int64 i = 0;
		 i < gl3_frame_buffer_attachment_cubemap::cubemap_side_count;
		 ++i)
	{
		auto_pointer<gl3_frame_buffer_attachment> depth_attachment
			(new gl3_frame_buffer_attachment_cubemap
				(*depth_texture_,
				 static_cast
					<gl3_frame_buffer_attachment_cubemap::cubemap_side>(i)));

		gl3_frame_buffer::attachment_map attachements;
		attachements.insert
			(gl3_frame_buffer::attachment_depth,
			 std::move(depth_attachment));

		depth_targets_.insert
			(auto_pointer<gl3_frame_buffer>
				(new gl3_frame_buffer
					(gl3_, vector2i(size_, size_), attachements)));
	}
}


gl3_shadow_map_point::~gl3_shadow_map_point() NOTHROW
{
	// Resolve member destructors to defined types here.
}


int64 gl3_shadow_map_point::size() const NOTHROW
	{ return size_; }


void gl3_shadow_map_point::update_z_transform(const camera& point_camera)
{
	float znear = point_camera.near_plane;
	float zfar = point_camera.far_plane;

	float z_factor_after_perspective = -2 * znear * zfar / (zfar - znear);
	float z_offset_after_perspective = (zfar + znear) / (zfar - znear);

	z_factor_ = 0.5f * z_factor_after_perspective;
	z_offset_ = 0.5f * z_offset_after_perspective + 0.5f;
}

float gl3_shadow_map_point::z_factor() const NOTHROW
	{ return z_factor_; }
float gl3_shadow_map_point::z_offset() const NOTHROW
	{ return z_offset_; }


void gl3_shadow_map_point::prepare_and_bind_target
	(gl3_frame_buffer_attachment_cubemap::cubemap_side side) const
{
	if (side < 0 ||
		side >= depth_targets_.size())
	{
		LOG_ERROR("Invalid cubemap side for binding.");
		throw invalid_parameter();
	}

	if (!depth_targets_.at(side))
	{
		LOG_ERROR("Depth targets array contains NULL pointer.");
		throw assert_failed();
	}

	gl3_frame_buffer::unbind
		(gl3_frame_buffer::binding_read, gl3_);
	depth_targets_.at(side)->bind
		(gl3_frame_buffer::binding_draw);

	glViewport
		(0, 0,
		 integer_operations::cast_or_throw<GLsizei>(size_),
		 integer_operations::cast_or_throw<GLsizei>(size_));

	glDisable(GL_SCISSOR_TEST);

	glDepthMask(GL_TRUE);
	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);
}


void gl3_shadow_map_point::bind_texture(GLenum texture_unit_index) const
	{ depth_texture_->bind(texture_unit_index); }




//////////////////////////////////////////////////////////////////////////
//
// gl3_shadow_map_spot
//
//////////////////////////////////////////////////////////////////////////


gl3_shadow_map_spot::gl3_shadow_map_spot
	(const gl3_api& gl3,
	 int64 size)
	:
	gl3_(gl3),
	size_(size)
{
	if (size_ < min_size || size_ > max_size)
	{
		LOG_ERROR("Invalid size for shadow map.");
		throw invalid_parameter();
	}

	depth_texture_.reset
		(new gl3_texture_object_2d
			(gl3_, vector2i(size_, size_),
			 gl3_texture_object::texture_format_depth32,
			 render_texture_data::wrap_none,
			 render_texture_data::filter_linear));

	auto_pointer<gl3_frame_buffer_attachment> depth_attachment
		(new gl3_frame_buffer_attachment_2d(*depth_texture_));

	gl3_frame_buffer::attachment_map attachements;
	attachements.insert
		(gl3_frame_buffer::attachment_depth,
		 std::move(depth_attachment));

	depth_target_.reset
		(new gl3_frame_buffer
			(gl3_, vector2i(size_, size_), attachements));
}


gl3_shadow_map_spot::~gl3_shadow_map_spot() NOTHROW
{
	// Resolve member destructors to defined types here.
}


int64 gl3_shadow_map_spot::size() const NOTHROW
	{ return size_; }


void gl3_shadow_map_spot::update_world_to_texture(const camera& spot_camera)
{
	world_to_texture_ =
		matrix::scale(0.5f) *
		matrix::translate(vector(1, 1, 1)) *
		spot_camera.build_projection() *
		spot_camera.build_world_to_camera();
}

const matrix& gl3_shadow_map_spot::world_to_texture() const NOTHROW
	{ return world_to_texture_; }


void gl3_shadow_map_spot::prepare_and_bind_target() const
{
	gl3_frame_buffer::unbind
		(gl3_frame_buffer::binding_read, gl3_);
	depth_target_->bind
		(gl3_frame_buffer::binding_draw);

	glViewport
		(0, 0,
		 integer_operations::cast_or_throw<GLsizei>(size_),
		 integer_operations::cast_or_throw<GLsizei>(size_));

	glDisable(GL_SCISSOR_TEST);

	glDepthMask(GL_TRUE);
	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);
}


void gl3_shadow_map_spot::bind_texture(GLenum texture_unit_index) const
	{ depth_texture_->bind(texture_unit_index); }




//////////////////////////////////////////////////////////////////////////
//
// gl3_shadow_map_parallel
//
//////////////////////////////////////////////////////////////////////////


gl3_shadow_map_parallel::gl3_shadow_map_parallel
	(const gl3_api& gl3,
	 int64 size,
	 int64 cascade_count)
	:
	gl3_(gl3),
	size_(size),
	cascade_count_(cascade_count)
{
	if (size_ < min_size || size_ > max_size)
	{
		LOG_ERROR("Invalid size for shadow map.");
		throw invalid_parameter();
	}

	if (cascade_count_ < 1 || cascade_count_ > max_cascade_count)
	{
		LOG_ERROR("Invalid cascade count for shadow map.");
		throw invalid_parameter();
	}

	for (int64 i = 0; i < cascade_count_; ++i)
	{
		world_to_texture_.push_back(matrix());
		valid_.push_back(false);
	}

	depth_textures_.reset
		(new gl3_texture_object_2d_array
			(gl3_, vector2i(size_, size_), cascade_count_,
			 gl3_texture_object::texture_format_depth32,
			 render_texture_data::wrap_none,
			 render_texture_data::filter_linear));
	
	for (int64 i = 0; i < cascade_count_; ++i)
	{
		auto_pointer<gl3_frame_buffer_attachment> depth_attachment
			(new gl3_frame_buffer_attachment_2d_array(*depth_textures_, i));

		gl3_frame_buffer::attachment_map attachements;
		attachements.insert
			(gl3_frame_buffer::attachment_depth,
			 std::move(depth_attachment));

		depth_targets_.insert
			(auto_pointer<gl3_frame_buffer>
				(new gl3_frame_buffer
					(gl3_, vector2i(size_, size_), attachements)));
	}
}


gl3_shadow_map_parallel::~gl3_shadow_map_parallel() NOTHROW
{
	// Resolve member destructors to defined types here.
}


int64 gl3_shadow_map_parallel::size() const NOTHROW
	{ return size_; }
int64 gl3_shadow_map_parallel::cascade_count() const NOTHROW
	{ return cascade_count_; }


void gl3_shadow_map_parallel::update_world_to_texture
	(const camera& entry_camera, int64 entry)
{
	if (entry < 0 || entry >= world_to_texture_.size())
	{
		LOG_ERROR("Requested entry exceeds cascade count.");
		throw invalid_parameter();
	}

	world_to_texture_[entry] =
		matrix::scale(0.5f) *
		matrix::translate(vector(1, 1, 1)) *
		entry_camera.build_projection() *
		entry_camera.build_world_to_camera();
}


matrix gl3_shadow_map_parallel::world_to_texture(int64 entry) const NOTHROW
{
	if (entry < 0 || entry >= world_to_texture_.size())
	{
		LOG_ERROR("Requested entry exceeds cascade count.");
		return matrix(); // NOTHROW
	}

	return world_to_texture_[entry];
}


void gl3_shadow_map_parallel::update_valid
	(bool valid, int64 entry)
{
	if (entry < 0 || entry >= world_to_texture_.size())
	{
		LOG_ERROR("Requested entry exceeds cascade count.");
		throw invalid_parameter();
	}

	valid_[entry] = valid;
}


bool gl3_shadow_map_parallel::valid(int64 entry) const NOTHROW
{
	if (entry < 0 || entry >= world_to_texture_.size())
	{
		LOG_ERROR("Requested entry exceeds cascade count.");
		return false; // NOTHROW
	}

	return valid_[entry];
}


void gl3_shadow_map_parallel::prepare_and_bind_target
	(int64 entry) const
{
	if (entry < 0 ||
		entry >= depth_targets_.size())
	{
		LOG_ERROR("Invalid cascade index for binding.");
		throw invalid_parameter();
	}

	if (!depth_targets_.at(entry))
	{
		LOG_ERROR("Depth targets array contains NULL pointer.");
		throw assert_failed();
	}

	gl3_frame_buffer::unbind
		(gl3_frame_buffer::binding_read, gl3_);
	depth_targets_.at(entry)->bind
		(gl3_frame_buffer::binding_draw);

	glViewport
		(0, 0,
		 integer_operations::cast_or_throw<GLsizei>(size_),
		 integer_operations::cast_or_throw<GLsizei>(size_));

	glDisable(GL_SCISSOR_TEST);

	glDepthMask(GL_TRUE);
	glClearDepth(1);
	glClear(GL_DEPTH_BUFFER_BIT);
}


void gl3_shadow_map_parallel::bind_texture(GLenum texture_unit_index) const
	{ depth_textures_->bind(texture_unit_index); }




} /* namespace viral_core */