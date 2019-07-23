/**
 *************************************************************************
 *
 * @file gui_image.hpp
 *
 * Non-interactive image elements.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_IMAGE_HPP)
#define INCLUDED__VIRAL_GUI__GUI_IMAGE_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/image_forward.hpp>


#include <viral_core/auto_pointer.hpp>
#include <viral_core/shared_pointer.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_image
 * @ingroup gui_module
 *
 * A descriptive, non-interactive image.
 *
 * Usage notes:
 * - Usually, an explicit material() and an explicit image_size()
 *		in pixels define contents of GUI images.
 * - Alternatively, clients may directly provide a source image_rgba
 *		and a render_material_data. The GUI image then automatically
 *		manages and commits a texture and a material for given data.
 *		In this case, clients should not use the resulting material(),
 *		as its data might be overwritten on subsequent image updates.
 *
 * Layout notes:
 * - GUI images derive the preferred_size() of their arrange_area()
 *		from image_size() and their gui_image_style options.
 * - GUI images always center in their arrange_area(). Depending on
 *		gui_image_style options, images also can adapt to excess
 *		or shortage of actual arrange area.
 * - GUI images always respect their clip_area().
 *
 ************************************************************************/
class gui_image :
	public gui_element
{

	public:

		enum aspect_mode
		{		
			/** Scale x and y independently. */
			no_aspect,


			/**
			 * Fit into arrange_area() either on x or y.
			 * Select other dimension to match aspect.
			 * Both options may shrink and enlarge images.
			 */
			aspect_fit_x, aspect_fit_y,


			/**
			 * Perform aspect correction in a way that
			 * always makes the image smaller or larger
			 * than its current arrange_area().
			 */
			aspect_fit_both, aspect_fill
		};


		gui_image
			(const string& name, gui_context& gui,
			 const shared_instance<gui_image_style>& image_style,
			 const shared_instance<render_material_id>& material,
			 const vector2i& image_size);

		gui_image
			(const string& name, gui_context& gui,
			 const shared_instance<gui_image_style>& image_style,
			 const image_rgba& source_image,
			 const render_material_data& material_data);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Render image with not-@NULL @bref{material}
		 * and explicit @bref{image_size}, usually equal
		 * to the pixel size of the material texture.
		 */
		void apply_material
			(const shared_instance<render_material_id>& material,
			 const vector2i& image_size);


		/**
		 * Set contents to CPU-side @bref{source_image},
		 * and render using a material with properties
		 * as given in @bref{material_data}.
		 */
		void apply_source_image
			(const image_rgba& source_image,
			 const render_material_data& material_data);


		/**
		 * Set image contents as in apply_source_image(),
		 * but optionally commit the @bref{source_image}
		 * on a client-side render command @bref{queue}.
		 * On frequently updating images, this may avoid
		 * expensive layout() and commit_render() passes.
		 */
		void apply_and_commit_source_image
			(const image_rgba& source_image,
			 const render_material_data& material_data,
			 render_command_queue& queue);


		const vector2i& image_size() const NOTHROW;
		const shared_pointer<render_material_id>& material() const NOTHROW;


		/**
		 * Determine the unclipped GUI-space rectangle 
		 * which the image uses after layout, scaling,
		 * and aspect corrections.
		 */
		rect2f build_unclipped_image_rect() const;


		const shared_instance<gui_image_style> image_style;
		

	private:

		void realize_layout() override;

		void commit_pending_image(render_command_queue& queue);
		void pack_image(gui_render_packer& packer);


		vector2i image_size_;
		shared_pointer<render_material_id> material_;

		auto_pointer<image_rgba> pending_image_;
		auto_pointer<render_material_data> pending_material_data_;

		shared_pointer<render_material_id> internal_material_id_;
		shared_pointer<render_texture_id> internal_texture_id_;
};




/**
 *************************************************************************
 *
 * @class gui_image_style
 * @ingroup gui_module
 *
 * Defines style-specific gui_image parameters.
 *
 ************************************************************************/
class gui_image_style :
	public gui_style
{
	public:

		gui_image_style
			(const vector2f& base_size,
			 float pixel_scale,
			 bool downscale, bool upscale,
			 gui_image::aspect_mode aspect,
			 bool allow_compression);


		/**
		 * Adjust calculation of gui_image::preferred_size().
		 *
		 * If @bref{pixel_scale} is 0, clients directly specify
		 * the preferred size as @bref{base_size}.
		 *
		 * On a @bref{pixel_scale} > 0, the preferred size takes
		 * gui_image::image_size() into account:
		 * @code
		 * preferred_size() =
		 *		@bref{base_size} * image_size() * @bref{pixel_scale}
		 * @endcode
		 */
		//@{
		const vector2f base_size;
		const float pixel_scale;
		//@}


		/**
		 * Define how to shrink or stretch image display
		 * when there is too little or too much space
		 * within the current arrange area.
		 */
		//@{
		const bool downscale;
		const bool upscale;
		const gui_image::aspect_mode aspect;
		//@}


		/**
		 * If set, the gui_image may apply lossy compression
		 * to incoming source image_rgba%s before rendering.
		 */
		const bool allow_compression;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_IMAGE_HPP) */
