/**
 *************************************************************************
 *
 * @file render_process.hpp
 *
 * Defines process-specific rendering data.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_PROCESS_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_PROCESS_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "list.hpp"
#include "color.hpp"
#include "image_raw.hpp"
#include "geo_2d.hpp"
#include "geo_camera.hpp"
#include "covert_data.hpp"
#include "render_scene.hpp"


namespace viral_core
{


/**
 * IDs for rendering processes and related objects.
 * @ingroup render_module
 */
//@{
class render_layer_id : public covert_id
{
	public:
		render_layer_id() { }
		render_layer_id(const string& debug_name)
			: covert_id(debug_name) { }
};


class render_canvas_id : public covert_id
{
	public:
		render_canvas_id() { }
		render_canvas_id(const string& debug_name)
			: covert_id(debug_name) { }
};


class render_process_id : public covert_id
{
	public:
		render_process_id() { }
		render_process_id(const string& debug_name)
			: covert_id(debug_name) { }
};
//@}




/**
 *************************************************************************
 *
 * @class render_layer_data
 * @ingroup render_module
 *
 * A single composition layer that renders the view of a @bref{cam}era
 * within a virtual @bref{scene} into a @bref{viewport} on some canvas
 * (see render_canvas_data).
 *
 ************************************************************************/
class render_layer_data : public covert_data
{
	public:

		typedef render_layer_id covert_id_type;


		enum background_mode
		{
			/** Clear background to @bref{background_color}. */
			background_clear,

			/** Render over existing canvas contents. */
			background_overlay,

			/** Copy background from some @bref{background_canvas}. */
			background_copy
		};


		render_layer_data();
	
		render_layer_data
			(const camera& cam,
			 const shared_pointer<render_scene_id>& scene,
			 const rect2f& viewport,
			 background_mode background,
			 const color& background_color,
			 const weak_pointer<render_canvas_id>& background_canvas);


		auto_pointer<covert_data> clone() const override;


		camera cam;
		shared_pointer<render_scene_id> scene;


		/**
		 * Normalized placement within render_canvas_data::pixel_size,
		 * (0, 0) for upper-left and (1, 1) for lower-right corner.
		 * See @ref camera for further coordinate conventions.
		 */
		rect2f viewport;	


		background_mode background;
		color background_color;


		/**
		 * Source canvas to use for @bref{background_copy}.
		 * Same usage notes as render_texture_canvas_data,
		 * but potentially applies an efficient blit operation.
		 * On size mismatch, scale background appropriately.
		 */
		weak_pointer<render_canvas_id> background_canvas;
};




/**
 *************************************************************************
 *
 * @class render_canvas_data
 * @ingroup render_module
 *
 * An off-screen canvas made from multiple composition layers
 * (see render_layer_data), rendered as part of a rendering
 * process (see render_process_data).
 *
 * Usage notes:
 * - See @bref{copy_to_window} and render_process_data::show_results
 *		for on-screen display of off-screen canvases.
 * - Back-ends usually realize each canvas as a separate render target
 *		(e.g. OpenGL framebuffer). Thus, canvases support advanced
 *		rendering features (see use in render_texture_canvas_data),
 *		but are more expensive than individual composition layers.
 *
 ************************************************************************/
class render_canvas_data : public covert_data
{
	public:

		typedef render_canvas_id covert_id_type;

		typedef list<shared_pointer<render_layer_id>> layer_list;


		render_canvas_data();

		render_canvas_data
			(const layer_list& layers,
			 const vector2i& pixel_size,
			 render_texture_data::wrap_mode wrap,
			 render_texture_data::filter_mode filter,
			 render_texture_data::color_space_mode color_space,
			 bool copy_to_window,
			 const rect2i& target_window_pixels,
			 bool query_result_rgba,
			 bool query_result_depth);
	

		auto_pointer<covert_data> clone() const override;


		/** FIFO list of composition layers in this canvas. */
		layer_list layers;


		/**
		 * Canvas size in pixels.
		 * Implementations may choose a different resolution,
		 * for example if they do not support the given size,
		 * which may cause slight rendering glitches.
		 */
		vector2i pixel_size;


		/**
		 * Properties of the canvas texture when used
		 * as render_texture_canvas_data::source_canvas.
		 * 
		 * @attent When a canvas is not used as a texture,
		 * clients should specify @bref{filter_nearest}
		 * to disable expensive mipmap rebuilds.
		 */
		//@{
		render_texture_data::wrap_mode wrap;
		render_texture_data::filter_mode filter;
		render_texture_data::color_space_mode color_space;
		//@}


		/**
		 * If @bref{copy_to_window} is set, copy final canvas
		 * to @bref{target_window_pixels} in a bound window.
		 * Note there may be some copy glitches, in particular
		 * if a concurrent window_context::update() changed
		 * the window_context::size().
		 */
		//@{
		bool copy_to_window;
		rect2i target_window_pixels;
		//@}


		/**
		 * Inefficient CPU-side readout of rendering results.
		 * If query flags are set on a render_canvas_id,
		 * a later covert_command_query fills result members.
		 * Only useful for taking screenshots.
		 */
		//@{
		bool query_result_rgba;
		image_rgba result_rgba;

		bool query_result_depth;
		image_depth result_depth;
		//@}
};




/**
 *************************************************************************
 *
 * @class render_process_data
 * @ingroup render_module
 *
 * Renders multiple canvases (see render_canvas_data)
 * when executed through a render_command_render.
 *
 ************************************************************************/
class render_process_data : public covert_data
{
	public:

		typedef render_process_id covert_id_type;

		typedef list<shared_pointer<render_canvas_id>> canvas_list;


		render_process_data();
		render_process_data
			(const canvas_list& canvases,
			 bool show_results);
	

		auto_pointer<covert_data> clone() const override;


		/** FIFO list of canvases this process should render. */	
		canvas_list canvases;


		/**
		 * Whether this process should show rendering results
		 * in a bound window (e.g. by a final buffer swap).
		 * Disable for pure off-screen or partial on-screen rendering,
		 * enable to show render_canvas_data::copy_to_window.
		 */
		bool show_results;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_PROCESS_HPP) */
