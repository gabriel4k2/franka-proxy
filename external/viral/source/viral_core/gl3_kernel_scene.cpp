/**
 *************************************************************************
 *
 * @file gl3_kernel_scene.cpp
 *
 * Scene parsing in OpenGL3 kernel, implementation.
 *
 ************************************************************************/


#include "gl3_kernel_scene.hpp"


#include <algorithm>


#include "log.hpp"
#include "render_scene.hpp"
#include "gl3_resource.hpp"
#include "gl3_scene.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel_scene
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel_scene::gl3_kernel_scene
	(const covert_object_manager& object_manager)
	:
	object_manager_(object_manager)
{ }


gl3_kernel_scene::~gl3_kernel_scene() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_kernel_scene::parse_scene
	(const gl3_scene& scene, const camera& cam)
{
	clear();
	apply_camera(cam);
	parse_scene_objects(scene);
	sort_objects();
}


void gl3_kernel_scene::apply_camera(const camera& cam)
{
	cam_ = cam;
	frustum_ = cam.build_frustum();
}


void gl3_kernel_scene::parse_scene_objects
	(const gl3_scene& scene)
{
	for (render_scene_data::object_set::const_iterator it
			(scene.scene_data().objects.first()); it; ++it)
	{
		if (!(*it))
		{
			LOG_ERROR("Scene contains NULL object id.");
			throw null_pointer();
		}

		if (typeid(**it) == typeid(render_light_id))
		{
			const gl3_light& light =
				object_manager_.object_for_id<gl3_light>
					(it->try_cast_exact<render_light_id>());

			parse_light(light);
		}
		else if (typeid(**it) == typeid(render_puppet_id))
		{
			const gl3_puppet& puppet =
				object_manager_.object_for_id<gl3_puppet>
					(it->try_cast_exact<render_puppet_id>());

			parse_puppet(puppet);
		}
		else if (typeid(**it) == typeid(render_stack_id))
		{
			const gl3_stack& stack =
				object_manager_.object_for_id<gl3_stack>
					(it->try_cast_exact<render_stack_id>());

			parse_stack(stack);
		}
		else
		{
			LOG_ERROR("Encountered scene object of unknown type.");
			throw unsupported_feature();
		}
	}
}


const gl3_light& gl3_kernel_scene::light(int64 index) const
{
	if (index < 0 || index >= lights_.size())
	{
		LOG_ERROR("Invalid index for scene light.");
		throw invalid_parameter();
	}

	if (!lights_[index])
	{
		LOG_ERROR("Light buffer contains NULL pointer.");
		throw assert_failed();
	}

	return *lights_[index];
}

int64 gl3_kernel_scene::light_count() const NOTHROW
	{ return lights_.size(); }


const gl3_light& gl3_kernel_scene::shadow_enabled_light(int64 index) const
{
	if (index < 0 || index >= shadow_enabled_light_indices_.size())
	{
		LOG_ERROR("invalid index for shadow enabled light.");
		throw invalid_parameter();
	}

	return light(shadow_enabled_light_indices_[index].index);
}

int64 gl3_kernel_scene::shadow_enabled_light_count() const NOTHROW
	{ return shadow_enabled_light_indices_.size(); }


const gl3_scene_object_tag& gl3_kernel_scene::renderable_object(int64 index) const
{
	if (index < 0 || index >= renderable_objects_.size())
	{
		LOG_ERROR("Invalid index for scene object.");
		throw invalid_parameter();
	}

	if (!renderable_objects_[index])
	{
		LOG_ERROR("Render object buffer contains NULL pointer.");
		throw invalid_parameter();
	}

	return *renderable_objects_[index];
}

int64 gl3_kernel_scene::renderable_object_count() const NOTHROW
	{ return renderable_objects_.size(); }


const gl3_scene_object_tag& gl3_kernel_scene::opaque_object(int64 index) const
{
	if (index < 0 || index >= opaque_object_indices_.size())
	{
		LOG_ERROR("Invalid index for opaque object.");
		throw invalid_parameter();
	}

	return renderable_object(opaque_object_indices_[index].index);
}

int64 gl3_kernel_scene::opaque_object_count() const NOTHROW
	{ return opaque_object_indices_.size(); }


const gl3_scene_object_tag& gl3_kernel_scene::blend_object(int64 index) const
{
	if (index < 0 || index >= blend_object_indices_.size())
	{
		LOG_ERROR("Invalid index for opaque object.");
		throw invalid_parameter();
	}

	return renderable_object(blend_object_indices_[index].index);
}

int64 gl3_kernel_scene::blend_object_count() const NOTHROW
	{ return blend_object_indices_.size(); }


const gl3_scene_object_tag& gl3_kernel_scene::shadow_caster(int64 index) const
{
	if (index < 0 || index >= shadow_caster_indices_.size())
	{
		LOG_ERROR("Invalid index for shadow caster.");
		throw invalid_parameter();
	}

	return renderable_object(shadow_caster_indices_[index]);
}

int64 gl3_kernel_scene::shadow_caster_count() const NOTHROW
	{ return shadow_caster_indices_.size(); }


int64 gl3_kernel_scene::shadow_enabled_light_index(int64 index) const
{
	if (index < 0 || index >= shadow_enabled_light_indices_.size())
	{
		LOG_ERROR("Invalid index for shadow enabled light.");
		throw invalid_parameter();
	}

	return shadow_enabled_light_indices_[index].index;
}


void gl3_kernel_scene::parse_light
	(const gl3_light& light)
{
	/**
	 * Add all lights to scene-wide light array,
	 * and separately track shadow-enabled lights
	 * for later shadow map rendering.
	 *
	 * We skip any disabled or out-of-frustum lights
	 * to remove them from light passes and shadow maps
	 * as early as possible.
	 */

	const render_light_data& light_data =
		light.light_data();

	if (!light_data.enabled)
		return;

	if (light_data.emitter == render_light_data::emitter_point ||
		light_data.emitter == render_light_data::emitter_spot)
	{
		sphere light_bounding_sphere
			(light_data.position, light_data.point_range());

		if (frustum_.is_definitely_outside(light_bounding_sphere))
			return;
	}

	lights_.push_back(&light);

	if (light_data.shadow_enabled)
		shadow_enabled_light_indices_.push_back
			(gl3_kernel_priority_index
				(lights_.size() - 1, light_data.shadow_priority));
}


void gl3_kernel_scene::parse_puppet
	(const gl3_puppet& puppet)
{
	/**
	 * Place a puppet into suitable rendering passes:
	 * shadow casting pass, opaque pass, blend pass,
	 * or a combination of these, depending on puppet
	 * and material properties.
	 *
	 * We error out on any missing material assignments.
	 * Otherwise, such puppets would not be in any pass,
	 * which in turn prevents client-side error handling.
	 */

	const render_puppet_data& puppet_data =
		puppet.puppet_data();

	bool has_opaque_group = false;
	bool has_blend_group = false;
	bool has_shadow_cast_group = false;

	if (puppet_data.materials.empty())
	{
		LOG_ERROR("Found puppet without any material assignment.");
		throw invalid_state();
	}

	for (render_puppet_data::material_map::const_iterator it
			(puppet_data.materials.first()); it; ++it)
	{
		if (!it->value)
		{
			LOG_ERROR("Found puppet with NULL material id.");
			throw null_pointer();
		}

		const gl3_material& material =
			object_manager_.object_for_id<gl3_material>(it->value);

		const gl3_material_data& material_data =
			material.material_data();

		if (puppet_data.visible &&
			material_data.visible)
		{
			if (material_data.blend == render_material_data::blend_opaque)
				has_opaque_group = true;
			else
				has_blend_group = true;
		}

		if (puppet_data.shadow_cast_enabled &&
			material_data.shadow_cast_enabled)
				has_shadow_cast_group = true;
	}

	if (!has_opaque_group &&
		!has_blend_group &&
		!has_shadow_cast_group)
			return;	

	renderable_objects_.push_back(&puppet);

	int64 object_index = renderable_objects_.size() - 1;
	float object_distance =
		(puppet_data.position - cam_.position).length();
	 
	if (has_opaque_group)
		opaque_object_indices_.push_back
			(gl3_kernel_priority_index(object_index, object_distance));

	if (has_blend_group)
		blend_object_indices_.push_back
			(gl3_kernel_priority_index(object_index, object_distance));

	if (has_shadow_cast_group)
		shadow_caster_indices_.push_back(object_index);
}


void gl3_kernel_scene::parse_stack
	(const gl3_stack& stack)
{
	/**
	 * Always place stacks in blending pass to ensure
	 * that contained puppets render in given order.
	 *
	 * This prohibits multipass lighting on opaque stack elements,
	 * but stacks anyways should only be used for blended content.
	 */

	renderable_objects_.push_back(&stack);

	int64 object_index = renderable_objects_.size() - 1;
	float object_distance =
		(stack.stack_data().anchor_hint - cam_.position).length();

	blend_object_indices_.push_back
		(gl3_kernel_priority_index(object_index, object_distance));
}


void gl3_kernel_scene::sort_objects()
{
	std::sort
		(shadow_enabled_light_indices_.data(),
		 shadow_enabled_light_indices_.data() +
			shadow_enabled_light_indices_.size(),
		 &gl3_kernel_priority_index::sort_descending);

	std::sort
		(opaque_object_indices_.data(),
		 opaque_object_indices_.data() +
			opaque_object_indices_.size(),
		 &gl3_kernel_priority_index::sort_ascending);

	std::sort
		(blend_object_indices_.data(),
		 blend_object_indices_.data() +
			blend_object_indices_.size(),
		 &gl3_kernel_priority_index::sort_descending);
}


void gl3_kernel_scene::clear() NOTHROW
{
	lights_.clear();
	shadow_enabled_light_indices_.clear();

	renderable_objects_.clear();
	opaque_object_indices_.clear();
	blend_object_indices_.clear();
	shadow_caster_indices_.clear();
}




} /* namespace viral_core */
