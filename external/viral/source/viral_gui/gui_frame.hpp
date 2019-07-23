/**
 *************************************************************************
 *
 * @file gui_frame.hpp
 *
 * Frames around other GUI elements.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_FRAME_HPP)
#define INCLUDED__VIRAL_GUI__GUI_FRAME_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_frame
 * @ingroup gui_module
 *
 * A frame for grouping or highlighting elements.
 *
 * Layout notes:
 * - Frames specify their preferred_size() at least for margin, borders,
 *		and padding. If a frame has content, content()->preferred_size()
 *		adds to the frame's preferred_size().
 * - Rendering ignores and exceeds an arrange_area() that is smaller
 *		than the preferred_size(). A clip_area() is necessary to
 *		restrict rendering then.
 * - Any extra space in the arrange_area() fills with frame tiles,
 *		and transfers to frame content().
 *
 * Usage notes:
 * - A frame only supports a single content() element. Use gui_grid or
 *		other containers to add multiple children elements to a frame.
 * - Content and frames may also be siblings in a single parent control.
 *		This allows to display frames in-situ without any modifications
 *		to the GUI hierarchy (e.g. as selection highlight). Parents can
 *		use various helpers to position sibling content within a frame
 *		(see frame_size_for_content_size() or build_content_layout()).
 *
 ************************************************************************/
class gui_frame :
	public gui_element
{
	public:

		gui_frame
			(const string& name, gui_context& gui,
			 const shared_instance<gui_frame_style>& frame_style);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		void set_content(const shared_pointer<gui_element>& content);
		const shared_pointer<gui_element>& content() const NOTHROW;
		void clear_content() NOTHROW;


		/**	
		 * Determine arrange_area().size a frame needs
		 * to enclose sibling content of given size.
		 */
		vector2f frame_size_for_content_size
			(const vector2f& content_size) const;


		/**
		 * Build arrange_area() and clip_area() that
		 * a frame would allocate for its borders and
		 * for content, given some minimum required
		 * @bref{content_size}.
		 *
		 * Returns @false if the frame cannot provide
		 * either area, and leaves parameters unchanged.
		 *
		 * @attent Apply frame layout() first.
		 */
		//@{
		bool build_border_layout
			(const vector2f& content_size,
			 rect2f& arrange, rect2f& clip) const;
		bool build_content_layout
			(const vector2f& content_size,
			 rect2f& arrange, rect2f& clip) const;
		//@}


		const shared_instance<gui_frame_style> frame_style;


	private:

		void realize_layout() override;
		void pack_frame(gui_render_packer& packer);

		static const int64 max_subdivisions_;
		shared_pointer<gui_element> content_;
};




/**
 *************************************************************************
 *
 * @class gui_frame_style
 * @ingroup gui_module
 *
 * Layout and styling for a gui_frame.
 *
 ************************************************************************/
class gui_frame_style :
	public gui_style
{
	public:

		gui_frame_style
			(const vector2f& margin_00, const vector2f& margin_22,
			 const vector2f& border_00, const vector2f& border_22,
			 const vector2f& padding_00, const vector2f& padding_22,
			 const shared_instance<render_material_id>& material,
			 const vector2i& material_pixel_size,
			 const vector2i& pixel_offset,
			 const vector2i& pixel_size_00,
			 const vector2i& pixel_size_11,
			 const vector2i& pixel_size_22,
			 const vector2f& size_per_tile);


		/**
		 * Distance in-between parent client area and
		 * upper-left (0, 0) or lower-right (2, 2) border.
		 */
		//{
		const vector2f margin_00;
		const vector2f margin_22;
		//@}


		/**
		 * Size of borders in upper-left (0, 0) and
		 * lower-right (2, 2) corner, inward from margin.
		 * Borders must not be negative.
		 */
		//@{
		const vector2f border_00;
		const vector2f border_22;
		//@}


		/**
		 * Distance from upper-left or lower-right border
		 * to client area and children margins.
		 * Children overlap borders (and margins)
		 * for negative padding.
		 */
		//@{
		const vector2f padding_00;
		const vector2f padding_22;
		//@}


		/** Material used to texture all frame areas. */
		const shared_instance<render_material_id> material;


		/** Size of the entire @bref{material} texture in pixels. */
		const vector2i material_pixel_size;


		/**
		 * Texture coordinates, in pixels, of the frame areas
		 * within the @bref{material} texture.
		 */
		//@{
		const vector2i pixel_offset;
		const vector2i pixel_size_00;
		const vector2i pixel_size_11;
		const vector2i pixel_size_22;
		//@}


		/**
		 * Size of a single center tile in GUI units,
		 * usually equal to @bref{pixel_size_11}.
		 * Also applies to vertical or horizontal tiling
		 * of frame border areas.
		 */
		const vector2f size_per_tile;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_FRAME_HPP) */