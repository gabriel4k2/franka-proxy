/**
 *************************************************************************
 *
 * @file gl3_kernel_light.cpp
 *
 * Per-pass light management in OpenGL3 kernel, implementation.
 *
 ************************************************************************/


#include "gl3_kernel_light.hpp"


#include <algorithm>


#include "log.hpp"
#include "render_scene.hpp"
#include "gl3_scene.hpp"
#include "gl3_kernel_scene.hpp"
#include "gl3_kernel_shadow.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel_light
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel_light::gl3_kernel_light
	(const gl3_kernel_scene& kernel_scene,
	 const gl3_kernel_shadow& kernel_shadow)
	:
	kernel_scene_(kernel_scene),
	kernel_shadow_(kernel_shadow),

	pass_shadow_map_point_index_
		(gl3_kernel_shadow::no_shadow_map_assigned),
	pass_shadow_map_spot_index_
		(gl3_kernel_shadow::no_shadow_map_assigned),
	pass_shadow_map_parallel_index_
		(gl3_kernel_shadow::no_shadow_map_assigned)
{ }


gl3_kernel_light::~gl3_kernel_light() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_kernel_light::select_relevant_lights
	(const sphere& bounding_sphere)
{
	relevant_light_indices_.clear();

	int64 light_count = kernel_scene_.light_count();
	for (int64 i = 0; i < light_count; ++i)
	{
		const render_light_data& light_data =
			kernel_scene_.light(i).light_data();

		float influence =
			light_data.influence(bounding_sphere);

		if (influence <= light_data.influence_epsilon)
			continue; // light too far or disabled

		relevant_light_indices_.push_back
			(gl3_kernel_priority_index(i, influence));
	}

	// We sort by ascending priority to minimize
	// the costs of later remove_ordered() calls
	// in select_pass_lights().
	std::sort
		(relevant_light_indices_.data(),
		 relevant_light_indices_.data() + relevant_light_indices_.size(),
		 &gl3_kernel_priority_index::sort_ascending);
}


void gl3_kernel_light::init_pending_lights()
{
	pending_light_indices_.clear();
	for (int64 i = 0; i < relevant_light_indices_.size(); ++i)
		pending_light_indices_.push_back
			(relevant_light_indices_[i].index);
}


bool gl3_kernel_light::has_pending_lights()
	{ return !pending_light_indices_.empty(); }


void gl3_kernel_light::select_pass_lights
	(bool multipass_shadows)
{
	pass_light_indices_.clear();
	pass_shadow_enabled_.clear();

	pass_shadow_map_point_index_ = gl3_kernel_shadow::no_shadow_map_assigned;
	pass_shadow_map_spot_index_ = gl3_kernel_shadow::no_shadow_map_assigned;
	pass_shadow_map_parallel_index_ = gl3_kernel_shadow::no_shadow_map_assigned;

	for (int64 i = pending_light_indices_.size() - 1; i >= 0; --i)
	{
		if (pass_light_indices_.size() >= gl3_kernel_limits::max_pass_lights)
			break;

		int64 light_index = pending_light_indices_[i];
		const render_light_data& light_data =
			kernel_scene_.light(light_index).light_data();


		// Check light against available shadow map slots.
		bool pass_shadow_enabled = false;
		int64 shadow_map_index = 
			kernel_shadow_.shadow_map_index(light_index);

		if (shadow_map_index != gl3_kernel_shadow::no_shadow_map_assigned)
		{
			int64* pass_shadow_map_slot =
				(light_data.emitter == render_light_data::emitter_point) ?
					&pass_shadow_map_point_index_ :
				(light_data.emitter == render_light_data::emitter_spot) ?
					&pass_shadow_map_spot_index_ :
				(light_data.emitter == render_light_data::emitter_parallel) ?
					&pass_shadow_map_parallel_index_ :
					NULL;

			if (!pass_shadow_map_slot)
			{
				LOG_ERROR("Unknown light emitter type for shadow map.");
				throw assert_failed();
			}

			if (*pass_shadow_map_slot == gl3_kernel_shadow::no_shadow_map_assigned)
			{
				*pass_shadow_map_slot = shadow_map_index;
				pass_shadow_enabled = true;
			}
			else if (multipass_shadows)
			{
				// We have no free shadow map slot,
				// so defer light to later pass.
				continue;
			}

			// We have no free shadow map slot
			// and we cannot multipass shadows,
			// so pass_shadow_enabled remains false.
		}


		// Light should be used in this pass,
		// but might have its shadow disabled.
		pass_light_indices_.push_back(light_index);
		pass_shadow_enabled_.push_back(pass_shadow_enabled);
		pending_light_indices_.remove_ordered(i);
	}

	if (!pending_light_indices_.empty() &&
		pass_light_indices_.empty())
	{
		// Avoid infinite loop on multipass rendering.
		// Safety precaution, should never happen.
		LOG_ERROR("Failed to find any lights for current pass.");
		throw assert_failed();
	}
}


const dynamic_buffer<int64>&
	gl3_kernel_light::pass_light_indices() const NOTHROW
		{ return pass_light_indices_; }

const dynamic_buffer<bool>&
	gl3_kernel_light::pass_shadow_enabled() const NOTHROW
		{ return pass_shadow_enabled_; }


int64 gl3_kernel_light::pass_shadow_map_point_index() const NOTHROW
	{ return pass_shadow_map_point_index_; }
int64 gl3_kernel_light::pass_shadow_map_spot_index() const NOTHROW
	{ return pass_shadow_map_spot_index_; }
int64 gl3_kernel_light::pass_shadow_map_parallel_index() const NOTHROW
	{ return pass_shadow_map_parallel_index_; }




} /* namespace viral_core */
