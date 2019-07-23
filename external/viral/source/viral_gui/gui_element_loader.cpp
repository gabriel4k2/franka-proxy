/**
 *************************************************************************
 *
 * @file gui_element_loader.cpp
 *
 * Loading of GUI elements from XML, implementation.
 *
 ************************************************************************/


#include "gui_element_loader.hpp"


#include <viral_core/auto_pointer.hpp>
#include <viral_core/log.hpp>
#include <viral_core/file.hpp>
#include <viral_core/xml.hpp>
#include <viral_core/xml_util.hpp>
#include <viral_core/render_resource.hpp>


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


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_element_loader_detail
//
//////////////////////////////////////////////////////////////////////////


/** Internal namespace with helpers for gui_element_loader. */
namespace gui_element_loader_detail
{
	gui_element::orientation_type
		to_orientation_type(const string& s)
	{
		if (s == "horizontal") return gui_element::horizontal;
		else if (s == "vertical") return gui_element::vertical;
		LOG_ERROR("The attribute: " + s + " is not a valid orientation type.");
		throw xml_exception();
	}


	gui_button::behaviour_type
		to_button_behaviour_type(const string& s)
	{
		if (s == "click_button") return gui_button::click_button;
		else if (s == "hold_button") return gui_button::hold_button;
		else if (s == "toggle_button") return gui_button::toggle_button;
		else if (s == "lock_button") return gui_button::lock_button;
		LOG_ERROR("The attribute: " + s + " is not a valid button behaviour.");
		throw xml_exception();
	}


	gui_tristate::select_mode
		to_tristate_select_mode(const string& s)
	{
		if (s == "mode_check") return gui_tristate::mode_check;
		else if (s == "mode_choice") return gui_tristate::mode_choice;
		LOG_ERROR("The attribute: " + s + " is not a valid tristate select mode.");
		throw xml_exception();
	}


	gui_tristate::select_state
		to_tristate_select_state(const string& s)
	{
		if (s == "state_yes") return gui_tristate::state_yes;
		else if (s == "state_no") return gui_tristate::state_no;
		else if (s == "state_maybe") return gui_tristate::state_maybe;
		LOG_ERROR("The attribute: " + s + " is not a valid tristate select state.");
		throw xml_exception();
	}


	gui_slider::snap_mode
		to_slider_snap_mode(const string& s)
	{
		if (s == "snap_none") return gui_slider::snap_none;
		else if (s == "snap_gui") return gui_slider::snap_gui;
		else if (s == "snap_force") return gui_slider::snap_force;
		LOG_ERROR("The attribute: " + s + " is not a valid slider snap mode.");
		throw xml_exception();
	}


	gui_list::item_select_mode
		to_list_item_select_mode(const string& s)
	{
		if (s == "select_single") return gui_list::select_single;
		else if (s == "select_toggle") return gui_list::select_toggle;
		else if (s == "select_advanced") return gui_list::select_advanced;
		LOG_ERROR("The attribute: " + s + " is not a valid list select mode.");
		throw xml_exception();
	}


	gui_window::window_resize_policy
		to_window_resize_policy(const string& s)
	{
		if (s == "fixed_size") return gui_window::fixed_size;
		else if (s == "match_content_size") return gui_window::match_content_size;
		else if (s == "user_resize") return gui_window::user_resize;
		else if (s == "user_resize_enlarge") return gui_window::user_resize_enlarge;
		LOG_ERROR("The attribute: " + s + " is not a valid window resize policy.");
		throw xml_exception();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gui_element_loader
//
//////////////////////////////////////////////////////////////////////////


gui_element_loader::gui_element_loader
	(const string& loader_name,
	 gui_context& gui,
	 file_context& files,
	 const gui_shader_cache& shader_cache,
	 const gui_material_cache& material_cache,
	 const gui_style_cache& style_cache,
	 gui_element_cache& element_cache)
	:
	loader_name(loader_name),
	gui(gui),
	files(files),
	shader_cache(shader_cache),
	material_cache(material_cache),
	style_cache(style_cache),
	element_cache(element_cache)
{
	element_handlers_.insert("spacer", &gui_element_loader::to_spacer);
	element_handlers_.insert("anchor", &gui_element_loader::to_anchor);
	element_handlers_.insert("stack", &gui_element_loader::to_stack);
	element_handlers_.insert("grid", &gui_element_loader::to_grid);

	element_handlers_.insert("label", &gui_element_loader::to_label);
	element_handlers_.insert("image", &gui_element_loader::to_image);
	element_handlers_.insert("frame", &gui_element_loader::to_frame);

	element_handlers_.insert("button", &gui_element_loader::to_button);
	element_handlers_.insert("tristate", &gui_element_loader::to_tristate);
	element_handlers_.insert("knob", &gui_element_loader::to_knob);

	element_handlers_.insert("slider", &gui_element_loader::to_slider);
	element_handlers_.insert("value_edit", &gui_element_loader::to_value_edit);
	element_handlers_.insert("progress_bar", &gui_element_loader::to_progress_bar);
	element_handlers_.insert("image_map", &gui_element_loader::to_image_map);

	element_handlers_.insert("split", &gui_element_loader::to_split);
	element_handlers_.insert("scrollbar", &gui_element_loader::to_scrollbar);
	element_handlers_.insert("scroll_area", &gui_element_loader::to_scroll_area);

	element_handlers_.insert("list", &gui_element_loader::to_list);
	element_handlers_.insert("listbox", &gui_element_loader::to_listbox);
	element_handlers_.insert("dropdown", &gui_element_loader::to_dropdown);
	element_handlers_.insert("tabview", &gui_element_loader::to_tabview);

	element_handlers_.insert("edit", &gui_element_loader::to_edit);
	element_handlers_.insert("editbox", &gui_element_loader::to_editbox);

	element_handlers_.insert("window", &gui_element_loader::to_window);
	element_handlers_.insert("window_area", &gui_element_loader::to_window_area);

	element_handlers_.insert("tooltip", &gui_element_loader::to_tooltip);
	element_handlers_.insert("tooltip_area", &gui_element_loader::to_tooltip_area);
}


void gui_element_loader::load_elements(const xml_node& root)
{
	if (root.name() != "gui_elements")
	{
		LOG_ERROR("Root node of GUI elements XML must be: gui_elements");
		throw corrupt_data();
	}

	for (xml_node::const_child_iterator it(root.first_child()); it; ++it)
	{
		const string& element_type = it->attribute("type");

		element_handler_map::const_iterator handler
			(element_handlers_.find(element_type));

		if (!handler)
		{
			LOG_ERROR("Unknown type of element: " + element_type);
			throw xml_exception();
		}

		if (element_cache.find(it->name()))
		{
			LOG_ERROR("Element with name: " + it->name() + " already exists.");
			throw object_creation_failed();
		}

		// Create and deserialize element from XML,
		// then insert into cache under given name.
		element_cache.insert
			(it->name(), (this->*(handler->value))(*it));
	}
}

void gui_element_loader::load_elements(file& xml_file)
	{ load_elements(*xml_node::load(xml_file)); }

void gui_element_loader::load_elements(const string& path)
	{ load_elements(*xml_node::load(path, files)); }


void gui_element_loader::to_slider_helper
	(const xml_node& node, gui_slider& slider)
{
	// Slider range must be set first
	// so that our value clamps correctly.
	slider.set_range
		(node.has_attribute("range_min") ?
			node.attribute("range_min").to_float() : slider.range_min(),
		 node.has_attribute("range_max") ?
			node.attribute("range_max").to_float() : slider.range_max());

	if (node.has_attribute("value") &&
		node.has_attribute("value_normalized"))
	{
		LOG_ERROR("Slider must not have both value and value_normalized.");
		throw xml_exception();
	}

	if (node.has_attribute("value"))
		slider.set_value
			(node.attribute("value").to_float());
	if (node.has_attribute("value_normalized"))
		slider.set_value_normalized
			(node.attribute("value_normalized").to_float());

	if (node.has_attribute("read_only"))
		slider.set_read_only
			(node.attribute("read_only").to_bool());

	if (node.has_attribute("click_increment"))
		slider.set_click_increment
			(node.attribute("click_increment").to_float());

	if (node.has_attribute("continuous_increment"))
		slider.set_continuous_increment
			(node.attribute("continuous_increment").to_float());

	if (node.has_child("marks"))
	{
		dynamic_buffer<float> marks;

		for (xml_node::const_child_iterator it
					(node.child("marks").first_child()); it; ++it)
		{
			if (it->name() != "mark")
			{
				LOG_ERROR("Name of XML nodes for marks must be mark.");
				throw xml_exception();
			}

			marks.push_back(it->attribute("value").to_float());
		}

		slider.set_marks(marks);
	}
}


void gui_element_loader::to_list_items_helper
	(const xml_node& node, gui_list& list)
{
	if (!node.has_child("items")) return;
	
	for (xml_node::const_child_iterator it
			(node.child("items").first_child()); it; ++it)
	{
		if (it->name() == "item")
		{
			auto_pointer<gui_list_item> item
				(new gui_list_item
					(xml_util::unescape_whitespace_prioritized
						(it->attribute("text")),
					 it->has_attribute("icon") ?
						material_cache.entry(it->attribute("icon")).get() :
						shared_pointer<render_material_id>()));

			if (it->has_attribute("selected"))
				item->selected = it->attribute("selected").to_bool();

			list.items.insert
				(std::move(item));
		}
		else if (it->name() == "null")
		{
			list.items.insert(auto_pointer<gui_list_item>());
		}
		else
		{
			LOG_ERROR("Name of XML nodes for items must be item or null.");
			throw xml_exception();
		}
	}
}


shared_instance<gui_element> gui_element_loader::to_spacer(const xml_node& node)
{
	return shared_instance<gui_element>
		(new gui_spacer
			(node.name(), gui,
			 style_cache.entry<gui_spacer_style>(node.attribute("spacer_style"))));
}


shared_instance<gui_element> gui_element_loader::to_anchor(const xml_node& node)
{
	shared_instance<gui_anchor> anchor
		(new gui_anchor
			(node.name(), gui,
			 style_cache.entry<gui_anchor_style>(node.attribute("anchor_style"))));

	if (node.has_attribute("content"))
		anchor().set_content
			(element_cache.entry(node.attribute("content")));

	return anchor;
}


shared_instance<gui_element> gui_element_loader::to_stack(const xml_node& node)
{
	shared_instance<gui_stack> stack
		(new gui_stack
			(node.name(), gui,
			 node.attribute("modal").to_bool())); 

	if (node.has_child("layers"))
	{
		for (xml_node::const_child_iterator it
				(node.child("layers").first_child()); it; ++it)
		{
			if (it->name() != "layer")
			{
				LOG_ERROR("Name of XML nodes for layers must be layer.");
				throw xml_exception();
			}

			stack().insert_layer
				(element_cache.entry(it->attribute("ref")));
		}
	}

	return stack;
}


shared_instance<gui_element> gui_element_loader::to_grid(const xml_node& node)
{
	shared_instance<gui_grid> grid
		(new gui_grid
			(node.name(), gui,
			 xml_converter::to_vector2i(node.child("size"))));

	// Default cell styles are just a shorthand for large grids
	// without direct member representation.
	shared_pointer<gui_grid_cell_style> default_grid_cell_style;
	if (node.has_attribute("default_grid_cell_style"))
		default_grid_cell_style =
			style_cache.entry<gui_grid_cell_style>
				(node.attribute("default_grid_cell_style"));

	if (node.has_child("rows"))
	{
		for (xml_node::const_child_iterator it
				(node.child("rows").first_child()); it; ++it)
		{
			if (it->name() != "row")
			{
				LOG_ERROR("Name of XML nodes for rows must be row.");
				throw xml_exception();
			}

			grid().set_row_weight
				(it->attribute("index").to_int64(),
				 it->attribute("weight").to_float());
		}
	}

	if (node.has_child("columns"))
	{
		for (xml_node::const_child_iterator it
				(node.child("columns").first_child()); it; ++it)
		{
			if (it->name() != "column")
			{
				LOG_ERROR("Name of XML nodes for columns must be column.");
				throw xml_exception();
			}

			grid().set_column_weight
				(it->attribute("index").to_int64(),
				 it->attribute("weight").to_float());
		}
	}

	if (node.has_child("cells"))
	{
		for (xml_node::const_child_iterator it
				(node.child("cells").first_child()); it; ++it)
		{
			if (it->name() != "cell")
			{
				LOG_ERROR("Name of XML nodes for cells must be cell.");
				throw xml_exception();
			}

			vector2i position
				(it->attribute("x").to_int64(),
				 it->attribute("y").to_int64());

			grid().insert_content
				(element_cache.entry(it->attribute("content")), position);

			if (it->has_attribute("stretch_left"))
				grid().set_stretch_left
					(position, it->attribute("stretch_left").to_int64());
			if (it->has_attribute("stretch_right"))
				grid().set_stretch_right
					(position, it->attribute("stretch_right").to_int64());

			if (it->has_attribute("stretch_up"))
				grid().set_stretch_up
					(position, it->attribute("stretch_up").to_int64());
			if (it->has_attribute("stretch_down"))
				grid().set_stretch_down
					(position, it->attribute("stretch_down").to_int64());

			if (it->has_attribute("grid_cell_style"))
				grid().set_grid_cell_style
					(position, style_cache.entry<gui_grid_cell_style>
								(it->attribute("grid_cell_style")));
			else if (default_grid_cell_style)
				grid().set_grid_cell_style
					(position, default_grid_cell_style.instance_or_throw());
		}
	}

	return grid;
}


shared_instance<gui_element> gui_element_loader::to_label(const xml_node& node)
{
	return shared_instance<gui_element>
		(new gui_label
			(node.name(), gui,
			 style_cache.entry<gui_label_style>(node.attribute("label_style")),
			 xml_util::unescape_whitespace_prioritized(node.attribute("text"))));
}


shared_instance<gui_element> gui_element_loader::to_image(const xml_node& node)
{
	return shared_instance<gui_element>
		(new gui_image
			(node.name(), gui,
			 style_cache.entry<gui_image_style>(node.attribute("image_style")),
			 material_cache.entry(node.attribute("material")),
			 xml_converter::to_vector2i(node.child("image_size"))));
}


shared_instance<gui_element> gui_element_loader::to_frame(const xml_node& node)
{
	shared_instance<gui_frame> frame
		(new gui_frame
			(node.name(), gui,
			 style_cache.entry<gui_frame_style>(node.attribute("frame_style"))));

	if (node.has_attribute("content"))
		frame().set_content
			(element_cache.entry(node.attribute("content")));

	return frame;
}


shared_instance<gui_element> gui_element_loader::to_button(const xml_node& node)
{
	shared_instance<gui_button> button
		(new gui_button
			(node.name(), gui,
			 style_cache.entry<gui_button_style>(node.attribute("button_style")),
			 gui_element_loader_detail::to_button_behaviour_type(node.attribute("behaviour"))));

	if (node.has_attribute("down"))
		button().set_down
			(node.attribute("down").to_bool());

	if (node.has_attribute("read_only"))
		button().set_read_only
			(node.attribute("read_only").to_bool());

	if (node.has_attribute("text"))
		button().set_text
			(xml_util::unescape_whitespace_prioritized(node.attribute("text")));

	if (node.has_attribute("material"))
		button().set_graphics
			(xml_converter::to_vector2i(node.child("image_size")),
			 material_cache.entry(node.attribute("material")));

	return button;
}


shared_instance<gui_element> gui_element_loader::to_tristate(const xml_node& node)
{
	shared_instance<gui_tristate> tristate
		(new gui_tristate
			(node.name(), gui,
			 style_cache.entry<gui_tristate_style>(node.attribute("tristate_style")),
			 gui_element_loader_detail::to_tristate_select_mode(node.attribute("mode"))));

	if (node.has_attribute("state"))
		tristate().set_state
			(gui_element_loader_detail::to_tristate_select_state(node.attribute("state")));

	if (node.has_attribute("read_only"))
		tristate().set_read_only
			(node.attribute("read_only").to_bool());

	if (node.has_attribute("text"))
		tristate().set_text
			(xml_util::unescape_whitespace_prioritized(node.attribute("text")));

	if (node.has_child("choices"))
	{
		gui_tristate::choice_list choices;

		// Always self-insert: Less error-prone,
		// and tristate is not in element_cache yet.
		choices.insert(tristate);

		for (xml_node::const_child_iterator it
					(node.child("choices").first_child()); it; ++it)
		{
			if (it->name() != "choice")
			{
				LOG_ERROR("Name of XML nodes for choices must be choice.");
				throw xml_exception();
			}

			choices.insert
				(element_cache.entry<gui_tristate>(it->attribute("ref")));
		}

		// Apply choices to all linked elements.
		// This avoids problems with cyclic dependencies:
		// Clients just specify choices on the last tristate.
		for (gui_tristate::choice_list::const_iterator it
				(choices.first()); it; ++it)
		{
			shared_pointer<gui_tristate> choice(it->lock());
			if (choice) choice->set_choices(choices);
		}
	}

	return tristate;
}


shared_instance<gui_element> gui_element_loader::to_knob(const xml_node& node)
{
	shared_instance<gui_knob> knob
		(new gui_knob
			(node.name(), gui,
			 style_cache.entry<gui_knob_style>(node.attribute("knob_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation"))));

	if (node.has_attribute("knob_size"))
		knob().set_knob_size
			(node.attribute("knob_size").to_float());

	if (node.has_attribute("position_normalized"))
		knob().set_position_normalized
			(node.attribute("position_normalized").to_float());

	if (node.has_attribute("read_only"))
		knob().set_read_only
			(node.attribute("read_only").to_bool());

	return knob; 
}


shared_instance<gui_element> gui_element_loader::to_slider(const xml_node& node)
{
	shared_instance<gui_slider> slider
		(new gui_slider
			(node.name(), gui,
			 style_cache.entry<gui_slider_style>(node.attribute("slider_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation")),
			 gui_element_loader_detail::to_slider_snap_mode(node.attribute("snap"))));

	to_slider_helper(node, slider());
	
	return slider;
}


shared_instance<gui_element> gui_element_loader::to_value_edit(const xml_node& node)
{
	shared_instance<gui_value_edit> value_edit
		(new gui_value_edit
			(node.name(), gui,
			 style_cache.entry<gui_value_edit_style>(node.attribute("value_edit_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation")),
			 gui_element_loader_detail::to_slider_snap_mode(node.attribute("snap"))));

	to_slider_helper
		(node, value_edit().slider());

	return value_edit;
}


shared_instance<gui_element> gui_element_loader::to_progress_bar(const xml_node& node)
{
	shared_instance<gui_progress_bar> progress_bar
		(new gui_progress_bar
			(node.name(), gui,
			 style_cache.entry<gui_progress_bar_style>(node.attribute("progress_bar_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation")),
			 node.attribute("auto_text").to_bool()));

	if (node.has_attribute("progress_normalized"))
		progress_bar().set_progress_normalized
			(node.attribute("progress_normalized").to_float());

	if (node.has_attribute("text"))
		progress_bar().set_text
			(xml_util::unescape_whitespace_prioritized(node.attribute("text")));

	return progress_bar;
}


shared_instance<gui_element> gui_element_loader::to_image_map(const xml_node& node)
{
	return shared_instance<gui_element>
		(new gui_image_map
			(node.name(), gui,
			 style_cache.entry<gui_image_map_style>(node.attribute("image_map_style")),
			 material_cache.entry(node.attribute("material")),
			 xml_converter::to_vector2i(node.child("image_size")),
			 node.attribute("continuous_events").to_bool()));
}


shared_instance<gui_element> gui_element_loader::to_split(const xml_node& node)
{
	shared_instance<gui_split> split
		(new gui_split
			(node.name(), gui,
			 style_cache.entry<gui_split_style>(node.attribute("split_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation"))));

	if (node.has_attribute("read_only"))
		split().set_read_only
			(node.attribute("read_only").to_bool());

	if (node.has_attribute("split_normalized"))
		split().set_split_normalized
			(node.attribute("split_normalized").to_float());

	if (node.has_attribute("neg_content"))
		split().set_neg_content
			(element_cache.entry(node.attribute("neg_content")));

	if (node.has_attribute("pos_content"))
		split().set_pos_content
			(element_cache.entry(node.attribute("pos_content")));

	if (node.has_child("links"))
	{
		gui_split::link_list links;

		// Always self-insert: Less error-prone,
		// and split is not in element_cache yet.
		links.insert(split);

		for (xml_node::const_child_iterator it
					(node.child("links").first_child()); it; ++it)
		{
			if (it->name() != "link")
			{
				LOG_ERROR("Name of XML nodes for links must be link.");
				throw xml_exception();
			}

			links.insert
				(element_cache.entry<gui_split>(it->attribute("ref")));
		}

		// Apply links to all linked elements.
		// This avoids problems with cyclic dependencies:
		// Clients just specify links on the last split.
		for (gui_split::link_list::const_iterator it
				(links.first()); it; ++it)
		{
			shared_pointer<gui_split> link(it->lock());
			if (link) link->set_links(links);
		}
	}

	return split;
}


shared_instance<gui_element> gui_element_loader::to_scrollbar(const xml_node& node)
{
	shared_instance<gui_scrollbar> scrollbar
		(new gui_scrollbar
			(node.name(), gui,
			 style_cache.entry<gui_scrollbar_style>(node.attribute("scrollbar_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation"))));

	// Scrollbar range must be set first
	// so that our position clamps correctly.
	scrollbar().set_range
		(node.has_attribute("range_min") ?
			node.attribute("range_min").to_float() : scrollbar().range_min(),
		 node.has_attribute("range_max") ?
			node.attribute("range_max").to_float() : scrollbar().range_max());

	if (node.has_attribute("position") &&
		node.has_attribute("position_normalized"))
	{
		LOG_ERROR("Scrollbar must not have both position and position_normalized.");
		throw xml_exception();
	}

	if (node.has_attribute("position"))
		scrollbar().set_position
			(node.attribute("position").to_float());
	if (node.has_attribute("position_normalized"))
		scrollbar().set_position_normalized
			(node.attribute("position_normalized").to_float());

	if (node.has_attribute("item_size"))
		scrollbar().set_item_size
			(node.attribute("item_size").to_float());
	if (node.has_attribute("page_size"))
		scrollbar().set_page_size
			(node.attribute("page_size").to_float());

	if (node.has_attribute("overlap_factor"))
		scrollbar().set_overlap_factor
			(node.attribute("overlap_factor").to_float());
	if (node.has_attribute("continuous_increment"))
		scrollbar().set_continuous_increment
			(node.attribute("continuous_increment").to_float());

	return scrollbar;
}


shared_instance<gui_element> gui_element_loader::to_scroll_area(const xml_node& node)
{
	shared_instance<gui_scroll_area> scroll_area
		(new gui_scroll_area
			(node.name(), gui,
			 style_cache.entry<gui_scroll_area_style>(node.attribute("scroll_area_style"))));

	if (node.has_attribute("content"))
		scroll_area().set_content
			(element_cache.entry(node.attribute("content")));

	// We cannot load position or position_normalized,
	// as these become relevant only after layouting.

	return scroll_area;
}


shared_instance<gui_element> gui_element_loader::to_list(const xml_node& node)
{
	shared_instance<gui_list> list
		(new gui_list
			(node.name(), gui,
			 style_cache.entry<gui_list_style>(node.attribute("list_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation")),
			 gui_element_loader_detail::to_list_item_select_mode(node.attribute("select_mode"))));

	to_list_items_helper(node, list());
	list().synchronize_item_indices();

	if (node.has_attribute("select_index"))
		list().select_index
			(node.attribute("select_index").to_int64());						 

	return list;
}


shared_instance<gui_element> gui_element_loader::to_listbox(const xml_node& node)
{
	shared_instance<gui_listbox> listbox
		(new gui_listbox
			(node.name(), gui,
			 style_cache.entry<gui_listbox_style>(node.attribute("listbox_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation")),
			 gui_element_loader_detail::to_list_item_select_mode(node.attribute("select_mode"))));

	to_list_items_helper(node, listbox().list());
	listbox().synchronize_item_indices();

	if (node.has_attribute("select_index"))
		listbox().select_index
			(node.attribute("select_index").to_int64());						 

	return listbox;
}


shared_instance<gui_element> gui_element_loader::to_dropdown(const xml_node& node)
{
	shared_instance<gui_dropdown> dropdown
		(new gui_dropdown
			(node.name(), gui,
			 style_cache.entry<gui_dropdown_style>(node.attribute("dropdown_style"))));

	to_list_items_helper(node, dropdown().list());
	dropdown().synchronize_item_indices();
	dropdown().select_index(0);

	if (node.has_attribute("select_index"))
		dropdown().select_index
			(node.attribute("select_index").to_int64());

	if (node.has_attribute("label_text"))
		dropdown().set_label_text
			(xml_util::unescape_whitespace_prioritized(node.attribute("label_text")));

	return dropdown;
}


shared_instance<gui_element> gui_element_loader::to_tabview(const xml_node& node)
{
	shared_instance<gui_tabview> tabview
		(new gui_tabview
			(node.name(), gui,
			 style_cache.entry<gui_tabview_style>(node.attribute("tabview_style")),
			 gui_element_loader_detail::to_orientation_type(node.attribute("orientation"))));

	if (node.has_child("pages"))
	{
		for (xml_node::const_child_iterator it
				(node.child("pages").first_child()); it; ++it)
		{
			if (it->name() != "page")
			{
				LOG_ERROR("Name of XML nodes for pages must be page.");
				throw xml_exception();
			}

			tabview().insert_page
				(xml_util::unescape_whitespace_prioritized(it->attribute("caption")),
				 (element_cache.entry(it->attribute("content"))));
		}
	}

	if (node.has_attribute("show_page"))
		tabview().show_page
			(node.attribute("show_page").to_int64());

	return tabview;
}


shared_instance<gui_element> gui_element_loader::to_edit(const xml_node& node)
{
	shared_instance<gui_edit> edit
		(new gui_edit
			(node.name(), gui,
			 style_cache.entry<gui_edit_style>(node.attribute("edit_style")),
			 node.attribute("multiline").to_bool(),
			 node.attribute("notify_accept").to_bool()));

	if (node.has_attribute("text"))
		edit().set_text
			(xml_util::unescape_whitespace_prioritized(node.attribute("text")));

	if (node.has_attribute("read_only"))
		edit().set_read_only
			(node.attribute("read_only").to_bool());

	if (node.has_attribute("history_step_limit"))
		edit().set_history_step_limit
			(node.attribute("history_step_limit").to_int64());

	if (node.has_attribute("history_memory_limit"))
		edit().set_history_memory_limit
			(node.attribute("history_memory_limit").to_int64());

	return edit;
}


shared_instance<gui_element> gui_element_loader::to_editbox(const xml_node& node)
{
	shared_instance<gui_editbox> editbox
		(new gui_editbox
			(node.name(), gui,
			 style_cache.entry<gui_editbox_style>(node.attribute("editbox_style")),
			 node.attribute("multiline").to_bool(),
			 node.attribute("notify_accept").to_bool()));

	if (node.has_attribute("text"))
		editbox().set_text
			(xml_util::unescape_whitespace_prioritized(node.attribute("text")));

	if (node.has_attribute("read_only"))
		editbox().set_read_only
			(node.attribute("read_only").to_bool());

	if (node.has_attribute("history_step_limit"))
		editbox().set_history_step_limit
			(node.attribute("history_step_limit").to_int64());

	if (node.has_attribute("history_memory_limit"))
		editbox().set_history_memory_limit
			(node.attribute("history_memory_limit").to_int64());

	return editbox;
}


shared_instance<gui_element> gui_element_loader::to_window(const xml_node& node)
{
	shared_instance<gui_window> window
		(new gui_window
			(node.name(), gui,
			 style_cache.entry<gui_window_style>(node.attribute("window_style")),
			 xml_util::unescape_whitespace_prioritized(node.attribute("caption")),
			 node.attribute("allow_close").to_bool(),			 
			 gui_element_loader_detail::to_window_resize_policy(node.attribute("resize_policy"))));

	if (node.has_attribute("content"))
		window().set_content
			(element_cache.entry(node.attribute("content")));

	if (node.has_child("client_size"))
	{
		window().set_client_size
			(xml_converter::to_vector2f(node.child("client_size")));
	}
	else
		window().resize_to_preferred_content();

	if (node.has_child("position"))
	{
		window().set_position
			(xml_converter::to_vector2f(node.child("position")));
	}
	else
		window().center_window_delayed();

	return window;
}


shared_instance<gui_element> gui_element_loader::to_window_area(const xml_node& node)
{
	shared_instance<gui_window_area> window_area
		(new gui_window_area
			(node.name(), gui));

	if (node.has_attribute("background"))
		window_area().set_background
			(element_cache.entry(node.attribute("background")));

	if (node.has_child("windows"))
	{
		for (xml_node::const_child_iterator it
				(node.child("windows").first_child()); it; ++it)
		{
			if (it->name() != "window")
			{
				LOG_ERROR("Name of XML nodes for windows must be window.");
				throw xml_exception();
			}

			window_area().insert_window
				(element_cache.entry<gui_window>(it->attribute("ref")));
		}
	}

	return window_area;
}


shared_instance<gui_element> gui_element_loader::to_tooltip(const xml_node& node)
{
	LOG_ERROR("Cannot create gui_tooltip directly, "
			  "use gui_tooltip_area instead.");
	throw unsupported_feature();
}


shared_instance<gui_element> gui_element_loader::to_tooltip_area(const xml_node& node)
{
	shared_instance<gui_tooltip_area> tooltip_area
		(new gui_tooltip_area
			(node.name(), gui, 
			 style_cache.entry<gui_tooltip_area_style>(node.attribute("tooltip_area_style"))));

	if (node.has_attribute("content"))
		tooltip_area().set_content
			(element_cache.entry(node.attribute("content")));

	if (node.has_child("tooltips"))
	{
		for (xml_node::const_child_iterator it
				(node.child("tooltips").first_child()); it; ++it)
		{
			if (it->name() != "tooltip")
			{
				LOG_ERROR("Name of XML nodes for tooltips must be tooltip.");
				throw xml_exception();
			}

			if (it->has_attribute("material") &&
				it->has_child("image_size"))
			{
				tooltip_area().register_tooltip
					(element_cache.entry(it->attribute("element")),
					 xml_util::unescape_whitespace_prioritized(it->attribute("text")),
					 material_cache.entry(node.attribute("material")),				 
					 xml_converter::to_vector2i(node.child("image_size")));
			}
			else
			{
				tooltip_area().register_tooltip
					(element_cache.entry(it->attribute("element")),
					 xml_util::unescape_whitespace_prioritized(it->attribute("text")));
			}
		}
	}

	return tooltip_area;
}




} /* namespace viral_gui */
