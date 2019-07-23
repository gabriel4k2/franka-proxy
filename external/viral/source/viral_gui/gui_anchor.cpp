/**
 *************************************************************************
 *
 * @file gui_anchor.cpp
 *
 * Anchoring of GUI elements, implementation.
 *
 ************************************************************************/


#include "gui_anchor.hpp"


#include <viral_core/log.hpp>


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_anchor
//
//////////////////////////////////////////////////////////////////////////


gui_anchor::gui_anchor
	(const string& name, gui_context& gui,
	 const shared_instance<gui_anchor_style>& anchor_style)
	:
	gui_element(name, gui),
	anchor_style(anchor_style)
{ }


vector2f gui_anchor::preferred_size() const
	{ return content_ ? content_->preferred_size() : vector2f(); }


void gui_anchor::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (content_)
		content_->process_input(input, output);
}


void gui_anchor::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (content_)
		content_->commit_render(packer, queue);
}


void gui_anchor::set_content
	(const shared_pointer<gui_element>& content)
{
	if (content)
		insert_child(content.instance_or_throw());

	// Exception safety: NOTHROW from here.
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_ = content;
	want_layout();
}

const shared_pointer<gui_element>&
	gui_anchor::content() const NOTHROW
		{ return content_; }

void gui_anchor::clear_content() NOTHROW
{
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_.reset();
	want_layout();
}


rect2f gui_anchor::build_layout
	(const vector2f& content_size,
	 const rect2f& arrange_area,
	 const gui_anchor_style& anchor_style)
{
	return build_layout
				(content_size, arrange_area,
				 anchor_style.horizontal_anchor_underflow,
				 anchor_style.vertical_anchor_underflow,
				 anchor_style.horizontal_anchor_overflow,
				 anchor_style.vertical_anchor_overflow);
}


rect2f gui_anchor::build_layout
	(const vector2f& content_size,
	 const rect2f& arrange_area,
	 anchor_type horizontal_anchor,
	 anchor_type vertical_anchor) NOTHROW
{
	return build_layout
				(content_size, arrange_area,
				 horizontal_anchor, vertical_anchor,
				 horizontal_anchor, vertical_anchor);
}


rect2f gui_anchor::build_layout
	(const vector2f& content_size,
	 const rect2f& arrange_area,
	 anchor_type horizontal_anchor_underflow,
	 anchor_type vertical_anchor_underflow,
	 anchor_type horizontal_anchor_overflow,
	 anchor_type vertical_anchor_overflow) NOTHROW
{
	rect2f result;

	build_layout(content_size.x,
				 arrange_area.pos.x, arrange_area.size.x,
				 result.pos.x, result.size.x,
				 horizontal_anchor_underflow, horizontal_anchor_overflow);

	build_layout(content_size.y,
				 arrange_area.pos.y, arrange_area.size.y,
				 result.pos.y, result.size.y,
				 vertical_anchor_underflow, vertical_anchor_overflow);

	return result;
}


void gui_anchor::build_layout
	(float content_size,
	 float arrange_area_pos, float arrange_area_size,
	 float& result_pos, float& result_size,
	 anchor_type anchor) NOTHROW
{
	if (anchor == anchor_fill)
	{
		result_pos = arrange_area_pos;
		result_size = arrange_area_size;
	}
	else if (anchor == anchor_neg)
	{
		result_pos = arrange_area_pos;
		result_size = content_size;
	}
	else if (anchor == anchor_center)
	{
		result_pos = arrange_area_pos + (arrange_area_size - content_size) * 0.5f;
		result_size = content_size;
	}
	else if (anchor == anchor_pos)
	{
		result_pos = arrange_area_pos + arrange_area_size - content_size;
		result_size = content_size;
	}
	else
	{
		// NOTHROW, so just default-fill.
		LOG_WARN("Unknown anchor type.");
		result_pos = arrange_area_pos;
		result_size = arrange_area_size;
	}
}


void gui_anchor::build_layout
	(float content_size,
	 float arrange_area_pos, float arrange_area_size,
	 float& result_pos, float& result_size,
	 anchor_type anchor_underflow,
	 anchor_type anchor_overflow) NOTHROW
{
	gui_anchor::build_layout
		(content_size,
		 arrange_area_pos, arrange_area_size,
		 result_pos, result_size,
		 arrange_area_size > content_size ? anchor_underflow : anchor_overflow);
}


void gui_anchor::realize_layout()
{
	if (!content_) return;

	if (layout_invalid())
	{
		content_->clip_entirely();
		return;
	}

	vector2f size(preferred_size());
	rect2f ar_area(arrange_area());

	rect2f content_arrange
		(build_layout
			(size, ar_area,
			 anchor_style().horizontal_anchor_underflow,
			 anchor_style().vertical_anchor_underflow,
			 anchor_style().horizontal_anchor_overflow,
			 anchor_style().vertical_anchor_overflow));

	rect2f content_clip;
	if (content_arrange.size.min() > 0 &&
		content_arrange.try_intersect(clip_area(), content_clip))
	{
		content_->layout(content_arrange, content_clip);
	}
	else
		content_->clip_entirely();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_anchor_style
//
//////////////////////////////////////////////////////////////////////////


gui_anchor_style::gui_anchor_style
	(gui_anchor::anchor_type horizontal_anchor_underflow,
	 gui_anchor::anchor_type vertical_anchor_underflow,
	 gui_anchor::anchor_type horizontal_anchor_overflow,
	 gui_anchor::anchor_type vertical_anchor_overflow)
	:
	horizontal_anchor_underflow(horizontal_anchor_underflow),
	vertical_anchor_underflow(vertical_anchor_underflow),
	horizontal_anchor_overflow(horizontal_anchor_overflow),
	vertical_anchor_overflow(vertical_anchor_overflow)
{ }




} /* namespace viral_gui */
