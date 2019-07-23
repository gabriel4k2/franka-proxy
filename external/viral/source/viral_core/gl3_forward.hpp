/**
 *************************************************************************
 *
 * @file gl3_forward.hpp
 *
 * Forward declarations for the @ref gl3_render_module
 * "OpenGL3 Rendering Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__GL3_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__GL3_FORWARD_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "image_forward.hpp"
#include "geo_forward.hpp"


namespace viral_core
{


// Fundamental classes.
class gl3_render_context;
class gl3_api;
class gl3_loader;

class gl3_kernel;

class gl3_kernel_pass;
class gl3_kernel_limits;
class gl3_kernel_priority_index;

class gl3_kernel_uniforms;
class gl3_kernel_scene;
class gl3_kernel_shadow;
class gl3_kernel_render;
class gl3_kernel_state;
class gl3_kernel_light;

class gl3_render_target;

class gl3_shadow_map_point;
class gl3_shadow_map_spot;
class gl3_shadow_map_parallel;


// Scenes, resources, processes.
class gl3_layer;
class gl3_canvas;
class gl3_process;

class gl3_texture;
class gl3_shader;
class gl3_material_data;
class gl3_material;
class gl3_model_data;
class gl3_model_group;
class gl3_model;

class gl3_scene_object_tag;
class gl3_light;
class gl3_puppet_data;
class gl3_puppet;
class gl3_stack;
class gl3_scene;


// OpenGL resource wrappers.
class gl3_texture_object;
class gl3_texture_object_2d;
class gl3_texture_object_2d_array;
class gl3_texture_object_cubemap;
class gl3_texture_canvas;

class gl3_frame_buffer_attachment;
class gl3_frame_buffer_attachment_2d;
class gl3_frame_buffer_attachment_2d_array;
class gl3_frame_buffer_attachment_cubemap;
class gl3_frame_buffer;

class gl3_vertex_buffer;
class gl3_vertex_array_attribute;
class gl3_vertex_array;
class gl3_mesh;


// OpenGL shader wrappers.
class gl3_shader_object;
class gl3_program_object;

class gl3_uniform;
class gl3_uniform_value;
class gl3_uniform_value_map;
typedef dynamic_buffer<const gl3_uniform_value_map*> gl3_uniform_value_stack;

template <typename T> class gl3_uniform_value_typed;
typedef gl3_uniform_value_typed<bool> gl3_uniform_value_bool;
typedef gl3_uniform_value_typed<int32> gl3_uniform_value_int32;
typedef gl3_uniform_value_typed<float> gl3_uniform_value_float;
typedef gl3_uniform_value_typed<color> gl3_uniform_value_color;
typedef gl3_uniform_value_typed<vector> gl3_uniform_value_vector;
typedef gl3_uniform_value_typed<matrix> gl3_uniform_value_matrix;
template <typename T, int64 count> class gl3_uniform_value_array_typed;

class gl3_shader_bricks;
class gl3_shader_workshop;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__GL3_FORWARD_HPP) */
