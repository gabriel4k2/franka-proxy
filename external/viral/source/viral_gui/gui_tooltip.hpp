/**
 *************************************************************************
 *
 * @file gui_tooltip.hpp
 *
 * Tooltips for the GUI.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_TOOLTIP_HPP)
#define INCLUDED__VIRAL_GUI__GUI_TOOLTIP_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/image_forward.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_tooltip
 * @ingroup gui_module
 *
 * A hovering tooltip inside a gui_tooltip_area.
 *
 * Usage notes:
 * - Clients should only assign tooltips through gui_tooltip_area,
 *		as opposed to directly using gui_tooltip.
 * - A gui_tooltip always expects its preferred_size() as arrange_area(),
 *		which the gui_tooltip_area does readily provide.
 *
 ************************************************************************/
class gui_tooltip :
	public gui_element
{

	public:

		gui_tooltip
			(const string& name, gui_context& gui,
			 const shared_instance<gui_tooltip_style>& tooltip_style);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/** Text shown on the tooltip. */
		//@{
		void set_text(const string& text);
		string text() const;
		void clear_text() NOTHROW;
		//@}


		/** Image shown to the left of text(). */
		//@{
		void set_image
			(const shared_instance<render_material_id>& material,
			 const vector2i& image_size);
		void set_image
			(const image_rgba& source_image,
			 const render_material_data& material_data);

		void clear_image() NOTHROW;
		//@}


		const shared_instance<gui_tooltip_style> tooltip_style;


	private:

		void realize_layout() override;

		vector2f calculate_content_size() const;


		const shared_instance<gui_frame> frame_;

		shared_pointer<gui_label> label_;
		shared_pointer<gui_image> image_;
};




/**
 *************************************************************************
 *
 * @class gui_tooltip_style
 * @ingroup gui_module
 *
 * Style for a gui_tooltip.
 *
 ************************************************************************/
class gui_tooltip_style :
	public gui_style
{

	public:

		gui_tooltip_style
			(const shared_instance<gui_frame_style>& frame_style,
			 const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_image_style>& image_style,
			 float image_label_spacing);


		const shared_instance<gui_frame_style> frame_style;
		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_image_style> image_style;


		/**
		 * Whitespace between the image and the label
		 * if both image and label are present.
		 */
		const float image_label_spacing;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_TOOLTIP_HPP) */
