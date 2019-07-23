/**
 *************************************************************************
 *
 * @file gui_button.hpp
 *
 * GUI buttons.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_BUTTON_HPP)
#define INCLUDED__VIRAL_GUI__GUI_BUTTON_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/image_forward.hpp>
#include <viral_core/timer_forward.hpp>


#include <viral_core/auto_pointer.hpp>
#include <viral_core/shared_pointer.hpp>


#include "gui_forward.hpp"


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_button
 * @ingroup gui_module
 *
 * A simple, clickable button, labelled with an image and/or a text.
 *
 * Layout notes:
 * - Buttons occupy the larger of arrange_area() and preferred_size().
 * - Regardless of arrangement, buttons always restrict rendering
 *		and input reactions to their clip_area().
 * - Button text and images always layout within the default frame
 *		(e.g. with gui_button_style::default_style). This ensures
 *		consistent layout regardless of any current button state.
 *		Notably, layouting ignores padding on other frame styles.
 *
 ************************************************************************/
class gui_button :
	public gui_element
{

	public:

		enum behaviour_type
		{
			/**
			 * Generates a single gui_button_event after
			 * each click-release cycle within the button.
			 */
			click_button,


			/**
			 * Generates gui_button_event%s both on click and
			 * on later release. Sends gui_button_hold_event%s
			 * on each update even without user input if down()
			 * for more than gui_button_style::hold_delay_seconds.
			 */
			hold_button,


			/**
			 * Generates a single gui_button_event and
			 * toggles down() after each click-release cycle.
			 */
			toggle_button,


			/**
			 * Similar to toggle_button, but the user cannot
			 * raise the button once it is down(). Instead,
			 * the button sends gui_button_locked_event%s.
			 */
			lock_button,


			/**
			 * Send a gui_button_event on each initial click,
			 * but never change the button down() state.
			 * Clients must set_down() explicitly if required.
			 */
			probe_button
		};


		gui_button
			(const string& name, gui_context& gui,
			 const shared_instance<gui_button_style>& button_style,
			 behaviour_type behaviour);


		vector2f preferred_size() const override;


		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		bool try_accept_key_focus() NOTHROW override;
		void lost_key_focus() NOTHROW override;

		bool try_accept_mouse_focus() NOTHROW override;
		void lost_mouse_focus() NOTHROW override;


		/**
		 * Access current button state.
		 *
		 * Buttons ignore set_down() if their @bref{behaviour}
		 * does not imply a permanently latching down() state
		 * (e.g. @bref{click_button} or @bref{hold_button}). 
		 */
		//@{
		void set_down(bool down) NOTHROW;
		bool down() const NOTHROW;
		//@}


		/**
		 * Disable mouse and keyboard reactions. 
		 * Note code can still modify the button state.
		 */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/** Text shown on the button. */
		//@{
		void set_text(const string& text);
		string text() const;
		void clear_text() NOTHROW;
		//@}


		/** Image shown to the left of text(). */
		//@{
		void set_graphics
			(const image_rgba& graphics,
			 const render_material_data& material_data);
		void set_graphics
			(const vector2i& image_size,
			 const shared_instance<render_material_id>& material);

		void clear_graphics() NOTHROW;
		//@}


		const shared_instance<gui_button_style> button_style;
		const behaviour_type behaviour;


	private:

		void realize_layout() override;
		vector2f calculate_content_size() const;

		void process_tick_event
			(gui_output_event_list& output);
		void process_input_source_event
			(gui_input_source_event& input_source_event,
			 gui_output_event_list& output);
		void process_mouse_event
			(gui_mouse_event &mouse_event,
			 gui_output_event_list& output);

		void start_click_release_cycle(gui_output_event_list& output);
		void finish_click_release_cycle(gui_output_event_list& output);
		void abort_click_release_cycle(gui_output_event_list& output);
		void abort_click_release_cycle_no_events() NOTHROW;

		void create_button_event(gui_output_event_list& output);
		void create_button_locked_event(gui_output_event_list& output);
		bool create_button_repeat_event(gui_output_event_list& output);

		void update_down(bool down) NOTHROW;
		void update_read_only(bool read_only) NOTHROW;


		rect2f hot_area_;
		rect2f hot_area_clipped_;
		bool hot_area_valid_;
		bool in_click_release_cycle_;

		bool down_;
		bool read_only_;

		const auto_pointer<free_timer> hold_timer_;
		bool lock_button_already_locked_;

		const auto_pointer<free_timer> repeat_timer_;
		bool repeat_timer_valid_;

		const shared_instance<gui_frame> frame_default_;
		const shared_instance<gui_frame> frame_down_;
		const shared_instance<gui_frame> frame_read_only_;
		const shared_instance<gui_frame> frame_key_focus_overlay_;
		const shared_instance<gui_frame> frame_mouse_focus_overlay_;

		shared_pointer<gui_label> label_;
		shared_pointer<gui_image> image_;
};


/**
 * Notifies clients that the user clicked a gui_button.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_button, 0> gui_button_event;


/**
 * Continuously notifies clients that a gui_button::hold_button
 * has been held down for quite some time.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_button, 1> gui_button_hold_event;


/**
 * Sent by gui_button::lock_button if clicked but locked already.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_button, 2> gui_button_locked_event;


/**
 * If gui_button_style::allow_repeat is set and the user clicks
 * the button repeatedly in less than gui_button_style::repeat_seconds,
 * a gui_button_repeat_event occurs in place of each gui_button_event.
 * Only works on gui_button::click_button and gui_button::probe_button.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_button, 3> gui_button_repeat_event;




/**
 *************************************************************************
 *
 * @class gui_button_style
 * @ingroup gui_module
 *
 * Styling for a clickable gui_button.
 *
 ************************************************************************/
class gui_button_style :
	public gui_style
{
	public:

		gui_button_style
			(const shared_instance<gui_frame_style>& default_style,
			 const shared_instance<gui_frame_style>& down_style,
			 const shared_instance<gui_frame_style>& read_only_style,
			 const shared_instance<gui_frame_style>& key_focus_overlay_style,
			 const shared_instance<gui_frame_style>& mouse_focus_overlay_style,
			 const shared_instance<gui_anchor_style>& content_anchor_style,
			 const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_image_style>& image_style,
			 float image_label_spacing,
			 float hold_delay_seconds,
			 bool allow_repeat,
			 float repeat_seconds);


		const shared_instance<gui_frame_style> default_style;
		const shared_instance<gui_frame_style> down_style;
		const shared_instance<gui_frame_style> read_only_style;
		const shared_instance<gui_frame_style> key_focus_overlay_style;
		const shared_instance<gui_frame_style> mouse_focus_overlay_style;

		const shared_instance<gui_anchor_style> content_anchor_style;
		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_image_style> image_style;
		const float image_label_spacing;

		const float hold_delay_seconds;

		const bool allow_repeat;
		const float repeat_seconds;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_BUTTON_HPP) */
