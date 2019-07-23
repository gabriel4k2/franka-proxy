/**
 *************************************************************************
 *
 * @file gui_modal.hpp
 *
 * GUI modality handling.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_MODAL_HPP)
#define INCLUDED__VIRAL_GUI__GUI_MODAL_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/util.hpp>


#include "gui.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_modal_interaction
 * @ingroup gui_module
 *
 * A modal user interaction that execute()s in-place until finish()ed.
 *
 * Usage notes:
 * - In-place modal interactions rely on gui_modal_context to perform
 *		client-specific logics, such as input handling or rendering.
 * - Multiple interaction instances are allowed to stack. Notably,
 *		a modal interaction may internally start another interaction.
 *		However, an already running interaction cannot be execute()d,
 *		but throws an exception instead.
 * - @attent Modal interactions (in particular without a new entry
 *		on the gui_context::modal_stack()) do not guarantee immutable
 *		program state. For instance, the user might still be able
 *		to change GUI elements, which propagates back to the place
 *		where gui_modal_interaction::execute() was originally invoked.
 *
 * Rationale:
 * - Without gui_modal_interaction, many modal elements (e.g. common
 *		yes/no message boxes) require complex program flow inbetween
 *		the modal call and the main application loop.
 * - In contrast, a gui_modal_interaction executes a client-specific
 *		application loop in place, then directly returns to the modal
 *		call by exiting from that loop once the interaction finishes.
 *
 ************************************************************************/
class gui_modal_interaction :
	private non_copyable
{
	friend class gui_modal_context;


	public:

		gui_modal_interaction(gui_context& gui);
		virtual ~gui_modal_interaction() NOTHROW;


		/**
		 * Loop over gui_modal_context::modal_iteration()s and
		 * update() the modal interaction until finish()ed.
		 */
		void execute();


		/**
		 * Drop out from a higher-level execute() call,
		 * thus finishing the modal interaction.
		 *
		 * @attent
		 * The modal interaction may still perform logics
		 * (e.g. due to some lingering gui_output_event%s)
		 * before returning from execute().
		 */
		void finish() NOTHROW;	


		gui_context& gui;


	protected:

		/**
		 * Setup the modal interaction before entering the modal loop.
		 * Implementations typically insert their required gui_element%s
		 * into gui_context::modal_stack().
		 */
		virtual void setup() = 0;


		/**
		 * Clean up the modal interaction after leaving the modal loop.
		 * Implementations typically remove any interaction elements
		 * from gui_context::modal_stack().
		 */
		virtual void clear() NOTHROW = 0;


		/**
		 * Implement to execute a specific modal interaction.
		 * Invoked through gui_modal_context::modal_iteration()
		 * and gui_modal_context::update() during the modal loop.
		 *
		 * Implementation outline:		
		 * - Call @bref{gui}.update().
		 * - Evaluate the resulting gui_output_event_list.
		 * - Potentially call finish() to terminate the interaction.
		 */
		virtual void update() = 0;


	private:

		bool finished_;
};




/**
 *************************************************************************
 *
 * @class gui_modal_context
 * @ingroup gui_module
 *
 * Implements client-specific logics that are shared
 * by all gui_modal_interaction%s in a gui_context.
 *
 ************************************************************************/
class gui_modal_context :
	private non_copyable
{

	public:

		virtual ~gui_modal_context() NOTHROW;


		/**
		 * A client-specific iteration within the modal loop
		 * of gui_modal_interaction::execute(). 
		 *
		 * Implementations usually match an iteration of the main loop
		 * in the client application (e.g. perform timing, update input
		 * and window, render, ...), but call gui_modal_context::update()
		 * instead of gui_context::update().
		 */
		virtual void modal_iteration
			(gui_modal_interaction& modal_interaction) = 0;	


	protected:

		void update
			(gui_modal_interaction& modal_interaction)
				{ modal_interaction.update(); }
};




/**
 *************************************************************************
 *
 * @class gui_modal_dialog
 * @ingroup gui_module
 *
 * Base class for modal dialogs.
 *
 * Usage notes:
 * - To create a custom modal dialog, derive from this class and
 *		implement dialog_window() as well as process_events().
 * - To show the custom dialog, create an instance of the custom
 *		dialog class on the program stack and call execute().
 * - The gui_context must outlive all gui_modal_dialog%s.
 *
 * Implementation notes:
 * - Modal dialogs clear mouse focus and set key focus to dialog_window().
 *		Implementations may change this behaviour by overriding setup(). 
 *
 ************************************************************************/
class gui_modal_dialog :
	public gui_modal_interaction
{
	public:

		gui_modal_dialog(gui_context& gui);


	protected:

		/**
		 * Insert dialog_window() and dialog_background()
		 * into the GUI hierarchy as a new modal layer.
		 *
		 * Dialog implementations may override setup()
		 * (e.g. to call gui_element::acquire_key_focus()),
		 * but must first call gui_modal_dialog::setup().
		 */
		void setup() override;


		/**
		 * Remove the modal dialog from the modal stack,
		 * thus allowing input to a prior modal level again.
		 *
		 * Dialog implementations may override clear()
		 * (e.g. to pass back key focus to another element),
		 * but must first call gui_modal_dialog::clear().
		 */
		void clear() NOTHROW override;


		/**
		 * Run GUI logics as specified in gui_modal_context::update().
		 * Automatically call process_events() on resulting events.
		 *
		 * Dialog implementations may override update()
		 * (e.g. to perform operations per logics frame),
		 * but should call gui_modal_dialog::update().
		 */
		void update() override;


		/**
		 * Return the gui_window of the modal dialog.
		 *
		 * Each gui_modal_dialog instance must have a unique window.
		 * However, a single modal dialog can reuse its window over
		 * repeated execute() calls.
		 */
		virtual shared_instance<gui_window> dialog_window() = 0;


		/**
		 * Optionally, re-implement to supply a background gui_element
		 * that is to be shown behind the dialog window area.
		 *
		 * The background element is part of the modal interaction,
		 * and receives hot user input. @NULL means no background,
		 * but the user still cannot interact with other modal layers.
		 *
		 * Each gui_modal_dialog instance must have a unique background.
		 * However, a single modal dialog can reuse its background over
		 * repeated execute() calls.
		 */
		virtual shared_pointer<gui_element> dialog_background();


		/**
		 * Process gui_output_event%s sent by the dialog window
		 * or its children gui_element%s.
		 *
		 * Implementation notes:
		 * - Event processing usually should store relevant element state
		 *		(e.g. checkbox selection, editbox contents) within suitable
		 *		member variables in order to return modal interaction results.
		 * - Implementations must finish() the modal interaction as reaction
		 *		to some GUI event here, e.g. if the user clicks an OK button
		 *		or closes the dialog window. Implementations may also finish()
		 *		on other exit conditions, e.g. window_context::close_flag().
		 * - It is explicitly allowed to start another gui_modal_interaction
		 *		(notably, another gui_modal_dialog) within event processing.
		 */
		virtual void process_events
			(const gui_output_event_list& output) = 0;


	private:

		shared_pointer<gui_window_area> modal_dialog_area_;
		weak_pointer<gui_element> original_key_focus_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_MODAL_HPP) */
