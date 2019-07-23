/**
 *************************************************************************
 *
 * @file gui_color_dialog.hpp
 *
 * Modal color selection dialog.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_COLOR_DIALOG_HPP)
#define INCLUDED__VIRAL_GUI__GUI_COLOR_DIALOG_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/color.hpp>
#include <viral_core/image_raw.hpp>


#include "gui.hpp"
#include "gui_modal.hpp"
#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_color_dialog
 * @ingroup gui_module
 *
 * A modal dialog that allows the user to select a color.
 *
 ************************************************************************/
class gui_color_dialog :
	public gui_modal_dialog
{
	public:

		/**
		 * Show a color selection dialog.
		 *
		 * The dialog initializes to @bref{in_out_color}.
		 * On success, the dialog stores the selected color
		 * in @bref{in_out_color} and show() returns @true.
		 * Otherwise, @bref{in_out_color} remains unchanged,
		 * and show() returns @false.
		 *
		 * On @bref{unsigned_char_mode}, the color dialog UI
		 * simulates traditional integers with [0, 255] range
		 * instead of the internal floats with [0, 1] range.
		 * This affects only the UI, the @bref{in_out_color}
		 * remains in (unquantized) [0, 1] float range.
		 */
		static bool show
			(gui_context& gui,
			 const shared_instance<gui_color_dialog_style>& color_dialog_style,
			 const string& caption,
			 color& in_out_color,
			 bool unsigned_char_mode); 


	private:		

		gui_color_dialog
			(gui_context& gui,
			 const shared_instance<gui_color_dialog_style>& color_dialog_style,
			 const string& caption,
			 color& in_out_color,
			 bool unsigned_char_mode);


		void setup() override; 

		shared_instance<gui_window> dialog_window() override;
		shared_pointer<gui_element> dialog_background() override;


		void process_events
			(const gui_output_event_list& output) override;

		void accept();
		void cancel();

		void synch_color_changed();
		void synch_hsv_edit_changed();
		void synch_rgba_edit_changed();

		void pick_color_h(const vector2f& position);
		void pick_color_sv(const vector2f& position);

		void update_images();


		static vector to_hsv(const color& rgb);
		static color to_rgb(const vector& hsv, float alpha);


		static shared_instance<gui_value_edit> create_value_edit
			(const string& value_edit_name, gui_context& gui,
			 const shared_instance<gui_value_edit_style>& value_edit_style,
			 bool unsigned_char_mode);

		shared_instance<gui_spacer> create_spacer();
		shared_instance<gui_grid> create_vertical_grid();
		shared_instance<gui_grid> create_horizontal_grid();
		shared_instance<gui_grid> create_slider_grid();
		shared_instance<gui_grid> create_button_grid();

		static image_rgba create_preview_image
			(const vector2i& image_size, const vector2i& grid_size,
			 const color& selected_color);
		static image_rgba create_color_picker_h_image
			(const vector2i& image_size, const color& selected_color);
		static image_rgba create_color_picker_sv_image
			(const vector2i& image_size, const color& selected_color);
		static render_material_data create_image_material
			(const shared_instance<render_shader_id>& image_shader);


		static const float hsv_black_epsilon_;
		static const float hsv_gray_epsilon_;
		static const float max_image_error_;

		const shared_instance<gui_color_dialog_style> color_dialog_style_;
		color& in_out_color_;
		const bool unsigned_char_mode_;

		color current_image_color_;

		const shared_instance<gui_window> window_;
		const shared_instance<gui_frame> background_;

		const shared_instance<gui_image_map> preview_;

		const shared_instance<gui_value_edit> hue_edit_;
		const shared_instance<gui_value_edit> saturation_edit_;
		const shared_instance<gui_value_edit> value_edit_;

		const shared_instance<gui_value_edit> red_edit_;
		const shared_instance<gui_value_edit> green_edit_;
		const shared_instance<gui_value_edit> blue_edit_;
		const shared_instance<gui_value_edit> alpha_edit_;

		const shared_instance<gui_image_map> color_picker_h_;
		const shared_instance<gui_image_map> color_picker_sv_;

		const shared_instance<gui_button> ok_button_;
		const shared_instance<gui_button> cancel_button_;

		bool success_;
};




/**
 *************************************************************************
 *
 * @class gui_color_dialog_style
 * @ingroup gui_module
 *
 * Styling options for a gui_color_dialog.
 *
 ************************************************************************/
class gui_color_dialog_style :
	public gui_style
{
	public:

		gui_color_dialog_style
			(const shared_instance<gui_spacer_style>& spacer_style,
			 const shared_instance<gui_label_style>& label_style,
			 const shared_instance<gui_button_style>& button_style,
			 const shared_instance<gui_value_edit_style>& value_edit_style,
			 const shared_instance<gui_image_map_style>& image_map_style,
			 const shared_instance<gui_window_style>& window_style,
			 const shared_instance<gui_frame_style>& background_style,
			 const shared_instance<render_shader_id>& image_shader,
			 const vector2i& preview_pixel_size,
			 const vector2i& preview_grid_pixel_size,
			 const vector2i& color_picker_h_pixel_size,
			 const vector2i& color_picker_sv_pixel_size);


		const shared_instance<gui_spacer_style> spacer_style;
		const shared_instance<gui_label_style> label_style;
		const shared_instance<gui_button_style> button_style;
		const shared_instance<gui_value_edit_style> value_edit_style;
		const shared_instance<gui_image_map_style> image_map_style;
		const shared_instance<gui_window_style> window_style;
		const shared_instance<gui_frame_style> background_style;


		/**
		 * Shader to use for run-time generated images
		 * in preview and color picking areas.
		 */
		const shared_instance<render_shader_id> image_shader; 


		/** Size, in pixels, of run-time generated images. */
		//@{
		const vector2i preview_pixel_size;
		const vector2i preview_grid_pixel_size;

		const vector2i color_picker_h_pixel_size;
		const vector2i color_picker_sv_pixel_size;
		//@}
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_COLOR_DIALOG_HPP) */
