/**
 *************************************************************************
 *
 * @file gui_slider.hpp
 *
 * Sliders for convenient value entry.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_SLIDER_HPP)
#define INCLUDED__VIRAL_GUI__GUI_SLIDER_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/auto_pointer.hpp>
#include <viral_core/buffer.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_slider
 * @ingroup gui_module
 *
 * A standalone value slider, optionally with slider buttons
 * and marks() for special slider value()s.
 *
 * Layout notes:
 * - Into @bref{orientation} direction, a slider prefers enough space
 *		to show its slider band at a style-specific minimum size.
 *		Additionally, the slider requests space for knob overhang
 *		and, if present, for slider buttons plus spacing.
 * - Against @bref{orientation} direction, a slider prefers the larger
 *		of either knob, band, or potentially button size.
 * - A slider at least occupies preferred_size(). With any excess size,
 *		the slider band grows into @bref{orientation}, and all content
 *		centers against @bref{orientation}.
 * - Client code can reasonably work with sliders even on layout_invalid().
 *		In particular, setters and getters for slider range and value
 *		still work as specified. The slider only ignores user GUI input.
 * - The gui_slider_style::min_band_size of a slider must not be less
 *		than gui_knob_style::min_drag_range to avoid usage artifacts.
 *		See gui_knob for general knob layout.
 *
 ************************************************************************/
class gui_slider :
	public gui_element
{
	public:

		enum snap_mode
		{
			/** Show all valid marks(), but do not snap to these. */
			snap_none,


			/**
			 * When the user drags the slider knob or clicks slider buttons,
			 * snap the slider value() to the nearest valid marks() entry
			 * within gui_slider_style::mark_snap_tolerance GUI units.
			 */
			snap_gui,


			/**
			 * Always snap value() to the nearest valid marks() entry.
			 * Forces snapping for set_value() and for any GUI input,
			 * regardless of gui_slider_style::mark_snap_tolerance.
			 * Both range_min() and range_max() are allowed as well,
			 * even if not explicitly included in the marks() array.
			 *
			 * @attent
			 * Client code should still expect and correctly handle
			 * value()s not contained in the current marks() array.
			 */
			snap_force
		};


		gui_slider
			(const string& name, gui_context& gui,
			 const shared_instance<gui_slider_style>& slider_style,
			 orientation_type orientation,
			 snap_mode snap);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Prohibit the user from changing value() via the UI.
		 * Does not prohibit function calls from changing value().
		 */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/**
		 * Marks for specific values on the slider.
		 * Behaviour depends on current @bref{snap} mode.
		 * The slider silently ignores any marks outside
		 * the current [range_min(), range_max()] interval.	
		 */
		//@{
		void set_marks(const dynamic_buffer<float>& marks);
		const dynamic_buffer<float>& marks() const NOTHROW;
		void clear_marks() NOTHROW;
		//@}


		/**
		 * Access range that restricts the slider value().
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
		 * Value increment per button click, and continuous increment
		 * per GUI logics frame while a button is held.
		 */
		//@{
		void set_click_increment(float click_increment);
		float click_increment() const NOTHROW;
		void set_continuous_increment(float continuous_increment);
		float continuous_increment() const NOTHROW;
		//@}


		/**
		 * Access slider value within range_min() and range_max().
		 * Setting @bref{value}s potentially @bref{snap}s to marks(),
		 * and invalid @bref{value}s automatically clamp_to_range().
		 */
		//@{
		void set_value(float value) NOTHROW;
		float value() const NOTHROW;
		float clamp_to_range(float value) const NOTHROW;
		//@}


		/**
		 * Slider value(), normalized to the [0, 1] interval.
		 * Queries may return inconsistent values on empty ranges.
		 */
		//@{
		void set_value_normalized(float value_normalized) NOTHROW;
		float value_normalized() const NOTHROW;
		//@}


		const shared_instance<gui_slider_style> slider_style;
		const orientation_type orientation;
		const snap_mode snap;


	private:

		void realize_layout() override;

		void process_child_input
			(gui_input_event& input, gui_output_event_list& output);
		void process_band_input
			(gui_input_event& input, gui_output_event_list& output);
		void create_output_event
			(gui_output_event_list& output);

		void update_read_only() NOTHROW;

		void pack_marks(gui_render_packer& packer);

		float snap_mark_gui(float value) const NOTHROW;
		float snap_mark_nearest(float value) const NOTHROW;


		static const int64 max_mark_count_;
		static const float range_epsilon_;

		bool read_only_;

		dynamic_buffer<float> marks_;

		float range_min_;
		float range_max_;

		float click_increment_;
		float continuous_increment_;

		float value_;

		rect2f hot_area_band_;
		rect2f hot_area_band_clipped_;
		bool hot_area_band_valid_;

		const shared_instance<gui_knob> knob_;
		const shared_instance<gui_button> increment_;
		const shared_instance<gui_button> decrement_;
		const shared_instance<gui_frame> band_;
};


/**
 * Notifies clients that the user changed the value of a gui_slider.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_slider> gui_slider_event;




/**
 *************************************************************************
 *
 * @class gui_slider_style
 * @ingroup gui_module
 *
 * Styling for a gui_slider.
 *
 ************************************************************************/
class gui_slider_style :
	public gui_style
{

	public:

		gui_slider_style
			(const shared_instance<gui_knob_style>& knob_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_frame_style>& band_style,
			 bool show_buttons,
			 float button_size,
			 float button_spacing,
			 const shared_instance<render_material_id>& increment_icon,
			 const shared_instance<render_material_id>& decrement_icon,
			 const vector2i& button_icon_pixel_size,
			 float fixed_band_size,
			 float min_band_size,
			 const shared_instance<render_material_id>& mark_icon,
			 float mark_icon_size,
			 float mark_snap_tolerance);


		const shared_instance<gui_knob_style> knob_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_frame_style> band_style;


		/** Size, spacing, and icons for optional spin buttons. */
		//@{
		const bool show_buttons;

		const float button_size;
		const float button_spacing;
		
		const shared_instance<render_material_id>
			increment_icon, decrement_icon;
		const vector2i button_icon_pixel_size;
		//@}


		/** Fixed width and minimum length of the slider band. */
		//@{
		const float fixed_band_size;
		const float min_band_size;
		//@}


		/** Graphics and snap tolerance for the slider marks. */
		//@{
		const shared_instance<render_material_id> mark_icon;
		const float mark_icon_size;
		const float mark_snap_tolerance;
		//@}
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_SLIDER_HPP) */