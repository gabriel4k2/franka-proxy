/**
 *************************************************************************
 *
 * @file render_resource.hpp
 *
 * Defines resource-specific renderer data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_RESOURCE_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_RESOURCE_HPP


#include "build_config.hpp"


#include "render_forward.hpp"


#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "color.hpp"
#include "image_raw.hpp"
#include "model.hpp"
#include "covert_data.hpp"


namespace viral_core
{


/**
 * IDs for large-scale resources.
 * @ingroup render_module
 */
//@{
class render_texture_id : public covert_id
{
	public:
		render_texture_id() { }
		render_texture_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class render_shader_id : public covert_id
{
	public:
		render_shader_id() { }
		render_shader_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class render_material_id : public covert_id
{
	public:
		render_material_id() { }
		render_material_id(const string& debug_name)
			: covert_id(debug_name) { }
};

class render_model_id : public covert_id
{
	public:
		render_model_id() { }
		render_model_id(const string& debug_name)
			: covert_id(debug_name) { }
};
//@}




/**
 *************************************************************************
 *
 * @class render_texture_data
 * @ingroup render_module
 *
 * Specifies any basic texture.
 *
 ************************************************************************/
class render_texture_data : public covert_data
{
	public:

		typedef render_texture_id covert_id_type;


		enum wrap_mode
			{ wrap_none, wrap_both,
			  wrap_horizontal, wrap_vertical };

		enum filter_mode
			{ filter_nearest, filter_linear, filter_trilinear };

		enum color_space_mode
			{ color_space_srgb, color_space_linear };

		enum compression_mode
			{ compression_none, compression_lossy };


		render_texture_data();

		render_texture_data
			(bool keep_host_image,
			 wrap_mode wrap,
			 filter_mode filter,
			 color_space_mode color_space,
			 compression_mode compression);


		/**
		 * Do not free CPU-side image memory after GPU upload.
		 * Required for later data queries, and for GPUs that
		 * invalidate memory on various system events.
		 */
		bool keep_host_image;


		/**
		 * Wrapping mode for texcoords outside [0,1] range.
		 * Only relevant for 2D textures.
		 */
		wrap_mode wrap;


		/** The filter to apply when scaling this texture. */
		filter_mode filter;


		/**
		 * The color space used by incoming images, outgoing images,
		 * and the internal GPU-side storage.
		 */
		color_space_mode color_space;


		/**
		 * In-situ image compression within the back-end.
		 * Compression happens on-the-fly and does not propagate back
		 * to clients through covert_command_query.
		 */
		compression_mode compression;
};




/**
 *************************************************************************
 *
 * @class render_texture_2d_data
 * @ingroup render_module
 *
 * Contains a 2D texture.
 *
 ************************************************************************/
class render_texture_2d_data : public render_texture_data
{
	public:

		typedef render_texture_id covert_id_type;


		render_texture_2d_data();

		render_texture_2d_data
			(bool keep_host_image,
			 wrap_mode wrap,
			 filter_mode filter,
			 color_space_mode color_space,
			 compression_mode compression,
			 auto_pointer<image_rgba> image_2d);

		render_texture_2d_data
			(const render_texture_2d_data& other);


		auto_pointer<covert_data> clone() const override;


		auto_pointer<image_rgba> image_2d;
};




/**
 *************************************************************************
 *
 * @class render_texture_cubemap_data
 * @ingroup render_module
 *
 * Contains images for a cubemap texture.
 *
 ************************************************************************/
class render_texture_cubemap_data : public render_texture_data
{
	public:

		typedef render_texture_id covert_id_type;


		render_texture_cubemap_data();

		render_texture_cubemap_data
			(bool keep_host_image,
			 filter_mode filter,
			 color_space_mode color_space,
			 compression_mode compression,
			 auto_pointer<image_rgba> image_xpos,
			 auto_pointer<image_rgba> image_ypos,
			 auto_pointer<image_rgba> image_zpos,
			 auto_pointer<image_rgba> image_xneg,
			 auto_pointer<image_rgba> image_yneg,
			 auto_pointer<image_rgba> image_zneg);

		render_texture_cubemap_data
			(const render_texture_cubemap_data& other);


		auto_pointer<covert_data> clone() const override;


		auto_pointer<image_rgba> image_xpos;
		auto_pointer<image_rgba> image_ypos;
		auto_pointer<image_rgba> image_zpos;

		auto_pointer<image_rgba> image_xneg;
		auto_pointer<image_rgba> image_yneg;
		auto_pointer<image_rgba> image_zneg;
};




/**
 *************************************************************************
 *
 * @class render_texture_canvas_data
 * @ingroup render_module
 *
 * Binds the current content of a @bref{source_canvas}
 * (see render_canvas_data) to a texture.
 *
 * General usage notes:
 * - A render_texture_canvas_data does not set texture parameters:
 *		Because the source canvas manages its texture data locally,
 *		clients must configure all parameters of the canvas texture 
 *		(e.g. mipmapping or color space) through render_canvas_data.
 * - Although render_texture_canvas_data is no render_texture_data,
 *		it still applies to render_texture_id%s.
 *
 * Source canvas considerations:
 * - The @bref{source_canvas} does not update automatically on use.
 *		Instead, clients must explicitly update canvas contents
 *		by rendering to the canvas (i.e. via render_process_data).
 * - To avoid read-write conflicts, rendering throws if clients
 *		use a canvas texture while rendering to its source canvas.
 * - The canvas back-reference does not keep the source canvas alive
 *		to avoid a dependency cycle. Clients must explicitly maintain
 *		an owning shared_pointer to @bref{source_canvas} during use,
 *		e.g. in a render_process_data.
 *
 ************************************************************************/
class render_texture_canvas_data : public covert_data
{
	public:

		typedef render_texture_id covert_id_type;

		render_texture_canvas_data();
		render_texture_canvas_data
			(weak_pointer<render_canvas_id>& source_canvas);

		auto_pointer<covert_data> clone() const override;

		weak_pointer<render_canvas_id> source_canvas;
};




/**
 *************************************************************************
 *
 * @class render_shader_data
 * @ingroup render_module
 *
 * Holds implementation-specific shader source code.
 *
 * Usage notes:
 * - Each type of render_context realizes a custom shading pipeline.
 *		This may range from a fixed-function pipeline to forward
 *		or deferred rendering paths with in-situ or uber-shaders.
 *		See the respective implementation for shading details.
 * - In particular, a context may arbitrarily interpret shaders,
 *		materials and lights. Clients might need to individually
 *		consider certain back-ends, e.g. with suitable load_options
 *		on their render_loader.
 * - See render_light_data and render_material_data for typical
 *		(and suggested) back-end lighting interpretation.
 *
 ************************************************************************/
class render_shader_data : public covert_data
{
	public:

		typedef render_shader_id covert_id_type;


		render_shader_data();

		render_shader_data
			(const string& vertex_shader,
			 const string& geometry_shader,
			 const string& fragment_shader,
			 const string& vertex_shader_virtual_path_hint,
			 const string& vertex_shader_absolute_path_hint,
			 const string& geometry_shader_virtual_path_hint,
			 const string& geometry_shader_absolute_path_hint,
			 const string& fragment_shader_virtual_path_hint,
			 const string& fragment_shader_absolute_path_hint);


		auto_pointer<covert_data> clone() const override;


		/** Shader source code. */
		//@{
		string vertex_shader;
		string geometry_shader;
		string fragment_shader;
		//@}


		/**
		 * Source files as virtual_path and absolute system path.
		 * Both paths are hints only, and may be empty or invalid.
		 */ 
		//@{
		string vertex_shader_virtual_path_hint;
		string vertex_shader_absolute_path_hint;

		string geometry_shader_virtual_path_hint;
		string geometry_shader_absolute_path_hint;

		string fragment_shader_virtual_path_hint;
		string fragment_shader_absolute_path_hint;
		//@}
};




/**
 *************************************************************************
 *
 * @class render_material_data
 * @ingroup render_module
 *
 * Defines surface parameters, for instance textures or shaders.
 *
 * Usage notes:
 * - Materials only act as a suggestion for the back-end renderer.
 *		See render_shader_data for further details.
 *
 ************************************************************************/
class render_material_data : public covert_data
{
	public:

		typedef render_material_id covert_id_type;


		enum raster_mode
			{ raster_solid, raster_wireframe, raster_points };

		enum blend_mode
			{ blend_opaque, blend_alpha, blend_modulate, blend_additive };

		enum cull_mode
			{ cull_back, cull_front, cull_none };


		render_material_data();

		render_material_data
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
			 cull_mode cull);


		auto_pointer<covert_data> clone() const override;


		/** Textures and main shader. */
		//@{
		shared_pointer<render_texture_id> texture;
		shared_pointer<render_texture_id> normal_map;
		shared_pointer<render_shader_id> shader;
		//@}


		/**
		 * Phong lighting properties.
		 *
		 * If a @bref{shader} does not apply any lighting,
		 * set @bref{unlit} to enable backend optimizations.
		 * Even if a material is @bref{unlit}, its shader
		 * may still access all phong lighting properties
		 * (e.g. @bref{diffuse_color} for colorization).
		 *
		 * See render_light_data for lighting equations.
		 */
		//@{
		color emissive_color;
		color ambient_color;
		color diffuse_color;
		color specular_color;
		float specular_exponent;
		bool unlit;
		//@}


		/**
		 * If @bref{shadow_cast_enabled} is set to @true,
		 * use @bref{shadow_cast_shader} to render shadows
		 * as cast by this material.
		 *
		 * Note materials can cast shadows even if not @bref{visible}.
		 * This allows to add artistic shadows to a virtual scene.
		 *
		 * See @bref{render_puppet_data::shadow_cast_enabled}
		 * for puppet-wide shadow control.
		 */
		//@{
		shared_pointer<render_shader_id> shadow_cast_shader;
		bool shadow_cast_enabled;
		//@}


		/**
		 * Whether to render this material at all.
		 * See @bref{render_puppet_data::visible}
		 * for puppet-wide visibility control.
		 */
		bool visible;


		/**
		 * Defines whether rendering shows mesh::triangles() 
		 * as solid shapes, as wireframe, or as points.
		 */
		raster_mode raster;


		/**
		 * Pixel width for rendering mesh::points() and mesh::lines().
		 * If @bref{raster} is @bref{raster_points} or @bref{raster_lines},
		 * the respective width also controls rendering of mesh::triangles().
		 * A size of 0 disables any explicit point or line size.
		 *
		 * Note backends tend to be particularly sloppy with pixel sizes.
		 * In particular, backends may round to the next supported size.
		 */
		//@{
		float point_size;
		float line_size;
		//@}
	

		/**
		 * Determines how to mix rendering into the final image.
		 * Note that blending may cause a variety of artifacts
		 * due to backend limits (e.g. broken depth or lighting).
		 */
		blend_mode blend;


		/** Defines which triangle sides should be rendered. */
		cull_mode cull;
};




/**
 *************************************************************************
 *
 * @class render_model_data
 * @ingroup render_module
 *
 * A rendering-specific representation of a @ref model.
 *
 * Usage notes:
 * - Back-ends try to maintain the order of meshes within a @ref model
 *		and the order of triangles within a @ref mesh where relevant
 *		(e.g. if blending is enabled).
 *
 ************************************************************************/
class render_model_data : public covert_data
{
	public:

		typedef render_model_id covert_id_type;


		render_model_data();

		render_model_data
			(const ::viral_core::model& model,
			 bool keep_host_model,
			 bool transient_hint);

		render_model_data
			(::viral_core::model&& model,
			 bool keep_host_model,
			 bool transient_hint);


		auto_pointer<covert_data> clone() const override;


		::viral_core::model model;


		/**
		 * Do not free CPU-side model memory after GPU upload.
		 * Required for later data queries, and for GPUs that
		 * invalidate memory on various system events.
		 */
		bool keep_host_model;


		/**
		 * If @bref{transient_hint} is set, the model is transient
		 * in nature and will probably be changed or discarded soon.
		 * This information helps rendering back-ends to choose
		 * an optimal storage space for the model.
		 */
		bool transient_hint;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_RESOURCE_HPP) */
