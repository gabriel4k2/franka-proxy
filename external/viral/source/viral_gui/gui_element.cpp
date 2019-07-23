/**
 *************************************************************************
 *
 * @file gui_element.cpp
 *
 * Basic GUI element hierarchy, implementation.
 *
 ************************************************************************/


#include "gui_element.hpp"


#include <viral_core/log.hpp>


#include "gui.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_element
//
//////////////////////////////////////////////////////////////////////////


gui_element::gui_element
	(const string& name, gui_context& gui)
	:
	name(name),
	gui(gui),
	parent_(NULL),
	layout_invalid_(true),
	layout_dirty_(true),
	stay_dirty_(false)
{ }


gui_element::~gui_element()
	{ remove_from_hierarchy_on_destroy(); }


gui_element* gui_element::parent() const NOTHROW
	{ return parent_; }


gui_element::children_list::const_iterator
	gui_element::first_child() const NOTHROW
		{ return children_.first(); }
gui_element::children_list::const_iterator
	gui_element::last_child() const NOTHROW
		{ return children_.last(); }


shared_pointer<gui_element>
	gui_element::fetch_shared_from_parent() const NOTHROW
{
	if (!parent_ || !parent_child_iterator_)
		return shared_pointer<gui_element>();

	return parent_child_iterator_->lock();
}


const rect2f& gui_element::arrange_area() const NOTHROW
	{ return arrange_area_; }
const rect2f& gui_element::clip_area() const NOTHROW
	{ return clip_area_; }

bool gui_element::layout_invalid() const NOTHROW
	{ return layout_invalid_; }
bool gui_element::layout_dirty() const NOTHROW
	{ return layout_dirty_; }


void gui_element::layout
	(const rect2f& arrange_area, const rect2f& clip_area)
{
	if (arrange_area_ == arrange_area &&
		clip_area_ == clip_area &&
		!layout_invalid_ && !layout_dirty_)
			return;

	arrange_area_ = arrange_area;
	clip_area_ = clip_area;
	layout_invalid_ = false;

	stay_dirty_ = false;

	realize_layout();
	want_commit_render();

	if (!stay_dirty_)
		layout_dirty_ =  false;

	stay_dirty_ = false;
}


void gui_element::clip_entirely()
{
	if (layout_invalid_ && !layout_dirty_)
		return;

	arrange_area_ = rect2f();
	clip_area_ = rect2f();
	layout_invalid_ = true;

	stay_dirty_ = false;

	realize_layout();
	want_commit_render();

	if (!stay_dirty_)
		layout_dirty_ =  false;

	stay_dirty_ = false;
}


void gui_element::want_layout() NOTHROW
{
	gui_element *dirty = this;
	while (dirty)
	{
		dirty->layout_dirty_ = true;
		dirty = dirty->parent_;
	}
}


void gui_element::stay_dirty() NOTHROW
{
	gui_element *dirty = this;
	while (dirty)
	{
		dirty->stay_dirty_ = true;
		dirty = dirty->parent_;
	}
}


void gui_element::want_commit_render() NOTHROW
	{ gui.raise_render_dirty(); }


void gui_element::acquire_key_focus() const NOTHROW
{
	shared_pointer<gui_element> shared_from_parent
		(fetch_shared_from_parent());

	// Cannot acquire focus if not part of a GUI hierarchy.
	if (!shared_from_parent) return;

	gui.set_key_focus(shared_from_parent);
}

void gui_element::acquire_mouse_focus() const NOTHROW
{
	shared_pointer<gui_element> shared_from_parent
		(fetch_shared_from_parent());

	// Cannot acquire focus if not part of a GUI hierarchy.
	if (!shared_from_parent) return;

	gui.set_mouse_focus(shared_from_parent);
}


void gui_element::release_key_focus() const NOTHROW
{
	shared_pointer<gui_element> shared_from_parent
		(fetch_shared_from_parent());

	// Cannot release focus if not part of a GUI hierarchy.
	if (!shared_from_parent) return;

	if (gui.key_focus() == shared_from_parent)
		gui.clear_key_focus();
}

void gui_element::release_mouse_focus() const NOTHROW
{
	shared_pointer<gui_element> shared_from_parent
		(fetch_shared_from_parent());

	// Cannot release focus if not part of a GUI hierarchy.
	if (!shared_from_parent) return;

	if (gui.mouse_focus() == shared_from_parent)
		gui.clear_mouse_focus();
}


void gui_element::release_key_focus_to_parent() const NOTHROW
{
	if (!has_key_focus()) return;
	if (parent_) parent_->acquire_key_focus();
		else release_key_focus();
}

void gui_element::release_mouse_focus_to_parent() const NOTHROW
{
	if (!has_mouse_focus()) return;
	if (parent_) parent_->acquire_mouse_focus();
		else release_mouse_focus();
}


bool gui_element::has_key_focus() const NOTHROW
	{ return gui.key_focus().lock().get() == this; }
bool gui_element::has_mouse_focus() const NOTHROW
	{ return gui.mouse_focus().lock().get() == this; }


bool gui_element::contains_key_focus() const NOTHROW
{
	// Avoid expensive search within children,
	// but check hierarchy above focus instead.
	for (gui_element* test_focus = gui.key_focus().lock().get();
		 test_focus; test_focus = test_focus->parent())
	{
		if (test_focus == this)
			return true;
	}

	return false;
}


bool gui_element::contains_mouse_focus() const NOTHROW
{
	// Avoid expensive search within children,
	// but check hierarchy above focus instead.
	for (gui_element* test_focus = gui.mouse_focus().lock().get();
		 test_focus; test_focus = test_focus->parent())
	{
		if (test_focus == this)
			return true;
	}

	return false;
}


bool gui_element::try_accept_key_focus() NOTHROW
	{ return true; }
bool gui_element::try_accept_mouse_focus() NOTHROW
	{ return true; }


void gui_element::lost_key_focus() NOTHROW { }
void gui_element::lost_mouse_focus() NOTHROW { }


void gui_element::insert_child
	(const shared_instance<gui_element>& child)
{
	// Check simple constraints:
	// Both elements need to match in their gui_context
	// and the new child must not have a parent yet.
	if (&(child().gui) != &gui)
	{
		LOG_ERROR("GUI element: " + child().name +
				  " and parent: " + name +
				  " have mismatching GUI contexts.");
		throw invalid_context();
	}

	if (child().parent_)
	{
		LOG_ERROR("GUI element: " + child().name +
				  " already has a parent: " + child().parent_->name +
				  ", cannot insert into: " + name);
		throw invalid_state();
	}


	// Determine combined height of both hierarchies.
	// In particular, note that the new child may already
	// have some children of its own.
	if (child().hierarchy_height() + element_depth() > max_hierarchy_height_)
	{
		LOG_ERROR("GUI element: " + child().name +
				  " and its new parent: " + name +
				  " would exceed the maximum height for a GUI hierarchy.");
		throw invalid_state();
	}


	// See if inserting the new child would cause a cycle.
	// We already know that the new child has no parent,
	// so a cycle is only possible if the new child
	// is an ancestor of the current element.
	for (const gui_element *test_cycle = this;
		 test_cycle; test_cycle = test_cycle->parent_)
	{
		if (test_cycle == &child())
		{
			LOG_ERROR("GUI element: " + child().name + 
					  " is equal to or parent/ancestor of: " + name +
					  ", cannot insert as child.");
			throw invalid_state();
		}
	}
	

	// Constraints are met,
	// so insert new child.
	children_list::iterator child_iterator
		(children_.insert(child));


	// Success.
	// Exception safety: NOTHROW from here.
	child().parent_ = this;
	child().parent_child_iterator_ = child_iterator;
}


void gui_element::remove_child
	(const shared_instance<gui_element>& child) NOTHROW
{
	if (child().parent_ != this)
	{
		LOG_ERROR("Cannot remove, GUI element: " + name +
				  " is not parent of: " + child().name);
		return; // NOTHROW
	}

	children_.remove(child().parent_child_iterator_);
	child().parent_ = NULL;
	child().parent_child_iterator_ = children_list::iterator();
}


int64 gui_element::hierarchy_height() const NOTHROW
{
	int64 max_child_height = 0;

	for (children_list::const_iterator child_iterator(children_.first());
		 child_iterator; ++child_iterator)
	{
		shared_pointer<gui_element> child
			(child_iterator->lock());

		if (!child) continue;

		max_child_height =
			max(max_child_height, child->hierarchy_height());
	}

	return max_child_height + 1;
}


int64 gui_element::element_depth() const NOTHROW
{
	int64 depth = 0;

	// Loop executes at least once.
	for (const gui_element *find_depth = this;
		 find_depth; find_depth = find_depth->parent_)
			++depth;

	return depth;
}


void gui_element::remove_from_hierarchy_on_destroy() NOTHROW
{
	/**
	 * Update GUI hierarchy because this gui_element
	 * is about to be destroyed.
	 *
	 * Implementation notes:
	 * - Both our parent and our children may be alive due to
	 *		remaining shared_pointers. In this case, clean up
	 *		our parent's children and break the GUI hierarchy
	 *		to allow for reuse of our own children.
	 * - Alternatively, our parent may currently be destroying,
	 *		which causes redundant but harmless children cleanup.
	 * - We cannot invoke parent_->remove_child() in here:
	 *		There are no remaining shared_pointer%s, which is
	 *		why this gui_element is actually under destruction.
	 * - GUI elements may be destroyed after their gui_context,
	 *		thus we must not use advanced gui_element methods.
	 */


	// If this element still has a parent pointer,
	// remove from the children list of our parent.
	// The parent_child_iterator_ avoids searching
	// for our position within the children list.
	if (parent_)
	{
		parent_->children_.remove(parent_child_iterator_);
		parent_ = NULL;
		parent_child_iterator_ = children_list::iterator();
	}
	

	// If this element still has any children,
	// reset their parent pointer and iterator.
	for (children_list::iterator it(children_.first()); it; ++it)
	{
		shared_pointer<gui_element> child(it->lock());
		if (!child) continue;
		child->parent_ = NULL;
		child->parent_child_iterator_ = children_list::iterator();
	}

	children_.clear();
}


const int64 gui_element::max_hierarchy_height_ = 64;




} /* namespace viral_gui */
