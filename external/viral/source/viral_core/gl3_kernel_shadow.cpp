/**
 *************************************************************************
 *
 * @file gl3_kernel_shadow.cpp
 *
 * Management of shadow maps in OpenGL3 kernel, implementation.
 *
 ************************************************************************/


#include "gl3_kernel_shadow.hpp"


#include <algorithm>


#include "log.hpp"
#include "geo_util.hpp"
#include "geo_camera.hpp"
#include "render_scene.hpp"
#include "gl3_shadow_map.hpp"
#include "gl3_resource.hpp"
#include "gl3_scene.hpp"
#include "gl3_kernel_scene.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_kernel_shadow
//
//////////////////////////////////////////////////////////////////////////


gl3_kernel_shadow::gl3_kernel_shadow
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3,
	 const gl3_kernel_scene& kernel_scene)
	:
	object_manager_(object_manager),
	gl3_(gl3),
	kernel_scene_(kernel_scene),
	kernel_state_(object_manager_, gl3_),
	shadow_map_point_used_(0),
	shadow_map_spot_used_(0),
	shadow_map_parallel_used_(0)
{ }


gl3_kernel_shadow::~gl3_kernel_shadow() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void gl3_kernel_shadow::render_shadow_maps
	(const camera& scene_camera)
{
	// Allocate a shadow map index for each scene light,
	// then clear any preceding shadow map assignment.
	shadow_map_indices_.resize(kernel_scene_.light_count());
	for (int64 i = 0 ; i < shadow_map_indices_.size(); ++i)
		shadow_map_indices_[i] = no_shadow_map_assigned;


	// Assign shadow maps to lights by shadow priority
	// and render any assigned shadow maps.
	shadow_map_point_used_ = 0;
	shadow_map_spot_used_ = 0;
	shadow_map_parallel_used_ = 0;

	int64 shadow_enabled_light_count =
		kernel_scene_.shadow_enabled_light_count();

	for (int64 i = 0; i < shadow_enabled_light_count; ++i)
	{
		int64 light_index =
			kernel_scene_.shadow_enabled_light_index(i);
		const render_light_data& light_data =
			kernel_scene_.light(i).light_data();

		if (!light_data.enabled ||
			!light_data.shadow_enabled) continue;

		if (light_data.emitter == render_light_data::emitter_point)
		{
			if (shadow_map_point_used_ >= gl3_kernel_limits::max_shadows_point)
				continue;

			render_shadow_map_point
				(light_data, assign_shadow_map_point(light_index));
		}

		else if (light_data.emitter == render_light_data::emitter_spot)
		{
			if (shadow_map_spot_used_ >= gl3_kernel_limits::max_shadows_spot)
				continue;

			render_shadow_map_spot
				(light_data, assign_shadow_map_spot(light_index));
		}

		else if (light_data.emitter == render_light_data::emitter_parallel)
		{
			if (shadow_map_parallel_used_ >= gl3_kernel_limits::max_shadows_parallel)
				continue;

			render_shadow_map_parallel
				(light_data,
				 assign_shadow_map_parallel(light_index),
				 scene_camera);
		}

		else
		{
			LOG_ERROR("Found unsupported light emitter type.");
			throw invalid_state();
		}
	}
}


int64 gl3_kernel_shadow::shadow_map_index(int64 light_index) const
{
	if (light_index < 0 || light_index >= shadow_map_indices_.size())
	{
		LOG_ERROR("Invalid light index for shadow map query.");
		throw invalid_parameter();
	}

	return shadow_map_indices_[light_index];
}


const gl3_shadow_map_point&
	gl3_kernel_shadow::shadow_map_point(int64 index) const
{
	if (index < 0 || index >= shadow_maps_point_.size())
	{
		LOG_ERROR("Invalid index for point shadow map.");	
		throw invalid_parameter();
	}

	if (!shadow_maps_point_[index])
	{
		LOG_ERROR("NULL pointer in array of point shadow maps.");
		throw assert_failed();
	}

	return *shadow_maps_point_[index];
}


const gl3_shadow_map_spot&
	gl3_kernel_shadow::shadow_map_spot(int64 index) const
{
	if (index < 0 || index >= shadow_maps_spot_.size())
	{
		LOG_ERROR("Invalid index for spot shadow map.");	
		throw invalid_parameter();
	}

	if (!shadow_maps_spot_[index])
	{
		LOG_ERROR("NULL pointer in array of spot shadow maps.");
		throw assert_failed();
	}

	return *shadow_maps_spot_[index];
}


const gl3_shadow_map_parallel&
	gl3_kernel_shadow::shadow_map_parallel(int64 index) const
{
	if (index < 0 || index >= shadow_maps_parallel_.size())
	{
		LOG_ERROR("Invalid index for parallel shadow map.");	
		throw invalid_parameter();
	}

	if (!shadow_maps_parallel_[index])
	{
		LOG_ERROR("NULL pointer in array of parallel shadow maps.");
		throw assert_failed();
	}

	return *shadow_maps_parallel_[index];
}


int64 gl3_kernel_shadow::shadow_map_point_count() const NOTHROW
	{ return shadow_maps_point_.size(); }
int64 gl3_kernel_shadow::shadow_map_spot_count() const NOTHROW
	{ return shadow_maps_spot_.size(); }
int64 gl3_kernel_shadow::shadow_map_parallel_count() const NOTHROW
	{ return shadow_maps_parallel_.size(); }


int64 gl3_kernel_shadow::shadow_map_point_used() const NOTHROW
	{ return shadow_map_point_used_; }
int64 gl3_kernel_shadow::shadow_map_spot_used() const NOTHROW
	{ return shadow_map_spot_used_; }
int64 gl3_kernel_shadow::shadow_map_parallel_used() const NOTHROW
	{ return shadow_map_parallel_used_; }


gl3_shadow_map_point&
	gl3_kernel_shadow::assign_shadow_map_point
		(int64 light_index)
{
	while (shadow_maps_point_.size() <= shadow_map_point_used_)
	{
		auto_pointer<gl3_shadow_map_point> shadow_map_point
			(new gl3_shadow_map_point
				(gl3_,
				 gl3_kernel_limits::shadow_map_resolution));

		shadow_maps_point_.insert
			(std::move(shadow_map_point));
	}

	++shadow_map_point_used_;

	shadow_map_indices_[light_index] = shadow_map_point_used_ - 1;
	return *shadow_maps_point_[shadow_map_point_used_ - 1];
}


gl3_shadow_map_spot&
	gl3_kernel_shadow::assign_shadow_map_spot
		(int64 light_index)
{
	while (shadow_maps_spot_.size() <= shadow_map_spot_used_)
	{
		auto_pointer<gl3_shadow_map_spot> shadow_map_spot
			(new gl3_shadow_map_spot
				(gl3_,
				 gl3_kernel_limits::shadow_map_resolution));

		shadow_maps_spot_.insert
			(std::move(shadow_map_spot));
	}

	++shadow_map_spot_used_;

	shadow_map_indices_[light_index] = shadow_map_spot_used_ - 1;
	return *shadow_maps_spot_[shadow_map_spot_used_ - 1];
}


gl3_shadow_map_parallel&
	gl3_kernel_shadow::assign_shadow_map_parallel
		(int64 light_index)
{
	while (shadow_maps_parallel_.size() <= shadow_map_parallel_used_)
	{
		auto_pointer<gl3_shadow_map_parallel> shadow_map_parallel
			(new gl3_shadow_map_parallel
				(gl3_,
				 gl3_kernel_limits::shadow_map_resolution,
				 gl3_kernel_limits::shadow_map_cascade_count));
	
		shadow_maps_parallel_.insert
			(std::move(shadow_map_parallel));
	}
		
	++shadow_map_parallel_used_;

	shadow_map_indices_[light_index] = shadow_map_parallel_used_ - 1;
	return *shadow_maps_parallel_[shadow_map_parallel_used_ - 1];
}


void gl3_kernel_shadow::render_shadow_map_point
	(const render_light_data& light_data,
	 gl3_shadow_map_point& shadow_map)
{
	// Prepare generic shadow map camera for a single cubemap face.
	// Note that the transform from world space to OpenGL cubemaps
	// is left-handed, and we cannot request right-handed semantics.
	// Instead, our camera must apply a suitable post-scaling factor.
	float point_range = light_data.point_range();
	camera point_camera
		(light_data.position, rotation(),
		 camera::perspective, 90.f, 1.f,
		 point_range * light_data.shadow_clip_near_normalized,
		 point_range * light_data.shadow_clip_far_normalized,
		 vector(), vector(1, -1, 1));

	shadow_map.update_z_transform(point_camera);


	// Render individual cubemap faces.
	// Reuse the same camera for all of these,
	// just change rotation appropriately.
	rotation point_camera_orientations
		[gl3_frame_buffer_attachment_cubemap::cubemap_side_count] =
			{
				rotation(vector(0, 1, 0), 90),
				rotation(vector(1, 0, 0), -90),
				rotation(vector(0, 1, 0), 0),
				rotation(vector(0, 1, 0), -90),
				rotation(vector(1, 0, 0), 90),
				rotation(vector(0, 1, 0), 180)
			};

	for (int64 i = 0;
		 i < gl3_frame_buffer_attachment_cubemap::cubemap_side_count;
		 ++i)
	{
		gl3_frame_buffer_attachment_cubemap::cubemap_side side =
			static_cast<gl3_frame_buffer_attachment_cubemap::cubemap_side>(i);

		point_camera.orientation =
			point_camera_orientations[i];

		shadow_map.prepare_and_bind_target(side);

		apply_shadow_map_camera(point_camera);
		render_pass_shadow_map();
	}
}


void gl3_kernel_shadow::render_shadow_map_spot
	(const render_light_data& light_data,
	 gl3_shadow_map_spot& shadow_map)
{
	float point_range = light_data.point_range();
	camera spot_camera
		(light_data.position, light_data.orientation,
		 camera::perspective, light_data.spot_angle, 1.f,
		 point_range * light_data.shadow_clip_near_normalized,
		 point_range * light_data.shadow_clip_far_normalized);

	shadow_map.update_world_to_texture(spot_camera);
	shadow_map.prepare_and_bind_target();

	apply_shadow_map_camera(spot_camera);
	render_pass_shadow_map();
}


void gl3_kernel_shadow::render_shadow_map_parallel
	(const render_light_data& light_data,
	 gl3_shadow_map_parallel& shadow_map,
	 const camera& scene_camera)
{
	// Determine normalized limits of individual cascade entries.
	// Entry size grows exponentially for perspective cameras,
	// and remains constant for orthographic cameras.
	int64 cascade_count = shadow_map.cascade_count();
	if (cascade_count != gl3_kernel_limits::shadow_map_cascade_count)
	{
		LOG_ERROR("Inconsistent cascade counts.");
		throw assert_failed();
	}

	float cascade_limits
		[gl3_kernel_limits::shadow_map_cascade_count + 1];

	cascade_limits[0] = 0;
	cascade_limits[1] = 1;
	for (int64 i = 2; i < cascade_count + 1; ++i)
	{
		float entry_size =
			((scene_camera.projection == camera::perspective) ? 2 : 1) *
				(cascade_limits[i - 1] - cascade_limits[i - 2]);

		cascade_limits[i] =
			cascade_limits[i - 1] + entry_size;
	}

	for (int64 i = 0; i < cascade_count + 1; ++i)
		cascade_limits[i] /= cascade_limits[cascade_count];
		
	
	// Build cascade entries over the frustum of the scene camera,
	// then find a light-space bounding box over per-entry contents,	
	// and finally render a shadow map for each entry.	
	frustum scene_frustum(scene_camera.build_frustum());
	for (int64 i = 0; i < shadow_map.cascade_count(); ++i)
	{
		static const float shadow_map_cascade_overlap = 0.001f;
		frustum cascade_frustum
			(scene_frustum.near_far_interpolate
				(cascade_limits[i] - shadow_map_cascade_overlap,
				 cascade_limits[i + 1] + shadow_map_cascade_overlap));

		matrix to_light_local
			(matrix::rotate(-light_data.orientation));

		box light_local_bounding_box
			(cascade_frustum.build_bounding_box
				(to_light_local));

		// First build depth extents, then lock to texels.
		// This gives slightly tighter depth extents.
		bool any_shadow_caster_in_range =
			bound_shadow_casters_parallel
				(light_local_bounding_box, to_light_local);
		lock_to_texels_parallel
			(light_local_bounding_box,
			 cascade_frustum, shadow_map.size()); 

		camera parallel_camera
			(light_data.orientation * light_local_bounding_box.mid(),
			 light_data.orientation,
			 camera::orthographic,
			 light_local_bounding_box.size_x(),
			 light_local_bounding_box.size_y() / light_local_bounding_box.size_x(),
			 0, light_local_bounding_box.size_z());

		shadow_map.update_world_to_texture(parallel_camera, i);
		shadow_map.update_valid(any_shadow_caster_in_range, i);

		if (!any_shadow_caster_in_range)
			continue; // Avoid rendering of empty cascade entries.

		shadow_map.prepare_and_bind_target(i);		
		apply_shadow_map_camera(parallel_camera);
		render_pass_shadow_map();
	}
}


bool gl3_kernel_shadow::bound_shadow_casters_parallel
	(box& light_local_bounding_box,
	 const matrix& to_light_local)
{
	/**
	 * Find tight z extents for the @bref{light_local_bounding_box}
	 * of an entry in the shadow map cascade for a parallel light
	 * by transforming all shadow casters @bref{to_light_local} space.
	 * Returns @false if there are no shadow casters in light range,
	 * @true otherwise.
	 *
	 * Implementation notes:
	 * - The @bref{light_local_bounding_box} already has valid,
	 *		light-space xy extents. This enables early rejection
	 *		of potential shadow casters for a better z fit.
	 * - The @bref{to_light_local} transformation only describes
	 *		a rotation to the light plane, and does not contain
	 *		any translation or scale. Notably, @bref{to_light_local}
	 *		transform of bounding spheres does not change radius.
	 */

	light_local_bounding_box.min.z =
		light_local_bounding_box.max.z = 0;

	bool any_shadow_caster_in_range = false;
	int64 shadow_caster_count =
		kernel_scene_.shadow_caster_count();

	for (int64 i = 0; i < shadow_caster_count; ++i)
	{
		const gl3_scene_object_tag& object =
			kernel_scene_.shadow_caster(i);

		float z_min_caster = 0;
		float z_max_caster = 0;

		if (typeid(object) == typeid(gl3_puppet))
		{
			const render_puppet_data& puppet_data =
				static_cast<const gl3_puppet&>(object).puppet_data();

			float bounding_radius =
				object_manager_.object_for_id<gl3_model>
					(puppet_data.model).bounding_radius();

			vector light_local_position
				(to_light_local * puppet_data.position);

			if (light_local_position.x + bounding_radius < light_local_bounding_box.min.x ||
				light_local_position.y + bounding_radius < light_local_bounding_box.min.y ||
				light_local_position.x - bounding_radius > light_local_bounding_box.max.x ||
				light_local_position.y - bounding_radius > light_local_bounding_box.max.y)
					continue; // Object outside cascade entry.

			z_min_caster = light_local_position.z - bounding_radius;
			z_max_caster = light_local_position.z + bounding_radius;
		}
		else
		{
			LOG_ERROR("Shadow casters contain unsupported object.");
			throw assert_failed();
		}

		if (!any_shadow_caster_in_range)
		{
			light_local_bounding_box.min.z = z_min_caster;
			light_local_bounding_box.max.z = z_max_caster;
			any_shadow_caster_in_range = true;
		}
		else
		{
			light_local_bounding_box.min.z =
				min(light_local_bounding_box.min.z, z_min_caster);
			light_local_bounding_box.max.z =
				max(light_local_bounding_box.max.z, z_max_caster);
		}
	}

	return any_shadow_caster_in_range;
}


void gl3_kernel_shadow::lock_to_texels_parallel
	(box& light_local_bounding_box,
	 const frustum& cascade_frustum,
	 int64 shadow_map_size)
{
	/**
	 * Lock a @bref{light_local_bounding_box} to shadow map texels.
	 * Find the light-space texel size from @bref{shadow_map_size}
	 * and a bounding sphere around @bref{cascade_frustum}.
	 *
	 * Without lock_to_texels_parallel(), camera movement causes
	 * shimmering shadow borders due to limited numerical precision 
	 * in the mapping of camera-dependent cascaded shadow maps
	 * to world space fragments.
	 */


	// Find frustum bounding sphere.
	vector frustum_center;
	for (int64 i = 0; i < frustum::vertex_count; ++i)
		frustum_center += cascade_frustum.vertices[i];
	frustum_center /= frustum::vertex_count;

	float bounding_radius = 0;
	for (int64 i = 0; i < frustum::vertex_count; ++i)
	{
		vector center_to_vertex
			(cascade_frustum.vertices[i] - frustum_center);
		bounding_radius =
			max(bounding_radius, center_to_vertex.length());
	}


	// Determine light space texel size.
	// We use an additional pixel as tolerance
	// on each side of the later shadow map.
	float bounding_diameter = 2 * bounding_radius;
	float texel_size =
		bounding_diameter / max(shadow_map_size - 2, 1);


	// Lock frustum bounding box to texel extents
	// in light-local XY plane, maintain Z extents.
	vector former_mid(light_local_bounding_box.mid());
	vector texel_locked_center
		(grid_math::round_to_grid(former_mid.x, texel_size),
		 grid_math::round_to_grid(former_mid.y, texel_size),
		 former_mid.z);

	vector new_size
		(shadow_map_size * texel_size,
		 shadow_map_size * texel_size,
		 light_local_bounding_box.max.z - light_local_bounding_box.min.z);

	light_local_bounding_box =
		box(texel_locked_center - new_size * 0.5f,
			texel_locked_center + new_size * 0.5f);
}


void gl3_kernel_shadow::apply_shadow_map_camera
	(const camera& shadow_map_camera)
{
	shadow_map_camera_ = shadow_map_camera;
	shadow_map_frustum_ = shadow_map_camera.build_frustum();
	kernel_state_.set_camera(shadow_map_camera);
}


void gl3_kernel_shadow::render_pass_shadow_map()
{
	int64 shadow_caster_count =
		kernel_scene_.shadow_caster_count();

	for (int64 i = 0; i < shadow_caster_count; ++i)
	{
		const gl3_scene_object_tag& object =
			kernel_scene_.shadow_caster(i);

		if (typeid(object) == typeid(gl3_puppet))
		{
			render_puppet
				(static_cast<const gl3_puppet&>(object));
		}
		else if (typeid(object) == typeid(gl3_stack))
		{
			render_stack
				(static_cast<const gl3_stack&>(object));
		}
		else
		{
			LOG_ERROR("Shadow caster array contains unsupported object.");
			throw assert_failed();
		}
	}
}


void gl3_kernel_shadow::render_puppet
	(const gl3_puppet& puppet)
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

	if (shadow_map_frustum_.is_definitely_outside
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


	// Render all individual model groups
	// in the model of the puppet.
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
		// Abort on materials that do not cast shadows,
		// which may still be part of shadow casters.
		render_puppet_data::material_map::const_iterator material_it
			(puppet_data.materials.find(group.name));

		if (!material_it || !material_it->value)
		{
			LOG_ERROR("Encountered a model group: " + group.name +
					  " without material assignment.");
			throw invalid_state();
		}

		const gl3_material& material =
			object_manager_.object_for_id<gl3_material>
				(material_it->value);
		const gl3_material_data& material_data =
			material.material_data();

		if (!material_data.shadow_cast_enabled)
			continue;

		
		// Apply material parameters for shadow map pass,
		// then render the mesh of the model group.
		kernel_state_.apply_material
			(material, gl3_kernel_pass::shadow_map,
			 &group.mesh, &model, &puppet);

		group.mesh.render();
	}
}


void gl3_kernel_shadow::render_stack
	(const gl3_stack& stack)
{
	const render_stack_data& stack_data = stack.stack_data();

	for (int64 i = 0; i < stack_data.puppets.size(); ++i)
	{
		gl3_puppet& puppet =
			object_manager_.object_for_id<gl3_puppet>
				(stack_data.puppets[i]);

		render_puppet(puppet);
	}
}




} /* namespace viral_core */

