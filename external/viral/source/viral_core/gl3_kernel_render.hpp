/**
 *************************************************************************
 *
 * @file gl3_kernel_render.hpp
 *
 * Rendering of scene objects in OpenGL3 kernel.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_RENDER_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_RENDER_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "geo_camera.hpp"
#include "gl3_kernel_state.hpp"
#include "gl3_kernel_light.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_render
 * @ingroup gl3_render_module
 *
 * Renders scene objects as parsed by a gl3_kernel_scene
 * and applies shadow maps from a gl3_kernel_shadow pass.
 *
 ************************************************************************/
class gl3_kernel_render :
	private non_copyable
{
	public:

		gl3_kernel_render
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3,
			 const gl3_kernel_scene& kernel_scene,
			 const gl3_kernel_shadow& kernel_shadow);


		~gl3_kernel_render() NOTHROW;


		/**
		 * Render all objects from @bref{kernel_scene_}
		 * onto a @bref{viewport} on a rendering @bref{target}
		 * through a scene @bref{cam}era.
		 */
		void render_scene
			(const camera& cam,
			 gl3_render_target& target,
			 const rect2f& viewport);


	private:

		void apply_render_target
			(gl3_render_target& target,
			 const rect2f& viewport);
		void apply_camera
			(const camera& cam);

		void render_pass_opaque();
		void render_pass_blend();

		void render_puppet
			(const gl3_puppet& puppet,
			 gl3_kernel_pass_type pass);
		void render_stack
			(const gl3_stack& stack);


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		const gl3_kernel_scene& kernel_scene_;
		const gl3_kernel_shadow& kernel_shadow_;

		gl3_kernel_state kernel_state_;
		gl3_kernel_light kernel_light_;			

		camera cam_;
		frustum frustum_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_RENDER_HPP) */