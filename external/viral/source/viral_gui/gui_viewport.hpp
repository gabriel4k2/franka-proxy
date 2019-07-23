/**
 *************************************************************************
 *
 * @file gui_viewport.hpp
 *
 * 3D viewport in GUI.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_VIEWPORT_HPP)
#define INCLUDED__VIRAL_GUI__GUI_VIEWPORT_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/auto_pointer.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_viewport
 * @ingroup gui_module
 *
 * Shows rendering results of a render_canvas_id in the GUI.
 *
 * Canvas rendering:
 * - Clients (or derived classes) must explicitly set a canvas()
 *		with rendering results that the viewport should show.
 * - The GUI does not implicitly render to the given canvas().
 *		Instead, clients must explicitly (re-)render the canvas.
 *		GUI responsiveness improves on static, yet complex scenes
 *		if canvas re-rendering occurs only after the scene changes.
 * - Updating the target canvas through a commit_render() override
 *		requires want_commit_render() and expensive mesh repacking.
 *		Implementations should instead add a custom rendering hook.
 * - @attent Clients must allocate and initialize canvas contents
 *		by rendering to it at least once before the first call to
 *		gui_context::commit_render(). Otherwise, later rendering
 *		throws an exception.
 *
 * Canvas texture considerations:
 * - Each gui_viewport renders its current canvas() into the GUI
 *		as a rectangle with a per-viewport texture and material.
 *		Material and texture derive from @bref{texture_template}
 *		and @bref{material_template}, respectively.
 * - For pixel-perfect rendering, render_canvas_data::pixel_size
 *		should usually match gui_viewport::arrange_area().size().
 *		In particular, the canvas should resize along the viewport.
 *		Clients can alternatively render at greater or smaller
 *		resolution to balance performance and image quality.
 * - Rendering throws if the @bref{texture_template} uses trilinear
 *		filtering without @bref{build_mipmaps} on the canvas().
 *
 * Implementation notes:
 * - Clients derive from gui_viewport to implement process_input(),
 *		e.g. to realize camera movement or object selection.
 * - @attent A process_input() implementation must not modify
 *		any other GUI elements. Instead, the implementation
 *		should send back suitable gui_output_event%s.
 * - A gui_style_loader does not support viewports due to
 *		required canvas binding and potential subclassing.
 *
 ************************************************************************/
class gui_viewport :
	public gui_element
{

	public:

		gui_viewport
			(const string& name, gui_context& gui,
			 const vector2f& preferred_size,
			 const render_texture_canvas_data& texture_template,
			 const render_material_data& material_template);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Access the offscreen @bref{canvas} shown in this viewport.
		 * A NULL canvas is allowed, but does not render.
		 * See gui_viewport notes for details.
		 */
		//@{
		void set_canvas(const shared_pointer<render_canvas_id>& canvas) NOTHROW;
		const shared_pointer<render_canvas_id>& canvas() const NOTHROW;
		//@}


	private:

		void realize_layout() override;


		const vector2f preferred_size_;

		auto_pointer<render_texture_canvas_data> texture_template_;
		auto_pointer<render_material_data> material_template_;

		shared_pointer<render_material_id> material_;
		shared_pointer<render_canvas_id> canvas_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_VIEWPORT_HPP) */

