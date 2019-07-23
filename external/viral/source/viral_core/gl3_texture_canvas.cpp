/**
 *************************************************************************
 *
 * @file gl3_texture_canvas.cpp
 *
 * Render-to-texture result bridge, implementation.
 *
 ************************************************************************/


#include "gl3_texture_canvas.hpp"


#include "log.hpp"
#include "covert_object.hpp"
#include "gl3_process.hpp"
#include "gl3_render_target.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_texture_object_canvas
//
//////////////////////////////////////////////////////////////////////////


gl3_texture_canvas::gl3_texture_canvas
	(const gl3_api& gl3,
	 const covert_object_manager& object_manager,
	 const weak_pointer<render_canvas_id>& source_canvas)
	:
	gl3_texture_object(gl3),
	object_manager_(object_manager),
	source_canvas_(source_canvas)
{
	if (!source_canvas_)
	{
		LOG_ERROR("Cannot create GL3 texture object for NULL source canvas.");
		throw null_pointer();
	}
}


void gl3_texture_canvas::build_mipmaps() const
{
	// This operation makes no sense for canvas bridges.
	// See gl3_texture_canvas::bind() for rationale.
	LOG_ERROR("Cannot rebuild mipmaps on canvas texture bridge.\n"
			  "Rebuild mipmaps on render target of canvas instead.");
	throw unsupported_feature();
}


void gl3_texture_canvas::bind
	(GLenum texture_unit_index) const
{
	// Fetch the source canvas and ensure that it is still live.
	shared_pointer<render_canvas_id> locked_source_canvas
		(source_canvas_.lock());

	if (!locked_source_canvas)
	{
		LOG_ERROR("The source canvas for this texture was destroyed.\n"
				  "Canvas textures do not keep canvases alive on their own.\n"
				  "Please maintain some shared_pointer on the canvas.");
		throw invalid_state();
	}

	const gl3_canvas& gl3_source_canvas =
		object_manager_.object_for_id<gl3_canvas>(locked_source_canvas);


	// Prohibit the use of this canvas texture while rendering
	// to its source canvas. We check the executing() flag
	// instead of actually comparing any canvas pointers,
	// because it is difficult to acquire a shared_pointer
	// for the current target canvas here.
	if (gl3_source_canvas.executing())
	{
		LOG_ERROR("Read/write conflict: Cannot use a canvas texture\n"
				  "for rendering to its associated source canvas.");
		throw invalid_state();
	}


	// Route bind call to source texture.
	const gl3_texture_object_2d& source_texture =
		gl3_source_canvas.canvas_target().color_texture();

	source_texture.bind(texture_unit_index);
}




} /* namespace viral_core */