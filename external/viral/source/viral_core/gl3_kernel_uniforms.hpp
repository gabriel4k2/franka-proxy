/**
 *************************************************************************
 *
 * @file gl3_kernel_uniforms.hpp
 *
 * Uniform names used by OpenGL3 kernel.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_UNIFORMS_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_UNIFORMS_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_uniforms
 * @ingroup gl3_render_module
 *
 * Names for built-in uniforms in gl3_kernel_state.
 *
 * Implementation rationale:
 * - We cache @ref string%s per gl3_kernel_state instead of const char*
 *		to avoid @ref string allocations per gl3_uniform_value_map request.
 * - We cannot use static const @ref string%s due to undefined order
 *		of static initialization against client modules.
 *
 ************************************************************************/
class gl3_kernel_uniforms
{

	public:

		gl3_kernel_uniforms();


		/** Uniforms with camera and model transformations. */
		//@{
		const string projection;
		const string camera_position;
		const string world_to_camera;
		const string model_to_world;
		//@}


		/**
		 * Matrices with per-joint transformations
		 * from unposed to posed model coordinates.
		 * Only set for models with a skeleton rig,
		 * and identity for puppets without a pose.
		 *
		 * Size is gl3_kernel_limits::max_skeleton_joints.
		 */
		//@{
		const string skeleton_absolute_pose;
		const string skeleton_joint_count;
		//@}


		/** General parameters of render_material_data. */
		//@{
		const string texture;
		const string normal_map;

		const string material_emissive_color;
		const string material_ambient_color;
		const string material_diffuse_color;
		const string material_specular_color;
		const string material_specular_exponent;
		//@}


		/**
		 * Light uniforms as set for any lit materials.
		 * Size is gl3_kernel_limits::max_pass_lights.
		 */
		//@{
		const string light_emitter;
		const string light_position;
		const string light_direction;

		const string light_ambient_color;
		const string light_diffuse_color;
		const string light_specular_color;

		const string light_intensity;
		const string light_attenuation;

		const string light_spot_cosine;
		const string light_spot_exponent;

		const string light_shadow_enabled;
		const string light_shadow_opacity;

		const string light_count;
		//@}


		/**
		 * Shadow maps, independent from per-pass lights.
		 * Available for any lit materials.
		 */
		//@{
		const string shadow_map_point;
		const string shadow_map_spot;
		const string shadow_map_parallel;

		const string shadow_point_z_factor;
		const string shadow_point_z_offset;
		const string shadow_spot_world_to_texture;
		const string shadow_parallel_world_to_texture;
		const string shadow_parallel_valid;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_UNIFORMS_HPP) */
