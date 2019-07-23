/**
 *************************************************************************
 *
 * @file gui_scrollbar.hpp
 *
 * GUI scrollbars.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_SCROLL_HPP)
#define INCLUDED__VIRAL_GUI__GUI_SCROLL_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_scrollbar
 * @ingroup gui_module
 *
 * A horizontal or vertical scrollbar with two scroll buttons
 * and a knob for dragging along a background scroll band.
 *
 * Layout notes:
 * - Scrollbars prefer space for two scroll buttons and a style-specific
 *		minimum-sized scroll band in-between the buttons. If less than
 *		this size is available, scrollbars exceed their arrange_area()
 *		but still respect their clip_area().
 * - In orientation direction, the scroll band exploits any excess size
 *		as dragging range. Against orientation direction, both band and
 *		buttons grow with excess space.
 * - Client code can still use scrollbars even on layout_invalid().
 *		Notably, setters and getters for position, range, and pages
 *		work as specified. The scrollbar only ignores GUI user input.
 * - A scrollbar must use a zero gui_knob_style::min_drag_range to
 *		avoid usage artifacts. See gui_knob for general knob layout.
 *
 ************************************************************************/
class gui_scrollbar :
	public gui_element
{
	public:

		gui_scrollbar
			(const string& name, gui_context& gui,
			 const shared_instance<gui_scrollbar_style>& scrollbar_style,
			 orientation_type orientation);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Access range that restricts the scrollbar position().
		 * Ranges must be valid (i.e. @bref{range_max} >= @bref{range_min}).
		 * Notably, empty ranges are valid.
		 */
		//@{
		void set_range
			(float range_min, float range_max);

		float range_min() const NOTHROW;
		float range_max() const NOTHROW;
		//@}


		/**
		 * Page size, item size, and overlap factor.
		 *
		 * Usage notes:
		 * - Page size should correspond to the size of a visible page
		 *		within the total range. It determines the content skipped
		 *		by page-wise scrolling and the size of the scrollbar knob.
		 * - Item size should correspond to a single entry within scrolled
		 *		content, and sets the scroll amount per button click.
		 * - The overlap factor multiplies with page size and item size
		 *		to provide a small viewport overlap on scrolling.
		 *		This helps users with tracking content while scrolling.
		 */
		//@{
		void set_page_size(float page_size);
		float page_size() const NOTHROW;
		void set_item_size(float item_size);
		float item_size() const NOTHROW;
		void set_overlap_factor(float overlap_factor);
		float overlap_factor() const NOTHROW;
		//@}


		/**
		 * Continuous increment determines scrolling per gui_context::update()
		 * while a scroll button is held down.
		 */
		//@{
		void set_continuous_increment(float continuous_increment);
		float continuous_increment() const NOTHROW;
		//@}


		/**
		 * Access scrollbar position within range_min() and range_max().
		 * Invalid values automatically clamp_to_range().
		 */
		//@{
		void set_position(float position) NOTHROW;
		float position() const NOTHROW;
		float clamp_to_range(float position) const NOTHROW;
		//@}


		/**
		 * Scrolling position(), normalized to the [0, 1] interval.
		 * Queries may return inconsistent values on empty ranges.
		 */
		//@{
		void set_position_normalized(float position_normalized) NOTHROW;
		float position_normalized() const NOTHROW;
		//@}


		/**
		 * Scroll by smallest amount so that a @bref{point} or
		 * an interval [@bref{start}, @bref{end}] becomes visible
		 * on the current page.
		 *
		 * If the request cannot be fulfilled (e.g. due to targets
		 * outside range or intervals larger than page_size()),
		 * scroll to a next-best match instead.
		 */
		//@{
		void scroll_page_to(float point) NOTHROW;
		void scroll_page_to(float start, float end) NOTHROW;
		//@}


		/**
		 * Determine if the current page shows some @bref{point}.
		 * Note this does not consider clipping or layout,
		 * but only takes position() and page_size() into account.
		 */
		bool is_visible_on_page(float point) const NOTHROW;
	

		const shared_instance<gui_scrollbar_style> scrollbar_style;
		const orientation_type orientation;


	private:

		void realize_layout() override;

		void process_child_input
			(gui_input_event& input,
			 gui_output_event_list& output);
		void process_band_input
			(gui_input_event& input,
			 gui_output_event_list& output);
		void create_output_event
			(gui_output_event_list& output);


		static const float range_epsilon_;

		float range_min_;
		float range_max_;

		float page_size_;
		float item_size_;
		float overlap_factor_;

		float continuous_increment_;

		float position_;

		rect2f hot_area_band_;
		rect2f hot_area_band_clipped_;
		bool hot_area_band_valid_;

		const shared_instance<gui_knob> knob_;
		const shared_instance<gui_button> increment_;
		const shared_instance<gui_button> decrement_;
		const shared_instance<gui_frame> band_;
};


/**
 * Notifies clients that the user changed a gui_scrollbar.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_scrollbar> gui_scrollbar_event;




/**
 *************************************************************************
 *
 * @class gui_scrollbar_style
 * @ingroup gui_module
 * 
 * Styling for a gui_scrollbar.
 *
 ************************************************************************/
class gui_scrollbar_style :
	public gui_style
{
	public:

		gui_scrollbar_style
			(const shared_instance<gui_knob_style>& knob_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_frame_style>& band_style,
			 bool buttons_only,
			 float button_size,			
			 float min_band_size,
			 const shared_instance<render_material_id>& left_icon,
			 const shared_instance<render_material_id>& up_icon,
			 const shared_instance<render_material_id>& right_icon,
			 const shared_instance<render_material_id>& down_icon,
			 const vector2i& icon_pixel_size);


		const shared_instance<gui_knob_style> knob_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_frame_style> band_style;


		/**
		 * If set, the scrollbar hides its band and knob.
		 * Useful for very tight layout, e.g. when the content
		 * is arranged in place of the scrollbar band.
		 * Note that even a button-only scrollbar respects
		 * the style-specific @bref{min_band_size}.
		 */
		const bool buttons_only;


		/**
		 * Size, in GUI coordinates, of scrollbar buttons.
		 * Also determines height for horizontal and width
		 * for vertical scrollbars.
		 */
		const float button_size;


		/** Minimum length of the scrollbar band. */
		const float min_band_size;
	

		const shared_instance<render_material_id>
			left_icon, up_icon, right_icon, down_icon;
		const vector2i icon_pixel_size;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_SCROLL_HPP) */