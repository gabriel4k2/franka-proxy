/**
 *************************************************************************
 *
 * @file gui_window.hpp
 *
 * GUI window facilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_WINDOW_HPP)
#define INCLUDED__VIRAL_GUI__GUI_WINDOW_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/list.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_window
 * @ingroup gui_module
 *
 * A window that floats within a gui_window_area.
 *
 * Usage notes:
 * - A window has but a single client-side child, which in turn holds
 *		any further gui_element%s within the window. This accomodates
 *		varying internal containers, e.g. gui_grid or gui_scroll_area.
 * - A window position()s itself in the arrange_area() of its parent,
 *		and always occupies space for window frames and client_size().
 * - Windows may automatically change their position() and size()
 *		to ensure that window frames remain partially visible.
 * - Only a gui_window_area must be the parent of any gui_window%s
 *		to ensure correct window rendering and input handling.
 *
 ************************************************************************/
class gui_window :
	public gui_element
{
	friend class gui_window_area;


	public:

		enum window_resize_policy
		{
			/**
			 * Use set_client_size() to apply a fixed size.
			 * The user cannot resize the window.
			 */
			fixed_size,


			/**
			 * Automatically resize the window's client area
			 * to match the preferred_size() of its content.
			 */
			match_content_size,


			/**
			 * Allow the user to arbitrarily resize the window
			 * within minimum and maximum possible size.
			 * Use set_client_size() or resize_to_content()
			 * to initialize window size.
			 */
			user_resize,


			/**
			 * Similar to the user_resize policy, but restricts
			 * minimum client size to preferred content size.
			 */
			user_resize_enlarge
		};


		gui_window
			(const string& name, gui_context& gui,
			 const shared_instance<gui_window_style>& window_style,
			 const string& caption,
			 bool allow_close,
			 const window_resize_policy resize_policy);


		vector2f preferred_size() const override;


		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		bool try_accept_key_focus() NOTHROW override;
		void lost_key_focus() NOTHROW override;


		void set_caption(const string& caption);
		const string& caption() const;


		/** Position and size of window within arrange_area(). */
		//@{
		void set_position(const vector2f& position);
		const vector2f& position() const;
		void set_client_size(const vector2f& client_size);
		const vector2f& client_size() const;
		//@}


		/** Center window within arrange_area() on next layout. */
		void center_window_delayed();

		/** Set client_size() to preferred_size() of content(). */
		void resize_to_preferred_content();


		void set_content(const shared_pointer<gui_element>& content);
		const shared_pointer<gui_element>& content() const NOTHROW;
		void clear_content() NOTHROW;


		const shared_instance<gui_window_style> window_style;


		/**
		 * If set, the window does not show the close button
		 * and does not generate gui_window_close_event%s.
		 */
		const bool allow_close;


		const window_resize_policy resize_policy;


	private:

		enum drag_type
			{ no_drag, drag_move, drag_resize_neg, drag_resize_pos };


		void realize_layout() override;

		void process_window_button_input
			(gui_input_event& input, gui_output_event_list& output);
		void process_window_drag_input
			(gui_input_event& input, gui_output_event_list& output);
		void process_window_background_input
			(gui_input_event& input);

		void drag_types_from_cursor
			(const vector2f& cursor,
			 drag_type& drag_type_x, drag_type& drag_type_y) const NOTHROW;

		void try_apply_resizing_cursor(const vector2f& cursor) const;

		bool try_start_dragging(const vector2f& cursor);
		void update_dragging(const vector2f& cursor);
		void stop_dragging();

		void enforce_position_and_size_constraints
			(bool resize_to_make_visible_x,
			 bool resize_to_make_visible_y,
			 bool resize_maintain_end_x,
			 bool resize_maintain_end_y);


		vector2f position_;
		vector2f client_size_;

		rect2f hot_area_move_;
		rect2f hot_area_move_clipped_;
		rect2f hot_area_resize_;
		rect2f hot_area_resize_clipped_;
		bool hot_areas_valid_;

		vector2f drag_start_;
		vector2f drag_start_position_;
		vector2f drag_start_client_size_;

		drag_type drag_type_x_, drag_type_y_;

		bool center_on_next_layout_;

		const shared_instance<gui_frame> frame_default_;
		const shared_instance<gui_frame> frame_key_focus_overlay_;

		const shared_instance<gui_label> caption_label_;
		const shared_instance<gui_button> close_button_;

		shared_pointer<gui_element> content_;
};


/**
 * Indicates that the user clicked the close button.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_window> gui_window_close_event;




/**
 *************************************************************************
 *
 * @class gui_window_style
 * @ingroup gui_module
 *
 * Styling for a gui_window.
 *
 ************************************************************************/
class gui_window_style :
	public gui_style
{
	public:

		gui_window_style
			(const shared_instance<gui_frame_style>& default_style,
			 const shared_instance<gui_frame_style>& key_focus_overlay_style,

			 const vector2f& caption_offset,
			 float caption_height,
			 float caption_right_spacing,
			 float caption_button_spacing,

			 const shared_instance<gui_label_style>& caption_label_style,

			 const shared_instance<gui_button_style>& close_button_style,
			 const shared_instance<render_material_id>& close_icon,
			 const vector2i& close_icon_pixel_size,

			 const shared_instance<gui_cursor_style>& cursor_resize_lr,
			 const shared_instance<gui_cursor_style>& cursor_resize_ud,
			 const shared_instance<gui_cursor_style>& cursor_resize_ul_lr,
			 const shared_instance<gui_cursor_style>& cursor_resize_ll_ur,

			 const vector2f& hot_areas_padding_00,
			 const vector2f& hot_areas_padding_22,
			 float hot_area_move_height,
			 float hot_area_resize_thickness,

			 const vector2f& min_visible_hot_area_move_00,
			 const vector2f& min_visible_hot_area_move_22,
			 const vector2f& min_client_size,
			 const vector2f& max_client_size);


		const shared_instance<gui_frame_style> default_style;
		const shared_instance<gui_frame_style> key_focus_overlay_style;


		/** Positioning of the caption bar within the window frame. */
		//@{
		const vector2f caption_offset;
		const float caption_height;
		const float caption_right_spacing;
		const float caption_button_spacing;
		//@}


		const shared_instance<gui_label_style> caption_label_style;


		const shared_instance<gui_button_style> close_button_style;
		const shared_instance<render_material_id> close_icon;
		const vector2i close_icon_pixel_size;


		const shared_instance<gui_cursor_style> cursor_resize_lr;
		const shared_instance<gui_cursor_style> cursor_resize_ud;
		const shared_instance<gui_cursor_style> cursor_resize_ul_lr;
		const shared_instance<gui_cursor_style> cursor_resize_ll_ur;


		/**
		 * Position of the hot areas for moving
		 * and resizing within the window frame.
		 */
		//@{
		const vector2f hot_areas_padding_00;
		const vector2f hot_areas_padding_22;
		const float hot_area_move_height;
		const float hot_area_resize_thickness;
		//@}


		/**
		 * Minimum parts of the hot area for window movement
		 * (i.e. the caption bar) that should be visible in
		 * the layout area of the window.
		 */
		//@{
		const vector2f min_visible_hot_area_move_00;
		const vector2f min_visible_hot_area_move_22;
		//@}


		const vector2f min_client_size;
		const vector2f max_client_size;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_WINDOW_HPP) */
