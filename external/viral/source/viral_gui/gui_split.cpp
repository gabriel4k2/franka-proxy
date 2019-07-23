/**
 *************************************************************************
 *
 * @file gui_split.cpp
 *
 * GUI splitter panes, implementation.
 *
 ************************************************************************/


#include "gui_split.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_knob.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_split
//
//////////////////////////////////////////////////////////////////////////


gui_split::gui_split
	(const string& name, gui_context& gui,
	 const shared_instance<gui_split_style>& split_style,
	 orientation_type orientation)
	:
	gui_element(name, gui),
	split_style(split_style),
	orientation(orientation),
	splitter_(new gui_knob(name + "_frame", gui,
						   split_style().knob_style,
						   orientation == horizontal ? vertical : horizontal))
{
	insert_child(splitter_);
	splitter_().set_knob_size(split_style().splitter_size);
}


vector2f gui_split::preferred_size() const
{
	// Exploit maximum required space of either
	// splitter or content in split direction
	// and sum minimum allowed sizes against
	// split direction.
	vector2f min_pane_size
		(split_style().min_pane_size, split_style().min_pane_size);

	vector2f neg_content_preferred
		(neg_content_ ? neg_content_->preferred_size() : vector2f());
	vector2f pos_content_preferred
		(pos_content_ ? pos_content_->preferred_size() : vector2f());

	vector2f neg_minimum_size
		(split_style().ensure_content_size_neg ?
		 neg_content_preferred.max(min_pane_size) : min_pane_size);
	vector2f pos_minimum_size
		(split_style().ensure_content_size_pos ?
		 pos_content_preferred.max(min_pane_size) : min_pane_size);

	vector2f max_content_size
		(neg_content_preferred.max(pos_content_preferred));

	vector2f splitter_size
		(vector2f(split_style().splitter_size, split_style().splitter_size).
			max(splitter_().preferred_size()));

	return
		orientation == horizontal ?
			vector2f(max(splitter_size.x, max_content_size.x),
					 neg_minimum_size.y + pos_minimum_size.y +
					 splitter_size.y) :
			vector2f(neg_minimum_size.x + pos_minimum_size.x +
					 splitter_size.x,
					 max(splitter_size.y, max_content_size.y));
}


void gui_split::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// See if user moved splitter.
	// If so, synchronize and report split event.
	gui_output_event_list splitter_output;
	splitter_().process_input(input, splitter_output);

	if (!splitter_output.empty() &&
		typeid(**splitter_output.first()) == typeid(gui_knob_event))
	{
		synchronize_links();

		auto_pointer<gui_output_event> event
			(new gui_split_event(*this));
		output.push_back
			(std::move(event));
	}


	// Pass on events to content.
	if (neg_content_) neg_content_->process_input(input, output);
	if (pos_content_) pos_content_->process_input(input, output);
}


void gui_split::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (neg_content_) neg_content_->commit_render(packer, queue);
	if (pos_content_) pos_content_->commit_render(packer, queue);

	splitter_().commit_render(packer, queue);
}


void gui_split::set_read_only(bool read_only) NOTHROW
	{ splitter_().set_read_only(read_only); }
bool gui_split::read_only() const NOTHROW
	{ return splitter_().read_only(); }


void gui_split::set_split_normalized(float split_normalized)
{
	splitter_().set_position_normalized(split_normalized);
	synchronize_links();
	want_layout();
}

float gui_split::split_normalized() const
	{ return splitter_().position_normalized(); }


void gui_split::set_pos_content
	(const shared_pointer<gui_element>& pos_content)
{
	if (pos_content)
		insert_child(pos_content.instance_or_throw());

	// Exception safety: NOTHROW from here.
	if (pos_content_)
		remove_child(pos_content_.instance_or_terminate());

	pos_content_ = pos_content;
	want_layout();
}

const shared_pointer<gui_element>&
	gui_split::pos_content() const NOTHROW
		{ return pos_content_; }

void gui_split::clear_pos_content() NOTHROW
{
	if (pos_content_)
		remove_child(pos_content_.instance_or_terminate());

	pos_content_.reset();
	want_layout();
}


void gui_split::set_neg_content
	(const shared_pointer<gui_element>& neg_content)
{
	if (neg_content)
		insert_child(neg_content.instance_or_throw());

	// Exception safety: NOTHROW from here.
	if (neg_content_)
		remove_child(neg_content_.instance_or_terminate());

	neg_content_ = neg_content;
	want_layout();
}

const shared_pointer<gui_element>&
	gui_split::neg_content() const NOTHROW
		{ return neg_content_; }

void gui_split::clear_neg_content() NOTHROW
{
	if (neg_content_)
		remove_child(neg_content_.instance_or_terminate());

	neg_content_.reset();
	want_layout();
}


void gui_split::set_links(const link_list& links)
{
	if (links.contains(weak_pointer<gui_split>()))
	{
		LOG_ERROR("Cannot link with NULL split.");
		throw null_pointer();
	}

	links_ = links;
}

const gui_split::link_list& gui_split::links() const
	{ return links_; }


void gui_split::realize_layout()
{
	if (layout_invalid())
	{
		if (neg_content_) neg_content_->clip_entirely();
		if (pos_content_) pos_content_->clip_entirely();
		splitter_().clip_entirely();
		return;
	}


	// Fetch layout parameters for splitter and content.
	rect2f ar_area(arrange_area());
	rect2f cl_area(clip_area());
	ar_area.size =
		ar_area.size.max(preferred_size());

	vector2f min_pane_size_neg
		(split_style().min_pane_size, split_style().min_pane_size);
	vector2f min_pane_size_pos
		(split_style().min_pane_size, split_style().min_pane_size);

	if (neg_content_ && split_style().ensure_content_size_neg)
		min_pane_size_neg =
			min_pane_size_neg.max(neg_content_->preferred_size());

	if (pos_content_ && split_style().ensure_content_size_pos)
		min_pane_size_pos =
			min_pane_size_pos.max(pos_content_->preferred_size());


	// Build layout for splitter movement area.
	// In particular, splitter moves up to min_pane_size
	// on either end, and exceeds the arrange_area() if
	// it doesn't accomodate a splitter and minimum panes.
	vector2f splitter_arrange_start
		(ar_area.pos + min_pane_size_neg);
	vector2f splitter_arrange_size
		(ar_area.size - min_pane_size_neg - min_pane_size_pos);

	rect2f splitter_arrange
		(orientation == horizontal ?
			rect2f(ar_area.pos.x, splitter_arrange_start.y,
				   ar_area.size.x, splitter_arrange_size.y) :
			rect2f(splitter_arrange_start.x, ar_area.pos.y,
				   splitter_arrange_size.x, ar_area.size.y));
		
	rect2f splitter_clip;
	if (splitter_arrange.try_intersect(cl_area, splitter_clip))
		splitter_().layout(splitter_arrange, splitter_clip);
	else
		splitter_().clip_entirely();


	// Build layout up to splitter position
	// for positive and negative pane.
	// Store relevant values either in x or in y,
	// depending on current orientation, to avoid
	// extensive formulas later on.
	vector2f splitter_size
		(split_style().splitter_size, split_style().splitter_size);

	if (neg_content_)
	{
		vector2f neg_content_size
			(min_pane_size_neg +
				(splitter_arrange_size - splitter_size) *
					splitter_().position_normalized());

		rect2f neg_content_arrange
			(orientation == horizontal ?
				rect2f(ar_area.pos.x, ar_area.pos.y,
					   ar_area.size.x, neg_content_size.y) :
				rect2f(ar_area.pos.x, ar_area.pos.y,
					   neg_content_size.x, ar_area.size.y));

		rect2f neg_content_clip;
		if (neg_content_arrange.try_intersect(cl_area, neg_content_clip))
			neg_content_->layout(neg_content_arrange, neg_content_clip);
		else
			neg_content_->clip_entirely();
	}

	if (pos_content_)
	{
		vector2f pos_content_offset
			(min_pane_size_neg + splitter_size +
				(splitter_arrange_size - splitter_size) *
					splitter_().position_normalized());

		vector2f pos_content_pos
			(ar_area.pos + pos_content_offset);
		vector2f pos_content_size
			(ar_area.size - pos_content_offset);

		rect2f pos_content_arrange
			(orientation == horizontal ?
				rect2f(ar_area.pos.x, pos_content_pos.y,
					   ar_area.size.x, pos_content_size.y) :
				rect2f(pos_content_pos.x, ar_area.pos.y,
					   pos_content_size.x, ar_area.size.y));

		rect2f pos_content_clip;
		if (pos_content_arrange.try_intersect(cl_area, pos_content_clip))
			pos_content_->layout(pos_content_arrange, pos_content_clip);
		else
			pos_content_->clip_entirely();
	}
}


void gui_split::synchronize_links()
{
	for (link_list::const_iterator it(links_.first()); it; ++it)
	{
		shared_pointer<gui_split> shared(it->lock());

		if (!shared) continue;
		if (shared.get() == this) continue;

		// Do not use set_split_normalized() here
		// to avoid infinite recursion.
		shared->splitter_().set_position_normalized
			(splitter_().position_normalized());
	}
}




//////////////////////////////////////////////////////////////////////////
//
// gui_split_style
//
//////////////////////////////////////////////////////////////////////////


gui_split_style::gui_split_style
	(const shared_instance<gui_knob_style>& knob_style,
	 float splitter_size,
	 float min_pane_size,
	 bool ensure_content_size_neg,
	 bool ensure_content_size_pos)
	:
	knob_style(knob_style),
	splitter_size(splitter_size),
	min_pane_size(min_pane_size),
	ensure_content_size_neg(ensure_content_size_neg),
	ensure_content_size_pos(ensure_content_size_pos)
{
	if (knob_style().min_drag_range != 0)
	{
		LOG_ERROR("Knob style of a splitter must have a zero minimum dragging range.");
		throw invalid_parameter();
	}

	if (splitter_size < 0)
	{
		LOG_ERROR("Splitter size must not be negative.");
		throw invalid_parameter();
	}

	if (min_pane_size < 0)
	{
		LOG_ERROR("Minimum split pane size must not be negative.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
