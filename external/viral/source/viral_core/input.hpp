/**
 *************************************************************************
 *
 * @file input.hpp
 *
 * General input facilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__INPUT_HPP)
#define INCLUDED__VIRAL_CORE__INPUT_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup input_module Input Module
 *
 * Grants OS- and device-independent access to keyboard, mouse,
 * and game controllers through the input_context interface.
 * 
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class input_source
 * @ingroup input_module
 *
 * Defines device-independent input source constants.
 *
 * Notes:
 * - Consecutive axi or buttons always have consecutive enum values.
 *		For instance, j2_axis3 is j2_axis0 + 3, and k_f is k_a + 5.
 *
 ************************************************************************/
class input_source :
	private non_constructable
{

	public:

		/** See input_source class. */
		enum type
		{

			/**
			 * This input source never creates any input,
			 * and may be used as an "unassigned source" value.
			 */	
			none,


			/** F keys */
			//@{
			k_f1, k_f2, k_f3, k_f4, k_f5, k_f6,
			k_f7, k_f8, k_f9, k_f10, k_f11, k_f12,
			//@}

			/** Number keys, upperhand side of the keyboard */
			//@{
			k_0, k_1, k_2, k_3, k_4, k_5,
			k_6, k_7, k_8, k_9,
			//@}

			/** Letter keys */
			//@{
			k_a, k_b, k_c, k_d, k_e, k_f, k_g, k_h,
			k_i, k_j, k_k, k_l, k_m, k_n, k_o, k_p,
			k_q, k_r, k_s, k_t, k_u, k_v, k_w, k_x,
			k_y, k_z,
			//@}


			/** Lowlevel, not localized sign keys: , . / ; ' [ ] - = \\ ~ <> */
			//@{
			k_comma, k_period, k_slash, k_semicolon, k_apostrophe,
			k_left_bracket, k_right_bracket,
			k_minus, k_equals, k_backslash, k_grave, k_angle_bracket,
			//@}

			/** Unified or side-specific modifiers, as supported by the OS. */
			//@{
			k_shift, k_control, k_alt,
			k_left_shift, k_right_shift,
			k_left_control, k_right_control,
			k_left_alt, k_right_alt,
			k_caps_lock,
			//@}

			/** Editing keys */
			//@{
			k_escape, k_space, k_tab, k_return, k_backspace,
			k_insert, k_home, k_page_up, k_delete, k_end, k_page_down,
			//@}

			/** Arrow keys */
			//@{
			k_up, k_down, k_left, k_right,
			//@}


			/** Numpad numbers */
			//@{
			k_num0, k_num1, k_num2, k_num3, k_num4, k_num5,
			k_num6, k_num7, k_num8, k_num9,
			//@}

			/** Numpad signs */
			//@{
			k_num_enter, k_num_plus, k_num_minus,
			k_num_multiply, k_num_divide, k_num_period,
			//@}


			/** Mouse relative movement, absolute position, buttons. */
			//@{
			m_axis0, m_axis1, m_axis2,
			m_absolute_x, m_absolute_y,
			m_button0, m_button1, m_button2, m_button3, m_button4,
			//@}


			/** First joystick / gamepad */
			//@{
			j0_axis0, j0_axis1, j0_axis2, j0_axis3,
			j0_axis4, j0_axis5, j0_axis6, j0_axis7,
			j0_button0, j0_button1, j0_button2, j0_button3,
			j0_button4, j0_button5, j0_button6, j0_button7,
			j0_button8, j0_button9, j0_button10, j0_button11,
			j0_button12, j0_button13, j0_button14, j0_button15,
			//@}

			/** Second joystick / gamepad */
			//@{
			j1_axis0, j1_axis1, j1_axis2, j1_axis3,
			j1_axis4, j1_axis5, j1_axis6, j1_axis7,
			j1_button0, j1_button1, j1_button2, j1_button3,
			j1_button4, j1_button5, j1_button6, j1_button7,
			j1_button8, j1_button9, j1_button10, j1_button11,
			j1_button12, j1_button13, j1_button14, j1_button15,
			//@}

			/** Third joystick / gamepad */
			//@{
			j2_axis0, j2_axis1, j2_axis2, j2_axis3,
			j2_axis4, j2_axis5, j2_axis6, j2_axis7,
			j2_button0, j2_button1, j2_button2, j2_button3,
			j2_button4, j2_button5, j2_button6, j2_button7,
			j2_button8, j2_button9, j2_button10, j2_button11,
			j2_button12, j2_button13, j2_button14, j2_button15,
			//@}

			/** Fourth joystick / gamepad */
			//@{
			j3_axis0, j3_axis1, j3_axis2, j3_axis3,
			j3_axis4, j3_axis5, j3_axis6, j3_axis7,
			j3_button0, j3_button1, j3_button2, j3_button3,
			j3_button4, j3_button5, j3_button6, j3_button7,
			j3_button8, j3_button9, j3_button10, j3_button11,
			j3_button12, j3_button13, j3_button14, j3_button15,
			//@}


			/** Total number of sources */
			count
		};
};


typedef input_source::type input_source_type;
typedef tree_map<input_source_type, string> input_source_name_map;




/**
 *************************************************************************
 *
 * @class text_input_event
 * @ingroup input_module
 *
 * A single input event within a text_input_buffer,
 * corresponding to some user text editing action.
 *
 ************************************************************************/
class text_input_event
{

	public:


		/**
		 * Extended events do not correspond to character entry,
		 * but control the editing application.
		 */
		enum extended_event_type
		{
			no_extended_event,

			cursor_left_event, cursor_right_event, cursor_up_event, cursor_down_event,
			space_event, tab_event, backspace_event, return_event, escape_event,
			insert_event, delete_event, home_event, end_event, page_up_event, page_down_event,

			extended_event_count			
		};


		/**
		 * Modifiers that had been down at the time of a character event
		 * or extended event. Character events may be arbitrary proxies
		 * (e.g. all-uppercase) if control or alt are down.		 
		 */
		enum modifier_flags
			{ modifier_shift = 1, modifier_control = 2, modifier_alt = 4 };


		/**
		 * Create a character typing event for the given @bref{localized_char}
		 * with a typematic @bref{repeat_count}, an implicit extended event type
		 * of @bref{no_extended_event}, and some @bref{modifiers} state.
		 */
		text_input_event
			(char localized_char,
			 int64 repeat_count,
			 char modifiers)
			:
			extended_event(no_extended_event), localized_char(localized_char),
			repeat_count(repeat_count), modifiers(modifiers)
		{ }


		/**
		 * Create an extended editing action @bref{extended_event} with
		 * a typematic @bref{repeat_count}, an associated character '\\0',
		 * and some @bref{modifiers} state.
		 */
		text_input_event
			(extended_event_type extended_event,
			 int64 repeat_count,
			 char modifiers)
			:
			extended_event(extended_event), localized_char('\0'),
			repeat_count(repeat_count), modifiers(modifiers)
		{ }


		extended_event_type extended_event;
		char localized_char;
		int64 repeat_count;
		char modifiers;
};




/**
 *************************************************************************
 *
 * @class input_context
 * @ingroup input_module
 *
 * Provides OS independent application input from keyboard,
 * mouse, and game controllers.
 *
 * Usage notes:
 * - Regular calls to input_context::update() retrieve new user input.
 * - In between update() calls, the queryable input remains constant.
 * - After a long time without any update()s, clients should employ
 *		an additional, standalone update() to clear buffered input.
 * - An input_context is threadsafe for an arbitrary number of threads.
 *		However, only one thread should update() to ensure consistency.
 *
 * Input values and states:
 * - Input values represent the elongation of an input_source from
 *		its rest position. Usually, input values are normalized to
 *		the [0, 1] or [-1, 1] range and zero-centered, where zero
 *		corresponds to the physical rest position of the device.
 * - @attent All input values could potentially exceed [-1, 1],
 *		and should be clamped manually before use.
 * - Input states classify whether an input_source currently is active
 *		(input value near 0) or inactive (input value near 1 or -1),
 *		corresponding to an implementation defined dead zone.
 * - Input values and input states consider but snapshots at the time
 *		input_context::update() was called, and may miss input for large
 *		update intervals.
 *
 * Input events:
 * - Input events test if an input source changed its input state
 *		at least once in between the last two update calls.
 * - Input events potentially provide a better resolution than input values
 *		and input states, as they can recognize events that happened entirely
 *		in between updates.
 * - @attent Input events need not be consistent. There might be directly
 *		consecutive activation or deactivation events on the same source,
 *		or input events might not be reflected in associated input values.
 *
 * General notes:
 * - Positive x and y axis values should by convention (see @ref camera)
 *		associate with device movement to the lower-right.
 * - Absolute mouse coordinates are given in pixels relative to the upper
 *		left corner of the screen. An explicit transformation to window
 *		coordinates is required for reasonable use within an application.
 *		The absolute mouse position does not produce states or events.
 * - Input sources provide a value of 0 and do not produce any input events
 *		if they are not connected, not available, or unsupported. Finally,
 *		an invalid input_source constant also gives zero values.
 * - Event-based, localized character input for text entry on the keyboard
 *		is enabled by text_input_event and input_context::text_input().
 *
 ************************************************************************/
class input_context :
	private non_copyable
{

	public:

		typedef dynamic_buffer<text_input_event> text_input_buffer;


		input_context(const string& context_name);
		virtual ~input_context() NOTHROW;


		/** Update the internal input state with new user input. */
		virtual void update() = 0;


		/**
		 * Retrieve input value or input state snapshot
		 * from the last update() call.
		 */
		//@{
		virtual float value_of(input_source_type source) const = 0;
		virtual bool is_inactive(input_source_type source) const = 0;
		virtual bool is_active(input_source_type source) const = 0;
		//@}


		/**
		 * Check whether an input event occurred
		 * in between the last two update()s.
		 */
		//@{
		virtual bool was_deactivated(input_source_type source) const = 0;
		virtual bool was_activated(input_source_type source) const = 0;
		//@}


		/**
		 * Retrieve localized user text input from in between
		 * the last two update()s into a @bref{text_input} buffer.
		 */
		virtual void get_text_input(text_input_buffer &text_input) const = 0;


		/**
		 * Set hint for freezing the cursor at its current position.
		 *
		 * The freeze hint automatically resets after each update(),
		 * thus must be specified perpetually to freeze the cursor.
		 * No freeze_mouse() is required to unfreeze, which avoids
		 * deadlocking the mouse cursor.
		 */
		virtual void freeze_mouse(bool freeze) = 0;


		/**
		 * Retrieve human-readable names for input_source_type constants.
		 * Names may be implementation- and locale-specific.
		 *
		 * Implementations need not return a value for each input source,
		 * e.g. if they do not support the respective source.
		 *
		 * The default implementation returns basic, english names.
		 */
		virtual input_source_name_map get_default_source_names() const;


		const string context_name;


	protected:

		static void safe_insert_source_name
			(input_source_name_map& map,
			 input_source_type value, const string& name);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__INPUT_HPP) */
