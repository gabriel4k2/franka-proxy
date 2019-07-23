/**
 *************************************************************************
 *
 * @file gui_knob.hpp
 *
 * GUI knob.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_KNOB_HPP)
#define INCLUDED__VIRAL_GUI__GUI_KNOB_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_knob
 * @ingroup gui_module
 *
 * Allows for vertical or horizontal dragging in a restricted area.
 *
 * Layout notes:
 * - A GUI knob prefers suitable size for a knob_size()d gui_button
 *		and some additional style-specific minimum dragging space.
 *		The knob clips if its arrange_area() provides less space.
 * - A knob uses any excess layout space in @bref{orientation} direction 
 *		as dragging range, and resizes its button in the other direction.
 *
 ************************************************************************/
class gui_knob :
	public gui_element
{

	public:

		gui_knob
			(const string& name, gui_context& gui,
			 const shared_instance<gui_knob_style>& knob_style,
			 orientation_type orientation);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/** See gui_button::read_only(). */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/**
		 * Display size (in GUI units) of knob.
		 * Clients can successfully set arbitrary knob_size()s,
		 * but the knob still occupies at least minimum_knob_size().
		 */
		//@{
		void set_knob_size(float knob_size);
		float knob_size() const;
		float minimum_knob_size() const;
		//@}


		/** Position within arrange_area(), normalized to [0,1]. */
		//@{
		void set_position_normalized(float position_normalized);
		float position_normalized() const;
		//@}

	
		const shared_instance<gui_knob_style> knob_style;
		const orientation_type orientation;


	private:

		void realize_layout() override;

		void start_dragging(const vector2f& drag_start);
		void stop_dragging();

		void update_dragging
			(const vector2f& drag_now,
			 gui_output_event_list& output);

	
		static const float drag_range_epsilon_;

		float knob_size_;
		float position_normalized_;

		rect2f hot_area_clipped_;
		bool hot_area_valid_;

		vector2f drag_start_;
		float drag_start_normalized_;
		bool dragging_;

		const shared_instance<gui_button> knob_button_;
};


/**
 * Notifies clients that the user dragged a gui_knob.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_knob> gui_knob_event;




/**
 *************************************************************************
 *
 * @class gui_knob_style
 * @ingroup gui_module
 *
 * Styling for a gui_knob.
 *
 ************************************************************************/
class gui_knob_style :
	public gui_style
{

	public:

		gui_knob_style
			(const shared_instance<gui_button_style>& button_style,
			 float min_drag_range,
			 float drag_tolerance);

		const shared_instance<gui_button_style> button_style;

		/** Minimum drag range a knob prefers before clipping. */
		const float min_drag_range;

		/** Position tolerance for continued mouse dragging of a knob. */
		const float drag_tolerance;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_KNOB_HPP) */