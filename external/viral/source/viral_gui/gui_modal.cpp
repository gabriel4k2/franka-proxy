/**
 *************************************************************************
 *
 * @file gui_modal.cpp
 *
 * GUI modality handling, implementation.
 *
 ************************************************************************/


#include "gui_modal.hpp"


#include <viral_core/log.hpp>
#include <viral_core/scope_guard.hpp>


#include "gui_event.hpp"
#include "gui_stack.hpp"
#include "gui_window.hpp"
#include "gui_window_area.hpp"


namespace viral_gui
{


//////////////////////////////////////////////////////////////////////////
//
// gui_modal_interaction
//
//////////////////////////////////////////////////////////////////////////


gui_modal_interaction::gui_modal_interaction(gui_context& gui)
	: gui(gui), finished_(true) { }


gui_modal_interaction::~gui_modal_interaction() NOTHROW
{
	if (!finished_)
		LOG_CRITICAL("Deleting an unfinished modal interaction.\n"
					 "This will potentially lead to a corrupt stack\n"
					 "and it will probably crash the program.");
}


void gui_modal_interaction::execute()
{
	if (!finished_)
	{
		LOG_ERROR("Modal interaction instance is already executing.");
		throw invalid_state();
	}

	finished_ = false;

	SCOPE_GUARD
		(clear_on_exit, (*this, &gui_modal_interaction::clear));
	SCOPE_GUARD
		(finish_on_exit, (*this, &gui_modal_interaction::finish));

	setup();

	while (!finished_)
		gui.modal_context.modal_iteration(*this);
}


void gui_modal_interaction::finish() NOTHROW
	{ finished_ = true; }




//////////////////////////////////////////////////////////////////////////
//
// gui_modal_context
//
//////////////////////////////////////////////////////////////////////////


gui_modal_context::~gui_modal_context() NOTHROW { }




//////////////////////////////////////////////////////////////////////////
//
// gui_modal_dialog
//
//////////////////////////////////////////////////////////////////////////


gui_modal_dialog::gui_modal_dialog(gui_context& gui)
	: gui_modal_interaction(gui) { }


void gui_modal_dialog::setup()
{
	shared_instance<gui_window_area> modal_dialog_area
		(new gui_window_area("modal_dialog_window_area", gui));

	shared_pointer<gui_element> background(dialog_background());
	if (background) modal_dialog_area().set_background(background);

	shared_instance<gui_window> window(dialog_window());
	modal_dialog_area().insert_window(window);

	gui.modal_stack().insert_layer(modal_dialog_area);


	// By default, assign keyboard focus to dialog window.
	// Implementations may override setup() to change this.
	// Also store original focus and restore on clear().
	original_key_focus_ = gui.key_focus();
	window().acquire_key_focus();


	// Clear mouse focus to stop inadverted clicks
	// onto GUI elements behind our background.
	gui.clear_mouse_focus();


	// Exception safety notes: 
	// - The modal_dialog_area_ pointer remains NULL on exception,
	//		and any intermediately created gui_elements are deleted.
	// - If a re-implementation throws after calling setup(),
	//		a scope_guard in gui_modal_interaction::execute()
	//		will clear() to remove the modal_dialog_area_.


	// NOTHROW from here.
	modal_dialog_area_ = modal_dialog_area;
}


void gui_modal_dialog::clear() NOTHROW
{
	if (modal_dialog_area_)
	{
		// Ensure that dialog window and background elements
		// can be reused as children within another execute(),
		// even if a stray pointer keeps the window area alive.
		modal_dialog_area_->clear_windows();
		modal_dialog_area_->clear_background();

		gui.modal_stack().remove_layer
			(modal_dialog_area_.instance_or_terminate());

		modal_dialog_area_.reset();
	}

	gui.set_key_focus
		(original_key_focus_.lock());
}


void gui_modal_dialog::update()
{
	gui_output_event_list output;
	gui.update(output);
	process_events(output);
}


shared_pointer<gui_element> gui_modal_dialog::dialog_background()
	{ return shared_pointer<gui_element>(); }




} /* namespace viral_gui */