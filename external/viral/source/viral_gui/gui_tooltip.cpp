/**
 *************************************************************************
 *
 * @file gui_tooltip.cpp
 *
 * Tooltips for the GUI, implementation.
 *
 ************************************************************************/


#include "gui_tooltip.hpp"


#include <viral_core/log.hpp>


#include "gui_label.hpp"
#include "gui_image.hpp"
#include "gui_frame.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip::gui_tooltip
	(const string& name, gui_context& gui,
	 const shared_instance<gui_tooltip_style>& tooltip_style)
	:
	gui_element(name, gui),
	tooltip_style(tooltip_style),

	frame_(new gui_frame(name + "_frame", gui,
						 tooltip_style().frame_style))
{
	insert_child(frame_);
}


vector2f gui_tooltip::preferred_size() const
{
	return
		frame_().frame_size_for_content_size
			(calculate_content_size());
}


void gui_tooltip::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (layout_invalid()) return;

	// Do not allow to click through tooltip.
	frame_().process_input(input, output);
}


void gui_tooltip::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	frame_().commit_render(packer, queue);

	if (label_) label_->commit_render(packer, queue);
	if (image_) image_->commit_render(packer, queue);
}


void gui_tooltip::set_text(const string& text)
{
	shared_instance<gui_label> new_label
		(new gui_label
			(name + "_tooltip_label", gui,
			 tooltip_style().label_style, text));

	insert_child(new_label);

	// NOTHROW from here.
	label_ = new_label;
	want_layout();
}

string gui_tooltip::text() const
	{ return label_ ? label_->text() : string(); }

void gui_tooltip::clear_text() NOTHROW
{
	label_.reset();
	want_layout();
}


void gui_tooltip::set_image
	(const shared_instance<render_material_id>& material,
	 const vector2i& image_size)
{
	shared_instance<gui_image> new_image
		(new gui_image(name + "_tooltip_image", gui,
					   tooltip_style().image_style,
					   material, image_size));

	insert_child(new_image);

	// NOTHROW from here.
	image_ = new_image;
	want_layout();
}

void gui_tooltip::set_image
	(const image_rgba& source_image,
	 const render_material_data& material_data)
{
	shared_instance<gui_image> new_image
		(new gui_image(name + "_tooltip_image", gui,
					   tooltip_style().image_style,
					   source_image, material_data));

	insert_child(new_image);

	// NOTHROW from here.
	image_ = new_image;
	want_layout();
}

void gui_tooltip::clear_image() NOTHROW
{
	image_.reset();
	want_layout();
}


void gui_tooltip::realize_layout()
{
	// Handle clipping of entire tooltip.
	if (layout_invalid())
	{
		frame_().clip_entirely();

		if (label_) label_->clip_entirely();
		if (image_) image_->clip_entirely();

		return;
	}


	// Provide layout for tooltip frame.
	// Note gui_tooltip_area always applies preferred_size(),
	// so we need not bound our arrange_area() in any way.
	frame_().layout(arrange_area(), clip_area());


	// Fetch content area from frame.
	// Frame already matches bound content size,
	// so no need to pass in content size again.
	rect2f content_arrange;
	rect2f content_clip;

	if (!frame_().build_content_layout
			(vector2f(), content_arrange, content_clip))
	{
		if (label_) label_->clip_entirely();
		if (image_) image_->clip_entirely();
		return;
	}


	// Position tooltip contents
	// in content arrange area.
	float position_x = content_arrange.pos.x;

	if (image_)
	{
		vector2f image_preferred
			(image_->preferred_size());

		float image_y =
			content_arrange.pos.y +
			(content_arrange.size.y - image_preferred.y) * 0.5f;

		rect2f image_arrange
			(vector2f(position_x, image_y), image_preferred);
		
		rect2f image_clip;
		if (image_arrange.try_intersect(clip_area(), image_clip))
			image_->layout(image_arrange, image_clip);
		else
			image_->clip_entirely();

		float offset_to_label =
			image_preferred.x + tooltip_style().image_label_spacing;

		position_x += offset_to_label;
	}

	if (label_)
	{
		vector2f label_preferred
			(label_->preferred_size());

		float label_y =
			content_arrange.pos.y +
			(content_arrange.size.y - label_preferred.y) * 0.5f;

		rect2f label_arrange
			(vector2f(position_x, label_y), label_preferred);

		rect2f label_clip;
		if (label_arrange.try_intersect(clip_area(), label_clip))
			label_->layout(label_arrange, label_clip);
		else
			label_->clip_entirely();
	}
}


vector2f gui_tooltip::calculate_content_size() const
{
	vector2f label_size
		(label_ ? label_->preferred_size() : vector2f());
	vector2f image_size
		(image_ ? image_->preferred_size() : vector2f());

	return vector2f
		(label_size.x + image_size.x +
				((label_ && image_) ? tooltip_style().image_label_spacing : 0),
			 max(label_size.y, image_size.y));
}




//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip_style
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip_style::gui_tooltip_style
	(const shared_instance<gui_frame_style>& frame_style,
	 const shared_instance<gui_label_style>& label_style,
	 const shared_instance<gui_image_style>& image_style,
	 float image_label_spacing)
	:
	frame_style(frame_style),
	label_style(label_style),
	image_style(image_style),
	image_label_spacing(image_label_spacing)
{
	if (image_label_spacing < 0)
	{
		LOG_ERROR("Spacing between image and label must be zero or greater.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
