/**
 *************************************************************************
 *
 * @file render_process.cpp
 *
 * Defines process-specific rendering data, implementation.
 *
 ************************************************************************/


#include "render_process.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// render_layer_data
//
//////////////////////////////////////////////////////////////////////////


render_layer_data::render_layer_data()
	:
	viewport(0, 0, 1, 1),
	background(background_clear),
	background_color(0, 0, 0, 1)
{ }


render_layer_data::render_layer_data
	(const camera& cam,
	 const shared_pointer<render_scene_id>& scene,
	 const rect2f& viewport,
	 background_mode background,
	 const color& background_color,
	 const weak_pointer<render_canvas_id>& background_canvas)
	:
	cam(cam),
	scene(scene),
	viewport(viewport),
	background(background),
	background_color(background_color),
	background_canvas(background_canvas)
{ }


auto_pointer<covert_data> render_layer_data::clone() const
	{ return auto_pointer<covert_data>(new render_layer_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_canvas_data
//
//////////////////////////////////////////////////////////////////////////


render_canvas_data::render_canvas_data()
	:
	pixel_size(1024, 768),
	wrap(render_texture_data::wrap_none),
	filter(render_texture_data::filter_nearest),
	color_space(render_texture_data::color_space_srgb),
	copy_to_window(true),
	target_window_pixels(0, 0, 1024, 768),
	query_result_rgba(false),
	query_result_depth(false)
{ }


render_canvas_data::render_canvas_data
	(const layer_list& layers,
	 const vector2i& pixel_size,
	 render_texture_data::wrap_mode wrap,
	 render_texture_data::filter_mode filter,
	 render_texture_data::color_space_mode color_space,
	 bool copy_to_window,
	 const rect2i& target_window_pixels,
	 bool query_result_rgba,
	 bool query_result_depth)
	:
	layers(layers),
	pixel_size(pixel_size),
	wrap(wrap),
	filter(filter),
	color_space(color_space),
	copy_to_window(copy_to_window),
	target_window_pixels(target_window_pixels),
	query_result_rgba(query_result_rgba),
	query_result_depth(query_result_depth)
{ }


auto_pointer<covert_data> render_canvas_data::clone() const
	{ return auto_pointer<covert_data>(new render_canvas_data(*this)); }




//////////////////////////////////////////////////////////////////////////
//
// render_process_data
//
//////////////////////////////////////////////////////////////////////////


render_process_data::render_process_data()
	: show_results(true) { }


render_process_data::render_process_data
	(const canvas_list& canvases,
	 bool show_results)
	:
	canvases(canvases),
	show_results(show_results)
{ }


auto_pointer<covert_data> render_process_data::clone() const
	{ return auto_pointer<covert_data>(new render_process_data(*this)); }	




} /* namespace viral_core */
