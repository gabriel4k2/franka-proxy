/**
 *************************************************************************
 *
 * @file gui_element.hpp
 *
 * Basic GUI element hierarchy.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_ELEMENT_HPP)
#define INCLUDED__VIRAL_GUI__GUI_ELEMENT_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/render_forward.hpp>


#include <viral_core/util.hpp>
#include <viral_core/shared_pointer.hpp>
#include <viral_core/list.hpp>
#include <viral_core/string.hpp>
#include <viral_core/geo_2d.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_element
 * @ingroup gui_module 
 *
 * Base class for GUI elements such as gui_button%s or gui_window%s.
 *
 * Usage notes:
 * - See @ref gui_module "GUI Module".
 *
 ************************************************************************/
class gui_element :
	private non_copyable
{
	public:

		/** List of generic children in a gui_element. */
		typedef list<weak_pointer<gui_element>> children_list;

		/** Orientation enum shared by many gui_element types. */
		enum orientation_type { horizontal, vertical };


		gui_element
			(const string& name, gui_context& gui);


		virtual ~gui_element() NOTHROW;


		/**
		 * Fetch parent element of this GUI element,
		 * or @NULL if not attached to a parent.
		 */
		gui_element* parent() const NOTHROW;


		/**
		 * Fetch iterator for the children in this GUI element.
		 * Iterators to a child pointer become invalid
		 * once the respective child is destroyed.
		 *
		 * @attent Resulting iterators expose internal children
		 * of the GUI element. Clients must not mess with these.
		 * Notably, iterators to internal children may invalidate
		 * due to GUI calls (e.g. layout() or process_input()).
		 */
		//@{
		children_list::const_iterator first_child() const NOTHROW;
		children_list::const_iterator last_child() const NOTHROW;
		//@}


		/**
		 * Fetch a shared_pointer to this element from its parent().
		 * Returns @NULL for all GUI elements without a parent,
		 * notably for the root gui_context::modal_stack().
		 */
		shared_pointer<gui_element> fetch_shared_from_parent() const NOTHROW;			


		/**
		 * Determine preferred size of this GUI element.
		 * Potentially recurses on an entire GUI branch.
		 *
		 * Parents invoke this to determine a size of their own.
		 * Preferred sizes are not mandatory, and parents can
		 * layout() their children in any other dimensions.
		 * In particular, this happens with constant-sized or
		 * user-sized parents (e.g. gui_window) that have
		 * too little or too much room for all their children.
		 */
		virtual vector2f preferred_size() const = 0;


		/**
		 * GUI per-element layouting.
		 * 
		 * Layout areas:
		 * - The arrange_area() defines a GUI-space rectangle
		 *		within which this GUI element should place
		 *		all its contents and its children elements.
		 * - The clip_area() indicates a rectangle in GUI-space
		 *		that this GUI element and all its children should
		 *		restrict rendering and input handling to.
		 * - Arrange and clip area do not depend on each other:
		 *		The clip area may be larger or smaller than the
		 *		arrange area, and both areas need not intersect.
		 * - GUI elements should, but need not, respect both areas.
		 *		An element may place content outside its arrange area
		 *		(e.g. to let the clip area cull excess content),
		 *		and	an element may draw or react outside its clip area
		 *		(e.g. to disable mouse-over graphics).
		 *
		 * Layout flags:
		 * - If layout_invalid() is set, the parent GUI element could
		 *		not provide layout areas due to clipping. In this case,
		 *		both arrange_area() and clip_area() must not be used.
		 *		The GUI element should not render or accept input,
		 *		and should clip_entirely() any further children.
		 * - If layout_dirty() is set, the element content changed,
		 *		but layout areas have not been rebuilt accordingly yet.
		 *		In this case, the element must be aware of potential
		 *		inconsistencies in-between areas and element content.
		 *
		 * Basic layout functions:
		 * - Both layout() and clip_entirely() recursively update GUI layout.
		 *		The former applies a (potentially new) arrange_area() and
		 *		clip_area(). The latter suppresses an element if its parent
		 *		could not provide either area due to clipping. Both update
		 *		layout_invalid() accordingly.
		 * - GUI elements implement individual handlers for their content
		 *		and children elements in realize_layout(), which executes
		 *		after updates through both layout() and clip_entirely().
		 *		Notably, realize_layout() should recursively layout()
		 *		or clip_entirely() all children elements.
		 *
		 * Layout coherency:
		 * - For performance reasons, layouting skips GUI branches
		 *		which do not change in arrange_area(), clip_area(),
		 *		or layout_invalid(), and which are not layout_dirty().
		 * - A call to want_layout() raises layout_dirty() on an element
		 *		and all its parents to enforce recursive layout rebuild
		 *		on the next gui_context::update(). Usually, setters and
		 *		input handling use this to forcibly update GUI elements
		 *		after modifying the element state.
		 * - After realize_layout() returns, layout() and clip_entirely()
		 *		reset layout_dirty() on touched elements. Therefore,
		 *		realize_layout() can call setters on elements or their
		 *		children without causing (usually redundant) layouting.
		 * - If an element cannot realize_layout() correctly (e.g. due to
		 *		lazily updating preferred_size()), it can stay_dirty().
		 *		In this case, layout_dirty() remains set on the element
		 *		and its parents to enforce a later layout() pass.
		 */
		//@{
		const rect2f& arrange_area() const NOTHROW;
		const rect2f& clip_area() const NOTHROW;

		bool layout_invalid() const NOTHROW;
		bool layout_dirty() const NOTHROW; 

		void layout(const rect2f& arrange_area, const rect2f& clip_area);
		void clip_entirely();

		void want_layout() NOTHROW;
		void stay_dirty() NOTHROW;
		//@}


		/**
		 * Process various types of @bref{input} events
		 * and store resulting @bref{output} events.
		 *
		 * Implementation notes:
		 * - Once an element has captured an @bref{input} event,
		 *		it should lower the gui_input_event::hot flag.
		 * - Usually, parent elements (even if layout_invalid())
		 *		should pass on all input to their children.
		 *		However, parents may also use differing input or
		 *		local output lists to collaborate with children.
		 * - In contrast to popular listener patterns, delayed
		 *		and separate processing of output events avoids
		 *		creating an inconsistent GUI state (e.g. when
		 *		deleting GUI elements due to an output event).
		 */
		virtual void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) = 0;


		/**
		 * Commit rendering representations of this element
		 * and all its children to the rendering system.
		 *
		 * Basic implementation outline:
		 * - Abort if layout_invalid().
		 * - Pack any element meshes into the render @bref{packer}.
		 *		Elements should cache expensive meshes (e.g. text)
		 *		to improve performance on frequent commit_render().
		 * - Commit any additional resources (e.g. material data)
		 *		to the render command @bref{queue}.
		 * - Recurse on children in back-to-front order.
		 *
		 * Note that a gui_context caches and reuses packed meshes,
		 * and does not invoke gui_element::commit_render() unless
		 * explicitly requested with want_commit_render().
		 */
		virtual void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) = 0;


		/**
		 * Request a commit_render() pass before next rendering,
		 * for instance after process_input() changed element state.
		 * Automatically invoked on return from realize_layout().
		 */
		void want_commit_render() NOTHROW;


		/**
		 * Manage per-element key_focus() or mouse_focus()
		 * through suitable calls to the @bref{gui} context.
		 * See gui_context::set_key_focus() for details.
		 *
		 * Both release_key_focus() and release_mouse_focus()
		 * ignore elements that currently do not have focus.
		 * In contrast to gui_context::clear_key_focus() and
		 * gui_context::clear_mouse_focus(), this prohibits
		 * releasing a conflicting focus of another element.		 
		 */
		//@{
		void acquire_key_focus() const NOTHROW;
		void acquire_mouse_focus() const NOTHROW;

		void release_key_focus() const NOTHROW;
		void release_mouse_focus() const NOTHROW;
		//@}


		/**
		 * If the element has_key_focus() or has_mouse_focus(),
		 * pass respective focus to any parent if one exists.
		 * Otherwise, release key focus or mouse focus entirely.
		 */
		//@{
		void release_key_focus_to_parent() const NOTHROW;
		void release_mouse_focus_to_parent() const NOTHROW;
		//@}


		/**
		 * Check if this element currently has or at least contains
		 * gui_context::key_focus() or gui_context::mouse_focus().
		 */
		//@{
		bool has_key_focus() const NOTHROW;
		bool has_mouse_focus() const NOTHROW;

		bool contains_key_focus() const NOTHROW;
		bool contains_mouse_focus() const NOTHROW;
		//@}


		/**
		 * Notify an element that it should accept
		 * or that it has lost key or mouse focus.
		 * Clients should not call these methods.
		 *
		 * Accepting focus:
		 * - Implementations must return @true to accept focus,
		 *		and, in this case, must not change global focus.
		 * - Implementations must return @false to deny focus.
		 *		In this case, global focus remains unchanged, 
		 *		unless elements explicitly set another focus
		 *		(e.g. acquire_key_focus() on a child element).
		 * - @attent Avoid cyclic release_key_focus_to_parent()
		 *		or release_mouse_focus_to_parent() calls.
		 *
		 * General notes:
		 * - Default implementations accept all focus,
		 *		and do nothing on losing focus.
		 * - Focus-sensitive elements may want_commit_render()
		 *		or want_layout() to reflect a new focus.
		 */
		//@{
		virtual bool try_accept_key_focus() NOTHROW;
		virtual bool try_accept_mouse_focus() NOTHROW;

		virtual void lost_key_focus() NOTHROW;
		virtual void lost_mouse_focus() NOTHROW;
		//@}


		/**
		 * Name of this GUI element for debug output.
		 * Not necessarily unique.
		 */
		const string name;


		/** GUI context for use with this GUI element. */
		gui_context& gui;


	protected:

		/**
		 * Manage generic @bref{children_} list within this element.
		 *
		 * Usage notes:
		 * - Parents must explicitly insert_child()ren so that GUI features
		 *		(e.g. layout(), key focus, tabstop) work as intended.
		 * - Insertion preemptively checks constraints of the GUI hierarchy.
		 *		In particular, insertion fails and throws if a @bref{child}
		 *		belongs to another @bref{gui} context, if a child already
		 *		has another @bref{parent_}, or if the combined hierarchy
		 *		would exceed @bref{max_hierarchy_height_}.
		 * - Parent destruction automatically removes any @bref{children_},
		 *		and children automatically remove themselves on destruction.
		 *		Parents need only explicitly remove children if these
		 *		shall be inserted into another GUI element later on.
		 * - Parents only hold weak ownership over generic children.
		 *		This avoids having to explicitly remove children to allow
		 *		their destruction. However, implementations must maintain
		 *		an additional shared_pointer to keep their children alive.
		 */
		//@{
		void insert_child(const shared_instance<gui_element>& child);
		void remove_child(const shared_instance<gui_element>& child) NOTHROW;
		//@}


		/**
		 * Find the number of gui_element%s to the most-distant leaf
		 * or to the top-most ancestor of this element.
		 *
		 * The calling element counts, and so do root and leaf elements.
		 * In particular, a gui_element without parent and children
		 * has a hierarchy_height() and an element_depth() of one.
		 */
		//@{
		int64 hierarchy_height() const NOTHROW;
		int64 element_depth() const NOTHROW;
		//@}


		/**
		 * Invoked by layout() and clip_entirely() to update
		 * implementation-specific contents and children elements.
		 *
		 * Implementation notes:
		 * - If layout_invalid(), clip_entirely() all children
		 *		and stop accepting user input.
		 * - Otherwise, rebuild contents for arrange_area()
		 *		and clip_area(), then layout() or clip_entirely()
		 *		all children as appropriate.
		 */
		virtual void realize_layout() = 0;


	private:

		void remove_from_hierarchy_on_destroy() NOTHROW;


		static const int64 max_hierarchy_height_;

		gui_element* parent_;
		children_list::iterator parent_child_iterator_;

		children_list children_;

		rect2f arrange_area_;
		rect2f clip_area_;

		bool layout_invalid_;
		bool layout_dirty_;
		bool stay_dirty_;
};


/**
 * Binds names to gui_element%s.
 * @ingroup gui_module
 */
typedef shared_cache<gui_element> gui_element_cache;




/**
 *************************************************************************
 *
 * @class gui_style
 * @ingroup gui_module
 *
 * Base class for GUI-specific rendering styles.
 *
 * Usage notes:
 * - All gui_element%s track associated styles by shared_instance.
 *		This reduces memory overhead for large styles (e.g. fonts).
 * - Styles are constant after creation. This allows to check styles
 *		for correctness once on construction, while GUI elements
 *		need not validate their bound styles on each use.
 *
 ************************************************************************/
class gui_style
{
	public:

		virtual ~gui_style() NOTHROW { }
};


/**
 * Binds names to gui_style%s.
 * @ingroup gui_module
 */
typedef shared_cache<gui_style> gui_style_cache;


/**
 * Binds names to rendering shaders for use in a GUI.
 * @ingroup gui_module
 */
typedef shared_cache<render_shader_id> gui_shader_cache;


/**
 * Binds names to rendering materials for use in a GUI.
 * @ingroup gui_module
 */
typedef shared_cache<render_material_id> gui_material_cache;




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_ELEMENT_HPP) */
