/**
 *************************************************************************
 *
 * @file gui_window_area.cpp
 *
 * GUI window area, implementation.
 *
 ************************************************************************/


#include "gui_window_area.hpp"


#include <viral_core/log.hpp>


#include "gui_window.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_window_area
//
//////////////////////////////////////////////////////////////////////////


gui_window_area::gui_window_area
	(const string& name, gui_context& gui)
	:
	gui_element(name, gui)
{ }


vector2f gui_window_area::preferred_size() const
{ 
	vector2f size;

	for (window_list::const_iterator it(windows_.first()); it; ++it)
		size = size.max((*it)().position() + (*it)().preferred_size());

	return size;
}


void gui_window_area::process_input
	(gui_input_event& input,
	 gui_output_event_list& output)
{
	/**
	 * Send input to background and windows in LIFO order,
	 * inverse to commit_render().
	 *
	 * We sort windows twice: The first time, we apply any key focus
	 * set by client code. The second time, we consider a key focus
	 * acquired by contained windows as a reaction to user clicks.
	 *
	 * @attent We must not resort the window list while iterating
	 * over this list to process input.
	 */

	if (layout_invalid()) return;

	sort_windows();

	for (window_list::const_iterator it(windows_.last()); it; --it)
		(*it)().process_input(input, output);

	if (background_)
		background_->process_input(input, output);

	sort_windows();
}


void gui_window_area::commit_render
	(gui_render_packer& packer,
	 render_command_queue& queue)
{
	/** Render background and windows in bottom-up order. */

	if (layout_invalid()) return;

	if (background_)
		background_->commit_render(packer, queue);

	for (window_list::const_iterator it(windows_.first()); it; ++it)
		(*it)().commit_render(packer, queue);
}


void gui_window_area::insert_window
	(const shared_instance<gui_window>& window)
{
	insert_child(window);
	SCOPE_GUARD
		(child_guard,
			(*this, &gui_window_area::remove_child, window));

	windows_.push_back(window);
	child_guard.dismiss();

	// Exception safety: NOTHROW from here.
	want_layout();
}


void gui_window_area::remove_window
	(const shared_instance<gui_window>& window) NOTHROW
{
	remove_child(window);
	windows_.remove(window);
	want_layout();
}


void gui_window_area::clear_windows() NOTHROW
{
	while (!windows_.empty())
		remove_window(*windows_.first());
}


void gui_window_area::set_background
	(const shared_pointer<gui_element>& background)
{
	if (background)
		insert_child(background.instance_or_throw());

	// Exception safety: NOTHROW from here.
	if (background_)
		remove_child(background_.instance_or_terminate());

	background_ = background;
	want_layout();
}

const shared_pointer<gui_element>&
	gui_window_area::background() const NOTHROW
		{ return background_; }

void gui_window_area::clear_background() NOTHROW
{
	if (background_)
		remove_child(background_.instance_or_terminate());

	background_.reset();
	want_layout();
}


void gui_window_area::realize_layout()
{
	for (window_list::const_iterator it(windows_.first()); it; ++it)
	{
		if (layout_invalid()) (*it)().clip_entirely();	
			else (*it)().layout(arrange_area(), clip_area());
	}	

	if (background_)
	{
		if (layout_invalid())
			background_->clip_entirely();
		else
			background_->layout(arrange_area(), clip_area());
	}
}


void gui_window_area::sort_windows() NOTHROW
{
	/**
	 * If any contained window has key focus,
	 * find that window and move it to front.
	 */


	// Early out without key focus, without windows,
	// or if the current sorting is correct.
	if (!contains_key_focus()) return;
	if (windows_.empty()) return;

	const shared_instance<gui_window>& last_window =
		*windows_.last();

	if (last_window().contains_key_focus()) return;


	// Find window with key focus
	// and carry to front of list.
	for (window_list::iterator it(windows_.first()); it; ++it)
	{
		if (!(*it)().contains_key_focus())
			continue;

		window_list::iterator next(it);
		++next;

		if (!next) continue;

		swap(*it, *next);
	}


	// Render to reflect new window order.
	want_commit_render();
}




} /* namespace viral_gui */
