/**
 *************************************************************************
 *
 * @file gui_tooltip_area.cpp
 *
 * Area with tooltips for the GUI, implementation.
 *
 ************************************************************************/


#include "gui_tooltip_area.hpp"


#include <viral_core/log.hpp>
#include <viral_core/timer.hpp>


#include "gui.hpp"
#include "gui_event.hpp"
#include "gui_tooltip.hpp"
#include "gui_tooltip_data.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip_area
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip_area::gui_tooltip_area
	(const string& name, gui_context& gui,
	 const shared_instance<gui_tooltip_area_style>& tooltip_area_style)
	:
	gui_element(name, gui),
	tooltip_area_style(tooltip_area_style),
	tooltip_timer_(new free_timer())
{ }


vector2f gui_tooltip_area::preferred_size() const
	{ return content_ ? content_->preferred_size() : vector2f(); }


void gui_tooltip_area::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	if (layout_invalid()) return;

	if (typeid(input) == typeid(gui_tick_event))
		process_tick_event();
	else if (typeid(input) == typeid(gui_mouse_event))
		process_mouse_event
			(static_cast<gui_mouse_event&>(input));

	if (tooltip_) tooltip_->process_input(input, output);
	if (content_) content_->process_input(input, output);
}


void gui_tooltip_area::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (content_) content_->commit_render(packer, queue);
	if (tooltip_) tooltip_->commit_render(packer, queue);
}


void gui_tooltip_area::register_tooltip
	(const shared_instance<gui_element>& element,
	 const string& text)
{
	auto_pointer<gui_tooltip_data> data
		(new gui_tooltip_data(text));

	tooltip_data_.insert
		(element, std::move(data));

	want_layout(); // In case the current tooltip changed.
}


void gui_tooltip_area::register_tooltip
	(const shared_instance<gui_element>& element,
	 const string& text,
	 const shared_instance<render_material_id>& material,
	 const vector2i& image_size)
{
	auto_pointer<gui_tooltip_data_material> data
		(new gui_tooltip_data_material
			(text, material, image_size));

	tooltip_data_.insert
		(element, std::move(data));

	want_layout(); // In case the current tooltip changed.
}


void gui_tooltip_area::register_tooltip
	(const shared_instance<gui_element>& element,
	 const string& text,
	 const image_rgba& source_image,
	 const render_material_data& material_data)
{
	auto_pointer<gui_tooltip_data_image> data
		(new gui_tooltip_data_image
			(text, source_image, material_data));

	tooltip_data_.insert
		(element, std::move(data));

	want_layout(); // In case the current tooltip changed.
}


void gui_tooltip_area::remove_tooltip
	(const shared_instance<gui_element>& element) NOTHROW
{
	tooltip_data_.remove(element);
	want_layout(); // In case current tooltip changed.
}


void gui_tooltip_area::purge_tooltips() NOTHROW
{
	for (tooltip_data_map::iterator it
			(tooltip_data_.first()); it;)
	{
		tooltip_data_map::iterator del_it(it);
		++it;


		// Remove any permanently deleted elements
		// from tooltip associations.
		if (!del_it->key.lock())
			tooltip_data_.remove(del_it);
	}


	// Deleting elements already caused a layout pass.
	// Thus, no need for explicit want_layout() here.
}


void gui_tooltip_area::set_content
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
	gui_tooltip_area::content() const NOTHROW
		{ return content_; }


void gui_tooltip_area::clear_content() NOTHROW
{
	if (content_)
		remove_child(content_.instance_or_terminate());

	content_.reset();
	want_layout();
}


void gui_tooltip_area::realize_layout()
{
	if (layout_invalid())
	{
		if (content_) content_->clip_entirely();
		if (tooltip_) tooltip_->clip_entirely();
	}


	// Pass on all areas to content.
	if (content_)
	{
		content_->layout
			(arrange_area(), clip_area());
	}


	// Try to fit tooltip into visible area
	// and outside the dead zone.
	if (tooltip_)
	{
		rect2f cl_area(clip_area());
		vector2f tooltip_size
			(tooltip_->preferred_size());

		vector2f dead_zone_min
			(tooltip_cursor_ + tooltip_area_style().tooltip_dead_zone.pos);
		vector2f dead_zone_max
			(dead_zone_min + tooltip_area_style().tooltip_dead_zone.size);		
	

		// Search for and apply a tooltip anchor.
		// Default to the lower-right of the cursor,
	    // notably when there is no better anchor.
		// Otherwise, priorize by index. 
		int64 use_anchor_index = 0; 

		vector2f tooltip_anchors[6] =
			{
				vector2f(dead_zone_max.x, dead_zone_max.y),
				vector2f(dead_zone_min.x, dead_zone_max.y),
				vector2f(dead_zone_min.x - tooltip_size.x, dead_zone_max.y),
				vector2f(dead_zone_max.x, dead_zone_min.y),
				vector2f(dead_zone_max.x, dead_zone_min.y - tooltip_size.y),
				vector2f(dead_zone_min.x, dead_zone_min.y) - tooltip_size
			};

		for (int64 i = 0; i < 6; ++i)
		{
			if (cl_area.contains(tooltip_anchors[i]) &&
				cl_area.contains(tooltip_anchors[i] + tooltip_size))
			{
				// Anchored tooltip is fully contained in client area.
				// Note we need not test all four corners here.
				use_anchor_index = i;
				break;
			}
		}

		tooltip_->layout
			(rect2f(tooltip_anchors[use_anchor_index],
					tooltip_->preferred_size()),
			 clip_area());
	}
}


void gui_tooltip_area::process_tick_event()
{
	update_tooltip_source();
	wait_or_show_tooltip();
	hide_tooltip_by_tolerance();
}


void gui_tooltip_area::process_mouse_event
	(gui_mouse_event &mouse_event)
{
	if (mouse_event.event != gui_mouse_event::cursor_move)
		return;

	// Track current cursor position
	// for placement of gui_tooltip.
	current_cursor_ = mouse_event.cursor;
}


void gui_tooltip_area::update_tooltip_source()
{
	/**
	 * See if mouse focus has changed,
	 * then update tooltip source.
	 */

	shared_pointer<gui_element> mouse_focus
		(gui.mouse_focus().lock());

	if (mouse_focus == tooltip_)
		return;

	if (mouse_focus == tooltip_focus_)
		return;


	// Tooltip source will change now,
	// so delete any current tooltip.
	// Exception safety: NOTHROW here.
	if (tooltip_)
	{
		remove_child(tooltip_.instance_or_terminate());
		tooltip_.reset();
		want_layout();
	}


	// Track current element with mouse focus,
	// then search for tooltip source element.
	tooltip_focus_ = mouse_focus;

	shared_pointer<gui_element> search_source(mouse_focus);
	while (search_source)
	{
		if (tooltip_data_.contains(search_source))
			break;

		gui_element* parent = search_source->parent();
		search_source =
			parent ? parent->fetch_shared_from_parent() :
					 shared_pointer<gui_element>();	
	}
		
	tooltip_source_ = search_source;


	// A new tooltip is pending for display,
	// show after timer has elapsed.
	if (tooltip_source_)
		tooltip_timer_->restart();
}


void gui_tooltip_area::wait_or_show_tooltip()
{
	/**
	 * Show tooltip from tooltip source, if any,
	 * and only after timer has elapsed.
	 */

	if (tooltip_) return;
	if (!tooltip_source_) return;

	if (tooltip_timer_->seconds_passed() <
		tooltip_area_style().tooltip_show_seconds)
			return;

	shared_instance<gui_tooltip> tooltip
		(new gui_tooltip
			(name + "_tooltip", gui,
			 tooltip_area_style().tooltip_style));

	tooltip_data_.find(tooltip_source_)->value->apply(tooltip());

	insert_child(tooltip);

	// Exception safety: NOTHROW from here.
	want_layout();
	tooltip_ = tooltip;		
	tooltip_cursor_ = current_cursor_;
}


void gui_tooltip_area::hide_tooltip_by_tolerance()
{
	if (!tooltip_) return;
	if ((tooltip_cursor_ - current_cursor_).abs().max() <
			tooltip_area_style().tooltip_show_tolerance)
				return;


	// Mouse moved from original position,
	// so delete any current tooltip.
	// Exception safety: NOTHROW here.
	remove_child(tooltip_.instance_or_terminate());
	tooltip_.reset();
	want_layout();


	// Reset timer to keep tooltip
	// from instantly reappearing.
	tooltip_timer_->restart();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_tooltip_area_style
//
//////////////////////////////////////////////////////////////////////////


gui_tooltip_area_style::gui_tooltip_area_style
	(const shared_instance<gui_tooltip_style>& tooltip_style,
	 const rect2f& tooltip_dead_zone,
	 float tooltip_show_tolerance,
	 float tooltip_show_seconds)
	:
	tooltip_style(tooltip_style),
	tooltip_dead_zone(tooltip_dead_zone),
	tooltip_show_tolerance(tooltip_show_tolerance),
	tooltip_show_seconds(tooltip_show_seconds)
{
	if (tooltip_show_tolerance < 0)
	{
		LOG_ERROR("Tooltip tolerance must be zero or greater.");
		throw invalid_parameter();
	}

	if (tooltip_show_seconds < 0)
	{
		LOG_ERROR("Tooltip must not show after negative seconds.");
		throw invalid_parameter();
	}
}




} /* namespace viral_gui */
