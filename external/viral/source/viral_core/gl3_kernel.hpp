/**
 *************************************************************************
 *
 * @file gl3_kernel.hpp
 *
 * OpenGL core rendering logics.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel
 * @ingroup gl3_render_module
 *
 * The core rendering kernel.
 *
 * Overview over rendering steps:
 * #- Scene parsing pass:
 *		Traverse entire scene to collect relevant scene objects
 *		(lights, puppets) for all later rendering passes.
  * #- Rendering pass for shadows:
 *		#- Select prominent, shadow-enabled lights for shadow maps.
 *		#- Render shadow-casting objects in range to shadow maps.
 * #- Rendering pass for opaque objects:
 *		#- Traverse opaque objects from front to back.
 *		#- For each object, if the object is lit:
 *			#- Find all relevant lights and shadow maps.
 *			#- Render opaque first pass with per-pass light count.
 *			#- Render additional lights via additive blending.
 *		#- Otherwise, render unlit object.
 * #- Rendering pass for blended objects:
 *		#- Traverse blended objects from back to front.
 *		#- For each object, if the object is lit:
 *			#- Find all relevant lights and shadow maps.
 *			#- Render a single pass with most prominent lights.
 *		#- Otherwise, render unlit object.
 *
 * Built-in uniforms:
 * - The kernel identifies certain built-in uniforms by name
 *		and automatically applies suitable gl3_uniform_value%s.
 *		For example, the kernel applies the current camera position
 *		to any uniform with the name @bref{camera_position}.
 * - Automatic uniform values depend on the current context.
 *		For instance, values for lighting-specific uniforms
 *		are not applied if the current material is unlit.
 * - See gl3_kernel_uniforms for names of built-in uniforms.
 *
 * Client shader usage:
 * - Clients should use gl3_shader_bricks and gl3_shader_workshop
 *		to ensure proper names and types for built-in uniforms.
 * - Clients can use custom, shader-specific uniforms. In this case,
 *		clients must explicitly provide associated uniform values,
 *		e.g. through gl3_puppet_data::uniform_values.
 * - Client uniform values replace any automatic ones of same name.
 *		See gl3_puppet_data and gl3_material_data for details.
 * - Rendering checks all uniforms in client shaders against
 *		available uniform values and throws on any discrepancies.
 *		See gl3_program_object::apply_uniform_values().
 *
 * Implementation notes:
 * - Kernel logics (e.g. gl3_kernel_scene) use simple data structures,
 *		in particular naive pointers and dynamic_buffer lookup tables,
 *		to reduce expensive dynamic allocations on threaded rendering.
 * - @attent The kernel must rebuild all data structures on each frame 
 *		to secure native pointers against deletion and exceptions. 
 *
 ************************************************************************/
class gl3_kernel :
	private non_copyable
{

	public:

		gl3_kernel
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);


		~gl3_kernel() NOTHROW;


		/**
		 * Render a @bref{scene} through a @bref{cam}era
		 * into a @bref{viewport} on a @bref{target}.
		 */
		void execute
			(const camera& cam, gl3_scene& scene,
			 gl3_render_target& target, const rect2f& viewport);


	private:

		const auto_pointer<gl3_kernel_scene> kernel_scene_;
		const auto_pointer<gl3_kernel_shadow> kernel_shadow_;
		const auto_pointer<gl3_kernel_render> kernel_render_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_HPP) */