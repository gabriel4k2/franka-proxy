/**
 *************************************************************************
 *
 * @file gui_event.hpp
 *
 * Base events for use in a GUI.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_EVENT_HPP)
#define INCLUDED__VIRAL_GUI__GUI_EVENT_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/util.hpp>
#include <viral_core/geo_2d.hpp>
#include <viral_core/owning_list.hpp>
#include <viral_core/shared_pointer.hpp>
#include <viral_core/input.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_input_event
 * @ingroup gui_module
 *
 * Base class for all input events sent to gui_element::process_input().
 *
 ************************************************************************/
class gui_input_event
{
	public:

		virtual ~gui_input_event() NOTHROW { }


		/**
		 * Hint that indicates whether this input event
		 * should trigger a relevant gui_output_event
		 * (e.g. a gui_button click).
		 *
		 * Usually, the first gui_element to actually process input
		 * lowers this flag. Input then reaches other GUI elements,
		 * but these do not create further output events. However,
		 * elements may still process any cold input, for instance
		 * to cancel operations on button release.
		 *
		 * Events may start out cold, for instance
		 * if the window_context is not foreground.
		 */
		bool hot;


	protected:

		gui_input_event(bool hot) : hot(hot) { }
};




/**
 *************************************************************************
 *
 * @class gui_tick_event
 * @ingroup gui_module
 *
 * Sent to gui_element%s once per gui_context::update(),
 * even without pending user input.
 *
 * Usage notes:
 * - If the application window_context is in the foreground,
 *		the gui_context generates @bref{hot} tick events.
 *		Otherwise, the gui_context creates cold tick events.
 *
 ************************************************************************/
class gui_tick_event :
	public gui_input_event
{
	public:

		gui_tick_event(bool hot) : gui_input_event(hot) { }
};




/**
 *************************************************************************
 *
 * @class gui_input_source_event
 * @ingroup gui_module
 *
 * Sent to gui_element%s on low-level events at an input_source.
 *
 * Usage notes:
 * - GUI input source events directly map to input_context input events.
 *		Therefore, @bref{source_activated} and @bref{source_deactivated}
 *		may occur inconsistently (e.g. deactivation without activation).
 *
 ************************************************************************/
class gui_input_source_event :
	public gui_input_event
{

	public:

		enum input_source_event_type
			{ source_activated, source_deactivated };


		gui_input_source_event
			(bool hot,
			 input_source::type source, input_source_event_type event,
			 bool shift_down, bool ctrl_down, bool alt_down)
			:
			gui_input_event(hot),
			source(source), event(event),
			shift_down(shift_down), ctrl_down(ctrl_down), alt_down(alt_down)
		{ }


		const input_source::type source;
		const input_source_event_type event;

		const bool shift_down;
		const bool ctrl_down;
		const bool alt_down;
};




/**
 *************************************************************************
 *
 * @class gui_mouse_event
 * @ingroup gui_module
 *
 * Sent to gui_element%s on mouse actions.
 *
 * Usage notes:
 * - Mouse events occur even if the user does not click a mouse button,
 *		in order to notify controls about cursor movement. In this case,
 *		@bref{button} reads @bref{no_button}, and @bref{event} reads
 *		@bref{cursor_move}.
 * - GUI mouse events derive directly from input_context input events
 *		and thus may be inconsistent (e.g. release without prior click).
 *
 ************************************************************************/
class gui_mouse_event :
	public gui_input_event
{

	public:

		enum mouse_button_type
			{ no_button, left_button, right_button, middle_button };

		enum mouse_event_type
			{ cursor_move, button_click, button_release };


		gui_mouse_event
			(bool hot, const vector2f& cursor,
			 mouse_button_type button, mouse_event_type event,
			 bool shift_down, bool ctrl_down, bool alt_down)
			:
			gui_input_event(hot),
			cursor(cursor),
			button(button), event(event),
			shift_down(shift_down), ctrl_down(ctrl_down), alt_down(alt_down)
		{ }


		const vector2f cursor;

		const mouse_button_type button;
		const mouse_event_type event;

		const bool shift_down;
		const bool ctrl_down;
		const bool alt_down;
};




/**
 *************************************************************************
 *
 * @class gui_mouse_wheel_event
 * @ingroup gui_module
 *
 * Sent to gui_element%s on movement of the mouse wheel.
 *
 * Usage notes:
 * - The @bref{wheel_movement} has an implementation-specific sensitivity.
 *		Clients should divide movement by @bref{wheel_movement_per_line}
 *		to translate to consistent scrolling.
 *
 ************************************************************************/
class gui_mouse_wheel_event :
	public gui_input_event
{

	public:

		gui_mouse_wheel_event
			(bool hot, const vector2f& cursor,
			 float wheel_movement, float wheel_movement_per_line,
			 bool shift_down, bool ctrl_down, bool alt_down)
			:
			gui_input_event(hot),
			cursor(cursor),
			wheel_movement(wheel_movement),
			wheel_movement_per_line(wheel_movement_per_line),
			shift_down(shift_down), ctrl_down(ctrl_down), alt_down(alt_down)
		{ }


		const vector2f cursor;

		const float wheel_movement;
		const float wheel_movement_per_line;

		const bool shift_down;
		const bool ctrl_down;
		const bool alt_down;
};




/**
 *************************************************************************
 *
 * @class gui_text_event
 * @ingroup gui_module
 *
 * Sent to gui_element%s on high-level text_input_event%s.
 *
 ************************************************************************/
class gui_text_event :
	public gui_input_event
{

	public:

		gui_text_event
			(bool hot,
			 const text_input_event& text_input)
			:
			gui_input_event(hot),
			text_input(text_input)
		{ }

		const text_input_event text_input;
};




/**
 *************************************************************************
 *
 * @class gui_output_event
 * @ingroup gui_module
 *
 * Generated by gui_element%s as response to gui_input_event%s.
 *
 * Usage notes:
 * - Unless noted otherwise, client code does not trigger output events
 *		when working with a gui_element (e.g. through element setters).
 *		Notably, client event handling cannot cause endless looping
 *		when changing gui_element%s as reaction to some output event.
 *
 ************************************************************************/
class gui_output_event
{
	public:

		virtual ~gui_output_event() NOTHROW { }

		const shared_instance<gui_element> source_untyped;

	protected:

		gui_output_event(gui_element& source_untyped);
};


/**
 * An ordered list of gui_output_event%s.
 * @ingroup gui_module
 */
typedef owning_list<gui_output_event> gui_output_event_list;




/**
 *************************************************************************
 *
 * @class gui_simple_output_event
 * @ingroup gui_module
 *
 * A simple output event that links to a @bref{source} gui_element
 * of subtype @bref{T}, but does not hold any additional data.
 *
 * Usage notes:
 * - One can attach an integral enumerator tag @bref{E} to distinguish
 *		multiple event classes for a single gui_element type @bref{T}.
 *
 ************************************************************************/
template <typename T, int64 E>
	class gui_simple_output_event : public gui_output_event
{
	public:

		gui_simple_output_event(T& source)
			:
			gui_output_event(source),
			source(source.fetch_shared_from_parent()
						 .try_cast<T>()
						 .instance_or_throw())
		{ }
		
		const shared_instance<T> source;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_EVENT_HPP) */