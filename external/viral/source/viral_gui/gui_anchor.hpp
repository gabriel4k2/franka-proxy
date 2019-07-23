/**
 *************************************************************************
 *
 * @file gui_anchor.hpp
 *
 * Anchoring of GUI elements.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_ANCHOR_HPP)
#define INCLUDED__VIRAL_GUI__GUI_ANCHOR_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_anchor
 * @ingroup gui_module
 *
 * Anchors, centers, or stretches content() in arrange_area().
 *
 * Layout notes:
 * - Most parents layout() their children to the largest possible area.
 *		A gui_anchor allows to override this behaviour by centering
 *		or anchoring content() in the parent-set arrange_area().
 * - Anchors directly pass their clip_area() through to their content.
 *
 ************************************************************************/
class gui_anchor :
	public gui_element
{

	public:

		enum anchor_type
		{
			/**
			 * Directly apply arrange area to content.
			 * On underflow, stretch content size to fill the area.
			 * On overflow, give content less space than preferred.
			 * Content that does not support overflow or underflow
			 * usually clips and silently uses anchor_neg instead.
			 */
			anchor_fill,


			/**
			 * Maintain content size, and instead center or anchor
			 * on negative or positive borders of arrange area. 
			 * Leaves free space on underflow and causes overhanging
			 * content on overflow.
			 */
			//@{
			anchor_neg,
			anchor_center,
			anchor_pos
			//@}
		};


		gui_anchor
			(const string& name, gui_context& gui,
			 const shared_instance<gui_anchor_style>& anchor_style);


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
		 * Anchor @bref{content_size} in @bref{arrange_area}
		 * according to given anchor_type%s and return result.
		 * Optionally, use separate anchor settings to handle
		 * surplus and insufficient space. One-dimensional
		 * variants are available, as well.
		 */
		//@{
		static rect2f build_layout
			(const vector2f& content_size,
			 const rect2f& arrange_area,
			 const gui_anchor_style& anchor_style);

		static rect2f build_layout
			(const vector2f& content_size,
			 const rect2f& arrange_area,
			 anchor_type horizontal_anchor,
			 anchor_type vertical_anchor) NOTHROW;

		static rect2f build_layout
			(const vector2f& content_size,
			 const rect2f& arrange_area,
			 anchor_type horizontal_anchor_underflow,
			 anchor_type vertical_anchor_underflow,
			 anchor_type horizontal_anchor_overflow,
			 anchor_type vertical_anchor_overflow) NOTHROW;

		static void build_layout
			(float content_size,
			 float arrange_area_pos, float arrange_area_size,
			 float& result_pos, float& result_size,
			 anchor_type anchor) NOTHROW;

		static void build_layout
			(float content_size,
			 float arrange_area_pos, float arrange_area_size,
			 float& result_pos, float& result_size,
			 anchor_type anchor_underflow,
			 anchor_type anchor_overflow) NOTHROW;
		//@}


		const shared_instance<gui_anchor_style> anchor_style;


	private:

		void realize_layout() override;

		shared_pointer<gui_element> content_;
};




/**
 *************************************************************************
 *
 * @class gui_anchor_style
 * @ingroup gui_module
 *
 * Styling for a gui_anchor element.
 *
 ************************************************************************/
class gui_anchor_style :
	public gui_style
{
	public:

		gui_anchor_style
			(gui_anchor::anchor_type horizontal_anchor_underflow,
			 gui_anchor::anchor_type vertical_anchor_underflow,
			 gui_anchor::anchor_type horizontal_anchor_overflow,
			 gui_anchor::anchor_type vertical_anchor_overflow);

		const gui_anchor::anchor_type horizontal_anchor_underflow;
		const gui_anchor::anchor_type vertical_anchor_underflow;
		const gui_anchor::anchor_type horizontal_anchor_overflow;
		const gui_anchor::anchor_type vertical_anchor_overflow;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_ANCHOR_HPP) */