/**
 *************************************************************************
 *
 * @file render_resource.cpp
 *
 * Resource-specific renderer data, implementation.
 *
 ************************************************************************/


#include "render_resource.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// render_texture_data
//
//////////////////////////////////////////////////////////////////////////


render_texture_data::render_texture_data()
	:
	keep_host_image(false),
	wrap(wrap_both),
	filter(filter_trilinear),
	color_space(color_space_srgb),
	compression(compression_lossy)
{ }


render_texture_data::render_texture_data
	(bool keep_host_image,
	 wrap_mode wrap,
	 filter_mode filter,
	 color_space_mode color_space,
	 compression_mode compression)
	:	
	keep_host_image(keep_host_image),
	wrap(wrap),
	filter(filter),
	color_space(color_space),
	compression(compression)
{ }




//////////////////////////////////////////////////////////////////////////
//
// render_texture_2d_data
//
//////////////////////////////////////////////////////////////////////////


render_texture_2d_data::render_texture_2d_data() { }


render_texture_2d_data::render_texture_2d_data
	(bool keep_host_image,
	 wrap_mode wrap,
	 filter_mode filter,
	 color_space_mode color_space,
	 compression_mode compression,
	 auto_pointer<image_rgba> image_2d)
	:
	render_texture_data
		(keep_host_image, wrap, filter, color_space, compression),

	image_2d(std::move(image_2d))
{ }


render_texture_2d_data::render_texture_2d_data
	(const render_texture_2d_data& other)
	:
	render_texture_data(other),
	image_2d(other.image_2d ?
			 other.image_2d->clone() : auto_pointer<image_rgba>())
{ }


auto_pointer<covert_data> render_texture_2d_data::clone() const
	{ return auto_pointer<covert_data>(new render_texture_2d_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_texture_cubemap_data
//
//////////////////////////////////////////////////////////////////////////


render_texture_cubemap_data::render_texture_cubemap_data() { }


render_texture_cubemap_data::render_texture_cubemap_data
	(bool keep_host_image,
	 filter_mode filter,
	 color_space_mode color_space,
	 compression_mode compression,
	 auto_pointer<image_rgba> image_xpos,
	 auto_pointer<image_rgba> image_ypos,
	 auto_pointer<image_rgba> image_zpos,
	 auto_pointer<image_rgba> image_xneg,
	 auto_pointer<image_rgba> image_yneg,
	 auto_pointer<image_rgba> image_zneg)
	:
	render_texture_data
		(keep_host_image, wrap_none, filter, color_space, compression),

	image_xpos(std::move(image_xpos)),
	image_ypos(std::move(image_ypos)),
	image_zpos(std::move(image_zpos)),
	image_xneg(std::move(image_xneg)),
	image_yneg(std::move(image_yneg)),
	image_zneg(std::move(image_zneg))
{ }


render_texture_cubemap_data::render_texture_cubemap_data
	(const render_texture_cubemap_data& other)
	:
	render_texture_data(other),

	image_xpos(other.image_xpos ?
			   other.image_xpos->clone() : auto_pointer<image_rgba>()),
	image_ypos(other.image_ypos ?
			   other.image_ypos->clone() : auto_pointer<image_rgba>()),
	image_zpos(other.image_zpos ?
			   other.image_zpos->clone() : auto_pointer<image_rgba>()),

	image_xneg(other.image_xneg ?
			   other.image_xneg->clone() : auto_pointer<image_rgba>()),
	image_yneg(other.image_yneg ?
			   other.image_yneg->clone() : auto_pointer<image_rgba>()),
	image_zneg(other.image_zneg ?
			   other.image_zneg->clone() : auto_pointer<image_rgba>())
{ }


auto_pointer<covert_data> render_texture_cubemap_data::clone() const
	{ return auto_pointer<covert_data>(new render_texture_cubemap_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_texture_canvas_data
//
//////////////////////////////////////////////////////////////////////////


render_texture_canvas_data::render_texture_canvas_data() { }


render_texture_canvas_data::render_texture_canvas_data
	(weak_pointer<render_canvas_id>& source_canvas)
	:
	source_canvas(source_canvas)
{ }
	

auto_pointer<covert_data> render_texture_canvas_data::clone() const
	{ return auto_pointer<covert_data>(new render_texture_canvas_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_shader_data
//
//////////////////////////////////////////////////////////////////////////


render_shader_data::render_shader_data() { }


render_shader_data::render_shader_data
	(const string& vertex_shader,
	 const string& geometry_shader,
	 const string& fragment_shader,
	 const string& vertex_shader_virtual_path_hint,
	 const string& vertex_shader_absolute_path_hint,
	 const string& geometry_shader_virtual_path_hint,
	 const string& geometry_shader_absolute_path_hint,
	 const string& fragment_shader_virtual_path_hint,
	 const string& fragment_shader_absolute_path_hint)
	:
	vertex_shader(vertex_shader),
	geometry_shader(geometry_shader),
	fragment_shader(fragment_shader),

	vertex_shader_virtual_path_hint(vertex_shader_virtual_path_hint),
	vertex_shader_absolute_path_hint(vertex_shader_absolute_path_hint),

	geometry_shader_virtual_path_hint(geometry_shader_virtual_path_hint),
	geometry_shader_absolute_path_hint(geometry_shader_absolute_path_hint),

	fragment_shader_virtual_path_hint(fragment_shader_virtual_path_hint),
	fragment_shader_absolute_path_hint(fragment_shader_absolute_path_hint)
{ }


auto_pointer<covert_data> render_shader_data::clone() const
	{ return auto_pointer<covert_data>(new render_shader_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_material_data
//
//////////////////////////////////////////////////////////////////////////


render_material_data::render_material_data()
	:
	emissive_color(0, 0, 0, 0),
	ambient_color(1, 1, 1, 0),
	diffuse_color(1, 1, 1, 0),
	specular_color(1, 1, 1, 0),
	specular_exponent(1),
	unlit(false),
	shadow_cast_enabled(false),
	visible(true),
	raster(raster_solid),
	point_size(0),
	line_size(0),
	blend(blend_opaque),
	cull(cull_back)
{ }


render_material_data::render_material_data
	(const shared_pointer<render_texture_id>& texture,
	 const shared_pointer<render_texture_id>& normal_map,
	 const shared_pointer<render_shader_id>& shader,
	 const color& emissive_color,
	 const color& ambient_color,
	 const color& diffuse_color,
	 const color& specular_color,
	 float specular_exponent,
	 bool unlit,
	 const shared_pointer<render_shader_id>& shadow_cast_shader,
	 bool shadow_cast_enabled,
	 bool visible,
	 raster_mode raster,
	 float point_size,
	 float line_size,
	 blend_mode blend,
	 cull_mode cull)
	:
	texture(texture),
	normal_map(normal_map),
	shader(shader),
	emissive_color(emissive_color),
	ambient_color(ambient_color),
	diffuse_color(diffuse_color),
	specular_color(specular_color),
	specular_exponent(specular_exponent),
	unlit(unlit),
	shadow_cast_shader(shadow_cast_shader),
	shadow_cast_enabled(shadow_cast_enabled),
	visible(visible),
	raster(raster),
	point_size(point_size),
	line_size(line_size),
	blend(blend),
	cull(cull)
{ }


auto_pointer<covert_data> render_material_data::clone() const
	{ return auto_pointer<covert_data>(new render_material_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_model_data
//
//////////////////////////////////////////////////////////////////////////


render_model_data::render_model_data()
	:
	keep_host_model(false),
	transient_hint(false)
{ }


render_model_data::render_model_data
	(const ::viral_core::model& model,
	 bool keep_host_model,
	 bool transient_hint)
	:
	model(model),
	keep_host_model(keep_host_model),
	transient_hint(transient_hint)
{ }


render_model_data::render_model_data
	(::viral_core::model&& model,
	 bool keep_host_model,
	 bool transient_hint)
	:
	model(std::move(model)),
	keep_host_model(keep_host_model),
	transient_hint(transient_hint)
{ }


auto_pointer<covert_data> render_model_data::clone() const
	{ return auto_pointer<covert_data>(new render_model_data(*this)); }




} /* namespace viral_core */
