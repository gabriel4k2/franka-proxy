/**
 *************************************************************************
 *
 * @file gui_tooltip_area.hpp
 *
 * Area with tooltips for the GUI.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_TOOLTIP_AREA_HPP)
#define INCLUDED__VIRAL_GUI__GUI_TOOLTIP_AREA_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/image_forward.hpp>
#include <viral_core/timer_forward.hpp>


#include <viral_core/owning_map.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_tooltip_area
 * @ingroup gui_module
 *
 * Shows gui_tooltip%s next to the mouse cursor.
 *
 * Tooltip overview:
 * - Clients can specify tooltips (through text and an optional image)
 *		that the gui_tooltip_area should show for given gui_element%s.
 * - On each gui_tick_event, the gui_tooltip_area considers tooltips
 *		for gui_context::mouse_focus() and its parent gui_element%s,
 *		and uses the tooltip nearest in hierarchy to mouse_focus().
 * - Registered tooltips do not keep respective gui_element%s alive.
 *		Destroyed elements do not show tooltips, but clients should
 *		remove_tooltip()s or purge_tooltips() at a convenient time
 *		to delete tooltips for any destroyed gui_element%s.
 * 
 * Layouting notes:
 * - The gui_tooltip_area transparently passes through all rendering
 *		(including layout() areas) and all input to its content().
 * - The gui_tooltip_area shows gui_tooltip%s at their preferred_size(),
 *		but if possible at a clipping-free position in arrange_area()
 *		and outside @bref{gui_tooltip_area_style::tooltip_dead_zone}
 *		around the mouse cursor.
 *
 ************************************************************************/
class gui_tooltip_area :
	public gui_element
{

	public:

		gui_tooltip_area
			(const string& name, gui_context& gui,
			 const shared_instance<gui_tooltip_area_style>& tooltip_area_style);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/**
		 * Register a tooltip @bref{text} for an @bref{element}.
		 * Optionally, add an accompanying image to the tooltip.
		 * See gui_image for image parameters.
		 *
		 * If the element already has any registered tooltip,
		 * that tooltip is silently replaced.
		 */
		//@{
		void register_tooltip
			(const shared_instance<gui_element>& element,
			 const string& text);

		void register_tooltip
			(const shared_instance<gui_element>& element,
			 const string& text,
			 const shared_instance<render_material_id>& material,
			 const vector2i& image_size);

		void register_tooltip
			(const shared_instance<gui_element>& element,
			 const string& text,
			 const image_rgba& source_image,
			 const render_material_data& material_data);
		//@}


		/**
		 * Remove the tooltip from some @bref{element}.
		 * Silently ignores non-contained elements.
		 */
		void remove_tooltip
			(const shared_instance<gui_element>& element) NOTHROW;


		/** Remove tooltips for deleted gui_element%s. */
		void purge_tooltips() NOTHROW;


		/** Manage content of the tooltip area. */
		//@{
		void set_content
			(const shared_pointer<gui_element>& content);

		const shared_pointer<gui_element>& content() const NOTHROW;
		void clear_content() NOTHROW;
		//@}


		const shared_instance<gui_tooltip_area_style> tooltip_area_style;


	private:

		void realize_layout() override;

		void process_tick_event();
		void process_mouse_event
			(gui_mouse_event &mouse_event);

		void update_tooltip_source();
		void wait_or_show_tooltip();
		void hide_tooltip_by_tolerance();

		
		typedef owning_map<weak_pointer<gui_element>, gui_tooltip_data>
			tooltip_data_map;

		tooltip_data_map tooltip_data_;

		shared_pointer<gui_element> content_;
		shared_pointer<gui_element> tooltip_;

		const auto_pointer<free_timer> tooltip_timer_;

		weak_pointer<gui_element> tooltip_focus_;
		weak_pointer<gui_element> tooltip_source_;

		vector2f tooltip_cursor_;
		vector2f current_cursor_;
};




/**
 *************************************************************************
 *
 * @class gui_tooltip_area_style
 * @ingroup gui_module
 *
 * Styling for a gui_tooltip_area element.
 *
 ************************************************************************/
class gui_tooltip_area_style :
	public gui_style
{

	public:

		gui_tooltip_area_style
			(const shared_instance<gui_tooltip_style>& tooltip_style,
			 const rect2f& tooltip_dead_zone,
			 float tooltip_show_tolerance,
			 float tooltip_show_seconds);


		const shared_instance<gui_tooltip_style> tooltip_style;


		/**
		 * Zone in which no tooltips should be displayed
		 * around the mouse cursor.
		 */
		const rect2f tooltip_dead_zone;


		/**
		 * If the mouse cursor moves by more than this tolerance,
		 * hide the gui_tooltip and restart timing.
		 */
		const float tooltip_show_tolerance;


		/**
		 * Seconds that the mouse cursor must move
		 * less than @bref{tooltip_show_tolerance}
		 * in a gui_element before tooltips appear.
		 */
		const float tooltip_show_seconds;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_TOOLTIP_AREA_HPP) */
