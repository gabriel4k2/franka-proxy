/**
 *************************************************************************
 *
 * @file gui_spacer.hpp
 *
 * GUI whitespace holder.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_SPACER_HPP)
#define INCLUDED__VIRAL_GUI__GUI_SPACER_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_spacer
 * @ingroup gui_module
 *
 * Requires fixed space within containers, but does not have any content.
 *
 ************************************************************************/
class gui_spacer :
	public gui_element
{

	public:

		gui_spacer
			(const string& name, gui_context& gui,
			 const shared_instance<gui_spacer_style>& spacer_style);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		const shared_instance<gui_spacer_style> spacer_style;


	private:

		void realize_layout() override;
};




/**
 *************************************************************************
 *
 * @class gui_spacer
 * @ingroup gui_module
 *
 * Styling for a gui_spacer.
 *
 ************************************************************************/
class gui_spacer_style :
	public gui_style
{

	public:

		gui_spacer_style(const vector2f& extents);
		const vector2f extents;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_SPACER_HPP) */