/**
 *************************************************************************
 *
 * @file gl3_kernel_render.cpp
 *
 * Rendering of scene objects in OpenGL3 kernel, implementation.
 *
 ************************************************************************/


#include "gl3_kernel_render.hpp"


#include "log.hpp"
#include "covert_object.hpp"
#include "gl3_render_target.hpp"
#include "gl3_mesh.hpp"
#include "gl3_resource.hpp"
#include "gl3_scene.hpp"
#include "gl3_kernel_scene.hpp"
#include "gl3_kernel_shadow.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel_render
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel_render::gl3_kernel_render
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3,
	 const gl3_kernel_scene& kernel_scene,
	 const gl3_kernel_shadow& kernel_shadow)
	:
	object_manager_(object_manager),
	gl3_(gl3),
	kernel_scene_(kernel_scene),
	kernel_shadow_(kernel_shadow),
	kernel_state_(object_manager_, gl3_),
	kernel_light_(kernel_scene_, kernel_shadow_)
{ }


gl3_kernel_render::~gl3_kernel_render() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_kernel_render::render_scene
	(const camera& cam,
	 gl3_render_target& target,
	 const rect2f& viewport)
{
	apply_render_target(target, viewport);
	apply_camera(cam);

	render_pass_opaque();
	render_pass_blend();
}


void gl3_kernel_render::apply_render_target
	(gl3_render_target& target, const rect2f& viewport)
{
	gl3_frame_buffer::unbind
		(gl3_frame_buffer::binding_read, gl3_);

	target.bind_color_draw();
	target.set_viewport(viewport);
}


void gl3_kernel_render::apply_camera(const camera& cam)
{
	cam_ = cam;
	frustum_ = cam_.build_frustum();
	kernel_state_.set_camera(cam);
}


void gl3_kernel_render::render_pass_opaque()
{
	int64 opaque_object_count =
		kernel_scene_.opaque_object_count();

	for (int64 i = 0; i < opaque_object_count; ++i)
	{
		const gl3_scene_object_tag& object =
			kernel_scene_.opaque_object(i);

		if (typeid(object) == typeid(gl3_puppet))
		{
			render_puppet
				(static_cast<const gl3_puppet&>(object),
				 gl3_kernel_pass::opaque);
		}
		else
		{
			LOG_ERROR("Opaque objects contain unsupported object.");
			throw assert_failed();
		}
	}
}


void gl3_kernel_render::render_pass_blend()
{
	int64 blend_object_count =
		kernel_scene_.blend_object_count();

	for (int64 i = 0; i < blend_object_count; ++i)
	{
		const gl3_scene_object_tag& object =
			kernel_scene_.blend_object(i);

		if (typeid(object) == typeid(gl3_puppet))
		{
			render_puppet
				(static_cast<const gl3_puppet&>(object),
				 gl3_kernel_pass::blend);
		}
		else if (typeid(object) == typeid(gl3_stack))
		{
			render_stack
				(static_cast<const gl3_stack&>(object));
		}
		else
		{
			LOG_ERROR("Blended objects contain unsupported object.");
			throw assert_failed();
		}
	}
}


void gl3_kernel_render::render_puppet
	(const gl3_puppet& puppet,
	 gl3_kernel_pass_type pass)
{
	// Fetch puppet data and puppet model.
	const gl3_puppet_data& puppet_data =
		puppet.puppet_data();

	if (!puppet_data.model)
	{
		LOG_ERROR("Encountered gl3_puppet without associated model.");
		throw null_pointer();
	}

	const gl3_model& model =
		object_manager_.object_for_id<gl3_model>
			(puppet_data.model);


	// Basic visibility tests.
	if (!puppet_data.visible)
		return;

	if (frustum_.is_definitely_outside
			(sphere(puppet_data.position,
					puppet_data.scale * model.bounding_radius())))
						return;


	// Apply puppet-wide rendering parameters.
	kernel_state_.set_modelview
		(puppet_data.position,
		 puppet_data.orientation,
		 puppet_data.scale);

	kernel_state_.set_skeleton
		(model.skeleton_rig(),
		 puppet_data.relative_pose.get());


	// Render individual model groups
	// in the model of the puppet.
	bool first_lit_group = true;

	for (gl3_model::group_iterator group_it(model.first_group());
		 group_it; ++group_it)
	{
		if (!(*group_it))
		{
			LOG_ERROR("There is a gl3_model with a NULL gl3_model_group.");
			throw null_pointer();
		}

		const gl3_model_group& group = **group_it;


		// Fetch material of this model group.
		// Early out on invisible materials.
		render_puppet_data::material_map::const_iterator material_it
			(puppet_data.materials.find(group.name));

		if (!material_it || !material_it->value)
		{
			LOG_ERROR("Encountered a gl3_model_group: " + group.name +
					  " without material assignment.");
			throw invalid_state();
		}

		const gl3_material& material =
			object_manager_.object_for_id<gl3_material>
				(material_it->value);
		const gl3_material_data& material_data =
			material.material_data();

		if (!material_data.visible)
			continue;


		// A puppet may be rendered over multiple passes,
		// so skip groups that belong to another pass.
		bool render_group_in_pass =
			(pass == gl3_kernel_pass::opaque &&
				material_data.blend == render_material_data::blend_opaque) ||
			(pass == gl3_kernel_pass::blend &&
				material_data.blend != render_material_data::blend_opaque) ||
			 pass == gl3_kernel_pass::stack;

		if (!render_group_in_pass)
			continue;


		// Render unlit groups directly,
		// do not apply lights or shadow maps.
		if (material_data.unlit)
		{
			kernel_state_.apply_material
				(material, pass, &group.mesh, &model, &puppet);

			group.mesh.render();			
			continue;
		}


		// Render lit groups with full lighting uniforms:
		// - Collect relevant lights on first lit model group,
		//		and reuse relevant lights on all later groups.
		// - Render initial lighting pass regardless of blending.
		// - Multipass additional lights for opaque objects only.
		// - Skip multipass on all puppets in a render stack:
		//		For these, we cannot differ between opaque and
		//		translucent materials based on pass type.
		if (first_lit_group)
		{
			kernel_light_.select_relevant_lights
				(sphere(puppet_data.position, model.bounding_radius()));

			first_lit_group = false;
		}

		gl3_kernel_pass_type material_pass = pass;

		kernel_light_.init_pending_lights();
		while (kernel_light_.has_pending_lights())
		{
			kernel_light_.select_pass_lights
				(pass == gl3_kernel_pass::opaque);

			kernel_state_.set_lights
				(kernel_scene_,
				 kernel_light_.pass_light_indices(),
				 kernel_light_.pass_shadow_enabled());
			kernel_state_.set_shadow_maps
				(kernel_shadow_,
				 kernel_light_.pass_shadow_map_point_index(),
				 kernel_light_.pass_shadow_map_spot_index(),
				 kernel_light_.pass_shadow_map_parallel_index());

			kernel_state_.apply_material
				(material, material_pass,
				 &group.mesh, &model, &puppet);

			group.mesh.render();

			if (pass != gl3_kernel_pass::opaque)
				break;

			material_pass =
				gl3_kernel_pass::opaque_multipass_light;
		}
	}
}


void gl3_kernel_render::render_stack
	(const gl3_stack& stack)
{
	const render_stack_data& stack_data = stack.stack_data();

	for (int64 i = 0; i < stack_data.puppets.size(); ++i)
		render_puppet
			(object_manager_.object_for_id<gl3_puppet>
				(stack_data.puppets[i]),
			 gl3_kernel_pass::stack);
}




} /* namespace viral_core */
