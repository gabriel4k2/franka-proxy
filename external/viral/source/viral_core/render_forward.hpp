/**
 *************************************************************************
 *
 * @file render_forward.hpp
 *
 * Forward declarations for the @ref render_module "Render Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_FORWARD_HPP


#include "build_config.hpp"


namespace viral_core
{


// Funamental classes.
class render_context;
class render_exclusive;
class render_task;
class render_loader;

class render_command_render;
class render_command_queue;


// Rendering resources.
class render_texture_id;
class render_texture_data;
class render_texture_2d_data;
class render_texture_cubemap_data;
class render_texture_canvas_data;

class render_shader_id;
class render_shader_data;
class render_model_id;
class render_model_data;
class render_material_id;
class render_material_data;


// Rendering scenes.
class render_scene_object_tag;
class render_light_id;
class render_light_data;
class render_puppet_id;
class render_puppet_data;
class render_stack_id;
class render_stack_data;
class render_scene_id;
class render_scene_data;


// Rendering processes.
class render_layer_id;
class render_layer_data;
class render_canvas_id;
class render_canvas_data;
class render_process_id;
class render_process_data;


// Null rendering context.
class null_render_context;
template <typename T> class null_render_object;

typedef null_render_object<render_texture_data> null_render_texture;
typedef null_render_object<render_shader_data> null_render_shader;
typedef null_render_object<render_material_data> null_render_material;
typedef null_render_object<render_model_data> null_render_model;

typedef null_render_object<render_light_data> null_render_light;
typedef null_render_object<render_puppet_data> null_render_puppet;
typedef null_render_object<render_stack_data> null_render_stack;
typedef null_render_object<render_scene_data> null_render_scene;

typedef null_render_object<render_layer_data> null_render_layer;
typedef null_render_object<render_canvas_data> null_render_canvas;
typedef null_render_object<render_process_data> null_render_process;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_FORWARD_HPP) */