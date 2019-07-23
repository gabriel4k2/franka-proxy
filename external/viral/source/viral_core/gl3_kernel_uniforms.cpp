/**
 *************************************************************************
 *
 * @file gl3_kernel_uniforms.cpp
 *
 * Uniform names used by OpenGL3 kernel, implementation.
 *
 ************************************************************************/


#include "gl3_kernel_uniforms.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel_uniforms
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel_uniforms::gl3_kernel_uniforms()
	:
	projection("projection"),
	camera_position("camera_position"),
	world_to_camera("world_to_camera"),
	model_to_world("model_to_world"),

	skeleton_absolute_pose("skeleton_absolute_pose[0]"),
	skeleton_joint_count("skeleton_joint_count"),

	texture("texture_map"),
	normal_map("normal_map"),

	material_emissive_color("material_emissive_color"),
	material_ambient_color("material_ambient_color"),
	material_diffuse_color("material_diffuse_color"),
	material_specular_color("material_specular_color"),
	material_specular_exponent("material_specular_exponent"),

	light_emitter("light_emitter[0]"),
	light_position("light_position[0]"),
	light_direction("light_direction[0]"),

	light_ambient_color("light_ambient_color[0]"),
	light_diffuse_color("light_diffuse_color[0]"),
	light_specular_color("light_specular_color[0]"),

	light_intensity("light_intensity[0]"),
	light_attenuation("light_attenuation[0]"),

	light_spot_cosine("light_spot_cosine[0]"),
	light_spot_exponent("light_spot_exponent[0]"),

	light_shadow_enabled("light_shadow_enabled[0]"),
	light_shadow_opacity("light_shadow_opacity[0]"),
	
	light_count("light_count"),

	shadow_map_point("shadow_map_point"),
	shadow_map_spot("shadow_map_spot"),
	shadow_map_parallel("shadow_map_parallel"),

	shadow_point_z_factor("shadow_point_z_factor"),
	shadow_point_z_offset("shadow_point_z_offset"),
	shadow_spot_world_to_texture("shadow_spot_world_to_texture"),
	shadow_parallel_world_to_texture("shadow_parallel_world_to_texture[0]"),
	shadow_parallel_valid("shadow_parallel_valid[0]")
{ }




} /* namespace viral_core */
