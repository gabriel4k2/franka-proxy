/**
 *************************************************************************
 *
 * @file render_scene.cpp
 *
 * Scene-specific rendering data, implementation.
 *
 ************************************************************************/


#include "render_scene.hpp"


#include <math.h>


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// render_light_data
//
//////////////////////////////////////////////////////////////////////////


render_light_data::render_light_data()
	:
	emitter(emitter_point),
	enabled(true),
	priority(1),
	influence_epsilon(2.f / 256.f),
	ambient_color(0.1f, 0.1f, 0.1f, 0),
	diffuse_color(0.8f, 0.8f, 0.8f, 0),
	specular_color(0.2f, 0.2f, 0.2f, 0),
	intensity(1),
	attenuation(1),
	spot_angle(90.f),
	spot_exponent(1),
	shadow_enabled(false),
	shadow_priority(1),
	shadow_opacity(1),
	shadow_clip_near_normalized(1e-5f),
	shadow_clip_far_normalized(1.f)
{ }


render_light_data::render_light_data
	(emitter_type emitter, bool enabled,
	 float priority, float influence_epsilon,
	 const color& ambient_color,
	 const color& diffuse_color,
	 const color& specular_color,
	 float intensity, float attenuation,
	 float spot_angle, float spot_exponent,
	 bool shadow_enabled,
	 float shadow_priority,
	 float shadow_opacity,
	 float shadow_clip_near_normalized,
	 float shadow_clip_far_normalized)
	:
	emitter(emitter),
	enabled(enabled),
	priority(priority),
	influence_epsilon(influence_epsilon),
	ambient_color(ambient_color),
	diffuse_color(diffuse_color),
	specular_color(specular_color),
	intensity(intensity),
	attenuation(attenuation),
	spot_angle(spot_angle),
	spot_exponent(spot_exponent),
	shadow_enabled(shadow_enabled),
	shadow_priority(shadow_priority),
	shadow_opacity(shadow_opacity),
	shadow_clip_near_normalized(shadow_clip_near_normalized),
	shadow_clip_far_normalized(shadow_clip_far_normalized)
{ }


auto_pointer<covert_data> render_light_data::clone() const
	{ return auto_pointer<covert_data>(new render_light_data(*this)); }


float render_light_data::influence(float distance) const NOTHROW
{
	if (!enabled) return 0;

	float color_factor =
		(ambient_color + diffuse_color + specular_color).component_sum();

	float att_factor = 1.f;

	if (emitter != emitter_parallel)
		att_factor /= (attenuation * distance * distance + 1.f); 

	return
		intensity * color_factor * att_factor;
}

float render_light_data::influence
	(const vector& target) const NOTHROW
		{ return influence((target - position).length()); }

float render_light_data::influence
	(const sphere& target) const NOTHROW
{
	float target_distance =
		(target.position - position).length();

	return influence
		(max(0.f, target_distance  - target.radius));
}	


float render_light_data::point_range() const NOTHROW
{
	if (!enabled) return 0;

	// Rationale: Let influence() = influence_epsilon,
	// then solve for corresponding distance.

	float color_factor =
		(ambient_color + diffuse_color + specular_color).component_sum();

	float att_factor =
		influence_epsilon / (intensity * color_factor);

	float distance_squared =
		fabsf((1.f - att_factor) / (att_factor * attenuation));

	return sqrtf(distance_squared);
}




//////////////////////////////////////////////////////////////////////////
//
// render_puppet_data
//
//////////////////////////////////////////////////////////////////////////


render_puppet_data::render_puppet_data()
	:
	scale(1),
	visible(true),
	shadow_cast_enabled(false)
{ }


render_puppet_data::render_puppet_data
	(const vector& position,
	 const rotation& orientation,
	 float scale,
	 const shared_pointer<render_model_id>& model,
	 auto_pointer<skeleton_transform> relative_pose,
	 const material_map& materials,
	 bool visible,
	 bool shadow_cast_enabled)
	:
	position(position),
	orientation(orientation),
	scale(scale),
	model(model),
	relative_pose(std::move(relative_pose)),
	materials(materials),
	visible(visible),
	shadow_cast_enabled(shadow_cast_enabled)
{ }


render_puppet_data::render_puppet_data
	(const render_puppet_data& other)
	:
	covert_data(other),
	position(other.position),
	orientation(other.orientation),
	scale(other.scale),
	model(other.model),
	relative_pose(other.relative_pose ?
		 other.relative_pose->clone() : auto_pointer<skeleton_transform>()),
	materials(other.materials),
	visible(other.visible),
	shadow_cast_enabled(other.shadow_cast_enabled)
{ }


auto_pointer<covert_data> render_puppet_data::clone() const
	{ return auto_pointer<covert_data>(new render_puppet_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_stack_data
//
//////////////////////////////////////////////////////////////////////////


render_stack_data::render_stack_data() { }


render_stack_data::render_stack_data
	(const puppet_array& puppets,
	 const vector& anchor_hint)
	:
	puppets(puppets),
	anchor_hint(anchor_hint)
{ }


auto_pointer<covert_data> render_stack_data::clone() const
	{ return auto_pointer<covert_data>(new render_stack_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_scene_data
//
//////////////////////////////////////////////////////////////////////////


render_scene_data::render_scene_data() { }


render_scene_data::render_scene_data
	(const object_set& objects)
	:
	objects(objects)
{ }


auto_pointer<covert_data> render_scene_data::clone() const
	{ return auto_pointer<covert_data>(new render_scene_data(*this)); }




} /* namespace viral_core */
