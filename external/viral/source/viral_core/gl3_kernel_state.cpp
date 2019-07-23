/**
 *************************************************************************
 *
 * @file gl3_kernel_state.cpp
 *
 * Rendering state management for OpenGL3 kernel, implementation.
 *
 ************************************************************************/


#include "gl3_kernel_state.hpp"


#include <math.h>


#include "log.hpp"
#include "integer_operations.hpp"
#include "geo_util.hpp"
#include "geo_camera.hpp"
#include "covert_object.hpp"
#include "render_resource.hpp"
#include "render_scene.hpp"
#include "gl3_shadow_map.hpp"
#include "gl3_resource.hpp"
#include "gl3_scene.hpp"
#include "gl3_kernel_scene.hpp"
#include "gl3_kernel_shadow.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel_state
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel_state::gl3_kernel_state
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager),
	gl3_(gl3),
	camera_right_handed_(true)
{ }


gl3_kernel_state::~gl3_kernel_state() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_kernel_state::set_camera
	(const camera& cam)
{
	camera_uniform_values_.request<gl3_uniform_value_vector>
		(uniforms_.camera_position).value =
			cam.position;
	camera_uniform_values_.request<gl3_uniform_value_matrix>
		(uniforms_.projection).value =
			cam.build_projection();
	camera_uniform_values_.request<gl3_uniform_value_matrix>
		(uniforms_.world_to_camera).value =
			cam.build_world_to_camera();

	camera_right_handed_ =
		cam.post_scale.product() >= 0;		
}


void gl3_kernel_state::set_modelview
	(const vector& position,
	 const rotation& orientation,
	 float scale)
{
	modelview_uniform_values_.request
		<gl3_uniform_value_matrix>
			(uniforms_.model_to_world).value =
				matrix::translate(position) *
				matrix::rotate(orientation) *
				matrix::scale(scale);
}


void gl3_kernel_state::set_skeleton
	(const skeleton* skeleton_rig,
	 const skeleton_transform* relative_pose)
{
	if (!skeleton_rig) return;


	// Build absolute pose from relative pose,
	// use default pose if no pose is provided.
	skeleton_transform default_pose;	
	if (!relative_pose)
	{
		default_pose.resize(skeleton_rig->joints().size());
		relative_pose = &default_pose;
	}

	skeleton_transform_matrix absolute_pose_matrix;
	skeleton_rig->build_absolute_pose_matrix
		(*relative_pose, absolute_pose_matrix);


	// Apply joint matrices and joint count to uniforms.
	// Note we must check our pose (and thus the skeleton)
	// against the constant size of the uniform array.
	if (absolute_pose_matrix.size() >
		gl3_kernel_limits::max_skeleton_joints)
	{
		LOG_ERROR("Skeleton exceeds maximum supported number of joints.");
		throw bounds_exceeded();
	}

	int32 absolute_pose_matrix_size =
		integer_operations::cast_or_throw<int32>
			(absolute_pose_matrix.size());

	typedef gl3_uniform_value_array_typed
				<matrix, gl3_kernel_limits::max_skeleton_joints>
					absolute_pose_type;

	absolute_pose_type& absolute_pose_uniform =
		skeleton_uniform_values_.request<absolute_pose_type>
			(uniforms_.skeleton_absolute_pose);

	for (int64 i = 0; i < absolute_pose_matrix.size(); ++i)
		absolute_pose_uniform.values[i] = absolute_pose_matrix[i];

	skeleton_uniform_values_.request<gl3_uniform_value_int32>
		(uniforms_.skeleton_joint_count).value = absolute_pose_matrix_size;
}


void gl3_kernel_state::set_lights
	(const gl3_kernel_scene& kernel_scene,
	 const dynamic_buffer<int64>& light_indices,
	 const dynamic_buffer<bool>& shadow_enabled_override)
{
	// Validate incoming light indices.
	if (light_indices.size() >
		gl3_kernel_limits::max_pass_lights)
	{
		LOG_ERROR("Lights exceed maximum number of lights per pass.");
		throw bounds_exceeded();
	}

	int32 light_indices_size =
		integer_operations::cast_or_throw<int32>
			(light_indices.size());


	// Fetch all arrays with per-light uniforms.
	// Typedefs make this somewhat more readable.
	typedef gl3_uniform_value_array_typed
		<bool, gl3_kernel_limits::max_pass_lights> light_bool_type;
	typedef gl3_uniform_value_array_typed
		<int32, gl3_kernel_limits::max_pass_lights> light_int_type;
	typedef gl3_uniform_value_array_typed
		<float, gl3_kernel_limits::max_pass_lights> light_float_type;
	typedef gl3_uniform_value_array_typed
		<color, gl3_kernel_limits::max_pass_lights> light_color_type;
	typedef gl3_uniform_value_array_typed
		<vector, gl3_kernel_limits::max_pass_lights> light_vector_type;

	light_int_type& emitter =
		light_uniform_values_.request<light_int_type>
			(uniforms_.light_emitter);
	light_vector_type& position =
		light_uniform_values_.request<light_vector_type>
			(uniforms_.light_position);
	light_vector_type& direction =
		light_uniform_values_.request<light_vector_type>
			(uniforms_.light_direction);

	light_color_type& ambient_color =
		light_uniform_values_.request<light_color_type>
			(uniforms_.light_ambient_color);
	light_color_type& diffuse_color =
		light_uniform_values_.request<light_color_type>
			(uniforms_.light_diffuse_color);
	light_color_type& specular_color =
		light_uniform_values_.request<light_color_type>
			(uniforms_.light_specular_color);

	light_float_type& intensity =
		light_uniform_values_.request<light_float_type>
			(uniforms_.light_intensity);
	light_float_type& attenuation =
		light_uniform_values_.request<light_float_type>
			(uniforms_.light_attenuation);

	light_float_type& spot_cosine =
		light_uniform_values_.request<light_float_type>
			(uniforms_.light_spot_cosine);
	light_float_type& spot_exponent =
		light_uniform_values_.request<light_float_type>
			(uniforms_.light_spot_exponent);

	light_int_type& shadow_enabled =
		light_uniform_values_.request<light_int_type>
			(uniforms_.light_shadow_enabled);
	light_float_type& shadow_opacity =
		light_uniform_values_.request<light_float_type>
			(uniforms_.light_shadow_opacity);


	// Store light parameters in the uniform arrays,
	// and store total number of lights in this pass.	
	for (int64 i = 0; i < light_indices.size(); ++i)
	{
		const render_light_data& light_data =
			kernel_scene.light(light_indices[i]).light_data();

		emitter.values[i] = light_data.emitter;
		position.values[i] = light_data.position;
		direction.values[i] = light_data.orientation * vector(0, 0, 1);

		ambient_color.values[i] = light_data.ambient_color;
		diffuse_color.values[i] = light_data.diffuse_color;
		specular_color.values[i] = light_data.specular_color;

		intensity.values[i] = light_data.intensity;
		attenuation.values[i] = light_data.attenuation;

		float spot_half_angle_rad =
			geo_constants::degrees_to_radians * light_data.spot_angle / 2;
		spot_cosine.values[i] = cosf(spot_half_angle_rad);
		spot_exponent.values[i] = light_data.spot_exponent;

		// Replace original render_light_data::shadow_enabled,
		// possibly discarding shadows for single-pass rendering.
		// We use ints instead of bools due to glUniform limits.
		shadow_enabled.values[i] =
			shadow_enabled_override[i] ? 1 : 0;

		shadow_opacity.values[i] = light_data.shadow_opacity;
	}

	light_uniform_values_.request<gl3_uniform_value_int32>
		(uniforms_.light_count).value = light_indices_size;
}


void gl3_kernel_state::set_shadow_maps
	(const gl3_kernel_shadow& kernel_shadow,
	 int64 shadow_map_point_index,
	 int64 shadow_map_spot_index,
	 int64 shadow_map_parallel_index)
{
	// Fetch shadow maps from shadow kernel.
	const gl3_shadow_map_point* shadow_map_point =
		(shadow_map_point_index ==
		 gl3_kernel_shadow::no_shadow_map_assigned) ?
			NULL :
			&kernel_shadow.shadow_map_point(shadow_map_point_index);

	const gl3_shadow_map_spot* shadow_map_spot =
		(shadow_map_spot_index ==
		 gl3_kernel_shadow::no_shadow_map_assigned) ?
			NULL :
			&kernel_shadow.shadow_map_spot(shadow_map_spot_index);

	const gl3_shadow_map_parallel* shadow_map_parallel =
		(shadow_map_parallel_index ==
		 gl3_kernel_shadow::no_shadow_map_assigned) ?
			NULL :
			&kernel_shadow.shadow_map_parallel(shadow_map_parallel_index);


	// Either bind shadow maps to shadow map slots,
	// or reset binding to no texture.
	if (shadow_map_point)
		shadow_map_point->bind_texture
			(texture_unit_index_shadow_map_point);
	else
		gl3_texture_object::unbind
			(texture_unit_index_shadow_map_point, gl3_);

	if (shadow_map_spot)
		shadow_map_spot->bind_texture
			(texture_unit_index_shadow_map_spot);
	else
		gl3_texture_object::unbind
			(texture_unit_index_shadow_map_spot, gl3_);

	if (shadow_map_parallel)
		shadow_map_parallel->bind_texture
			(texture_unit_index_shadow_map_parallel);
	else
		gl3_texture_object::unbind
			(texture_unit_index_shadow_map_parallel, gl3_);


	// Set sampler uniforms to shadow map slots.
	light_uniform_values_.request<gl3_uniform_value_int32>
		(uniforms_.shadow_map_point).value =
			texture_unit_index_shadow_map_point;

	light_uniform_values_.request<gl3_uniform_value_int32>
		(uniforms_.shadow_map_spot).value =
			texture_unit_index_shadow_map_spot;

	light_uniform_values_.request<gl3_uniform_value_int32>
		(uniforms_.shadow_map_parallel).value =
			texture_unit_index_shadow_map_parallel;


	// Apply individual uniforms for each shadow map type.
	// We must apply these even if a shadow map is not active,
	// so that later uniform assignment checks do not fail.
	light_uniform_values_.request<gl3_uniform_value_float>
		(uniforms_.shadow_point_z_factor).value =
			shadow_map_point ?
				shadow_map_point->z_factor() : 0;
	light_uniform_values_.request<gl3_uniform_value_float>
		(uniforms_.shadow_point_z_offset).value =
			shadow_map_point ?
				shadow_map_point->z_offset() : 0;

	light_uniform_values_.request<gl3_uniform_value_matrix>
		(uniforms_.shadow_spot_world_to_texture).value =
			shadow_map_spot ?
				shadow_map_spot->world_to_texture() : matrix();

	typedef gl3_uniform_value_array_typed
		<matrix, gl3_kernel_limits::shadow_map_cascade_count>
			shadow_parallel_world_to_texture_type;
	shadow_parallel_world_to_texture_type&
		shadow_parallel_world_to_texture =
			light_uniform_values_.request<shadow_parallel_world_to_texture_type>
				(uniforms_.shadow_parallel_world_to_texture);

	typedef gl3_uniform_value_array_typed
		<int32, gl3_kernel_limits::shadow_map_cascade_count>
			shadow_parallel_valid_type;
	shadow_parallel_valid_type&
		shadow_parallel_valid =
			light_uniform_values_.request<shadow_parallel_valid_type>
				(uniforms_.shadow_parallel_valid);

	for (int64 i = 0; i < gl3_kernel_limits::shadow_map_cascade_count; ++i)
	{
		shadow_parallel_world_to_texture.values[i] =
			shadow_map_parallel ?
				shadow_map_parallel->world_to_texture(i) : matrix();

		shadow_parallel_valid.values[i] =
			(shadow_map_parallel && shadow_map_parallel->valid(i)) ? 1 : 0;
	}
}


void gl3_kernel_state::apply_material
	(const gl3_material& material,
	 gl3_kernel_pass_type pass,
	 const gl3_mesh* mesh,
	 const gl3_model* model,
	 const gl3_puppet* puppet)
{
	const gl3_material_data& material_data =
		material.material_data();

	apply_parameters(material_data);
	apply_textures(material_data);

	apply_shader
		(material_data, pass,
		 mesh, model, puppet);

	apply_blend(material_data, pass);
	apply_depth_test(material_data, pass);
	apply_fill(material_data);
	apply_point_and_line_size(material_data);
}


void gl3_kernel_state::apply_parameters
	(const gl3_material_data& material_data)
{
	material_uniform_values_.request<gl3_uniform_value_color>
		(uniforms_.material_emissive_color).value =
			material_data.emissive_color;
	material_uniform_values_.request<gl3_uniform_value_color>
		(uniforms_.material_ambient_color).value =
			material_data.ambient_color;
	material_uniform_values_.request<gl3_uniform_value_color>
		(uniforms_.material_diffuse_color).value =
			material_data.diffuse_color;
	material_uniform_values_.request<gl3_uniform_value_color>
		(uniforms_.material_specular_color).value =
			material_data.specular_color;
	material_uniform_values_.request<gl3_uniform_value_float>
		(uniforms_.material_specular_exponent).value =
			material_data.specular_exponent;
}


void gl3_kernel_state::apply_textures
	(const gl3_material_data& material_data)
{
	if (material_data.texture)
	{
		gl3_texture& texture =
			object_manager_.object_for_id<gl3_texture>
				(material_data.texture);

		texture.texture_object().bind(texture_unit_index_texture);

		texture_uniform_values_.request<gl3_uniform_value_int32>
			(uniforms_.texture).value = texture_unit_index_texture;
	}

	if (material_data.normal_map)
	{
		gl3_texture& normal_map =
			object_manager_.object_for_id<gl3_texture>
				(material_data.normal_map);

		normal_map.texture_object().bind(texture_unit_index_normal_map);

		normal_map_uniform_values_.request<gl3_uniform_value_int32>
			(uniforms_.normal_map).value = texture_unit_index_normal_map;
	}
}


void gl3_kernel_state::apply_shader
	(const gl3_material_data& material_data,	
	 gl3_kernel_pass_type pass,
	 const gl3_mesh* mesh,
	 const gl3_model* model,
	 const gl3_puppet* puppet)
{
	const shared_pointer<render_shader_id>& shader_id =
		(pass == gl3_kernel_pass::shadow_map) ?
			material_data.shadow_cast_shader :
			material_data.shader;

	if (!shader_id)
	{
		if (pass == gl3_kernel_pass::shadow_map)
		{
			LOG_ERROR("Shadow-casting material is missing shadow shader.");
			throw null_pointer();
		}
		else
		{
			LOG_ERROR("Material is missing shader.");
			throw null_pointer();
		}
	}

	gl3_shader& shader =
		object_manager_.object_for_id<gl3_shader>
			(shader_id);

	apply_program_object
		(shader.program_object(), material_data,
		 pass, mesh, model, puppet);
}


void gl3_kernel_state::apply_program_object
	(const gl3_program_object& program_object,
	 const gl3_material_data& material_data,	
	 gl3_kernel_pass_type pass,
	 const gl3_mesh* mesh,
	 const gl3_model* model,
	 const gl3_puppet* puppet)
{	
	// Potentially check if @bref{model} carries
	// all vertex attributes required by the program.
	if (mesh)
		program_object.check_matching_locations(*mesh);


	// Collect all uniform values relevant
	// within the current kernel context.
	// Reuse value stack to avoid reallocation.
	uniform_value_stack_.clear();
	uniform_value_stack_.push_back(&camera_uniform_values_);
	uniform_value_stack_.push_back(&modelview_uniform_values_);

	if (model && model->skeleton_rig())
		uniform_value_stack_.push_back(&skeleton_uniform_values_);

	if (material_data.texture)
		uniform_value_stack_.push_back(&texture_uniform_values_);

	if (material_data.normal_map)
		uniform_value_stack_.push_back(&normal_map_uniform_values_);

	uniform_value_stack_.push_back(&material_uniform_values_);

	if (pass != gl3_kernel_pass::shadow_map && !material_data.unlit)
		uniform_value_stack_.push_back(&light_uniform_values_);


	// Allow client-side uniform values
	// to override any other values.
	if (!material_data.uniform_values.empty())
		uniform_value_stack_.push_back(&material_data.uniform_values);

	if (puppet && !puppet->puppet_data().uniform_values.empty())
		uniform_value_stack_.push_back
			(&puppet->puppet_data().uniform_values);


	// Bind program object and any relevant uniform values.
	// Throw on any unassigned uniforms.
	program_object.bind();
	program_object.apply_uniform_values(uniform_value_stack_);
}


void gl3_kernel_state::apply_blend
	(const gl3_material_data& material_data,
	 gl3_kernel_pass_type pass)
{
	if (pass == gl3_kernel_pass::shadow_map)
	{
		// Shadow map rendering ignores material blending.
		glDisable(GL_BLEND);
		return;
	}

	if (material_data.blend == render_material_data::blend_opaque)
	{
		if (pass == gl3_kernel_pass::opaque)
		{
			glDisable(GL_BLEND);
		}
		else if (pass == gl3_kernel_pass::opaque_multipass_light)
		{
			// Additive light blending on subsequent passes.
			glEnable(GL_BLEND);
			glBlendFunc(GL_ONE, GL_ONE);
		}
		else if (pass == gl3_kernel_pass::stack)
		{
			// Single-pass opaque material as part of a render stack.
			glDisable(GL_BLEND);
		}
		else
		{
			LOG_ERROR("Unsupported pass on opaque object.");
			throw unsupported_feature();
		}
	}
	else if (material_data.blend == render_material_data::blend_alpha)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	}
	else if (material_data.blend == render_material_data::blend_modulate)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ZERO, GL_SRC_COLOR);
	}
	else if (material_data.blend == render_material_data::blend_additive)
	{
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);
	}
	else
	{
		LOG_ERROR("Unsupported blending mode encountered.");
		throw unsupported_feature();
	}
}


void gl3_kernel_state::apply_depth_test
	(const gl3_material_data& material_data,
	 gl3_kernel_pass_type pass)
{
	glEnable(GL_DEPTH_TEST);

	if (pass == gl3_kernel_pass::shadow_map)
	{
		// Shadow map rendering ignores blending-specific depth.
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		return;
	}

	if (material_data.blend == render_material_data::blend_opaque)
	{
		if (pass == gl3_kernel_pass::opaque)
		{
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
		}
		else if (pass == gl3_kernel_pass::opaque_multipass_light)
		{
			// Render only atop opaque base layer,
			// and disable redundant depth updates.
			glDepthFunc(GL_EQUAL);
			glDepthMask(GL_FALSE);
		}
		else if (pass == gl3_kernel_pass::stack)
		{
			// Single-pass opaque material as part of a render stack.
			glDepthFunc(GL_LESS);
			glDepthMask(GL_TRUE);
		}
		else
		{
			LOG_ERROR("Unsupported pass on opaque object.");
			throw unsupported_feature();
		}
	}
	else if (material_data.blend == render_material_data::blend_alpha ||
			 material_data.blend == render_material_data::blend_modulate ||
			 material_data.blend == render_material_data::blend_additive)
	{
		// No depth updates when blending,
		// favor wrong depth order over disappearing objects.
		glDepthFunc(GL_LESS);
		glDepthMask(GL_FALSE);
	}
	else
	{
		LOG_ERROR("Unsupported blending mode encountered.");
		throw unsupported_feature();
	}
}


void gl3_kernel_state::apply_fill
	(const gl3_material_data& material_data)
{
	if (material_data.raster == render_material_data::raster_solid)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else if (material_data.raster == render_material_data::raster_wireframe)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else if (material_data.raster == render_material_data::raster_points)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	}
	else
	{
		LOG_ERROR("Unsupported raster mode encountered.");
		throw unsupported_feature();
	}

	// Note right-handed CCW front-faces have CW winding in OpenGL,
	// and we only use OpenGL CCW on left-handed camera systems.
	// See gl3_render_context for a rationale.
	glFrontFace
		(integer_operations::cast_or_throw<GLenum>
			(camera_right_handed_ ? GL_CW : GL_CCW));

	if (material_data.cull == render_material_data::cull_back)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_BACK);
	}
	else if (material_data.cull == render_material_data::cull_front)
	{
		glEnable(GL_CULL_FACE);
		glCullFace(GL_FRONT);
	}
	else if (material_data.cull == render_material_data::cull_none)
	{
		glDisable(GL_CULL_FACE);
	}
	else
	{
		LOG_ERROR("Unsupported culling mode encountered.");
		throw unsupported_feature();
	}
}


void gl3_kernel_state::apply_point_and_line_size
	(const gl3_material_data& material_data)
{
	glPointSize
		(material_data.point_size == 0 ?
		 1.f : material_data.point_size);

	glLineWidth
		(material_data.line_size == 0 ?
		 1.f : material_data.line_size);
}




} /* namespace viral_core */