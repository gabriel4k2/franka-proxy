/**
 *************************************************************************
 *
 * @file render_loader.cpp
 *
 * Loader for rendering-specific covert data, implementation.
 *
 ************************************************************************/


#include "render_loader.hpp"


#include "log.hpp"
#include "file.hpp"
#include "image_file.hpp"
#include "xml.hpp"
#include "xml_util.hpp"


namespace viral_core 
{


//////////////////////////////////////////////////////////////////////////
//
// render_loader
//
//////////////////////////////////////////////////////////////////////////


render_loader::render_loader
	(const string& loader_name, file_context& files,
	 covert_data_cache& data_cache, covert_id_cache& id_cache)
	:
	covert_loader
		(loader_name, files,
		 data_cache, id_cache)
{
	register_handler("texture_2d", static_cast<data_handler>
						(&render_loader::load_data<render_texture_2d_data>));
	register_handler("texture_cubemap", static_cast<data_handler>
						(&render_loader::load_data<render_texture_cubemap_data>));
	register_handler("texture_canvas", static_cast<data_handler>
						(&render_loader::load_data<render_texture_canvas_data>));
	register_handler("shader", static_cast<data_handler>
						(&render_loader::load_data<render_shader_data>));
	register_handler("material", static_cast<data_handler>
						(&render_loader::load_data<render_material_data>));
	register_handler("model", static_cast<data_handler>
						(&render_loader::load_data<render_model_data>));

	register_handler("light", static_cast<data_handler>
						(&render_loader::load_data<render_light_data>));
	register_handler("puppet", static_cast<data_handler>
						(&render_loader::load_data<render_puppet_data>));
	register_handler("stack", static_cast<data_handler>
						(&render_loader::load_data<render_stack_data>));
	register_handler("scene", static_cast<data_handler>
						(&render_loader::load_data<render_scene_data>));

	register_handler("layer", static_cast<data_handler>
						(&render_loader::load_data<render_layer_data>));
	register_handler("canvas", static_cast<data_handler>
						(&render_loader::load_data<render_canvas_data>));
	register_handler("process", static_cast<data_handler>
						(&render_loader::load_data<render_process_data>));
}


string render_loader::load_text_file(const string& path)
	{ return files.open_file(path, file::read_only)->read_text(); }

image_rgba render_loader::load_image_rgba(const string& path)
	{ return image_file::load(path, files); }

model render_loader::load_model(const string& path)
	{ return model::load(path, files, model::rendering_usage); }


render_texture_data::wrap_mode
	render_loader::to_texture_wrap_mode(const string& s) const
{
	if (s == "wrap_none")
		return render_texture_data::wrap_none;
	else if (s == "wrap_both")
		return render_texture_data::wrap_both;
	else if (s == "wrap_horizontal")
		return render_texture_data::wrap_horizontal;
	else if (s == "wrap_vertical")
		return render_texture_data::wrap_vertical;
		
	LOG_ERROR("Unknown texture wrap mode: " + s);
	throw xml_exception();
}


render_texture_data::filter_mode
	render_loader::to_texture_filter_mode(const string& s) const
{
	if (s == "filter_nearest")
		return render_texture_data::filter_nearest;
	else if (s == "filter_linear")
		return render_texture_data::filter_linear;
	else if (s == "filter_trilinear")
		return render_texture_data::filter_trilinear;
		
	LOG_ERROR("Unknown texture filter mode: " + s);
	throw xml_exception();
}


render_texture_data::color_space_mode
	render_loader::to_texture_color_space_mode(const string& s) const
{
	if (s == "color_space_srgb")
		return render_texture_data::color_space_srgb;
	else if (s == "color_space_linear")
		return render_texture_data::color_space_linear;

	LOG_ERROR("Unknown texture color space: " + s);
	throw xml_exception();
}


void render_loader::to_data_texture_helper
	(const xml_node& node, render_texture_data& texture) const
{
	if (node.has_child("keep_host_image"))
		texture.keep_host_image =
			xml_converter::to_bool(node.child("keep_host_image"));

	if (node.has_child("wrap"))
		texture.wrap = 
			to_texture_wrap_mode
				(node.child("wrap").attribute("value"));
	if (node.has_child("filter"))
		texture.filter = 
			to_texture_filter_mode
				(node.child("filter").attribute("value"));

	if (node.has_child("color_space"))
		texture.color_space = 
			to_texture_color_space_mode
				(node.child("color_space").attribute("value"));

	if (node.has_child("compression"))
	{
		const string& compression =
			node.child("compression").attribute("value");

		if (compression == "compression_none")
			texture.compression = render_texture_data::compression_none;
		else if (compression == "compression_lossy")
			texture.compression = render_texture_data::compression_lossy;
		else
		{
			LOG_ERROR("Unknown in-situ compression in texture: " + compression);
			throw xml_exception();
		}
	}
}


void render_loader::to_data
	(const xml_node& node, render_texture_2d_data& texture_2d)
{
	to_data_texture_helper(node, texture_2d);

	if (node.has_child("image_2d"))
		texture_2d.image_2d.reset
			(new image_rgba
				(load_image_rgba(node.child("image_2d").attribute("file"))));
}


void render_loader::to_data
	(const xml_node& node, render_texture_cubemap_data& texture_cubemap)
{
	to_data_texture_helper(node, texture_cubemap);

	if (node.has_child("image_xpos"))
		texture_cubemap.image_xpos.reset
			(new image_rgba
				(load_image_rgba(node.child("image_xpos").attribute("file"))));
	if (node.has_child("image_ypos"))
		texture_cubemap.image_ypos.reset
			(new image_rgba
				(load_image_rgba(node.child("image_ypos").attribute("file"))));
	if (node.has_child("image_zpos"))
		texture_cubemap.image_zpos.reset
			(new image_rgba
				(load_image_rgba(node.child("image_zpos").attribute("file"))));

	if (node.has_child("image_xneg"))
		texture_cubemap.image_xneg.reset
			(new image_rgba
				(load_image_rgba(node.child("image_xneg").attribute("file"))));
	if (node.has_child("image_yneg"))
		texture_cubemap.image_yneg.reset
			(new image_rgba
				(load_image_rgba(node.child("image_yneg").attribute("file"))));
	if (node.has_child("image_zneg"))
		texture_cubemap.image_zneg.reset
			(new image_rgba
				(load_image_rgba(node.child("image_zneg").attribute("file"))));
}


void render_loader::to_data
	(const xml_node& node, render_texture_canvas_data& texture_canvas)
{
	if (node.has_child("source_canvas"))
		texture_canvas.source_canvas =
			id_cache.entry<render_canvas_id>
				(node.child("source_canvas").attribute("ref"));
}


void render_loader::to_data
	(const xml_node& node, render_shader_data& shader)
{
	if (node.has_child("vertex_shader"))
	{
		const string& vertex_file =
			node.child("vertex_shader").attribute("file");

		shader.vertex_shader = load_text_file(vertex_file);
		shader.vertex_shader_virtual_path_hint = vertex_file;
		shader.vertex_shader_absolute_path_hint = files.to_absolute_path(vertex_file);
	}

	if (node.has_child("geometry_shader"))
	{
		const string& geometry_file =
			node.child("geometry_shader").attribute("file");

		shader.geometry_shader = load_text_file(geometry_file);
		shader.geometry_shader_virtual_path_hint = geometry_file;
		shader.geometry_shader_absolute_path_hint = files.to_absolute_path(geometry_file);
	}

	if (node.has_child("fragment_shader"))
	{
		const string& fragment_file =
			node.child("fragment_shader").attribute("file");

		shader.fragment_shader = load_text_file(fragment_file);	
		shader.fragment_shader_virtual_path_hint = fragment_file;
		shader.fragment_shader_absolute_path_hint = files.to_absolute_path(fragment_file);
	}
}


void render_loader::to_data
	(const xml_node& node, render_material_data& material)
{
	if (node.has_child("texture"))
		material.texture =
			id_cache.entry<render_texture_id>
				(node.child("texture").attribute("ref"));
	if (node.has_child("normal_map"))
		material.normal_map =
			id_cache.entry<render_texture_id>
				(node.child("normal_map").attribute("ref"));
	if (node.has_child("shader"))
		material.shader =
			id_cache.entry<render_shader_id>
				(node.child("shader").attribute("ref"));

	if (node.has_child("emissive_color"))
		material.emissive_color =
			xml_converter::to_color(node.child("emissive_color"));
	if (node.has_child("ambient_color"))
		material.ambient_color =
			xml_converter::to_color(node.child("ambient_color"));
	if (node.has_child("diffuse_color"))
		material.diffuse_color =
			xml_converter::to_color(node.child("diffuse_color"));
	if (node.has_child("specular_color"))
		material.specular_color =
			xml_converter::to_color(node.child("specular_color"));
	if (node.has_child("specular_exponent"))
		material.specular_exponent =
			xml_converter::to_float(node.child("specular_exponent"));
	if (node.has_child("unlit"))
		material.unlit =
			xml_converter::to_bool(node.child("unlit"));

	if (node.has_child("shadow_cast_shader"))
		material.shadow_cast_shader =
			id_cache.entry<render_shader_id>
				(node.child("shadow_cast_shader").attribute("ref"));
	if (node.has_child("shadow_cast_enabled"))
		material.shadow_cast_enabled =
			xml_converter::to_bool(node.child("shadow_cast_enabled"));

	if (node.has_child("visible"))
		material.visible =
			xml_converter::to_bool(node.child("visible"));

	if (node.has_child("raster"))
	{
		const string& raster = node.child("raster").attribute("value");

		if (raster == "raster_solid")
			material.raster = render_material_data::raster_solid;
		else if (raster == "raster_wireframe")
			material.raster = render_material_data::raster_wireframe;
		else if (raster == "raster_points")
			material.raster = render_material_data::raster_points;
		else
		{
			LOG_ERROR("Unknown raster mode in material: " + raster);
			throw xml_exception();
		}
	}

	if (node.has_child("point_size"))
		material.point_size =
			xml_converter::to_float(node.child("point_size"));
	if (node.has_child("line_size"))
		material.line_size =
			xml_converter::to_float(node.child("line_size"));

	if (node.has_child("blend"))
	{
		const string& blend = node.child("blend").attribute("value");

		if (blend == "blend_opaque")
			material.blend = render_material_data::blend_opaque;
		else if (blend == "blend_alpha")
			material.blend = render_material_data::blend_alpha;
		else if (blend == "blend_modulate")
			material.blend = render_material_data::blend_modulate;
		else if (blend == "blend_additive")
			material.blend = render_material_data::blend_additive;
		else
		{
			LOG_ERROR("Unknown blend mode in material: " + blend);
			throw xml_exception();
		}
	}

	if (node.has_child("cull"))
	{
		const string& cull = node.child("cull").attribute("value");

		if (cull == "cull_back")
			material.cull = render_material_data::cull_back;
		else if (cull == "cull_front")
			material.cull = render_material_data::cull_front;
		else if (cull == "cull_none")
			material.cull = render_material_data::cull_none;
		else
		{
			LOG_ERROR("Unknown cull mode in material: " + cull);
			throw xml_exception();
		}
	}
}


void render_loader::to_data
	(const xml_node& node, render_model_data& model)
{
	if (node.has_child("model"))
		model.model =
			load_model(node.child("model").attribute("file"));
	if (node.has_child("keep_host_model"))
		model.keep_host_model =
			xml_converter::to_bool(node.child("keep_host_model"));
}


void render_loader::to_data
	(const xml_node& node, render_light_data& light)
{
	if (node.has_child("emitter"))
	{
		const string& emitter =
			node.child("emitter").attribute("value");

		if (emitter == "emitter_point")
			light.emitter = render_light_data::emitter_point;
		else if (emitter == "emitter_spot")
			light.emitter = render_light_data::emitter_spot;
		else if (emitter == "emitter_parallel")
			light.emitter = render_light_data::emitter_parallel;
		else
		{
			LOG_ERROR("Unknown emitter type in light: " + emitter);
			throw xml_exception();
		}
	}

	if (node.has_child("enabled"))
		light.enabled =
			xml_converter::to_bool(node.child("enabled"));
	if (node.has_child("priority"))
		light.priority =
			xml_converter::to_float(node.child("priority"));

	if (node.has_child("position"))
		light.position =
			xml_converter::to_vector(node.child("position"));
	if (node.has_child("orientation"))
		light.orientation =
			xml_converter::to_rotation(node.child("orientation"));

	if (node.has_child("ambient_color"))
		light.ambient_color =
			xml_converter::to_color(node.child("ambient_color"));
	if (node.has_child("diffuse_color"))
		light.diffuse_color =
			xml_converter::to_color(node.child("diffuse_color"));
	if (node.has_child("specular_color"))
		light.specular_color =
			xml_converter::to_color(node.child("specular_color"));

	if (node.has_child("intensity"))
		light.intensity =
			xml_converter::to_float(node.child("intensity"));
	if (node.has_child("attenuation"))
		light.attenuation =
			xml_converter::to_float(node.child("attenuation"));

	if (node.has_child("spot_angle"))
		light.spot_angle =
			xml_converter::to_float(node.child("spot_angle"));
	if (node.has_child("spot_exponent"))
		light.spot_exponent =
			xml_converter::to_float(node.child("spot_exponent"));

	if (node.has_child("shadow_enabled"))
		light.shadow_enabled =
			xml_converter::to_bool(node.child("shadow_enabled"));
	if (node.has_child("shadow_priority"))
		light.shadow_priority =
			xml_converter::to_float(node.child("shadow_priority"));
	if (node.has_child("shadow_opacity"))
		light.shadow_opacity =
			xml_converter::to_float(node.child("shadow_opacity"));

	if (node.has_child("shadow_clip_near_normalized"))
		light.shadow_clip_near_normalized =
			xml_converter::to_float
				(node.child("shadow_clip_near_normalized"));
	if (node.has_child("shadow_clip_far_normalized"))
		light.shadow_clip_far_normalized =
			xml_converter::to_float
				(node.child("shadow_clip_far_normalized"));
}


void render_loader::to_data
	(const xml_node& node, render_puppet_data& puppet)
{
	if (node.has_child("position"))
		puppet.position =
			xml_converter::to_vector(node.child("position"));
	if (node.has_child("orientation"))
		puppet.orientation =
			xml_converter::to_rotation(node.child("orientation"));
	if (node.has_child("scale"))
		puppet.scale =
			xml_converter::to_float(node.child("scale"));

	if (node.has_child("model"))
		puppet.model =
			id_cache.entry<render_model_id>
				(node.child("model").attribute("ref"));

	if (node.has_child("materials"))
	{
		render_puppet_data::material_map materials;
	
		for (xml_node::const_child_iterator it
				(node.child("materials").first_child()); it; ++it)
		{
			if (it->name() != "material")
			{
				LOG_ERROR("Puppet material nodes must be named material.");
				throw xml_exception();
			}

			materials.insert
				(it->attribute("model_group"),
				 id_cache.entry<render_material_id>(it->attribute("ref")));
		}
	
		swap(puppet.materials, materials);
	}

	if (node.has_child("visible"))
		puppet.visible =
			xml_converter::to_bool(node.child("visible"));

	if (node.has_child("shadow_cast_enabled"))
		puppet.shadow_cast_enabled =
			xml_converter::to_bool(node.child("shadow_cast_enabled"));
}


void render_loader::to_data
	(const xml_node& node, render_stack_data& stack)
{
	if (node.has_child("puppets"))
	{
		render_stack_data::puppet_array puppets;

		for (xml_node::const_child_iterator it
				(node.child("puppets").first_child()); it; ++it)
		{
			if (it->name() != "puppet")
			{
				LOG_ERROR("Stack entry nodes must be named puppet.");
				throw xml_exception();
			}

			puppets.push_back
				(id_cache.entry<render_puppet_id>(it->attribute("ref")));
		}

		swap(puppets, stack.puppets);
	}

	if (node.has_child("anchor_hint"))
		stack.anchor_hint =
			xml_converter::to_vector(node.child("anchor_hint"));
}


void render_loader::to_data
	(const xml_node& node, render_scene_data& scene)
{
	if (node.has_child("objects"))
	{
		render_scene_data::object_set objects;

		for (xml_node::const_child_iterator it
				(node.child("objects").first_child()); it; ++it)
		{
			if (it->name() != "object")
			{
				LOG_ERROR("Scene object nodes must be named object.");
				throw xml_exception();
			}

			objects.insert
				(id_cache.entry<render_scene_object_tag>
					(it->attribute("ref")));
		}

		swap(scene.objects, objects);
	}
}


void render_loader::to_data
	(const xml_node& node, render_layer_data& layer)
{
	if (node.has_child("cam"))
		layer.cam =
			xml_converter::to_camera(node.child("cam"));
	if (node.has_child("scene"))
		layer.scene =
			id_cache.entry<render_scene_id>
				(node.child("scene").attribute("ref"));

	if (node.has_child("viewport"))
		layer.viewport =
			xml_converter::to_rect2f(node.child("viewport"));

	if (node.has_child("background"))
	{
		const string& background =
			node.child("background").attribute("value");

		if (background == "background_clear")
			layer.background = render_layer_data::background_clear;
		else if (background == "background_overlay")
			layer.background = render_layer_data::background_overlay;
		else if (background == "background_copy")
			layer.background = render_layer_data::background_copy;
		else
		{
			LOG_ERROR("Unknown background mode in layer: " + background);
			throw xml_exception();
		}
	}

	if (node.has_child("background_color"))
		layer.background_color =
			xml_converter::to_color(node.child("background_color"));

	if (node.has_child("background_canvas"))
		layer.background_canvas =
			id_cache.entry<render_canvas_id>
				(node.child("background_canvas").attribute("ref"));
}


void render_loader::to_data
	(const xml_node& node, render_canvas_data& canvas)
{
	if (node.has_child("layers"))
	{
		render_canvas_data::layer_list layers;

		for (xml_node::const_child_iterator it
				(node.child("layers").first_child()); it; ++it)
		{
			if (it->name() != "layer")
			{
				LOG_ERROR("Layer list nodes must be named layer.");
				throw xml_exception();
			}

			layers.insert
				(id_cache.entry<render_layer_id>(it->attribute("ref")));
		}

		swap(canvas.layers, layers);
	}

	if (node.has_child("pixel_size"))
		canvas.pixel_size =
			xml_converter::to_vector2i(node.child("pixel_size"));

	if (node.has_child("wrap"))
		canvas.wrap = 
			to_texture_wrap_mode
				(node.child("wrap").attribute("value"));
	if (node.has_child("filter"))
		canvas.filter = 
			to_texture_filter_mode
				(node.child("filter").attribute("value"));
	if (node.has_child("color_space"))
		canvas.color_space = 
			to_texture_color_space_mode
				(node.child("color_space").attribute("value"));

	if (node.has_child("copy_to_window"))
		canvas.copy_to_window =
			xml_converter::to_bool(node.child("copy_to_window"));
	if (node.has_child("target_window_pixels"))
		canvas.target_window_pixels =
			xml_converter::to_rect2i(node.child("target_window_pixels"));

	if (node.has_child("query_result_rgba"))
		canvas.query_result_rgba =
			xml_converter::to_bool(node.child("query_result_rgba"));
	if (node.has_child("query_result_depth"))
		canvas.query_result_depth =
			xml_converter::to_bool(node.child("query_result_depth"));
}


void render_loader::to_data
	(const xml_node& node, render_process_data& process)
{
	if (node.has_child("canvases"))
	{
		render_process_data::canvas_list canvases;

		for (xml_node::const_child_iterator it
				(node.child("canvases").first_child()); it; ++it)
		{
			if (it->name() != "canvas")
			{
				LOG_ERROR("Canvas list nodes must be named canvas.");
				throw xml_exception();
			}

			canvases.insert
				(id_cache.entry<render_canvas_id>
					(it->attribute("ref")));
		}

		swap(process.canvases, canvases);
	}

	if (node.has_child("show_results"))
		process.show_results =
			xml_converter::to_bool(node.child("show_results"));
}


template <typename T>
	void render_loader::load_data
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
