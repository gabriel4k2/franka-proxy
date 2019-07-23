/**
 *************************************************************************
 *
 * @file gui_scroll_area.hpp
 *
 * Scrolling areas.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_SCROLL_AREA_HPP)
#define INCLUDED__VIRAL_GUI__GUI_SCROLL_AREA_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_scroll_area
 * @ingroup gui_module
 *
 * Allows for scrolling content() when the content's preferred_size()
 * overflows the arrange_area() set by the parent gui_element.
 *
 * Layout notes:
 * - A scroll area always places content() in an arrange_area() at least
 *		of content()->preferred_size(), plus any extra space available.
 * - If the arrange_area() of a scroll area cannot hold the fully-sized
 *		content arrangement area (as above), the scroll area clips its
 *		content to some visible section and optionally shows scrollbars.
 *		See gui_scrollbar_style::scrollbar_show_type for display options.
 * - Usually, a scroll area prefers size to show its entire content().
 *		Containers that favor clipping over reducing content size
 *		(e.g. an unweighted gui_grid) thus will never show scrollbars.
 *		Clients can fix this by specifing an explicit override size
 *		which the scroll area reports instead of actual content size.
 *		See gui_scroll_area_style::prefer_override_size.
 * - If the arrange_area() of a scroll area has less size than needed
 *		for scrollbars, the scroll area exceeds its arrange_area() to
 *		show the scrollbars, but still respects its clip_area().
 *
 ************************************************************************/
class gui_scroll_area :
	public gui_element
{

	public:

		enum scrollbar_show_type
		{
			/**
			 * Only show scrollbar on content() overflow.
			 * Hide scrollbar if content() fits into arrange_area().
			 */
			show_if_required,


			/** 
			 * Do not show scrollbar on content() overflow.
			 * Scrolling with set_position() remains possible.
			 */
			show_never,


			/**
			 * Show scrollbar even if content() fits into arrange_area().
			 * Ensures a consistent layout for frequently-resizing content.
			 */
			show_always
		};


		enum scrollbar_arrange_type
		{
			/** Places scrollbars on the right and on the bottom. */
			arrange_default,

			/** Only shows four-way scrollbar buttons. */
			arrange_compact
		};


		gui_scroll_area
			(const string& name, gui_context& gui,
			 const shared_instance<gui_scroll_area_style>& scroll_area_style);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Fetch minimum and maximum position().
		 * May return inconsistent values on layout_invalid()
		 * or if the entire content() is visible at once.
		 */
		//@{
		vector2f range_min() const NOTHROW;
		vector2f range_max() const NOTHROW;
		//@}


		/**
		 * Control behaviour of the internal scrollbars,
		 * as by respective gui_scrollbar methods.
		 *
		 * The scroll area manages page sizes internally,
		 * thus clients cannot explicitly address these.
		 */
		//@{
		void set_item_size(const vector2f& item_size);
		vector2f item_size() const NOTHROW;
		void set_overlap_factor(const vector2f& overlap_factor);
		vector2f overlap_factor() const NOTHROW;
		void set_continuous_increment(const vector2f& continuous_increment);
		vector2f continuous_increment() const NOTHROW;
		//@}


		/**
		 * Fetch or set relative scrolling position in GUI units.
		 * Silently clamps invalid positions into valid range.
		 */
		//@{
		void set_position(const vector2f& position) NOTHROW;
		vector2f position() const NOTHROW;
		//@}


		/**
		 * Scrolling position(), normalized to the [0, 1] interval.
		 * Queries may return inconsistent values on empty ranges.
		 */
		//@{
		void set_position_normalized(const vector2f& position_normalized) NOTHROW;
		vector2f position_normalized() const NOTHROW;
		//@}


		/**
		 * Scroll by smallest amount so that a @bref{point} or
		 * @bref{rect}angle in scrolling space becomes visible
		 * on the current page.
		 *
		 * If the request cannot be fulfilled (e.g. due to targets
		 * outside range or rectangles larger than page size),
		 * scroll to a next-best match instead.
		 */
		//@{
		void scroll_page_to(const vector2f& point) NOTHROW;
		void scroll_page_to(const rect2f& rect) NOTHROW;
		//@}


		/**
		 * Determine if the current page shows some @bref{point}.
		 * Note this does not consider clipping or layout,
		 * but only takes position() and page size into account.
		 */
		bool is_visible_on_page(const vector2f& point) const NOTHROW;


		/**
		 * Convert between scroll-space [range_min(), range_max()]
		 * and absolute GUI space coordinates.
		 * Returns zero vectors on layout_invalid().
		 */
		//@{
		vector2f to_scroll_space(const vector2f& gui_space) const NOTHROW;
		vector2f to_gui_space(const vector2f& scroll_space) const NOTHROW;
		//@}
	

		void set_content(const shared_pointer<gui_element>& content);
		const shared_pointer<gui_element>& content() const NOTHROW;
		void clear_content() NOTHROW;


		const shared_instance<gui_scroll_area_style> scroll_area_style;


	private:

		void realize_layout() override;


		static const float range_epsilon_;

		rect2f main_area_;
		bool main_area_valid_;

		shared_pointer<gui_element> content_;

		const shared_instance<gui_scrollbar> scrollbar_horizontal_;
		const shared_instance<gui_scrollbar> scrollbar_vertical_;
};


/**
 * Notifies clients that the user scrolled a gui_scroll_area.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_scroll_area> gui_scroll_area_event;




/**
 *************************************************************************
 *
 * @class gui_scroll_area_style
 * @ingroup gui_module
 * 
 * Styling for a gui_scroll_area.
 *
 ************************************************************************/
class gui_scroll_area_style :
	public gui_style
{
	public:

		gui_scroll_area_style
			(const shared_instance<gui_scrollbar_style>& scrollbar_style,
			 bool prefer_override_size,
			 const vector2f& override_size,
			 gui_scroll_area::scrollbar_show_type scrollbar_show_horizontal,
			 gui_scroll_area::scrollbar_show_type scrollbar_show_vertical,
			 gui_scroll_area::scrollbar_arrange_type scrollbar_arrange);


		const shared_instance<gui_scrollbar_style> scrollbar_style;

		const bool prefer_override_size;
		const vector2f override_size;

		const gui_scroll_area::scrollbar_show_type scrollbar_show_horizontal;
		const gui_scroll_area::scrollbar_show_type scrollbar_show_vertical;
		const gui_scroll_area::scrollbar_arrange_type scrollbar_arrange;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_SCROLL_AREA_HPP) */