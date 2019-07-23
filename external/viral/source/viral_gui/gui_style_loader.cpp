/**
 *************************************************************************
 *
 * @file gui_style_loader.cpp
 *
 * Loading GUI styles, implementation.
 *
 ************************************************************************/


#include "gui_style_loader.hpp"


#include <viral_core/log.hpp>
#include <viral_core/file.hpp>
#include <viral_core/image_file.hpp>
#include <viral_core/xml.hpp>
#include <viral_core/xml_util.hpp>
#include <viral_core/render_resource.hpp>
#include <viral_core/render_command.hpp>


#include "gui_style_util.hpp"

#include "gui_spacer.hpp"
#include "gui_anchor.hpp"
#include "gui_stack.hpp"
#include "gui_grid.hpp"

#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_frame.hpp"

#include "gui_button.hpp"
#include "gui_tristate.hpp"
#include "gui_knob.hpp"

#include "gui_slider.hpp"
#include "gui_value_edit.hpp"
#include "gui_progress_bar.hpp"
#include "gui_image_map.hpp"

#include "gui_split.hpp"
#include "gui_scrollbar.hpp"
#include "gui_scroll_area.hpp"

#include "gui_list.hpp"
#include "gui_listbox.hpp"
#include "gui_dropdown.hpp"
#include "gui_tabview.hpp"

#include "gui_edit.hpp"
#include "gui_editbox.hpp"

#include "gui_window.hpp"
#include "gui_window_area.hpp"

#include "gui_tooltip.hpp"
#include "gui_tooltip_area.hpp"

#include "gui_msgbox.hpp"
#include "gui_progress_box.hpp"
#include "gui_string_dialog.hpp"
#include "gui_file_dialog.hpp"
#include "gui_color_dialog.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_style_loader_detail
//
//////////////////////////////////////////////////////////////////////////


/** Internal namespace with helpers for gui_style_loader. */
namespace gui_style_loader_detail
{
	bool to_bool(const string& attrib)
		{ return attrib.to_bool(); }
	float to_float(const string& attrib)
		{ return attrib.to_float(); }

	vector2i to_vector2i(const xml_node& node)
		{ return xml_converter::to_vector2i(node); }
	vector2f to_vector2f(const xml_node& node)
		{ return xml_converter::to_vector2f(node); }
	rect2f to_rect2f(const xml_node& node)
		{ return xml_converter::to_rect2f(node); }


	gui_anchor::anchor_type
		to_anchor_type(const string& s)
	{
		if (s == "anchor_fill") return gui_anchor::anchor_fill;
		else if (s == "anchor_neg") return gui_anchor::anchor_neg;
		else if (s == "anchor_center") return gui_anchor::anchor_center;
		else if (s == "anchor_pos") return gui_anchor::anchor_pos;
		LOG_ERROR("The attribute: " + s + " is not a valid anchor type.");
		throw xml_exception();
	}	


	gui_image::aspect_mode
		to_image_aspect_mode(const string& s)
	{
		if (s == "no_aspect") return gui_image::no_aspect;
		else if (s == "aspect_fit_x") return gui_image::aspect_fit_x;
		else if (s == "aspect_fit_y") return gui_image::aspect_fit_y;
		else if (s == "aspect_fit_both") return gui_image::aspect_fit_both;
		else if (s == "aspect_fill") return gui_image::aspect_fill;
		LOG_ERROR("The attribute: " + s + " is not a valid image aspect mode.");
		throw xml_exception();
	}


	gui_scroll_area::scrollbar_show_type
		to_scroll_area_scrollbar_show_type(const string& s)
	{
		if (s == "show_if_required") return gui_scroll_area::show_if_required;
		else if (s == "show_never") return gui_scroll_area::show_never;
		else if (s == "show_always") return gui_scroll_area::show_always;
		LOG_ERROR("The attribute: " + s + " is not a valid scrollbar show type.");
		throw xml_exception();
	}


	gui_scroll_area::scrollbar_arrange_type
		to_scroll_area_scrollbar_arrange_type(const string& s)
	{
		if (s == "arrange_default") return gui_scroll_area::arrange_default;
		else if (s == "arrange_compact") return gui_scroll_area::arrange_compact;
		LOG_ERROR("The attribute: " + s + " is not a valid scrollbar arrange type.");
		throw xml_exception();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gui_style_loader
//
//////////////////////////////////////////////////////////////////////////


#define PREPARE_INHERIT(style_type) \
	shared_pointer<style_type> parent \
		(node.has_attribute("inherit") ? \
			style_cache.entry<style_type> \
				(node.attribute("inherit")).get() : \
			shared_pointer<style_type>())

#define LOAD_OR_INHERIT_STYLE(member, style_type) \
	(parent && !node.has_attribute(#member)) ? \
		parent->member : \
		style_cache.entry<style_type>(node.attribute(#member))

#define LOAD_OR_INHERIT_SHADER(member) \
	(parent && !node.has_attribute(#member)) ? \
		parent->member : \
		shader_cache.entry(node.attribute(#member))

#define LOAD_OR_INHERIT_MATERIAL(member) \
	(parent && !node.has_attribute(#member)) ? \
		parent->member : \
		material_cache.entry(node.attribute(#member))

#define LOAD_OR_INHERIT_FONT(member) \
	(parent && !node.has_attribute(#member)) ? \
		parent->member : \
		shared_instance<font> \
			(new font \
				(font::load(node.attribute(#member), files)))

#define LOAD_OR_INHERIT_CONVERT_CHILD(member, convert_func) \
	(parent && !node.has_child(#member)) ? \
		parent->member : \
		gui_style_loader_detail::convert_func(node.child(#member))

#define LOAD_OR_INHERIT_CONVERT_ATTRIB(member, convert_func) \
	(parent && !node.has_attribute(#member)) ? \
		parent->member : \
		gui_style_loader_detail::convert_func(node.attribute(#member))


gui_style_loader::gui_style_loader
	(const string& loader_name,
	 file_context& files,			 
	 gui_shader_cache& shader_cache,
	 gui_material_cache& material_cache,
	 gui_style_cache& style_cache)
	:
	loader_name(loader_name),
	files(files),
	shader_cache(shader_cache),
	material_cache(material_cache),
	style_cache(style_cache)
{
	style_handlers_.insert("cursor_style", &gui_style_loader::to_cursor_style);
	style_handlers_.insert("font_style", &gui_style_loader::to_font_style);

	style_handlers_.insert("spacer_style", &gui_style_loader::to_spacer_style);
	style_handlers_.insert("anchor_style", &gui_style_loader::to_anchor_style);
	style_handlers_.insert("grid_cell_style", &gui_style_loader::to_grid_cell_style);

	style_handlers_.insert("label_style", &gui_style_loader::to_label_style);
	style_handlers_.insert("image_style", &gui_style_loader::to_image_style);
	style_handlers_.insert("frame_style", &gui_style_loader::to_frame_style);

	style_handlers_.insert("button_style", &gui_style_loader::to_button_style);
	style_handlers_.insert("tristate_style", &gui_style_loader::to_tristate_style);
	style_handlers_.insert("knob_style", &gui_style_loader::to_knob_style);

	style_handlers_.insert("slider_style", &gui_style_loader::to_slider_style);
	style_handlers_.insert("value_edit_style", &gui_style_loader::to_value_edit_style);
	style_handlers_.insert("progress_bar_style", &gui_style_loader::to_progress_bar_style);
	style_handlers_.insert("image_map_style", &gui_style_loader::to_image_map_style);

	style_handlers_.insert("split_style", &gui_style_loader::to_split_style);
	style_handlers_.insert("scrollbar_style", &gui_style_loader::to_scrollbar_style);
	style_handlers_.insert("scroll_area_style", &gui_style_loader::to_scroll_area_style);

	style_handlers_.insert("list_style", &gui_style_loader::to_list_style);
	style_handlers_.insert("listbox_style", &gui_style_loader::to_listbox_style);
	style_handlers_.insert("dropdown_style", &gui_style_loader::to_dropdown_style);
	style_handlers_.insert("tabview_style", &gui_style_loader::to_tabview_style);

	style_handlers_.insert("edit_style", &gui_style_loader::to_edit_style);
	style_handlers_.insert("editbox_style", &gui_style_loader::to_editbox_style);

	style_handlers_.insert("window_style", &gui_style_loader::to_window_style);

	style_handlers_.insert("tooltip_style", &gui_style_loader::to_tooltip_style);
	style_handlers_.insert("tooltip_area_style", &gui_style_loader::to_tooltip_area_style);

	style_handlers_.insert("msgbox_style", &gui_style_loader::to_msgbox_style);
	style_handlers_.insert("progress_box_style", &gui_style_loader::to_progress_box_style);
	style_handlers_.insert("string_dialog_style", &gui_style_loader::to_string_dialog_style);
	style_handlers_.insert("file_dialog_style", &gui_style_loader::to_file_dialog_style);
	style_handlers_.insert("color_dialog_style", &gui_style_loader::to_color_dialog_style);
}


void gui_style_loader::load(const xml_node& root)
{
	if (root.name() != "gui_styles")
	{
		LOG_ERROR("Root node of styles XML must be: gui_styles");
		throw corrupt_data();
	}

	for (xml_node::const_child_iterator it(root.first_child()); it; ++it)
	{
		const string& style_type = it->attribute("type");

		style_handler_map::const_iterator handler
			(style_handlers_.find(style_type));

		if (!handler)
		{
			LOG_ERROR("Unknown type of style: " + style_type);
			throw xml_exception();
		}

		if (style_cache.find(it->name()))
		{
			LOG_ERROR("Style with name: " + it->name() + " already exists.");
			throw object_creation_failed();
		}

		// Create and deserialize style from XML,
		// then insert into cache under given name.
		style_cache.insert
			(it->name(), (this->*(handler->value))(*it));
	}
}


void gui_style_loader::load(file& source_file)
	{ load(*xml_node::load(source_file)); }
void gui_style_loader::load(const string& path)
	{ load(*xml_node::load(path, files)); }


void gui_style_loader::create_materials
	(const string& folder,
	 const string& extension,
	 const render_texture_2d_data& texture_template,
	 const render_material_data& material_template,
	 render_command_queue& queue)
{
	path_index index;
	files.build_path_index
		(index, folder, extension, false, false);

	for (path_index::file_path_set::const_iterator it
			(index.file_paths.first()); it; ++it)
	{
		string name
			(virtual_path::remove_extension
				(virtual_path::extract_target(*it)));

		if (material_cache.contains(name))
		{
			LOG_ERROR("Material: " + name + " already contained in material map.");
			throw object_registration_failed();
		}

		shared_instance<render_texture_id> texture_id
			(new render_texture_id(name + "_texture"));
		auto_pointer<render_texture_2d_data> texture_data
			(clone_dynamic(texture_template));
		texture_data->image_2d.reset
			(new image_rgba(image_file::load(*it, files)));
		queue.commit
			(std::move(texture_data), texture_id);

		shared_instance<render_material_id> material_id
			(new render_material_id(name + "_material"));
		auto_pointer<render_material_data> material_data
			(clone_dynamic(material_template));
		material_data->texture = texture_id;
		queue.commit
			(std::move(material_data), material_id);
	
		material_cache.insert(name, material_id);
	}
}


shared_instance<gui_style> gui_style_loader::to_cursor_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_cursor_style);

	return shared_instance<gui_style>
		(new gui_cursor_style
			(LOAD_OR_INHERIT_MATERIAL(material),
			 LOAD_OR_INHERIT_CONVERT_CHILD(extents, to_rect2f)));
}


shared_instance<gui_style> gui_style_loader::to_font_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_font_style);

	return shared_instance<gui_style>
		(new gui_font_style
			(LOAD_OR_INHERIT_MATERIAL(material),
			 LOAD_OR_INHERIT_FONT(f)));
}


shared_instance<gui_style> gui_style_loader::to_spacer_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_spacer_style);

	return shared_instance<gui_style>
		(new gui_spacer_style
			(LOAD_OR_INHERIT_CONVERT_CHILD(extents, to_vector2f)));
}


shared_instance<gui_style> gui_style_loader::to_anchor_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_anchor_style);

	return shared_instance<gui_style>
		(new gui_anchor_style
			(LOAD_OR_INHERIT_CONVERT_ATTRIB(horizontal_anchor_underflow, to_anchor_type),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(vertical_anchor_underflow, to_anchor_type),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(horizontal_anchor_overflow, to_anchor_type),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(vertical_anchor_overflow, to_anchor_type)));
}


shared_instance<gui_style> gui_style_loader::to_grid_cell_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_grid_cell_style);

	return shared_instance<gui_style>
		(new gui_grid_cell_style
			(LOAD_OR_INHERIT_CONVERT_CHILD(padding_00, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(padding_22, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(horizontal_anchor, to_anchor_type),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(vertical_anchor, to_anchor_type)));
}


shared_instance<gui_style> gui_style_loader::to_label_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_label_style);

	return shared_instance<gui_style>
		(new gui_label_style
			(LOAD_OR_INHERIT_STYLE(font_style, gui_font_style),
			 LOAD_OR_INHERIT_STYLE(anchor_style, gui_anchor_style)));
}


shared_instance<gui_style> gui_style_loader::to_image_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_image_style);

	return shared_instance<gui_style>
		(new gui_image_style
			(LOAD_OR_INHERIT_CONVERT_CHILD(base_size, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(pixel_scale, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(downscale, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(upscale, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(aspect, to_image_aspect_mode),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(allow_compression, to_bool)));
}


shared_instance<gui_style> gui_style_loader::to_frame_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_frame_style);

	return shared_instance<gui_style>
		(new gui_frame_style
			(LOAD_OR_INHERIT_CONVERT_CHILD(margin_00, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(margin_22, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(border_00, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(border_22, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(padding_00, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(padding_22, to_vector2f),
			 LOAD_OR_INHERIT_MATERIAL(material),
			 LOAD_OR_INHERIT_CONVERT_CHILD(material_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(pixel_offset, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(pixel_size_00, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(pixel_size_11, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(pixel_size_22, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(size_per_tile, to_vector2f)));
}


shared_instance<gui_style> gui_style_loader::to_button_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_button_style);

	return shared_instance<gui_style>
		(new gui_button_style
			(LOAD_OR_INHERIT_STYLE(default_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(down_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(read_only_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(key_focus_overlay_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(mouse_focus_overlay_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(content_anchor_style, gui_anchor_style),
			 LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(image_style, gui_image_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(image_label_spacing, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(hold_delay_seconds, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(allow_repeat, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(repeat_seconds, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_tristate_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_tristate_style);

	return shared_instance<gui_style>
		(new gui_tristate_style
			(LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_MATERIAL(yes_icon),
			 LOAD_OR_INHERIT_MATERIAL(no_icon),
			 LOAD_OR_INHERIT_MATERIAL(maybe_icon),
			 LOAD_OR_INHERIT_CONVERT_CHILD(icon_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(button_label_spacing, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_knob_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_knob_style);

	return shared_instance<gui_style>
		(new gui_knob_style
			(LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(min_drag_range, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(drag_tolerance, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_slider_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_slider_style);

	return shared_instance<gui_style>
		(new gui_slider_style
			(LOAD_OR_INHERIT_STYLE(knob_style, gui_knob_style),
 			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(band_style, gui_frame_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(show_buttons, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(button_size, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(button_spacing, to_float),
			 LOAD_OR_INHERIT_MATERIAL(increment_icon),
			 LOAD_OR_INHERIT_MATERIAL(decrement_icon),
			 LOAD_OR_INHERIT_CONVERT_CHILD(button_icon_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(fixed_band_size, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(min_band_size, to_float),
			 LOAD_OR_INHERIT_MATERIAL(mark_icon),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(mark_icon_size, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(mark_snap_tolerance, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_value_edit_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_value_edit_style);

	return shared_instance<gui_style>
		(new gui_value_edit_style
			(LOAD_OR_INHERIT_STYLE(slider_style, gui_slider_style),
 			 LOAD_OR_INHERIT_STYLE(editbox_style, gui_editbox_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(slider_edit_spacing, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_progress_bar_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_progress_bar_style);

	return shared_instance<gui_style>
		(new gui_progress_bar_style
			(LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(fill_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(min_length, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_image_map_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_image_map_style);

	return shared_instance<gui_style>
		(new gui_image_map_style
			(LOAD_OR_INHERIT_STYLE(image_style, gui_image_style),
			 LOAD_OR_INHERIT_STYLE(frame_style, gui_frame_style)));
}


shared_instance<gui_style> gui_style_loader::to_split_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_split_style);

	return shared_instance<gui_style>
		(new gui_split_style
			(LOAD_OR_INHERIT_STYLE(knob_style, gui_knob_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(splitter_size, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(min_pane_size, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(ensure_content_size_neg, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(ensure_content_size_pos, to_bool)));
}


shared_instance<gui_style> gui_style_loader::to_scrollbar_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_scrollbar_style);

	return shared_instance<gui_style>
		(new gui_scrollbar_style
			(LOAD_OR_INHERIT_STYLE(knob_style, gui_knob_style),
			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(band_style, gui_frame_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(buttons_only, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(button_size, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(min_band_size, to_float),
			 LOAD_OR_INHERIT_MATERIAL(left_icon),
			 LOAD_OR_INHERIT_MATERIAL(up_icon),
			 LOAD_OR_INHERIT_MATERIAL(right_icon),
			 LOAD_OR_INHERIT_MATERIAL(down_icon),
			 LOAD_OR_INHERIT_CONVERT_CHILD(icon_pixel_size, to_vector2i)));
}


shared_instance<gui_style> gui_style_loader::to_scroll_area_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_scroll_area_style);

	return shared_instance<gui_style>
		(new gui_scroll_area_style
			(LOAD_OR_INHERIT_STYLE(scrollbar_style, gui_scrollbar_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(prefer_override_size, to_bool),
			 LOAD_OR_INHERIT_CONVERT_CHILD(override_size, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(scrollbar_show_horizontal, to_scroll_area_scrollbar_show_type),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(scrollbar_show_vertical, to_scroll_area_scrollbar_show_type),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(scrollbar_arrange, to_scroll_area_scrollbar_arrange_type)));
}


shared_instance<gui_style> gui_style_loader::to_list_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_list_style);

	return shared_instance<gui_style>
		(new gui_list_style
			(LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_CONVERT_CHILD(min_item_size, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(icon_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(allow_grow, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(allow_stack, to_bool),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(allow_stretch, to_bool)));
}


shared_instance<gui_style> gui_style_loader::to_listbox_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_listbox_style);

	return shared_instance<gui_style>
		(new gui_listbox_style
			(LOAD_OR_INHERIT_STYLE(frame_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(scroll_area_style, gui_scroll_area_style),
			 LOAD_OR_INHERIT_STYLE(list_style, gui_list_style)));
}


shared_instance<gui_style> gui_style_loader::to_dropdown_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_dropdown_style);

	return shared_instance<gui_style>
		(new gui_dropdown_style
			(LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(listbox_style, gui_listbox_style),
			 LOAD_OR_INHERIT_CONVERT_CHILD(min_item_button_size, to_vector2f),
			 LOAD_OR_INHERIT_MATERIAL(dropdown_icon),
			 LOAD_OR_INHERIT_CONVERT_CHILD(icon_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style)));
}


shared_instance<gui_style> gui_style_loader::to_tabview_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_tabview_style);

	return shared_instance<gui_style>
		(new gui_tabview_style
			(LOAD_OR_INHERIT_STYLE(split_style, gui_split_style),
			 LOAD_OR_INHERIT_STYLE(header_scroll_area_style, gui_scroll_area_style),
			 LOAD_OR_INHERIT_STYLE(header_list_style, gui_list_style)));
}


shared_instance<gui_style> gui_style_loader::to_edit_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_edit_style);

	return shared_instance<gui_style>
		(new gui_edit_style
			(LOAD_OR_INHERIT_STYLE(font_style, gui_font_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(min_line_width, to_float),
			 LOAD_OR_INHERIT_MATERIAL(selection_material),
			 LOAD_OR_INHERIT_MATERIAL(caret_material),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(caret_width, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(caret_overwrite_width, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(caret_blink_seconds, to_float),
			 LOAD_OR_INHERIT_STYLE(caret_cursor, gui_cursor_style)));
}


shared_instance<gui_style> gui_style_loader::to_editbox_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_editbox_style);

	return shared_instance<gui_style>
		(new gui_editbox_style
			(LOAD_OR_INHERIT_STYLE(frame_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(scroll_area_style, gui_scroll_area_style),
			 LOAD_OR_INHERIT_STYLE(edit_style, gui_edit_style)));
}


shared_instance<gui_style> gui_style_loader::to_window_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_window_style);

	return shared_instance<gui_style>
		(new gui_window_style
			(LOAD_OR_INHERIT_STYLE(default_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(key_focus_overlay_style, gui_frame_style),

			 LOAD_OR_INHERIT_CONVERT_CHILD(caption_offset, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(caption_height, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(caption_right_spacing, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(caption_button_spacing, to_float),
			 
			 LOAD_OR_INHERIT_STYLE(caption_label_style, gui_label_style),

			 LOAD_OR_INHERIT_STYLE(close_button_style, gui_button_style),
			 LOAD_OR_INHERIT_MATERIAL(close_icon),
			 LOAD_OR_INHERIT_CONVERT_CHILD(close_icon_pixel_size, to_vector2i),

			 LOAD_OR_INHERIT_STYLE(cursor_resize_lr, gui_cursor_style),
			 LOAD_OR_INHERIT_STYLE(cursor_resize_ud, gui_cursor_style),
			 LOAD_OR_INHERIT_STYLE(cursor_resize_ul_lr, gui_cursor_style),
			 LOAD_OR_INHERIT_STYLE(cursor_resize_ll_ur, gui_cursor_style),
			
			 LOAD_OR_INHERIT_CONVERT_CHILD(hot_areas_padding_00, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(hot_areas_padding_22, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(hot_area_move_height, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(hot_area_resize_thickness, to_float),

			 LOAD_OR_INHERIT_CONVERT_CHILD(min_visible_hot_area_move_00, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(min_visible_hot_area_move_22, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(min_client_size, to_vector2f),
			 LOAD_OR_INHERIT_CONVERT_CHILD(max_client_size, to_vector2f)));
}


shared_instance<gui_style> gui_style_loader::to_tooltip_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_tooltip_style);

	return shared_instance<gui_style>
		(new gui_tooltip_style
			(LOAD_OR_INHERIT_STYLE(frame_style, gui_frame_style),
			 LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(image_style, gui_image_style),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(image_label_spacing, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_tooltip_area_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_tooltip_area_style);

	return shared_instance<gui_style>
		(new gui_tooltip_area_style
			(LOAD_OR_INHERIT_STYLE(tooltip_style, gui_tooltip_style),
			 LOAD_OR_INHERIT_CONVERT_CHILD(tooltip_dead_zone, to_rect2f),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(tooltip_show_tolerance, to_float),
			 LOAD_OR_INHERIT_CONVERT_ATTRIB(tooltip_show_seconds, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_msgbox_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_msgbox_style);

	return shared_instance<gui_style>
		(new gui_msgbox_style
			(LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(window_style, gui_window_style),

			 LOAD_OR_INHERIT_STYLE(icon_image_style, gui_image_style),
 			 LOAD_OR_INHERIT_MATERIAL(icon_question),
 			 LOAD_OR_INHERIT_MATERIAL(icon_info),
 			 LOAD_OR_INHERIT_MATERIAL(icon_warn),
 			 LOAD_OR_INHERIT_MATERIAL(icon_error),
			 LOAD_OR_INHERIT_CONVERT_CHILD(icon_pixel_size, to_vector2i),

			 LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style)));
}


shared_instance<gui_style> gui_style_loader::to_progress_box_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_progress_box_style);

	return shared_instance<gui_style>
		(new gui_progress_box_style
			(LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(progress_indicator_style, gui_progress_bar_style),
			 LOAD_OR_INHERIT_STYLE(busy_indicator_style, gui_knob_style),

			 LOAD_OR_INHERIT_STYLE(icon_image_style, gui_image_style),
 			 LOAD_OR_INHERIT_MATERIAL(icon_material),
			 LOAD_OR_INHERIT_CONVERT_CHILD(icon_pixel_size, to_vector2i),

			 LOAD_OR_INHERIT_STYLE(window_style, gui_window_style),
			 LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style),

			 LOAD_OR_INHERIT_CONVERT_ATTRIB(busy_indicator_frequency, to_float)));
}


shared_instance<gui_style> gui_style_loader::to_string_dialog_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_string_dialog_style);

	return shared_instance<gui_style>
		(new gui_string_dialog_style
			(LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(editbox_style, gui_editbox_style),
			 LOAD_OR_INHERIT_STYLE(window_style, gui_window_style),
			 LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style),
			 LOAD_OR_INHERIT_CONVERT_CHILD(initial_client_size, to_vector2f)));
}


shared_instance<gui_style> gui_style_loader::to_file_dialog_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_file_dialog_style);

	return shared_instance<gui_style>
		(new gui_file_dialog_style
			(LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(listbox_style, gui_listbox_style),
			 LOAD_OR_INHERIT_STYLE(dropdown_style, gui_dropdown_style),
			 LOAD_OR_INHERIT_STYLE(editbox_style, gui_editbox_style),
			 LOAD_OR_INHERIT_STYLE(window_style, gui_window_style),

			 LOAD_OR_INHERIT_MATERIAL(icon_folder),
			 LOAD_OR_INHERIT_MATERIAL(icon_file),

			 LOAD_OR_INHERIT_STYLE(msgbox_style, gui_msgbox_style),
			 LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style),

			 LOAD_OR_INHERIT_CONVERT_CHILD(initial_client_size, to_vector2f)));
}


shared_instance<gui_style> gui_style_loader::to_color_dialog_style(const xml_node& node)
{
	PREPARE_INHERIT(gui_color_dialog_style);

	return shared_instance<gui_style>
		(new gui_color_dialog_style
			(LOAD_OR_INHERIT_STYLE(spacer_style, gui_spacer_style),
			 LOAD_OR_INHERIT_STYLE(label_style, gui_label_style),
			 LOAD_OR_INHERIT_STYLE(button_style, gui_button_style),
			 LOAD_OR_INHERIT_STYLE(value_edit_style, gui_value_edit_style),
			 LOAD_OR_INHERIT_STYLE(image_map_style, gui_image_map_style),
			 LOAD_OR_INHERIT_STYLE(window_style, gui_window_style),
			 LOAD_OR_INHERIT_STYLE(background_style, gui_frame_style),
			 LOAD_OR_INHERIT_SHADER(image_shader),
			 LOAD_OR_INHERIT_CONVERT_CHILD(preview_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(preview_grid_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(color_picker_h_pixel_size, to_vector2i),
			 LOAD_OR_INHERIT_CONVERT_CHILD(color_picker_sv_pixel_size, to_vector2i)));
}




} /* namespace viral_gui */
