/**
 *************************************************************************
 *
 * @file gl3_loader.cpp
 *
 * Specialized loader for OpenGL3 rendering data, implementation.
 *
 ************************************************************************/


#include "gl3_loader.hpp"


#include "log.hpp"
#include "xml.hpp"
#include "xml_util.hpp"
#include "gl3_uniform.hpp"
#include "gl3_resource.hpp"
#include "gl3_scene.hpp"
#include "gl3_shader_workshop.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// gl3_loader
//
//////////////////////////////////////////////////////////////////////////


gl3_loader::gl3_loader
	(const string& loader_name, file_context& files,
	 covert_data_cache& data_cache, covert_id_cache& id_cache)
	:
	render_loader
		(loader_name, files,
		 data_cache, id_cache)
{
	LOG_INFO("Using gl3_loader implementation.");

	register_handler("shader", static_cast<data_handler>
						(&gl3_loader::load_data<render_shader_data>));
	register_handler("material", static_cast<data_handler>
						(&gl3_loader::load_data<gl3_material_data>));
	register_handler("model", static_cast<data_handler>
						(&gl3_loader::load_data<gl3_model_data>));
	register_handler("puppet", static_cast<data_handler>
						(&gl3_loader::load_data<gl3_puppet_data>));
}


void gl3_loader::to_uniform_values_helper
	(const xml_node& node, gl3_uniform_value_map& uniform_values)
{
	gl3_uniform_value_map new_values;

	for (xml_node::const_child_iterator it(node.first_child()); it; ++it)
	{
		const string& uniform_name = it->name();
		const string& uniform_type = it->attribute("type");

		if (new_values.find(uniform_name))
		{
			LOG_ERROR("Duplicate uniform: " + uniform_name);
			throw xml_exception();
		}

		auto_pointer<gl3_uniform_value> uniform_value;

		if (uniform_type == "bool")
		{
			uniform_value.reset
				(new gl3_uniform_value_bool(xml_converter::to_bool(*it)));		
		}
		else if (uniform_type == "int32")
		{
			uniform_value.reset
				(new gl3_uniform_value_int32(xml_converter::to_int32(*it)));		
		}
		else if (uniform_type == "float")
		{
			uniform_value.reset
				(new gl3_uniform_value_float(xml_converter::to_float(*it)));		
		}
		else if (uniform_type == "color")
		{
			uniform_value.reset
				(new gl3_uniform_value_color(xml_converter::to_color(*it)));
		}
		else if (uniform_type == "vector")
		{
			uniform_value.reset
				(new gl3_uniform_value_vector(xml_converter::to_vector(*it)));
		}
		else if (uniform_type == "matrix")
		{
			uniform_value.reset
				(new gl3_uniform_value_matrix(xml_converter::to_matrix(*it)));
		}
		else
		{
			LOG_ERROR("Unknown type of gl3_uniform_value: " + uniform_type);
			throw xml_exception();
		}

		new_values.insert
			(uniform_name, std::move(uniform_value));
	}	

	swap(new_values, uniform_values); // Strong exception safety.
}


void gl3_loader::to_data
	(const xml_node& node, render_shader_data& shader)
{
	render_loader::to_data(node, shader);

	if (!node.has_attribute("shader_workshop") ||
		node.attribute("shader_workshop").to_bool())
	{
		shader.fragment_shader =
			gl3_shader_workshop::preprocess
				(shader.fragment_shader,
				 gl3_shader_workshop::fragment_shader,
				 files);

		shader.vertex_shader =
			gl3_shader_workshop::preprocess
				(shader.vertex_shader,
				 gl3_shader_workshop::vertex_shader,
				 files);
	}
}


void gl3_loader::to_data
	(const xml_node& node, gl3_material_data& material)
{
	render_loader::to_data(node, material);

	if (node.has_child("uniform_values"))
		to_uniform_values_helper
			(node.child("uniform_values"), material.uniform_values);
}


void gl3_loader::to_data
	(const xml_node& node, gl3_model_data& model)
{
	render_loader::to_data(node, model);

	if (node.has_child("stream_locations"))
	{
		gl3_model_data::stream_location_map stream_locations;

		for (xml_node::const_child_iterator it
				(node.child("stream_locations").first_child()); it; ++it)
		{
			if (it->name() != "location")
			{
				LOG_ERROR("Model location nodes must be named location.");
				throw xml_exception();
			}

			stream_locations.insert
				(it->attribute("mesh_stream"),
				 it->attribute("value").to_int32());
		}

		// Swapping replaces default locations on purpose.
		// Otherwise, there would be no way to get rid of these.
		swap(model.stream_locations, stream_locations);
	}
}


void gl3_loader::to_data
	(const xml_node& node, gl3_puppet_data& puppet)
{
	render_loader::to_data(node, puppet);

	if (node.has_child("uniform_values"))
		to_uniform_values_helper
			(node.child("uniform_values"), puppet.uniform_values);
}


template <typename T>
	void gl3_loader::load_data
		(const xml_node& node, const string& name)
{
	auto_pointer<T> data(new T());
	to_data(node, *data);
	data_cache.insert
		(name, std::move(data));

	request_covert_id
		<typename T::covert_id_type>(name);
}	




} /* namespace viral_core */
