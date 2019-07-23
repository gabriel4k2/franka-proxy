/**
 *************************************************************************
 *
 * @file gl3_kernel.cpp
 *
 * OpenGL core rendering logics, implementation.
 *
 ************************************************************************/


#include "gl3_kernel.hpp"


#include "gl3_kernel_scene.hpp"
#include "gl3_kernel_shadow.hpp"
#include "gl3_kernel_render.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel::gl3_kernel
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	kernel_scene_
		(new gl3_kernel_scene(object_manager)),

	kernel_shadow_
		(new gl3_kernel_shadow
			(object_manager, gl3, *kernel_scene_)),

	kernel_render_
		(new gl3_kernel_render
			(object_manager, gl3, *kernel_scene_, *kernel_shadow_))
{ }


gl3_kernel::~gl3_kernel() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_kernel::execute
	(const camera& cam, gl3_scene& scene,
	 gl3_render_target& target, const rect2f& viewport)
{
	kernel_scene_->parse_scene(scene, cam);
	kernel_shadow_->render_shadow_maps(cam);
	kernel_render_->render_scene(cam, target, viewport);
}




} /* namespace viral_core */
