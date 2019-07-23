/**
 *************************************************************************
 *
 * @file gui_color_dialog.cpp
 *
 * Modal color selection dialog, implementation.
 *
 ************************************************************************/


#include "gui_color_dialog.hpp"


#include <math.h>


#include <viral_core/log.hpp>
#include <viral_core/integer_operations.hpp>
#include <viral_core/render_resource.hpp>


#include "gui_event.hpp"
#include "gui_spacer.hpp"
#include "gui_grid.hpp"
#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_frame.hpp"
#include "gui_button.hpp"
#include "gui_value_edit.hpp"
#include "gui_image_map.hpp"
#include "gui_window.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_color_dialog_converter
 * @ingroup gui_module
 *
 * Creates text from color values for display in a gui_color_dialog.
 *
 ************************************************************************/
class gui_color_dialog_converter :
	public gui_value_edit_converter
{
	friend class gui_color_dialog;

	private:

		gui_color_dialog_converter(bool unsigned_char_mode) :
			unsigned_char_mode_(unsigned_char_mode) { }


		float to_value(const string& text) override
		{
			return
				max(0.f, min(1.f, 
					unsigned_char_mode_ ?
						(text.to_int32() / 255.f) : text.to_float()));
		}

		string to_text(float value) override
		{ 
			string value_text
				(integer_operations::cast_or_clamp<int64>
					(max(0.f, min(1.f, value)) *
					 (unsigned_char_mode_ ? 255.f : 1000.f)));

			if (unsigned_char_mode_)
				return value_text;

			// Somewhat cumbersome fixed-precision workaround.
			// Avoids the uglities of variable float precision 
			// within normal string-from-float constructors.
			while (value_text.size() < 4)
				value_text.push_front('0');

			string fixed_precision;
			fixed_precision.append(value_text.at(0));
			fixed_precision.append('.');
			fixed_precision.append(value_text.at(1));
			fixed_precision.append(value_text.at(2));
			fixed_precision.append(value_text.at(3));
			return fixed_precision;
		}


		const bool unsigned_char_mode_;
};




//////////////////////////////////////////////////////////////////////////
//
// gui_color_dialog
//
//////////////////////////////////////////////////////////////////////////


bool gui_color_dialog::show
	(gui_context& gui,
	 const shared_instance<gui_color_dialog_style>& color_dialog_style,
	 const string& caption,
	 color& in_out_color,
	 bool unsigned_char_mode)
{
	color internal_in_out_color(in_out_color);

	gui_color_dialog color_dialog
		(gui, color_dialog_style,
		 caption, internal_in_out_color, unsigned_char_mode);

	color_dialog.execute();

	if (color_dialog.success_)
		in_out_color = internal_in_out_color;

	return color_dialog.success_;
}


gui_color_dialog::gui_color_dialog
	(gui_context& gui,
	 const shared_instance<gui_color_dialog_style>& color_dialog_style,
	 const string& caption,
	 color& in_out_color,
	 bool unsigned_char_mode)
	:
	gui_modal_dialog(gui),
	color_dialog_style_(color_dialog_style),
	in_out_color_(in_out_color),
	unsigned_char_mode_(unsigned_char_mode),

	current_image_color_(in_out_color),

	window_(new gui_window
				("color_dialog_window", gui, color_dialog_style().window_style,
				 caption, true, gui_window::match_content_size)),
	background_(new gui_frame("color_dialog_background", gui,
							  color_dialog_style().background_style)),

	preview_(new gui_image_map
		("color_dialog_preview", gui,
		 color_dialog_style().image_map_style,
		 create_preview_image
			(color_dialog_style().preview_pixel_size,
			 color_dialog_style().preview_grid_pixel_size,
			 current_image_color_),
		 create_image_material
			(color_dialog_style().image_shader),
		 false)),

	hue_edit_
		(create_value_edit("color_dialog_hue", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),
	saturation_edit_
		(create_value_edit("color_dialog_saturation", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),
	value_edit_
		(create_value_edit("color_dialog_value", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),

	red_edit_
		(create_value_edit("color_dialog_red", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),
	green_edit_
		(create_value_edit("color_dialog_green", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),
	blue_edit_
		(create_value_edit("color_dialog_blue", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),
	alpha_edit_
		(create_value_edit("color_dialog_alpha", gui,
						   color_dialog_style().value_edit_style, unsigned_char_mode)),

	color_picker_h_(new gui_image_map
		("color_dialog_color_picker_h", gui,
		 color_dialog_style().image_map_style,
		 create_color_picker_h_image
			(color_dialog_style().color_picker_h_pixel_size,
			 current_image_color_),
		 create_image_material
			(color_dialog_style().image_shader),
		 true)),

	color_picker_sv_(new gui_image_map
		("color_dialog_color_picker_sv", gui,
		 color_dialog_style().image_map_style,
		 create_color_picker_sv_image
			(color_dialog_style().color_picker_sv_pixel_size,
			 current_image_color_),
		 create_image_material
			(color_dialog_style().image_shader),
		 true)),

	ok_button_(new gui_button
		("file_dialog_ok_button", gui,
		 color_dialog_style().button_style, gui_button::click_button)),
	cancel_button_(new gui_button
		("file_dialog_cancel_button", gui,
		 color_dialog_style().button_style, gui_button::click_button)),

	success_(false)
{
	ok_button_().set_text("OK");
	cancel_button_().set_text("Cancel");

	window_().set_content(create_vertical_grid());
	window_().resize_to_preferred_content();
	window_().center_window_delayed();

	synch_color_changed();
}


void gui_color_dialog::setup()
{
	gui_modal_dialog::setup();
	ok_button_().acquire_key_focus();
}


shared_instance<gui_window> gui_color_dialog::dialog_window()
	{ return window_; }
shared_pointer<gui_element> gui_color_dialog::dialog_background()
	{ return background_; }


void gui_color_dialog::process_events
	(const gui_output_event_list& output)
{
	for (gui_output_event_list::const_iterator it
			(output.first()); it; ++it)
	{
		if (!*it) continue;

		const shared_instance<gui_element> &source =
			(*it)->source_untyped;
		
		if (typeid(**it) == typeid(gui_button_event))
		{
			if (source == ok_button_)
				accept();
			else if (source == cancel_button_)
				cancel();
		}

		if (typeid(**it) == typeid(gui_value_edit_event))
		{
			if (source == hue_edit_ ||
				source == saturation_edit_ ||
				source == value_edit_)
			{
				synch_hsv_edit_changed();
			}
			else if (source == red_edit_ ||
					 source == green_edit_ ||
					 source == blue_edit_ ||
					 source == alpha_edit_)
			{
				synch_rgba_edit_changed();
			}
		}

		if (typeid(**it) == typeid(gui_image_map_event))
		{
			const gui_image_map_event& image_map_event =
				static_cast<const gui_image_map_event&>(**it);

			if (source == color_picker_h_)
				pick_color_h(image_map_event.normalized_position);
			else if (source == color_picker_sv_)
				pick_color_sv(image_map_event.normalized_position);
		}

		if (typeid(**it) == typeid(gui_window_close_event) &&
			source == window_)
				cancel();
	}
}


void gui_color_dialog::accept()
{
	success_ = true;
	finish();
}


void gui_color_dialog::cancel()
{
	success_ = false;
	finish();
}


void gui_color_dialog::synch_color_changed()
{
	vector hsv(to_hsv(in_out_color_));
	hue_edit_().try_set_value(hsv.x);
	saturation_edit_().try_set_value(hsv.y);
	value_edit_().try_set_value(hsv.z);

	red_edit_().try_set_value(in_out_color_.r);
	green_edit_().try_set_value(in_out_color_.g);
	blue_edit_().try_set_value(in_out_color_.b);
	alpha_edit_().try_set_value(in_out_color_.a);

	update_images();
}


void gui_color_dialog::synch_hsv_edit_changed()
{
	vector hsv
		(hue_edit_().value(),
		 saturation_edit_().value(),
		 value_edit_().value());

	in_out_color_ = to_rgb(hsv, in_out_color_.a);
	red_edit_().try_set_value(in_out_color_.r);
	green_edit_().try_set_value(in_out_color_.g);
	blue_edit_().try_set_value(in_out_color_.b);
	alpha_edit_().try_set_value(in_out_color_.a);

	update_images();
}


void gui_color_dialog::synch_rgba_edit_changed()
{
	in_out_color_ = color
		(red_edit_().value(),
		 green_edit_().value(),
		 blue_edit_().value(),
		 alpha_edit_().value());

	vector hsv(to_hsv(in_out_color_));
	hue_edit_().try_set_value(hsv.x);
	saturation_edit_().try_set_value(hsv.y);
	value_edit_().try_set_value(hsv.z);

	update_images();
}


void gui_color_dialog::pick_color_h(const vector2f& position)
{
	vector hsv(to_hsv(in_out_color_));
	hsv.x = max(0.f, min(1.f, position.y));
	in_out_color_ = to_rgb(hsv, in_out_color_.a);

	synch_color_changed();
}


void gui_color_dialog::pick_color_sv(const vector2f& position)
{
	vector hsv(to_hsv(in_out_color_));
	hsv.y = max(0.f, min(1.f, position.x));
	hsv.z = max(0.f, min(1.f, position.y));
	in_out_color_ = to_rgb(hsv, in_out_color_.a);

	synch_color_changed();
}


void gui_color_dialog::update_images()
{
	// Only update images if the selected color
	// has changed by at least a small threshold.
	// This avoids expensive redundant updates.
	color image_error(current_image_color_ - in_out_color_);
	if (image_error.abs().component_sum() < max_image_error_)
		return;

	current_image_color_ = in_out_color_;

	preview_().apply_source_image
		(create_preview_image
			(color_dialog_style_().preview_pixel_size,
			 color_dialog_style_().preview_grid_pixel_size,
			 current_image_color_),
		 create_image_material
			(color_dialog_style_().image_shader));

	color_picker_h_().apply_source_image
		(create_color_picker_h_image
			(color_dialog_style_().color_picker_h_pixel_size,
			 current_image_color_),
		 create_image_material
			(color_dialog_style_().image_shader));	

	color_picker_sv_().apply_source_image
		(create_color_picker_sv_image
			(color_dialog_style_().color_picker_sv_pixel_size,
			 current_image_color_),
		 create_image_material
			(color_dialog_style_().image_shader));	
}


vector gui_color_dialog::to_hsv(const color& rgb)
{
	/**
	 * Traditional hexcone transformation
	 * that maps from RGB in range [0,1]
	 * to HSV in range [0,1].
	 */

	float r = max(0.f, min(1.f, rgb.r));
	float g = max(0.f, min(1.f, rgb.g));
	float b = max(0.f, min(1.f, rgb.b));

	float rgbmax = max(max(r, g), b);
	float rgbmin = min(min(r, g), b);
	float rgbdif = rgbmax - rgbmin;

	bool has_chroma =
		rgbdif > hsv_gray_epsilon_;

	float hue_denorm =
		(has_chroma && rgbmax == r) ? (0.f + (g - b) / rgbdif) :
		(has_chroma && rgbmax == g) ? (2.f + (b - r) / rgbdif) :
		(has_chroma && rgbmax == b) ? (4.f + (r - g) / rgbdif) : 0.f;

	// Denormalized hue has a range of [-1, 5].
	// We must wrap-around the negative blueish-red hues,
	// and we have to scale the result to [0, 1] range.

	float h =
		(hue_denorm / 6.f) +
			((hue_denorm < 0.f) ? 1.f : 0.f);

	float s =
		rgbmax > hsv_black_epsilon_ ?
			(rgbdif / rgbmax) : 0.f;

	float v = rgbmax;

	// Safety against round-off issues.
	h = max(0.f, min(1.f, h)); 
	s = max(0.f, min(1.f, s));
	v = max(0.f, min(1.f, v));

	return vector(h, s, v);
}


color gui_color_dialog::to_rgb
	(const vector& hsv, float alpha)
{
	/** Inverse transform of the above. */

	float h = max(0.f, min(1.f, hsv.x));
	float s = max(0.f, min(1.f, hsv.y));
	float v = max(0.f, min(1.f, hsv.z));
	float a = max(0.f, min(1.f, alpha));

	float rgbmax = v;
	float rgbdif = s * rgbmax;
	float rgbmin = rgbmax - rgbdif;

	// Scale and offset hue back to denormalized [-1, 5] range.
	// Then determine hue sector, and bias within this sector.

	float hue_denorm =
		(h - ((h > 5.f / 6.f) ? 1.f : 0.f)) * 6.f;

	int32 r_offset = 0;
	if (hue_denorm > 3.f)
		{ r_offset = 1; hue_denorm -= 4.f; }
	else if (hue_denorm > 1.f)
		{ r_offset = 2; hue_denorm -= 2.f; }

	float cols[3] =
		{ rgbmax,
		  rgbmin + rgbdif * max(hue_denorm, 0.f),
		  rgbmin - rgbdif * min(hue_denorm, 0.f) };

	float r = cols[r_offset];
	float g = cols[(r_offset + 1) % 3];
	float b = cols[(r_offset + 2) % 3];

	// Safety against round-off issues.
	r = max(0.f, min(1.f, r));
	g = max(0.f, min(1.f, g));
	b = max(0.f, min(1.f, b));

	return color(r, g, b, a);
}


shared_instance<gui_value_edit>
	gui_color_dialog::create_value_edit
		(const string& value_edit_name, gui_context& gui,
		 const shared_instance<gui_value_edit_style>& value_edit_style,
		 bool unsigned_char_mode)
{
	// Invoked half-way through member initialization.
	// Static to avoid access to uninitialized members.

	shared_instance<gui_value_edit> value_edit
		(new gui_value_edit
			(value_edit_name, gui, value_edit_style,
			 gui_element::horizontal, gui_slider::snap_none));

	value_edit().slider().set_range(0, 1);
	value_edit().set_converter
		(auto_pointer<gui_value_edit_converter>
			(new gui_color_dialog_converter(unsigned_char_mode)));

	return value_edit;
}


shared_instance<gui_spacer> gui_color_dialog::create_spacer()
{
	return shared_instance<gui_spacer>
				(new gui_spacer("color_dialog_spacer", gui,
								color_dialog_style_().spacer_style));
}


shared_instance<gui_grid> gui_color_dialog::create_vertical_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("color_dialog_vgrid", gui, vector2i(1, 3)));
	grid().set_column_weight(0, 1.f);

	grid().insert_content(create_horizontal_grid(), vector2i(0, 0));
	grid().insert_content(create_spacer(), vector2i(0, 1));
	grid().insert_content(create_button_grid(), vector2i(0, 2));

	return grid;
}


shared_instance<gui_grid> gui_color_dialog::create_horizontal_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("color_dialog_hgrid", gui, vector2i(6, 1)));
	grid().set_column_weight(2, 1.f);

	grid().insert_content(preview_, vector2i(0, 0));
	grid().insert_content(create_spacer(), vector2i(1, 0));
	grid().insert_content(create_slider_grid(), vector2i(2, 0));
	grid().insert_content(create_spacer(), vector2i(3, 0));
	grid().insert_content(color_picker_h_, vector2i(4, 0));
	grid().insert_content(color_picker_sv_, vector2i(5, 0));

	return grid;
}


shared_instance<gui_grid> gui_color_dialog::create_slider_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("color_dialog_hgrid", gui, vector2i(2, 9)));
	grid().set_column_weight(1, 1.f);

	shared_instance<gui_label> hue_label
		(new gui_label("color_dialog_hue_label", gui,
					   color_dialog_style_().label_style, "Hue: "));
	shared_instance<gui_label> saturation_label
		(new gui_label("color_dialog_saturation_label", gui,
					   color_dialog_style_().label_style, "Saturation: "));
	shared_instance<gui_label> value_label
		(new gui_label("color_dialog_value_label", gui,
					   color_dialog_style_().label_style, "Value: "));

	shared_instance<gui_label> red_label
		(new gui_label("color_dialog_red_label", gui,
					   color_dialog_style_().label_style, "Red: "));
	shared_instance<gui_label> green_label
		(new gui_label("color_dialog_green_label", gui,
					   color_dialog_style_().label_style, "Green: "));
	shared_instance<gui_label> blue_label
		(new gui_label("color_dialog_blue_label", gui,
					   color_dialog_style_().label_style, "Blue: "));
	shared_instance<gui_label> alpha_label
		(new gui_label("color_dialog_alpha_label", gui,
					   color_dialog_style_().label_style, "Alpha: "));

	grid().insert_content(hue_label, vector2i(0, 0));
	grid().insert_content(hue_edit_, vector2i(1, 0));
	grid().insert_content(saturation_label, vector2i(0, 1));
	grid().insert_content(saturation_edit_, vector2i(1, 1));
	grid().insert_content(value_label, vector2i(0, 2));
	grid().insert_content(value_edit_, vector2i(1, 2));

	grid().insert_content(create_spacer(), vector2i(0, 3));

	grid().insert_content(red_label, vector2i(0, 4));
	grid().insert_content(red_edit_, vector2i(1, 4));
	grid().insert_content(green_label, vector2i(0, 5));
	grid().insert_content(green_edit_, vector2i(1, 5));
	grid().insert_content(blue_label, vector2i(0, 6));
	grid().insert_content(blue_edit_, vector2i(1, 6));

	grid().insert_content(create_spacer(), vector2i(0, 7));

	grid().insert_content(alpha_label, vector2i(0, 8));
	grid().insert_content(alpha_edit_, vector2i(1, 8));

	return grid;
}


shared_instance<gui_grid> gui_color_dialog::create_button_grid()
{
	shared_instance<gui_grid> grid
		(new gui_grid("file_dialog_bgrid", gui, vector2i(3, 1)));
	grid().set_column_weight(0, 1.f);

	grid().insert_content(ok_button_, vector2i(1, 0));
	grid().insert_content(cancel_button_, vector2i(2, 0));
	
	return grid;
}


image_rgba gui_color_dialog::create_preview_image
	(const vector2i& image_size, const vector2i& grid_size,
	 const color& selected_color)
{
	// Invoked half-way through member initialization.
	// Static to avoid access to uninitialized members.


	// Fill upper half of preview image with plain color.
	// Perform float-to-char casts but once for all pixels.
	image_rgba preview_image(image_size);

	unsigned char s_r =
		integer_operations::cast_or_clamp<unsigned char>
			(selected_color.r * 255);
	unsigned char s_g =
		integer_operations::cast_or_clamp<unsigned char>
			(selected_color.g * 255);
	unsigned char s_b = 
		integer_operations::cast_or_clamp<unsigned char>
			(selected_color.b * 255);

	for (int64 y = 0; y < image_size.y / 2; ++y)
	for (int64 x = 0; x < image_size.x; ++x)
	{
		image_rgba::pixel_type& p = preview_image.at(x, y);
		p.r = s_r; p.g = s_g; p.b = s_b; p.a = 255;
	}


	// Render translucent color over checkerboard
	// on lower half of preview to show color alpha.
	// Again perform casts but once for all pixels.
	color grid_color_a
		(color(0, 0, 0, 1).lerp(selected_color, selected_color.a));

	unsigned char ga_r =
		integer_operations::cast_or_clamp<unsigned char>
			(grid_color_a.r * 255);
	unsigned char ga_g =
		integer_operations::cast_or_clamp<unsigned char>
			(grid_color_a.g * 255);
	unsigned char ga_b =
		integer_operations::cast_or_clamp<unsigned char>
			(grid_color_a.b * 255);

	color grid_color_b
		(color(1, 1, 1, 1).lerp(selected_color, selected_color.a));

	unsigned char gb_r =
		integer_operations::cast_or_clamp<unsigned char>
			(grid_color_b.r * 255);
	unsigned char gb_g =
		integer_operations::cast_or_clamp<unsigned char>
			(grid_color_b.g * 255);
	unsigned char gb_b =
		integer_operations::cast_or_clamp<unsigned char>
			(grid_color_b.b * 255);

	for (int64 y = image_size.y / 2; y < image_size.y; ++y)
	for (int64 x = 0; x < image_size.x; ++x)
	{
		int64 grid_linear_index =
			((y - image_size.y / 2) / grid_size.y) + (x / grid_size.x);
		bool grid_a =
			(grid_linear_index % 2 == 0);

		image_rgba::pixel_type& p = preview_image.at(x, y);
		p.r = grid_a ? ga_r : gb_r;
		p.g = grid_a ? ga_g : gb_g;
		p.b = grid_a ? ga_b : gb_b;
		p.a = 255;
	}


	return preview_image;
}


image_rgba gui_color_dialog::create_color_picker_h_image
	(const vector2i& image_size, const color& selected_color)
{
	// Invoked half-way through member initialization.
	// Static to avoid access to uninitialized members.


	vector selected_hsv(to_hsv(selected_color));
	int64 selected_y =
		integer_operations::cast_or_clamp<int64>
			(selected_hsv.x * (image_size.y - 1));


	// Render vertical bar with full-bright,
	// fully saturated color hues.
	image_rgba color_picker_h_image(image_size);
	for (int64 y = 0; y < image_size.y; ++y)
	{
		color pixel_color
			(to_rgb(vector(y / static_cast<float>(image_size.y - 1), 1, 1), 1));

		unsigned char r =
			integer_operations::cast_or_clamp<unsigned char>
				(pixel_color.r * 255);
		unsigned char g =
			integer_operations::cast_or_clamp<unsigned char>
				(pixel_color.g * 255);
		unsigned char b =
			integer_operations::cast_or_clamp<unsigned char>
				(pixel_color.b * 255);

		for (int64 x = 0; x < image_size.x; ++x)			
		{
			image_rgba::pixel_type& p = color_picker_h_image.at(x, y);
			p.r = r; p.g = g; p.b = b; p.a = 255;
		}
	}


	// Render selection marker rows.
	for (int64 y = selected_y - 1; y <= selected_y + 1; ++y)
	{
		if (y < 0 || y >= image_size.y) continue;

		unsigned char value =
			static_cast<unsigned char>
				((y == selected_y) ? 255 : 0);

		for (int64 x = 0; x < image_size.x; ++x)
		{
			image_rgba::pixel_type& p = color_picker_h_image.at(x, y);
			p.r = value; p.g = value; p.b = value; p.a = 255;
		}
	}


	return color_picker_h_image;
}


image_rgba gui_color_dialog::create_color_picker_sv_image
	(const vector2i& image_size, const color& selected_color)
{
	// Invoked half-way through member initialization.
	// Static to avoid access to uninitialized members.

	vector selected_hsv(to_hsv(selected_color));
	color rgb_full_sv(to_rgb(vector(selected_hsv.x, 1, 1), 1));

	int64 selected_x =
		integer_operations::cast_or_clamp<int64>
			(selected_hsv.y * (image_size.x - 1));
	int64 selected_y =
		integer_operations::cast_or_clamp<int64>
			(selected_hsv.z * (image_size.y - 1));

	image_rgba color_picker_sv_image(image_size);
	for (int64 x = 0; x < image_size.x; ++x)	
	{
		// Use fast, linear interpolation between white, black,
		// and fully-saturated, full-bright color of input hue
		// to avoid expensive calls of to_rgb() for each pixel.
		// To see why this works, consider linearly changing
		// s and v parameters for const hue in to_rgb().

		color column_color
			(color(1, 1, 1, 1).lerp
				(rgb_full_sv, static_cast<float>(x) / (image_size.x - 1)));

		for (int64 y = 0; y < image_size.y; ++y)	
		{
			color pixel_color
				(color(0, 0, 0, 1).lerp
					(column_color, static_cast<float>(y) / (image_size.y - 1)));

			image_rgba::pixel_type& p = color_picker_sv_image.at(x, y);
			p.r = integer_operations::cast_or_clamp<unsigned char>(pixel_color.r * 255);
			p.g = integer_operations::cast_or_clamp<unsigned char>(pixel_color.g * 255);
			p.b = integer_operations::cast_or_clamp<unsigned char>(pixel_color.b * 255);
			p.a = 255;
		}
	}


	// Render selection marker rows.
	for (int64 y = selected_y - 1; y <= selected_y + 1; ++y)
	{
		if (y < 0 || y >= image_size.y) continue;
		for (int64 x = 0; x < image_size.x; ++x)
		{
			image_rgba::pixel_type& p =
				color_picker_sv_image.at(x, y);

			p.r = 0; p.g = 0; p.b = 0; p.a = 255;
		}
	}

	for (int64 x = selected_x - 1; x <= selected_x + 1; ++x)
	{
		if (x < 0 || x >= image_size.x) continue;
		for (int64 y = 0; y < image_size.y; ++y)
		{
			image_rgba::pixel_type& p =
				color_picker_sv_image.at(x, y);

			p.r = 0; p.g = 0; p.b = 0; p.a = 255;
		}
	}

	if (selected_y >= 0 && selected_y < image_size.y)
	{
		for (int64 x = 0; x < image_size.x; ++x)
		{
			image_rgba::pixel_type& p =
				color_picker_sv_image.at(x, selected_y);

			p.r = 255; p.g = 255; p.b = 255; p.a = 255;
		}
	}

	if (selected_x >= 0 && selected_x < image_size.x)
	{
		for (int64 y = 0; y < image_size.y; ++y)
		{
			image_rgba::pixel_type& p =
				color_picker_sv_image.at(selected_x, y);

			p.r = 255; p.g = 255; p.b = 255; p.a = 255;
		}
	}


	return color_picker_sv_image;
}


render_material_data gui_color_dialog::create_image_material
	(const shared_instance<render_shader_id>& image_shader)
{
	// Invoked half-way through member initialization.
	// Static to avoid access to uninitialized members.

	render_material_data image_material;
	image_material.shader = image_shader;
	image_material.blend = render_material_data::blend_alpha;	
	image_material.unlit = true;
	return image_material;
}


const float gui_color_dialog::hsv_black_epsilon_ = 1e-5f;
const float gui_color_dialog::hsv_gray_epsilon_ = 1e-5f;
const float gui_color_dialog::max_image_error_ = 0.5f / 255.f;




//////////////////////////////////////////////////////////////////////////
//
// gui_color_dialog_style
//
//////////////////////////////////////////////////////////////////////////


gui_color_dialog_style::gui_color_dialog_style
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
	 const vector2i& color_picker_sv_pixel_size)
	:
	spacer_style(spacer_style),
	label_style(label_style),
	button_style(button_style),
	value_edit_style(value_edit_style),
	image_map_style(image_map_style),
	window_style(window_style),
	background_style(background_style),
	image_shader(image_shader),
	preview_pixel_size(preview_pixel_size),
	preview_grid_pixel_size(preview_grid_pixel_size),
	color_picker_h_pixel_size(color_picker_h_pixel_size),
	color_picker_sv_pixel_size(color_picker_sv_pixel_size)
{
	if (preview_pixel_size.min() <= 0 ||
		preview_grid_pixel_size.min() <= 0)
	{
		LOG_ERROR("Preview size and preview grid size must be positive.");
		throw invalid_parameter();
	}

	if (color_picker_h_pixel_size.min() <= 0 ||
		color_picker_sv_pixel_size.min() <= 0)
	{
		LOG_ERROR("Both color picker sizes must be positive.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
