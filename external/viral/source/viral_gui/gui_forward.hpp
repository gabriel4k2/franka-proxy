/**
 *************************************************************************
 *
 * @file gui_forward.hpp
 *
 * Forward declarations for the @ref gui_module "GUI Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_FORWARD_HPP)
#define INCLUDED__VIRAL_GUI__GUI_FORWARD_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/container_forward.hpp>
#include <viral_core/render_forward.hpp>


namespace viral_gui
{


using namespace viral_core;


// High-level GUI applications.
class gui_app;
class gui_event_handler;
template <typename T, typename O> class gui_event_handler_callback;


// Fundamental GUI classes.
class gui_context;

class gui_element;
typedef shared_cache<gui_element> gui_element_cache;
class gui_element_loader;

class gui_style;
typedef shared_cache<gui_style> gui_style_cache;
typedef shared_cache<render_shader_id> gui_shader_cache;
typedef shared_cache<render_material_id> gui_material_cache;
class gui_style_loader;

class gui_cursor_style;
class gui_font_style;


// GUI rendering utilities.
class gui_render_layer;
class gui_render_packer;
class gui_render_util;


// GUI event handling.
class gui_input_event;
class gui_tick_event;
class gui_input_source_event;
class gui_mouse_event;
class gui_mouse_wheel_event;
class gui_text_event;

class gui_output_event;
template <typename T, int64 E = 0> class gui_simple_output_event;
typedef owning_list<gui_output_event> gui_output_event_list;


// Layouting utilities.
class gui_spacer;
class gui_spacer_style;
class gui_anchor;
class gui_anchor_style;
class gui_stack;
class gui_grid;
class gui_grid_cell;
class gui_grid_cell_style;


// Static GUI elements.
class gui_label;
class gui_label_style;
class gui_image;
class gui_image_style;

class gui_frame;
class gui_frame_style;


// Button elements.
class gui_button;
class gui_button_style;
typedef gui_simple_output_event<gui_button, 0> gui_button_event;
typedef gui_simple_output_event<gui_button, 1> gui_button_hold_event;
typedef gui_simple_output_event<gui_button, 2> gui_button_locked_event;
typedef gui_simple_output_event<gui_button, 3> gui_button_repeat_event;

class gui_tristate;
class gui_tristate_style;
typedef gui_simple_output_event<gui_tristate> gui_tristate_event;

class gui_knob;
class gui_knob_style;
typedef gui_simple_output_event<gui_knob> gui_knob_event;


// Value editing and visualizing elements.
class gui_slider;
class gui_slider_style;
typedef gui_simple_output_event<gui_slider> gui_slider_event;

class gui_value_edit_converter;
class gui_value_edit;
class gui_value_edit_style;
typedef gui_simple_output_event<gui_value_edit> gui_value_edit_event;

class gui_progress_bar;
class gui_progress_bar_style;
typedef gui_simple_output_event<gui_progress_bar> gui_progress_bar_event;

class gui_image_map;
class gui_image_map_style;
class gui_image_map_event;


// Splitting and scrolling elements.
class gui_split;
class gui_split_style;
typedef gui_simple_output_event<gui_split> gui_split_event;

class gui_scrollbar;
class gui_scrollbar_style;
typedef gui_simple_output_event<gui_scrollbar> gui_scrollbar_event;

class gui_scroll_area;
class gui_scroll_area_style;
typedef gui_simple_output_event<gui_scroll_area> gui_scroll_area_event;


// Item selection in listbox, dropbox, tabview.
class gui_list;
class gui_list_style;
class gui_list_item;
typedef gui_simple_output_event<gui_list, 0> gui_list_event;
typedef gui_simple_output_event<gui_list, 1> gui_list_accept_event;

class gui_listbox;
class gui_listbox_style;
typedef gui_simple_output_event<gui_listbox, 0> gui_listbox_event;
typedef gui_simple_output_event<gui_listbox, 1> gui_listbox_accept_event;

class gui_dropdown;
class gui_dropdown_style;
typedef gui_simple_output_event<gui_dropdown> gui_dropdown_event;
class gui_dropdown_modal_helper;

class gui_tabview;
class gui_tabview_style;
class gui_tabview_header;
class gui_tabview_switch;


// Text editing.
class gui_edit;
class gui_edit_style;
typedef gui_simple_output_event<gui_edit, 0> gui_edit_event;
typedef gui_simple_output_event<gui_edit, 1> gui_edit_accept_event;
typedef gui_simple_output_event<gui_edit, 2> gui_edit_cancel_event;
class gui_edit_lines;
class gui_edit_selection;
class gui_edit_operation;
class gui_edit_kernel;

class gui_editbox;
class gui_editbox_style;
typedef gui_simple_output_event<gui_editbox, 0> gui_editbox_event;
typedef gui_simple_output_event<gui_editbox, 1> gui_editbox_accept_event;
typedef gui_simple_output_event<gui_editbox, 2> gui_editbox_cancel_event;


// Windowing and background viewports.
class gui_viewport;
class gui_window;
class gui_window_style;
typedef gui_simple_output_event<gui_window> gui_window_close_event;
class gui_window_area;


// Tooltips.
class gui_tooltip_data;
class gui_tooltip_data_material;
class gui_tooltip_data_image;

class gui_tooltip;
class gui_tooltip_style;
class gui_tooltip_area;
class gui_tooltip_area_style;


// Modal interactions.
class gui_modal_interaction;
class gui_modal_context;
class gui_modal_dialog;

class gui_msgbox;
class gui_msgbox_style;

class gui_progress_box;
class gui_progress_box_style;
class gui_progress_box_thread;

class gui_string_dialog;
class gui_string_dialog_style;

class gui_file_dialog;
class gui_file_dialog_style;
class gui_file_dialog_item;

class gui_color_dialog;
class gui_color_dialog_style;




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_FORWARD_HPP) */
