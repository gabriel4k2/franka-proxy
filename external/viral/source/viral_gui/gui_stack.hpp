/**
 *************************************************************************
 *
 * @file gui_stack.hpp
 *
 * GUI element stack.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_STACK_HPP)
#define INCLUDED__VIRAL_GUI__GUI_STACK_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/list.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_stack
 * @ingroup gui_module
 *
 * A stack of children which share arrange_area() and clip_area().
 *
 * Layout notes:
 * - The GUI stack passes its arrange_area() and clip_area()
 *		to its children without any further adjustments.
 *		Hence, clients should only attach resizing containers
 *		as direct children, e.g. gui_window_area or gui_grid.
 *
 *************************************************************************/
class gui_stack :
	public gui_element
{
	friend class gui_context;


	public:

		gui_stack
			(const string& name, gui_context& gui, bool modal);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override; 

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		void insert_layer(const shared_instance<gui_element>& layer);
		void remove_layer(const shared_instance<gui_element>& layer) NOTHROW;


		/**
		 * If @bref{modal} is set, this stack realizes modal input:
		 * Only the topmost layer receives hot gui_input_event%s,
		 * while the stack sends deactivated events to all other layers. 
		 * See gui_context::modal_stack() and gui_modal_interaction.
		 */
		const bool modal;


	private:

		void realize_layout() override;

		typedef list<shared_instance<gui_element>> layer_list;
		layer_list layers_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_STACK_HPP) */