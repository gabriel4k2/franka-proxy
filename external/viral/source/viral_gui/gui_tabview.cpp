/**
 *************************************************************************
 *
 * @file gui_tabview.cpp
 *
 * GUI tabview element, implementation.
 *
 ************************************************************************/


#include "gui_tabview.hpp"


#include <viral_core/log.hpp>


#include "gui_event.hpp"
#include "gui_split.hpp"
#include "gui_scroll_area.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_tabview
//
//////////////////////////////////////////////////////////////////////////


gui_tabview::gui_tabview
	(const string& name, gui_context& gui,
	 const shared_instance<gui_tabview_style>& tabview_style,
	 orientation_type orientation)
	:
	gui_element(name, gui),
	tabview_style(tabview_style),
	orientation(orientation),
	selected_page_(0),

	header_page_split_
		(new gui_split
			(name + "_split", gui,
			 tabview_style().split_style,
			 orientation)),

	header_scroll_
		(new gui_scroll_area
			(name + "_scroll", gui,	tabview_style().header_scroll_area_style)),

	header_list_
		(new gui_list
			(name + "_list", gui, tabview_style().header_list_style,
			 orientation, gui_list::select_single)),

	tabview_switch_
		(new gui_tabview_switch
			(name + "_tabview_switch", gui))
{
	header_scroll_().set_content(header_list_);

	insert_child(header_page_split_);
	header_page_split_().set_neg_content(header_scroll_);
	header_page_split_().set_pos_content(tabview_switch_);
}


vector2f gui_tabview::preferred_size() const
	{ return header_page_split_().preferred_size(); }


void gui_tabview::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	// See if any header button was clicked.
	// If so, change visible tabview page.
	// We don't expose header events to clients.
	gui_output_event_list header_output;
	header_page_split_().process_input(input, header_output);

	for (gui_output_event_list::iterator it
			(header_output.first()); it; ++it)
	{
		if (!*it) continue;
		if (typeid(**it) != typeid(gui_list_event)) continue;
		if ((*it)->source_untyped != header_list_) continue;
		show_page(header_list_().first_selected_index());
		break;
	}


	// Let current page separately handle input,
	// and use client-side output event list now.
	if (tabview_switch_().current_page_)
		tabview_switch_().current_page_->process_input(input, output);
}


void gui_tabview::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;
	header_page_split_().commit_render(packer, queue);
}


void gui_tabview::realize_layout()
{
	if (layout_invalid())
	{
		header_page_split_().clip_entirely();
		return;
	}

	header_page_split_().layout
		(arrange_area(), clip_area());
}


int64 gui_tabview::insert_page
	(const string& caption,
	 const shared_instance<gui_element>& page)
{
	return insert_page
		(caption, shared_pointer<render_material_id>(), page);
}


int64 gui_tabview::insert_page
	(const string& caption,
	 const shared_pointer<render_material_id>& icon,
	 const shared_instance<gui_element>& page)
{
	// Tabview switch takes ownership over page.
	// This avoids duplicate pages or accidential
	// page reuse as content in other elements.
	tabview_switch_().insert_page(page);
	SCOPE_GUARD
		(switch_guard,
			(tabview_switch_(), &gui_tabview_switch::remove_page, page));

	header_list_().items.push_back
		(auto_pointer<gui_tabview_header>
			(new gui_tabview_header(caption, icon, page)));

	header_list_().synchronize_item_indices(); // NOTHROW

	switch_guard.dismiss();

	return header_list_().items.size() - 1;
}


int64 gui_tabview::page_count() const NOTHROW
	{ return header_list_().items.size(); }
int64 gui_tabview::selected_page() const NOTHROW
	{ return selected_page_; }


shared_pointer<gui_element> gui_tabview::page(int64 index) const NOTHROW
{
	if (index < 0 || index >= header_list_().items.size())
		return shared_pointer<gui_element>(); // NOTHROW

	if (!header_list_().items[index])
		return shared_pointer<gui_element>(); // NOTHROW

	if (typeid(*header_list_().items[index]) != typeid(gui_tabview_header))
		return shared_pointer<gui_element>(); // NOTHROW

	gui_tabview_header& header =
		static_cast<gui_tabview_header&>(*header_list_().items[index]);

	return header.page;
}


void gui_tabview::show_page(int64 index) NOTHROW
{
	if (index < 0 || index >= header_list_().items.size())
	{
		LOG_ERROR("Cannot show page with invalid index: " + index);
		return; // NOTHROW
	}

	shared_pointer<gui_element> page_to_show(page(index));

	selected_page_ = index;

	header_list_().select_index(selected_page_);
	tabview_switch_().select_page(page_to_show);

	if (page_to_show)
		page_to_show->want_layout();
}


void gui_tabview::remove_page(int64 index) NOTHROW
{
	if (index < 0 || index >= header_list_().items.size())
	{
		LOG_ERROR("Cannot remove page with invalid index: " + index);
		return; // NOTHROW
	}

	shared_pointer<gui_element> page_to_remove(page(index));

	header_list_().items.remove_ordered(index);
	header_list_().synchronize_item_indices();

	tabview_switch_().remove_page(page_to_remove);


	// If current page was removed,
	// try to switch to another valid page.
	int64 select_valid_page =
		min(selected_page_, header_list_().items.size());

	if (select_valid_page >= 0 &&
		select_valid_page < header_list_().items.size())
			show_page(select_valid_page);
}


void gui_tabview::enable_page(int64 index) NOTHROW
{
	if (index < 0 || index >= header_list_().items.size())
	{
		LOG_ERROR("Cannot remove page with invalid index: " + index);
		return; // NOTHROW
	}

	if (!header_list_().items[index])
	{
		LOG_ERROR("Page list contains NULL page.");
		return; // NOTHROW
	}

	if (header_list_().items[index]->read_only)
	{
		header_list_().items[index]->read_only = false;
		header_list_().want_layout();
	}
}


void gui_tabview::disable_page(int64 index) NOTHROW
{
	if (index < 0 || index >= header_list_().items.size())
	{
		LOG_ERROR("Cannot remove page with invalid index: " + index);
		return; // NOTHROW
	}

	if (!header_list_().items[index])
	{
		LOG_ERROR("Page list contains NULL page.");
		return; // NOTHROW
	}

	if (!header_list_().items[index]->read_only)
	{
		header_list_().items[index]->read_only = true;
		header_list_().want_layout();
	}
}


void gui_tabview::set_split_read_only(bool split_read_only)
	{ header_page_split_().set_read_only(split_read_only); }
bool gui_tabview::split_read_only() const
	{ return header_page_split_().read_only(); }

void gui_tabview::set_split_normalized(float split_normalized)
	{ header_page_split_().set_split_normalized(split_normalized); }
float gui_tabview::split_normalized() const
	{ return header_page_split_().split_normalized(); }




//////////////////////////////////////////////////////////////////////////
//
// gui_tabview_switch
//
//////////////////////////////////////////////////////////////////////////


vector2f gui_tabview_switch::preferred_size() const
{
	vector2f max_pagesize;

	for (page_set::const_iterator it(pages_.first()); it; ++it)
		max_pagesize = max_pagesize.max((*it)().preferred_size());

	return max_pagesize;
}


void gui_tabview_switch::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	/**
	 * Our @bref{output} event list is a temporary list
	 * for capturing header button output, so we cannot
	 * handle output of the @bref{current_page_} here.
	 * Instead, let gui_tabview::process_input() do this.
	 */
}


void gui_tabview_switch::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	if (layout_invalid()) return;

	if (current_page_)
		current_page_->commit_render(packer, queue);
}


gui_tabview_switch::gui_tabview_switch
	(const string& name, gui_context& gui)
	:
	gui_element(name, gui)
{ }


void gui_tabview_switch::realize_layout()
{
	// Always clip all unselected pages.
	for (page_set::const_iterator it(pages_.first()); it; ++it)
	{
		if (*it == current_page_) continue;
		(*it)().clip_entirely();
	}


	// Layout or clip selected page.
	if (current_page_)
	{
		if (layout_invalid())
			current_page_->clip_entirely();
		else
			current_page_->layout(arrange_area(), clip_area());
	}
}


void gui_tabview_switch::insert_page
	(const shared_pointer<gui_element>& page)
{
	if (!page)
	{
		LOG_ERROR("Cannot insert NULL page.");
		throw null_pointer();
	}

	shared_instance<gui_element> page_instance
		(page.instance_or_throw());

	insert_child(page_instance);
	SCOPE_GUARD
		(child_guard,
			(*this, &gui_tabview_switch::remove_child, page_instance));

	pages_.insert(page_instance);
	child_guard.dismiss();

	// Exception safety: NOTHROW from here.
	want_layout();
}


void gui_tabview_switch::remove_page
	(const shared_pointer<gui_element>& page) NOTHROW
{
	if (!page)
	{
		LOG_ERROR("Cannot insert NULL page.");
		return; // NOTHROW
	}

	shared_instance<gui_element> page_instance
		(page.instance_or_terminate());

	if (!pages_.contains(page_instance))
	{
		LOG_ERROR("Page not part of tabview, cannot remove.");
		return; // NOTHROW
	}

	if (current_page_ == page) 
		current_page_.reset();

	remove_child(page_instance);
	pages_.remove(page_instance);
	want_layout();
}


void gui_tabview_switch::select_page
	(const shared_pointer<gui_element>& page) NOTHROW
{
	if (page)
	{
		shared_instance<gui_element> page_instance
			(page.instance_or_terminate());

		if (!pages_.contains(page_instance))
		{
			LOG_ERROR("Page not part of tabview, cannot select.");
			return; // NOTHROW
		}
	}

	current_page_ = page;
	want_layout();
}




//////////////////////////////////////////////////////////////////////////
//
// gui_tabview_style
//
//////////////////////////////////////////////////////////////////////////


gui_tabview_style::gui_tabview_style
	(const shared_instance<gui_split_style>& split_style,
	 const shared_instance<gui_scroll_area_style>& header_scroll_area_style,
	 const shared_instance<gui_list_style>& header_list_style)
	:
	split_style(split_style),
	header_scroll_area_style(header_scroll_area_style),
	header_list_style(header_list_style)
{ }




} /* namespace viral_gui */
