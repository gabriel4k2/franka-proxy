/**
 *************************************************************************
 *
 * @file gui_scroll_area.cpp
 *
 * Scrolling areas, implementation.
 *
 ************************************************************************/


#include "gui_scroll_area.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_scrollbar.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_scroll_area
//
//////////////////////////////////////////////////////////////////////////


gui_scroll_area::gui_scroll_area
	(const string& name, gui_context& gui,
	 const shared_instance<gui_scroll_area_style>& scroll_area_style)
	:
	gui_element(name, gui),
	scroll_area_style(scroll_area_style),

	main_area_valid_(false),

	scrollbar_horizontal_(new gui_scrollbar
		(name + "_scroll_h", gui,
		 scroll_area_style().scrollbar_style, horizontal)),
	scrollbar_vertical_(new gui_scrollbar
		(name + "_scroll_v", gui,
		 scroll_area_style().scrollbar_style, vertical))
{
	insert_child(scrollbar_horizontal_);
	insert_child(scrollbar_vertical_);

	scrollbar_horizontal_().set_range(0, 0);
	scrollbar_horizontal_().set_page_size(0);
	scrollbar_vertical_().set_range(0, 0);
	scrollbar_vertical_().set_page_size(0);
}


vector2f gui_scroll_area::preferred_size() const
{
	// Preferred size includes size for content and
	// for scrollbars in current arrangement mode.
	// Note scrollbars add up in default arrangement,
	// and overlap in compact arrangement.

	vector2f content_size
		(scroll_area_style().prefer_override_size ?
			scroll_area_style().override_size :
		 content_ ? content_->preferred_size() : vector2f());

	vector2f scrollbar_h_size
		(scroll_area_style().scrollbar_show_horizontal != show_never ?
			 scrollbar_horizontal_().preferred_size() : vector2f());
	vector2f scrollbar_v_size
		(scroll_area_style().scrollbar_show_vertical != show_never ?
			 scrollbar_vertical_().preferred_size() : vector2f());

	return
		content_size.max
			(scroll_area_style().scrollbar_arrange == arrange_compact ?
				scrollbar_h_size.max(scrollbar_v_size) :
				(scrollbar_h_size + scrollbar_v_size));
}


void gui_scroll_area::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// Send on events to children scrollbar elements.
	// Consume any scrolling notifications produced by children,
	// then rebuild layout and notify client via @bref{output}.
	gui_output_event_list child_output;
	scrollbar_horizontal_().process_input(input, child_output);
	scrollbar_vertical_().process_input(input, child_output);

	if (!child_output.empty())
	{
		want_layout();

		auto_pointer<gui_output_event> event
			(new gui_scroll_area_event(*this));
		output.push_back
			(std::move(event));		
	}


	// Always let content know of any passing events,
	// even if not hot anymore.
	if (content_)
		content_->process_input(input, output);
}


void gui_scroll_area::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	scrollbar_horizontal_().commit_render(packer, queue);
	scrollbar_vertical_().commit_render(packer, queue);

	if (content_)
		content_->commit_render(packer, queue);
}


vector2f gui_scroll_area::range_min() const NOTHROW
	{ return vector2f(scrollbar_horizontal_().range_min(),
					  scrollbar_vertical_().range_min()); }
vector2f gui_scroll_area::range_max() const NOTHROW
	{ return vector2f(scrollbar_horizontal_().range_max(),
					  scrollbar_vertical_().range_max()); }


void gui_scroll_area::set_item_size(const vector2f& item_size)
{
	scrollbar_horizontal_().set_item_size(item_size.x);
	scrollbar_vertical_().set_item_size(item_size.y);
}

vector2f gui_scroll_area::item_size() const NOTHROW
{
	return vector2f(scrollbar_horizontal_().item_size(),
					scrollbar_vertical_().item_size());
}


void gui_scroll_area::set_overlap_factor(const vector2f& overlap_factor)
{
	scrollbar_horizontal_().set_overlap_factor(overlap_factor.x);
	scrollbar_vertical_().set_overlap_factor(overlap_factor.y);
}

vector2f gui_scroll_area::overlap_factor() const NOTHROW
{
	return vector2f(scrollbar_horizontal_().overlap_factor(),
					scrollbar_vertical_().overlap_factor());
}


void gui_scroll_area::set_continuous_increment
	(const vector2f& continuous_increment)
{
	scrollbar_horizontal_().set_continuous_increment(continuous_increment.x);
	scrollbar_vertical_().set_continuous_increment(continuous_increment.y);
}

vector2f gui_scroll_area::continuous_increment() const NOTHROW
{
	return vector2f(scrollbar_horizontal_().continuous_increment(),
					scrollbar_vertical_().continuous_increment());
}


void gui_scroll_area::set_position(const vector2f& position) NOTHROW
{
	scrollbar_horizontal_().set_position(position.x);
	scrollbar_vertical_().set_position(position.y);
	want_layout();
}

vector2f gui_scroll_area::position() const NOTHROW
{
	return vector2f(scrollbar_horizontal_().position(),
					scrollbar_vertical_().position());
}


void gui_scroll_area::set_position_normalized(const vector2f& position) NOTHROW
{
	scrollbar_horizontal_().set_position_normalized(position.x);
	scrollbar_vertical_().set_position_normalized(position.y);
	want_layout();
}

vector2f gui_scroll_area::position_normalized() const NOTHROW
{
	return vector2f
		(scrollbar_horizontal_().position_normalized(),
		 scrollbar_vertical_().position_normalized());
}


void gui_scroll_area::scroll_page_to(const vector2f& point) NOTHROW
{
	scrollbar_horizontal_().scroll_page_to(point.x);
	scrollbar_vertical_().scroll_page_to(point.y);
	want_layout();
}

void gui_scroll_area::scroll_page_to(const rect2f& rect) NOTHROW
{
	scrollbar_horizontal_().scroll_page_to(rect.pos.x, rect.end_x());
	scrollbar_vertical_().scroll_page_to(rect.pos.y, rect.end_y());
	want_layout();
}


bool gui_scroll_area::is_visible_on_page(const vector2f& point) const NOTHROW
{
	if (layout_invalid()) return false;

	return scrollbar_horizontal_().is_visible_on_page(point.x) &&
		   scrollbar_vertical_().is_visible_on_page(point.y);
}


vector2f gui_scroll_area::to_scroll_space(const vector2f& gui_space) const NOTHROW
{
	if (layout_invalid() || !main_area_valid_) return vector2f();
	return gui_space + position() - main_area_.pos;
}

vector2f gui_scroll_area::to_gui_space(const vector2f& scroll_space) const NOTHROW
{
	if (layout_invalid() || !main_area_valid_) return vector2f();
	return scroll_space - position() + main_area_.pos;
}


void gui_scroll_area::set_content
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
	gui_scroll_area::content() const NOTHROW
		{ return content_; }

void gui_scroll_area::clear_content() NOTHROW
{
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_.reset();
	want_layout();
}


void gui_scroll_area::realize_layout()
{
	// Clip entire scrolling area if parent
	// could not provide us with layout areas.
	if (layout_invalid())
	{
		main_area_valid_ = false;

		scrollbar_horizontal_().set_range(0, 0);
		scrollbar_horizontal_().set_page_size(0);
		scrollbar_horizontal_().clip_entirely();

		scrollbar_vertical_().set_range(0, 0);
		scrollbar_vertical_().set_page_size(0);
		scrollbar_vertical_().clip_entirely();

		if (content_) content_->clip_entirely();
		return;
	}


	// Build main_area_ within our arrange_area()
	// that displays the scrolled content.
	// We require enough space for two scrollbars,
	// and clip on smaller arrangement areas.
	main_area_ = arrange_area();
	main_area_valid_ = true;
	
	vector2f scrollbar_h_size
		(scroll_area_style().scrollbar_show_horizontal != show_never ?
		 scrollbar_horizontal_().preferred_size() : vector2f());
	vector2f scrollbar_v_size
		(scroll_area_style().scrollbar_show_vertical != show_never ?
		 scrollbar_vertical_().preferred_size() : vector2f());

	main_area_.size =
		main_area_.size.max
			(scroll_area_style().scrollbar_arrange == arrange_compact ?
				scrollbar_h_size.max(scrollbar_v_size) :
				(scrollbar_h_size + scrollbar_v_size));


	// Determine whether scrollbars should be shown with
	// available main_area, actual content size, and
	// current scrollbar behaviour.
	vector2f content_size
		(content_ ? content_->preferred_size() : vector2f());

	bool need_scrollbar_h =
		scroll_area_style().scrollbar_show_horizontal != show_never &&
		(scroll_area_style().scrollbar_show_horizontal == show_always ||
		 main_area_.size.x < content_size.x);
	bool need_scrollbar_v =
		scroll_area_style().scrollbar_show_vertical != show_never &&
		(scroll_area_style().scrollbar_show_vertical == show_always ||
		 main_area_.size.y < content_size.y);


	// Adapt main area to leave space for scrollbars.
	// For default mode, showing one scrollbar reduces
	// available main area in the other direction,
	// which may require showing the other scrollbar as well.
	float button_size =
		scroll_area_style().scrollbar_style().button_size;

	if (scroll_area_style().scrollbar_arrange == arrange_default)
	{	
		if (need_scrollbar_h &&
			scroll_area_style().scrollbar_show_vertical == show_if_required)
				need_scrollbar_v |=
					main_area_.size.y < content_size.y + scrollbar_h_size.y;

		if (need_scrollbar_v &&
			scroll_area_style().scrollbar_show_horizontal == show_if_required)
				need_scrollbar_h |=
					main_area_.size.x < content_size.x + scrollbar_v_size.x;

		if (need_scrollbar_h)
			main_area_.size.y -= scrollbar_h_size.y;

		if (need_scrollbar_v)
			main_area_.size.x -= scrollbar_v_size.x;
	}
	else if (scroll_area_style().scrollbar_arrange == arrange_compact)
	{
		if (need_scrollbar_h)
		{
			main_area_.pos.x += button_size;
			main_area_.size.x -= button_size * 2;
		}

		if (need_scrollbar_v)
		{
			main_area_.pos.y += button_size;
			main_area_.size.y -= button_size * 2;
		}
	}


	// Apply ranges and page sizes to scrollbars.
	// We need these regardless of scrollbar show behaviour
	// for alternative scrolling (e.g. via scroll_page_to()).
	// If the entire content is visible, use empty ranges.
	scrollbar_horizontal_().set_range
		(0.f, max(0.f, content_size.x - main_area_.size.x));
	scrollbar_horizontal_().set_page_size
		(max(0.f, main_area_.size.x));

	scrollbar_vertical_().set_range
		(0.f, max(0.f, content_size.y - main_area_.size.y));
	scrollbar_vertical_().set_page_size
		 (max(0.f, main_area_.size.y));


	// Apply arrangement and clipping areas to scrollbars
	// if these are shown, otherwise clip scrollbars entirely.
	// Note that clipped scrollbars still work reliably
	// over the previously assigned ranges.
	rect2f scrollbar_h_area;
	if (need_scrollbar_h)
	{
		scrollbar_h_area =
			scroll_area_style().scrollbar_arrange == arrange_compact ?
			rect2f(vector2f(arrange_area().pos.x, main_area_.pos.y),
				   vector2f(arrange_area().size.x, main_area_.size.y)) :
			rect2f(main_area_.pos + vector2f(0, main_area_.size.y),
				   vector2f(main_area_.size.x, scrollbar_h_size.y));
	}

	rect2f scrollbar_h_clip;
	if (need_scrollbar_h &&
		scrollbar_h_area.try_intersect(clip_area(), scrollbar_h_clip))
	{
		scrollbar_horizontal_().layout
			(scrollbar_h_area, scrollbar_h_clip);
	}
	else
		scrollbar_horizontal_().clip_entirely();

	rect2f scrollbar_v_area;
	if (need_scrollbar_v)
	{
		scrollbar_v_area =
			scroll_area_style().scrollbar_arrange == arrange_compact ?
			rect2f(vector2f(main_area_.pos.x, arrange_area().pos.y),
				   vector2f(main_area_.size.x, arrange_area().size.y)) :
			rect2f(main_area_.pos + vector2f(main_area_.size.x, 0),
				   vector2f(scrollbar_v_size.x, main_area_.size.y));
	}

	rect2f scrollbar_v_clip;
	if (need_scrollbar_v &&
		scrollbar_v_area.try_intersect(clip_area(), scrollbar_v_clip))
	{
		scrollbar_vertical_().layout
			(scrollbar_v_area, scrollbar_v_clip);
	}
	else
		scrollbar_vertical_().clip_entirely();


	// Scroll, arrange, and clip content.
	// Notably, provide content with at least content-sized,
	// but offset arrangement area, and any additional space.
	// Then clip content results to the main and clip areas.
	if (!content_) return;

	vector2f content_offset
		(scrollbar_horizontal_().position(),
		 scrollbar_vertical_().position());

	rect2f content_arrange
		(main_area_.pos - content_offset, content_size);

	if (content_arrange.size.x < main_area_.size.x)
		content_arrange.size.x = main_area_.size.x;

	if (content_arrange.size.y < main_area_.size.y)
		content_arrange.size.y = main_area_.size.y;

	rect2f main_area_clip;
	rect2f content_clip;
	if (main_area_.try_intersect(clip_area(), main_area_clip) &&
		content_arrange.try_intersect(main_area_clip, content_clip))
	{
		content_->layout(content_arrange, content_clip);
	}
	else
		content_->clip_entirely();
}


const float gui_scroll_area::range_epsilon_ = 0.001f;




//////////////////////////////////////////////////////////////////////////
//
// gui_scroll_area_style
//
//////////////////////////////////////////////////////////////////////////


gui_scroll_area_style::gui_scroll_area_style
	(const shared_instance<gui_scrollbar_style>& scrollbar_style,
	 bool prefer_override_size,
	 const vector2f& override_size,
	 gui_scroll_area::scrollbar_show_type scrollbar_show_horizontal,
	 gui_scroll_area::scrollbar_show_type scrollbar_show_vertical,
	 gui_scroll_area::scrollbar_arrange_type scrollbar_arrange)
	:
	scrollbar_style(scrollbar_style),
	prefer_override_size(prefer_override_size),
	override_size(override_size),
	scrollbar_show_horizontal(scrollbar_show_horizontal),
	scrollbar_show_vertical(scrollbar_show_vertical),
	scrollbar_arrange(scrollbar_arrange)
{
	if (scrollbar_arrange == gui_scroll_area::arrange_compact &&
		!scrollbar_style().buttons_only)
	{
		LOG_ERROR("Compact scroll areas must use button-only scrollbars.")
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */