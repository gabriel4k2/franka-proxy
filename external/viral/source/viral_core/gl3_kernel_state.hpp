/**
 *************************************************************************
 *
 * @file gl3_kernel_state.hpp
 *
 * Rendering state management for OpenGL3 kernel.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_STATE_HPP)
#define INCLUDED__VIRAL_CORE__GL3_KERNEL_STATE_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "mesh_forward.hpp"
#include "gl3_forward.hpp"


#include "util.hpp"
#include "buffer.hpp"
#include "gl3_uniform.hpp"
#include "gl3_kernel_util.hpp"
#include "gl3_kernel_uniforms.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class gl3_kernel_state
 * @ingroup gl3_render_module
 *
 * Manages rendering states within a gl3_kernel,
 * including material parameters and uniform values.
 *
 * Usage notes:
 * - Any calls that set uniform values (e.g. set_camera())
 *		do not apply to uniforms on the current program object.
 *		Instead, the gl3_kernel_state applies uniform values
 *		alongside the next apply_material() call. 
 *
 ************************************************************************/
class gl3_kernel_state :
	private non_copyable
{

	public:

		gl3_kernel_state
			(const covert_object_manager& object_manager,
			 const gl3_api& gl3);


		~gl3_kernel_state() NOTHROW;


		/**
		 * Set uniform values for camera and modelview transforms.
		 * See @ref camera and gl3_render_context for conventions.
		 */
		//@{
		void set_camera(const camera& cam);
		void set_modelview
			(const vector& position,
			 const rotation& orientation,
			 float scale);
		//@}


		/**
		 * Set uniform values for a @bref{skeleton_rig}
		 * in a given @bref{relative_pose}.
		 *
		 * A @NULL @bref{relative_pose} applies the default pose,
		 * and a @NULL @bref{skeleton_rig} is ignored silently.
		 */
		void set_skeleton
			(const skeleton* skeleton_rig,
			 const skeleton_transform* relative_pose);


		/**
		 * Set uniform values for lights and shadow maps
		 * and bind shadow maps to texture units.
		 *
		 * Uses @bref{kernel_scene} and @bref{kernel_shadow}
		 * to resolve given light and shadow map indices.
		 * This avoids permanent circular dependencies.
		 */
		//@{
		void set_lights
			(const gl3_kernel_scene& kernel_scene,
			 const dynamic_buffer<int64>& light_indices,
			 const dynamic_buffer<bool>& shadow_enabled_override);
		void set_shadow_maps
			(const gl3_kernel_shadow& kernel_shadow,
			 int64 shadow_map_point_index,
			 int64 shadow_map_spot_index,
			 int64 shadow_map_parallel_index);
		//@}


		/**
		 * Apply @bref{material} states for some rendering @bref{pass},
		 * including any previously set uniform values.
		 *
		 * Use optional kernel context (e.g. mesh, model, puppet)
		 * to supply additional states and to detect inconsistencies,
		 * such as missing mesh streams.
		 */
		void apply_material
			(const gl3_material& material,
			 gl3_kernel_pass_type pass,
			 const gl3_mesh* mesh,
			 const gl3_model* model,
			 const gl3_puppet* puppet);


		/** Static texture unit assignments. */
		//@{
		static const GLenum texture_unit_index_texture = 0;
		static const GLenum texture_unit_index_normal_map = 1;

		static const GLenum texture_unit_index_shadow_map_point = 4;
		static const GLenum texture_unit_index_shadow_map_spot = 5;
		static const GLenum texture_unit_index_shadow_map_parallel = 6;
		//@}

		
	private:

		void apply_parameters
			(const gl3_material_data& material_data);
		void apply_textures
			(const gl3_material_data& material_data);

		void apply_shader
			(const gl3_material_data& material_data,	
			 gl3_kernel_pass_type pass,
			 const gl3_mesh* mesh,
			 const gl3_model* model,
			 const gl3_puppet* puppet);

		void apply_program_object
			(const gl3_program_object& program_object,
			 const gl3_material_data& material_data,	
			 gl3_kernel_pass_type pass,
			 const gl3_mesh* mesh,
			 const gl3_model* model,
			 const gl3_puppet* puppet);

		void apply_blend
			(const gl3_material_data& material_data,
			 gl3_kernel_pass_type pass);
		void apply_depth_test
			(const gl3_material_data& material_data,
			 gl3_kernel_pass_type pass);

		void apply_fill
			(const gl3_material_data& material_data);
		void apply_point_and_line_size
			(const gl3_material_data& material_data);


		const covert_object_manager& object_manager_;
		const gl3_api& gl3_;

		bool camera_right_handed_;

		const gl3_kernel_uniforms uniforms_;

		gl3_uniform_value_map camera_uniform_values_;
		gl3_uniform_value_map modelview_uniform_values_;
		gl3_uniform_value_map skeleton_uniform_values_;
		gl3_uniform_value_map texture_uniform_values_;
		gl3_uniform_value_map normal_map_uniform_values_;
		gl3_uniform_value_map material_uniform_values_;
		gl3_uniform_value_map light_uniform_values_;

		gl3_uniform_value_stack uniform_value_stack_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_KERNEL_STATE_HPP) */