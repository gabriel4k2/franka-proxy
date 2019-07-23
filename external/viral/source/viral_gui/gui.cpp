/**
 *************************************************************************
 *
 * @file gui.cpp
 *
 * Main GUI implementation file.
 *
 ************************************************************************/


#include "gui.hpp"


#include <viral_core/log.hpp>
#include <viral_core/window.hpp>
#include <viral_core/input.hpp>
#include <viral_core/render_scene.hpp>
#include <viral_core/render_process.hpp>
#include <viral_core/render_command.hpp>


#include "gui_event.hpp"
#include "gui_render.hpp"
#include "gui_style_util.hpp"
#include "gui_stack.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_context
//
//////////////////////////////////////////////////////////////////////////


gui_context::gui_context
	(const string& name,
	 window_context& window,
	 input_context& input,
	 gui_modal_context& modal_context,
	 const shared_instance<gui_cursor_style>& cursor_style,
	 bool pixel_integral_rendering)
	:
	name(name),
	window(window),
	input(input),
	modal_context(modal_context),
	cursor_style(cursor_style),
	pixel_integral_rendering(pixel_integral_rendering),

	modal_stack_(new gui_stack("root", *this, true)),

	render_dirty_(true),

	gui_puppet_(new render_puppet_id(name + "_gui_puppet")),
	gui_model_(new render_model_id(name + "_gui_model")),

	cursor_puppet_(new render_puppet_id(name + "_cursor_puppet")),
	cursor_model_(new render_model_id(name + "_cursor_model")), 

	render_stack_(new render_stack_id(name + "_render_stack")),
	render_scene_(new render_scene_id(name + "_render_scene")),
	render_layer_(new render_layer_id(name + "_render_layer"))
{
	LOG_INFO("Creating GUI: " + name);
}


gui_context::~gui_context() NOTHROW
	{ LOG_INFO("Destroying GUI: " + name); }


gui_stack& gui_context::modal_stack() NOTHROW
	{ return modal_stack_(); }
const gui_stack& gui_context::modal_stack() const NOTHROW
	{ return modal_stack_(); }


bool gui_context::contains(const gui_element& element) NOTHROW
{
	// Element with mismatched gui_context
	// can never be part of this GUI.
	if (&element.gui != this)
		return false;


	// Ascend from element to modal stack
	// instead of searching through children.
	for (const gui_element* test_modal_stack = &element;
		 test_modal_stack;
		 test_modal_stack = test_modal_stack->parent())
	{
		if (test_modal_stack == &modal_stack_())
			return true;
	}

	return false;
}


void gui_context::update(gui_output_event_list& output)
{
	// Two separate updates to layout:
	// - One to adapt the GUI to the current window size.
	// - Another one to adapt to results of user input.
	update_layout();

	set_temporary_cursor_style
		(shared_pointer<gui_cursor_style>());

	handle_input(output);

	update_layout();
}


void gui_context::commit_render
	(render_command_queue& queue)
{
	// Build stack with GUI and cursor,
	// then commit stack data to queue.
	render_stack_data stack_data;
	commit_render_gui(stack_data, queue);
	commit_render_cursor(stack_data, queue);
	queue.commit
		(std::move(stack_data), render_stack_);


	// Build scene and layer around stack,
	// then commit these to queue as well.
	render_scene_data scene_data;
	scene_data.objects.insert(render_stack_);
	queue.commit
		(std::move(scene_data), render_scene_);

	vector2f window_size(window.size());

	render_layer_data layer_data;
	layer_data.scene = render_scene_;
	layer_data.cam =
		camera(vector(window_size.x, window_size.y, 0) * 0.5f,
			   rotation(), camera::orthographic,
			   window_size.x, window_size.y / window_size.x,
			   -1, 1);
	queue.commit
		(std::move(layer_data), render_layer_);	
}


void gui_context::raise_render_dirty() NOTHROW
	{ render_dirty_ = true; }


const shared_instance<render_layer_id>&
	gui_context::render_layer() const
		{ return render_layer_; }


void gui_context::set_key_focus
	(const shared_pointer<gui_element>& key_focus) NOTHROW
{
	if (key_focus == key_focus_) return;

	bool key_focus_accepted =
		key_focus ?
			key_focus->try_accept_key_focus() : true;

	// Immediately abort if key focus was not accepted.
	// There may have been a recursive set_key_focus() call.
	if (!key_focus_accepted) return;
	
	shared_pointer<gui_element>
		old_key_focus(key_focus_.lock());

	key_focus_ = key_focus;
	
	if (old_key_focus)
		old_key_focus->lost_key_focus();
}


void gui_context::set_mouse_focus
	(const shared_pointer<gui_element>& mouse_focus) NOTHROW
{
	if (mouse_focus == mouse_focus_) return;

	bool mouse_focus_accepted =
		mouse_focus ?
			mouse_focus->try_accept_mouse_focus() : true;

	// Immediately abort if mouse focus was not accepted.
	// There may have been a recursive set_mouse_focus() call.
	if (!mouse_focus_accepted) return;

	shared_pointer<gui_element>
		old_mouse_focus(mouse_focus_.lock());

	mouse_focus_ = mouse_focus;

	if (old_mouse_focus)
		old_mouse_focus->lost_mouse_focus();
}


void gui_context::clear_key_focus() NOTHROW
	{ set_key_focus(shared_pointer<gui_element>()); }
void gui_context::clear_mouse_focus() NOTHROW
	{ set_mouse_focus(shared_pointer<gui_element>()); }


weak_pointer<gui_element> gui_context::key_focus() const NOTHROW
	{ return key_focus_; }
weak_pointer<gui_element> gui_context::mouse_focus() const NOTHROW
	{ return mouse_focus_; }


void gui_context::set_temporary_cursor_style
	(const shared_pointer<gui_cursor_style>& temporary_cursor_style) NOTHROW
		{ temporary_cursor_style_ = temporary_cursor_style; }


void gui_context::update_layout()
{
	vector2f gui_size
		(window.size().max
			(vector2f(gui_size_min_, gui_size_min_)).min
				(vector2f(gui_size_max_, gui_size_max_)));
	
	rect2f gui_area(vector2f(), gui_size);
	modal_stack_().layout(gui_area, gui_area);

	// If some GUI element wanted to stay_dirty(),
	// give the element just a single chance to recover.
	// We do not loop infinitely on faulty elements.
	if (modal_stack_().layout_dirty())
		modal_stack_().layout(gui_area, gui_area);
}


void gui_context::handle_input
	(gui_output_event_list& output)
{
	// Fetch general event flags.
	// This avoids many redundant queries later on.
	bool event_hot =
		((window.notify_flags() & window_context::active) != 0);

	bool shift_down =
		input.is_active(input_source::k_shift) ||
		input.is_active(input_source::k_left_shift) ||
		input.is_active(input_source::k_right_shift);

	bool ctrl_down =
		input.is_active(input_source::k_control) ||
		input.is_active(input_source::k_left_control) ||
		input.is_active(input_source::k_right_control);

	bool alt_down =
		input.is_active(input_source::k_alt) ||
		input.is_active(input_source::k_left_alt) ||
		input.is_active(input_source::k_right_alt);


	// Create and process individual gui_input_event types.
	handle_tick_input
		(output, event_hot);
	handle_input_source_input
		(output, event_hot, shift_down, ctrl_down, alt_down);
	handle_mouse_input
		(output, event_hot, shift_down, ctrl_down, alt_down);
	handle_text_input
		(output, event_hot, shift_down, ctrl_down, alt_down);
}


void gui_context::handle_tick_input
	(gui_output_event_list& output, bool event_hot)
{
	gui_tick_event tick_event(event_hot);
	modal_stack_().process_input(tick_event, output);
}


void gui_context::handle_input_source_input
	(gui_output_event_list& output,
	 bool event_hot,
	 bool shift_down, bool ctrl_down, bool alt_down)
{
	for (int32 i = 0; i < input_source::count; ++i)
	{
		input_source::type s =
			static_cast<input_source::type>(i);

		if (input.was_activated(s))
		{
			gui_input_source_event activated_event
				(event_hot, s, gui_input_source_event::source_activated,
				 shift_down, ctrl_down, alt_down);

			modal_stack_().process_input
				(activated_event, output);
		}

		if (input.was_deactivated(s))
		{
			gui_input_source_event deactivated_event
				(event_hot, s, gui_input_source_event::source_deactivated,
				 shift_down, ctrl_down, alt_down);

			modal_stack_().process_input
				(deactivated_event, output);
		}
	}
}


void gui_context::handle_mouse_input
	(gui_output_event_list& output,
	 bool event_hot,
	 bool shift_down, bool ctrl_down, bool alt_down)
{
	// Fetch mouse input from input subsystem.
	// Note we need to transform from screen space
	// to GUI-internal window coordinates.
	vector2f cursor
		(vector2f(input.value_of(input_source::m_absolute_x),
				  input.value_of(input_source::m_absolute_y)) -
						window.position());


	// Only react to mouse events
	// while mouse is over window.
	event_hot = event_hot &&
		rect2f(vector2f(), window.size()).contains(cursor);


	// Create and process mouse button events.
	if (input.was_activated(input_source::m_button0))
	{
		gui_mouse_event left_click
			(event_hot, cursor, gui_mouse_event::left_button,
			 gui_mouse_event::button_click,
			 shift_down, ctrl_down, alt_down);

		modal_stack_().process_input(left_click, output);
	}

	if (input.was_deactivated(input_source::m_button0))
	{
		gui_mouse_event left_release
			(event_hot, cursor, gui_mouse_event::left_button,
			 gui_mouse_event::button_release,
			 shift_down, ctrl_down, alt_down);

		modal_stack_().process_input(left_release, output);
	}

	if (input.was_activated(input_source::m_button1))
	{
		gui_mouse_event right_click
			(event_hot, cursor, gui_mouse_event::right_button,
			 gui_mouse_event::button_click,
			 shift_down, ctrl_down, alt_down);

		modal_stack_().process_input(right_click, output);
	}

	if (input.was_deactivated(input_source::m_button1))
	{
		gui_mouse_event right_release
			(event_hot, cursor, gui_mouse_event::right_button,
			 gui_mouse_event::button_release,
			 shift_down, ctrl_down, alt_down);

		modal_stack_().process_input(right_release, output);
	}

	if (input.was_activated(input_source::m_button2))
	{
		gui_mouse_event middle_click
			(event_hot, cursor, gui_mouse_event::middle_button,
			 gui_mouse_event::button_click,
			 shift_down, ctrl_down, alt_down);

		modal_stack_().process_input(middle_click, output);
	}

	if (input.was_deactivated(input_source::m_button2))
	{
		gui_mouse_event middle_release
			(event_hot, cursor, gui_mouse_event::middle_button,
			 gui_mouse_event::button_release,
			 shift_down, ctrl_down, alt_down);

		modal_stack_().process_input(middle_release, output);
	}


	// Create and process mouse wheel events.
	if (input.value_of(input_source::m_axis2) != 0)
	{
		gui_mouse_wheel_event mouse_wheel_event
			(event_hot, cursor,
			 input.value_of(input_source::m_axis2),
			 mouse_wheel_movement_per_line_,
			 shift_down, ctrl_down, alt_down);
	}


	// Cursor positioning events occur always,
	// even if the mouse position did not change.
	gui_mouse_event cursor_move_event
		(event_hot, cursor, gui_mouse_event::no_button,
		 gui_mouse_event::cursor_move,
		 shift_down, ctrl_down, alt_down);

	modal_stack_().process_input
		(cursor_move_event, output);
}


void gui_context::handle_text_input
	(gui_output_event_list& output,
	 bool event_hot,
	 bool shift_down, bool ctrl_down, bool alt_down)
{
	input_context::text_input_buffer text_input;
	input.get_text_input(text_input);

	for (int64 i = 0; i < text_input.size(); ++i)
	{
		gui_text_event text_event
			(event_hot, text_input[i]);

		modal_stack_().process_input
			(text_event, output);
	}
}


void gui_context::commit_render_gui
	(render_stack_data& stack,
	 render_command_queue& queue)
{
	// GUI elements always show on stack.
	stack.puppets.push_back(gui_puppet_);
	if (!render_dirty_) return;


	// Collect (probably modified) element geometry.
	gui_render_packer packer;
	modal_stack_().commit_render(packer, queue);


	// Build total geometry from packed render layers.
	// Maintain the order of original layers, and provide
	// index-based material assignment for each layer.
	model mdl;

	render_model_data model_data;
	render_puppet_data puppet_data;

	int64 layer_index = 0;
	for (gui_render_packer::const_render_layer_iterator it
			(packer.first_render_layer()); it; ++it)
	{
		if (!*it) continue;

		mdl.insert_group
			(string(layer_index),
			 std::move(*(*it)->release_target_mesh()));
		puppet_data.materials.insert
			(string(layer_index), (*it)->material);

		++layer_index;
	}

	mdl.validate();
	mdl.rebuild_boundings();


	// Linkup model and puppet,
	// then commit results to rendering.
	model_data.model = std::move(mdl);
	model_data.transient_hint = true;
	puppet_data.model = gui_model_;

	queue.commit
		(std::move(model_data), gui_model_);
	queue.commit
		(std::move(puppet_data), gui_puppet_);

	render_dirty_ = false;
}


void gui_context::commit_render_cursor
	(render_stack_data& stack,
	 render_command_queue& queue)
{
	// Abort if cursor currently is outside our window.
	// This avoids inconsistent display of both the OS cursor
	// and our custom GUI cursor.
	vector2f mouse_pos
		(vector2f(input.value_of(input_source::m_absolute_x),
				  input.value_of(input_source::m_absolute_y)) -
			window.position());

	if (mouse_pos.min() < 0 ||
		mouse_pos.x >= window.size().x ||
		mouse_pos.y >= window.size().y)
			return;


	// Update cursor model to match current cursor style.
	// Avoid per-frame recreation of unchanged geometry.
	// We don't use gui_render_packer as the cursor usually
	// has a distinct texture and updates very frequently.
	shared_instance<gui_cursor_style> current_cursor_style
		(temporary_cursor_style_ ?
			temporary_cursor_style_.instance_or_throw() :
			cursor_style);

	if (current_cursor_style != render_cursor_style_)		
	{
		model mdl
			(gui_render_util::create_model_from_rect
				("cursor", current_cursor_style().extents, rect2f(0, 0, 1, 1),
				 rect2f(), false, pixel_integral_rendering));

		render_model_data model_data;
		model_data.model = std::move(mdl);
		queue.commit
			(std::move(model_data), cursor_model_);

		render_cursor_style_ = current_cursor_style;
	}


	// Move cursor to current mouse position
	// within window coordinates.	 
	render_puppet_data puppet_data;
	puppet_data.position =
		vector(mouse_pos.x, mouse_pos.y, 0);
	puppet_data.materials.insert
		("cursor", current_cursor_style().material);
	puppet_data.model = cursor_model_;
	queue.commit
		(std::move(puppet_data), cursor_puppet_);


	// Add cursor atop rendering stack.
	stack.puppets.push_back(cursor_puppet_);
}


const float gui_context::gui_size_min_ = 128.f;
const float gui_context::gui_size_max_ = 4096.f;


#if defined(VIRAL__OS_TARGET_WINDOWS)
	const float gui_context::mouse_wheel_movement_per_line_ = WHEEL_DELTA;
#else
	#error "Mouse wheel movement per line not set for current build target."
#endif




} /* namespace viral_gui */

