/**
 *************************************************************************
 *
 * @file gui_tristate.hpp
 *
 * Clickable GUI elements with three states.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_TRISTATE_HPP)
#define INCLUDED__VIRAL_GUI__GUI_TRISTATE_HPP


#include <viral_core/build_config.hpp>


#include "gui_element.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_tristate
 * @ingroup gui_module
 *
 * A button that switches through selected, deselected, and mixed states.
 *
 * Layout notes:
 * - If a tristate receives less horizontal arrange_area() than preferred,
 *		the tristate shrinks its label to allow for automated ellipsis.
 * - Apart from this, tristates clip rendering and input handling
 *		if parents provide less space than indicated by preferred_size(),
 *		and center automatically if parents provide excess space.
 *
 ************************************************************************/
class gui_tristate :
	public gui_element
{

	public:

		/**
		 * Selection behaviour on click.
		 * Check mode allows for individual selection and deselection.
		 * Choice mode selects on click, and deselects on the remaining
		 * tristates in the available choices().
		 */
		enum select_mode
			{ mode_check, mode_choice };


		/**	
		 * Current selection state.
		 * User interaction cannot cause @bref{state_maybe},
		 * only set_state() can do so.
		 */
		enum select_state
			{ state_yes, state_no, state_maybe };


		gui_tristate
			(const string& name, gui_context& gui,
			 const shared_instance<gui_tristate_style>& tristate_style,
			 select_mode mode);


		vector2f preferred_size() const override;

		void process_input
			(gui_input_event& input,
			 gui_output_event_list& output) override;

		void commit_render
			(gui_render_packer& packer,
			 render_command_queue& queue) override;


		/** See gui_button::read_only(). */
		//@{
		void set_read_only(bool read_only) NOTHROW;
		bool read_only() const NOTHROW;
		//@}


		/**
		 * Access to state of tristate.
		 *
		 * Note @bref{mode_choice} tristates change choices()
		 * alongside their state.
		 */
		//@{
		void set_state(select_state state);
		void toggle_state();
		select_state state() const;
		//@}


		/** Set text shown next to tristate button. */
		//@{
		void set_text(const string& text);
		string text() const;
		//@}


		/**
		 * Not-@NULL exclusive choices for @bref{mode_choice}.
		 * The @bref{choices} list may contain the current tristate.
		 */
		//@{
		typedef list<weak_pointer<gui_tristate>> choice_list;
		void set_choices(const choice_list& choices);
		const choice_list& choices() const;
		//@}
		

		const shared_instance<gui_tristate_style> tristate_style;
		const select_mode mode;


	private:

		void realize_layout() override;

		void update_icon();
		void update_choices();


		select_state state_;

		const shared_instance<gui_button> button_;
		const shared_instance<gui_label> label_;

		choice_list choices_;
};


/**
 * Notifies clients that the user clicked a gui_tristate.
 * @ingroup gui_module
 */
typedef gui_simple_output_event<gui_tristate> gui_tristate_event;




/**
 *************************************************************************
 *
 * @class gui_tristate_style
 * @ingroup gui_module
 *
 * Icons, size, and style for a gui_tristate checkbox or radio button.
 *
 ************************************************************************/
class gui_tristate_style :
	public gui_style
{
	public:

		gui_tristate_style
			(const shared_instance<gui_button_style>& button_style,
			 const shared_instance<render_material_id>& yes_icon,
			 const shared_instance<render_material_id>& no_icon,
			 const shared_instance<render_material_id>& maybe_icon,
			 const vector2i& icon_pixel_size,
			 float button_label_spacing);


		const shared_instance<gui_button_style> button_style;

		const shared_instance<render_material_id>
			yes_icon, no_icon, maybe_icon;
		const vector2i icon_pixel_size;

		const float button_label_spacing;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_TRISTATE_HPP) */
