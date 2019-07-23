/**
 *************************************************************************
 *
 * @file gui_label.hpp
 *
 * GUI labels with static text.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_LABEL_HPP)
#define INCLUDED__VIRAL_GUI__GUI_LABEL_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/mesh_forward.hpp>


#include <viral_core/auto_pointer.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_label
 * @ingroup gui_module
 *
 * A descriptive, non-interactive text.
 *
 * Layout notes:
 * - The preferred_size() of a label fits for the entire text and uses
 *		integral lines (see font::calculate_line_integral_extents()).
 *		The latter avoids layout inconsistencies when adding characters
 *		with vertical overhang (e.g. 'g') to an existing line.
 * - Labels overflow on arrange_area()s smaller than preferred_size().
 *		However, clip_area()s still cull text to paintable bounds.
 *		Single-line labels additionally try to shorten text with
 *		ellipsis dots to match their arrange_area().
 *
 ************************************************************************/
class gui_label :
	public gui_element
{

	public:

		gui_label
			(const string& name, gui_context& gui,
			 const shared_instance<gui_label_style>& label_style,
			 const string& text);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		void set_text(const string& text);
		const string& text() const;
		void clear_text() NOTHROW;


		const shared_instance<gui_label_style> label_style;


	private:

		void realize_layout() override;

		void update_unclipped_text_extents() NOTHROW;
		void update_render_mesh();
		void pack_render_mesh(gui_render_packer& packer);


		string text_;
		vector2f unclipped_text_extents_;
		auto_pointer<mesh> render_mesh_;
};




/**
 *************************************************************************
 *
 * @class gui_label_style
 * @ingroup gui_module
 *
 * Styling for a single text label.
 *
 ************************************************************************/
class gui_label_style :
	public gui_style
{
	public:

		gui_label_style
			(const shared_instance<gui_font_style>& font_style,
			 const shared_instance<gui_anchor_style>& anchor_style);

		const shared_instance<gui_font_style> font_style;
		const shared_instance<gui_anchor_style> anchor_style;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_LABEL_HPP) */
