/**
 *************************************************************************
 *
 * @file gui_image_map.cpp
 *
 * Clickable image maps, implementation.
 *
 ************************************************************************/


#include "gui_image_map.hpp"


#include "gui_image.hpp"
#include "gui_frame.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_image_map
//
//////////////////////////////////////////////////////////////////////////


gui_image_map::gui_image_map
	(const string& name, gui_context& gui,
	 const shared_instance<gui_image_map_style>& image_map_style,
	 const shared_instance<render_material_id>& material,
	 const vector2i& image_size,
	 bool continuous_events)
	:
	gui_element(name, gui),
	image_map_style(image_map_style),
	continuous_events(continuous_events),

	dragging_(false),

	image_(new gui_image
		(name + "_image", gui, image_map_style().image_style,
		 material, image_size)),

	frame_(new gui_frame
		(name + "_frame", gui, image_map_style().frame_style))
{
	frame_().set_content(image_);
}


gui_image_map::gui_image_map
	(const string& name, gui_context& gui,
	 const shared_instance<gui_image_map_style>& image_map_style,
	 const image_rgba& source_image,
	 const render_material_data& material_data,
	 bool continuous_events)
	:
	gui_element(name, gui),
	image_map_style(image_map_style),
	continuous_events(continuous_events),

	dragging_(false),

	image_(new gui_image
		(name + "_image", gui, image_map_style().image_style,
		 source_image, material_data)),

	frame_(new gui_frame
		(name + "_frame", gui, image_map_style().frame_style))
{
	frame_().set_content(image_);
}


vector2f gui_image_map::preferred_size() const
	{ return frame_().preferred_size(); }


void gui_image_map::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	process_image_map_clicks(input, output);
	process_image_map_dragging(input, output);


	// The frame consumes any remaining hot clicks
	// within both clip_area() and arrange_area().
	frame_().process_input(input, output);
}


void gui_image_map::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;
	frame_().commit_render(packer, queue);
}


void gui_image_map::apply_material
	(const shared_instance<render_material_id>& material,
	 const vector2i& image_size)
{
	image_().apply_material(material, image_size);
	want_layout();
}


void gui_image_map::apply_source_image
	(const image_rgba& source_image,
	 const render_material_data& material_data)
{
	image_().apply_source_image(source_image, material_data);
	want_layout();
}


void gui_image_map::realize_layout()
{
	if (layout_invalid())
	{
		frame_().clip_entirely();
		return;
	}

	frame_().layout(arrange_area(), clip_area());
}


void gui_image_map::process_image_map_clicks
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (continuous_events) return;


	// Ensure that image map and input event are valid.
	if (layout_invalid()) return;
	if (image_().layout_invalid()) return;

	if (!input.hot) return;
	if (typeid(input) != typeid(gui_mouse_event)) return;

	const gui_mouse_event& mouse_input =
		static_cast<const gui_mouse_event&>(input);

	if (mouse_input.event != gui_mouse_event::button_click ||
		mouse_input.button != gui_mouse_event::left_button)
			return;


	// Check for click in image and generate event.
	rect2f image_arrange(image_().arrange_area());
	rect2f image_clip(image_().clip_area());

	if (!image_arrange.contains(mouse_input.cursor) ||
		!image_clip.contains(mouse_input.cursor))
			return;

	input.hot = false;
	create_image_map_event(mouse_input, output, image_arrange);
}


void gui_image_map::process_image_map_dragging
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (!continuous_events) return;


	// Fetch layout of image.
	if (layout_invalid() ||
		image_().layout_invalid())
	{
		dragging_ = false;
		return;
	}

	rect2f image_arrange(image_().arrange_area());
	rect2f image_clip(image_().clip_area());


	// Evaluate mouse input:
	// Start and stop dragging, create dragging events.
	if (typeid(input) != typeid(gui_mouse_event)) return;

	const gui_mouse_event& mouse_input =
		static_cast<const gui_mouse_event&>(input);

	if (mouse_input.event == gui_mouse_event::button_release &&
		mouse_input.button == gui_mouse_event::left_button)
			dragging_ = false;
	
	if (mouse_input.event == gui_mouse_event::button_click &&
		mouse_input.button == gui_mouse_event::left_button)
	{
		if (input.hot &&
			image_arrange.contains(mouse_input.cursor) &&
			image_clip.contains(mouse_input.cursor))
		{
			input.hot = false;
			dragging_ = true;
			create_image_map_event(mouse_input, output, image_arrange);
		}
	}

	if (mouse_input.event == gui_mouse_event::cursor_move)
	{
		if (dragging_)
			create_image_map_event(mouse_input, output, image_arrange);
	}
}


void gui_image_map::create_image_map_event
	(const gui_mouse_event& mouse_input,
	 gui_output_event_list& output,
	 const rect2f& image_arrange)
{
	/**
	 * Create relative event within image arrangement area.
	 *
	 * Implementation notes:
	 * - To avoid numerical issues on degenerate images,
	 *		we generate a (0, 0) normalized position on images
	 *		below some epsilon size.
	 * - We must clamp normalized and relative positions
	 *		because dragging triggers create_image_map_event()
	 *		even when dragging outside the image map.
	 */

	vector2f relative_gui_position
		((mouse_input.cursor - image_arrange.pos).
			max(vector2f()).min(image_arrange.size));

	vector2f normalized_position
		(image_arrange.size.min() < clickable_size_epsilon_ ?
			vector2f() : // Degenerate image with one min()mal dimension.
			(relative_gui_position / image_arrange.size).
				max(vector2f()).min(vector2f(1, 1)));

	auto_pointer<gui_image_map_event> output_event
		(new gui_image_map_event
			(*this, normalized_position, relative_gui_position));
	output.insert
		(std::move(output_event));
}


const float gui_image_map::clickable_size_epsilon_ = 0.001f;




//////////////////////////////////////////////////////////////////////////
//
// gui_image_map_style
//
//////////////////////////////////////////////////////////////////////////


gui_image_map_style::gui_image_map_style
	(const shared_instance<gui_image_style>& image_style,
	 const shared_instance<gui_frame_style>& frame_style)
	:
	image_style(image_style),
	frame_style(frame_style)
{ }




} /* namespace viral_gui */
