/**
 *************************************************************************
 *
 * @file gl3_resource.cpp
 *
 * Large-scale backend objects of OpenGL3, implementation.
 *
 ************************************************************************/


#include "gl3_resource.hpp"


#include <typeinfo>


#include "log.hpp"
#include "gl3_texture_canvas.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_texture
//
//////////////////////////////////////////////////////////////////////////


const gl3_texture_object& gl3_texture::texture_object() const
{
	if (!texture_object_)
	{
		LOG_ERROR("No internal texture object assigned.\r\n"
				  "Please commit appropriate render_texture_data.");
		throw invalid_state();
	}

	return *texture_object_;
}


gl3_texture::gl3_texture
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_texture::set_data(auto_pointer<covert_data> data)
{
	// Validate incoming data.
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	if (typeid(*data) != typeid(render_texture_2d_data) &&
		typeid(*data) != typeid(render_texture_cubemap_data) &&
		typeid(*data) != typeid(render_texture_canvas_data))
	{
		LOG_ERROR("Incoming data has invalid type.");
		throw type_mismatch();
	}


	// Create offhand texture object.
	auto_pointer<gl3_texture_object> new_object;

	if (typeid(*data) == typeid(render_texture_2d_data))
	{
		render_texture_2d_data& texture_2d_data =	
			static_cast<render_texture_2d_data&>(*data);

		if (!texture_2d_data.image_2d)
		{
			LOG_ERROR("Cannot apply data with NULL image.");
			throw null_pointer();
		}

		new_object.reset
			(new gl3_texture_object_2d
				(gl3_,
				 *texture_2d_data.image_2d,
				 texture_2d_data.wrap,
				 texture_2d_data.filter,
				 texture_2d_data.color_space,
				 texture_2d_data.compression));

		if (!texture_2d_data.keep_host_image)
			texture_2d_data.image_2d.reset();
	}

	if (typeid(*data) == typeid(render_texture_cubemap_data))
	{
		render_texture_cubemap_data& texture_cubemap_data =	
			static_cast<render_texture_cubemap_data&>(*data);

		if (!texture_cubemap_data.image_xpos ||
			!texture_cubemap_data.image_ypos ||
			!texture_cubemap_data.image_zpos ||
			!texture_cubemap_data.image_xpos ||
			!texture_cubemap_data.image_yneg ||
			!texture_cubemap_data.image_zneg)
		{
			LOG_ERROR("Cannot apply data with NULL image.");
			throw null_pointer();
		}

		new_object.reset
			(new gl3_texture_object_cubemap
				(gl3_,
				 *texture_cubemap_data.image_xpos,
				 *texture_cubemap_data.image_ypos,
				 *texture_cubemap_data.image_zpos,
				 *texture_cubemap_data.image_xneg,
				 *texture_cubemap_data.image_yneg,
				 *texture_cubemap_data.image_zneg,
				 texture_cubemap_data.filter,
				 texture_cubemap_data.color_space,
				 texture_cubemap_data.compression));

		if (!texture_cubemap_data.keep_host_image)
		{
			texture_cubemap_data.image_xpos.reset();
			texture_cubemap_data.image_ypos.reset();
			texture_cubemap_data.image_zpos.reset();
			texture_cubemap_data.image_xneg.reset();
			texture_cubemap_data.image_yneg.reset();
			texture_cubemap_data.image_zneg.reset();
		}
	}

	if (typeid(*data) == typeid(render_texture_canvas_data))
	{
		render_texture_canvas_data& texture_canvas_data =	
			static_cast<render_texture_canvas_data&>(*data);

		new_object.reset
			(new gl3_texture_canvas
				(gl3_, object_manager_, 
				 texture_canvas_data.source_canvas));
	}


	// Apply results.
	// Strong exception safety for convenience.
	// NOTHROW from here.
	texture_data_.reset
		(static_cast<render_texture_data*>
			(data.release_ownership()));

	texture_object_ =
		std::move(new_object);
}


auto_pointer<covert_data> gl3_texture::data()
{
	return texture_data_ ?
				texture_data_->clone() :
				auto_pointer<covert_data>();
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_shader
//
//////////////////////////////////////////////////////////////////////////


const gl3_program_object& gl3_shader::program_object() const
{
	if (!program_object_)
	{
		LOG_ERROR("No internal program object assigned.\r\n"
				  "Please commit appropriate render_shader_data.");
		throw invalid_state();
	}

	return *program_object_;
}


gl3_shader::gl3_shader
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_shader::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	auto_pointer<render_shader_data> shader_data
		(data.try_cast_exact<render_shader_data>());

	if (!shader_data)
	{
		LOG_ERROR("Incoming data must be render_shader_data.");
		throw type_mismatch();
	}

	shader_data_ = *shader_data;
	
	auto_pointer<gl3_shader_object> vertex_shader
		(shader_data_.vertex_shader == "" ?
		 NULL :
		 new gl3_shader_object
			(gl3_, shader_data_.vertex_shader,
			 gl3_shader_object::vertex_shader,
			 shader_data_.vertex_shader_virtual_path_hint,
			 shader_data_.vertex_shader_absolute_path_hint));

	auto_pointer<gl3_shader_object> geometry_shader
		(shader_data_.geometry_shader == "" ?
		 NULL :
		 new gl3_shader_object
			(gl3_, shader_data_.geometry_shader,
			 gl3_shader_object::geometry_shader,
			 shader_data_.geometry_shader_virtual_path_hint,
			 shader_data_.geometry_shader_absolute_path_hint));

	auto_pointer<gl3_shader_object> fragment_shader
		(shader_data_.fragment_shader == "" ?
		 NULL :
		 new gl3_shader_object
			(gl3_, shader_data_.fragment_shader,
			 gl3_shader_object::fragment_shader,
			 shader_data_.fragment_shader_virtual_path_hint,
			 shader_data_.fragment_shader_absolute_path_hint));

	program_object_.reset
		(new gl3_program_object
			(gl3_,
			 std::move(vertex_shader),
			 std::move(geometry_shader),
			 std::move(fragment_shader)));
}


auto_pointer<covert_data> gl3_shader::data()
	{ return shader_data_.clone(); }




//////////////////////////////////////////////////////////////////////////
//
// gl3_material
//
//////////////////////////////////////////////////////////////////////////


const gl3_material_data& gl3_material::material_data() const
{
	if (!material_data_)
	{
		LOG_ERROR("No internal material data assigned.\r\n"
				  "Please commit appropriate render_material_data.");
		throw invalid_state();
	}

	return *material_data_;
}


gl3_material::gl3_material
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3)
{ }


void gl3_material::set_data(auto_pointer<covert_data> data)
{
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}

	if (typeid(*data) == typeid(gl3_material_data))
	{
		material_data_ =
			data.try_cast_exact<gl3_material_data>();
	}
	else if (typeid(*data) == typeid(render_material_data))
	{
		auto_pointer<render_material_data> material_data
			(data.try_cast_exact<render_material_data>());

		material_data_.reset
			(new gl3_material_data(*material_data));
	}
	else
	{
		LOG_ERROR("Incoming data must be render_material_data.");
		throw type_mismatch();
	}
}


auto_pointer<covert_data> gl3_material::data()
{
	return material_data_ ?
			material_data_->clone() :
			auto_pointer<covert_data>();
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_model_data
//
//////////////////////////////////////////////////////////////////////////


gl3_model_data::stream_location_map
	gl3_model_data::default_stream_locations()
{
	stream_location_map stream_locations;

	// Vertex stream must always map to location 0.
	stream_locations.insert
		(mesh_stream_registry::vertex_stream_name, 0);
	stream_locations.insert
		(mesh_stream_registry::normal_stream_name, 1);
	stream_locations.insert
		(mesh_stream_registry::color_stream_name, 2);
	stream_locations.insert
		(mesh_stream_registry::texcoords_stream_name, 3);
	stream_locations.insert
		(mesh_stream_registry::tangent_stream_name, 4);
	stream_locations.insert
		(mesh_stream_registry::bitangent_stream_name, 5);

	stream_locations.insert
		(mesh_stream_registry::skeleton_index_stream_name, 6);
	stream_locations.insert
		(mesh_stream_registry::skeleton_weight_stream_name, 7);

	return stream_locations;
}




//////////////////////////////////////////////////////////////////////////
//
// gl3_model
//
//////////////////////////////////////////////////////////////////////////


gl3_model::group_iterator gl3_model::first_group() const
	{ return groups_.first(); }
gl3_model::group_iterator gl3_model::last_group() const
	{ return groups_.last(); }

const skeleton* gl3_model::skeleton_rig() const
	{ return skeleton_rig_.get(); }

const box& gl3_model::bounding_box() const
	{ return bounding_box_; }
float gl3_model::bounding_radius() const
	{ return bounding_radius_; }


gl3_model::gl3_model
	(const covert_object_manager& object_manager,
	 const gl3_api& gl3)
	:
	object_manager_(object_manager), gl3_(gl3),
	bounding_radius_(0)
{ }


void gl3_model::set_data(auto_pointer<covert_data> data)
{
	// Validate input data.
	if (!data)
	{
		LOG_ERROR("Cannot apply NULL data.");
		throw null_pointer();
	}


	// Create new internal data.
	// Either directly accept incoming gl3_model_data,
	// or copy-construct from basic render_model_data.
	// In the latter case, avoid a copy on the model.
	auto_pointer<gl3_model_data> model_data;

	if (typeid(*data) == typeid(gl3_model_data))
	{
		model_data =
			data.try_cast_exact<gl3_model_data>();
	}
	else if (typeid(*data) == typeid(render_model_data))
	{
		auto_pointer<render_model_data> base_data
			(data.try_cast_exact<render_model_data>());

		model_data.reset
			(new gl3_model_data(std::move(*base_data)));
	}
	else
	{
		LOG_ERROR("Incoming data has invalid type.");
		throw type_mismatch();
	}


	// Fetch all model data still required
	// even after dropping the CPU-side model.
	auto_pointer<skeleton> skeleton_rig
		(model_data->model.skeleton_rig() ?
			new skeleton(*model_data->model.skeleton_rig()) : NULL);

	box bounding_box(model_data->model.bounding_box());
	float bounding_radius = model_data->model.bounding_radius();


	// Create GPU-side geometry offhand.
	group_list new_groups;

	for (model::group_iterator it
			(model_data->model.first_group()); it; ++it)
	{
		if (!(*it))
		{
			LOG_ERROR("Incoming model has NULL model group.");
			throw null_pointer();
		}	

		auto_pointer<gl3_model_group> new_group
			(new gl3_model_group
				(gl3_, **it, model_data->stream_locations,
				 model_data->transient_hint));

		new_groups.insert(std::move(new_group));
	}

	if (!model_data->keep_host_model)
		model_data->model.clear();


	// Apply results.
	// Strong exception safety: NOTHROW from here.
	model_data_ = std::move(model_data);
	swap(groups_, new_groups);

	skeleton_rig_ = std::move(skeleton_rig);
	bounding_box_ = bounding_box;
	bounding_radius_ = bounding_radius;
}


auto_pointer<covert_data> gl3_model::data()
{
	return model_data_ ?
			model_data_->clone() :
			auto_pointer<covert_data>();
}




} /* namespace viral_core */
