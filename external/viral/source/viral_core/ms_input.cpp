/**
 *************************************************************************
 *
 * @file ms_input.cpp
 * 
 * Microsoft Windows-specific input system, implementation.
 *
 ************************************************************************/


#include "ms_input.hpp"


#if defined(VIRAL__HAVE_MS_INPUT)


#include <math.h>
#include <limits.h>
#include <Xinput.h>


#include "log.hpp"
#include "exception.hpp"
#include "memory.hpp"
#include "scope_guard.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	ms_input_context
//
//////////////////////////////////////////////////////////////////////////


ms_input_context::ms_input_context
	(const string& context_name,
	 ms_window_context& window)
	:
	input_context(context_name),
	window_(window),
	window_handle_(window_.window_handle()),
	freeze_mouse_(false)
{
	LOG_INFO("Using ms_input_context implementation.");


	// We might need the window_handle_
	// for future DirectInput support.
	if (window_handle_ == NULL)
	{
		LOG_ERROR("Invalid HWND handle inside window.");
		throw invalid_parameter();
	}	


	memory::fill_with_zero_typed
		(input_values_, input_source::count);
	memory::fill_with_zero_typed
		(input_activated_, input_source::count);
	memory::fill_with_zero_typed
		(input_deactivated_, input_source::count);
	memory::fill_with_zero_typed
		(mouse_axis_accum_, mouse_axis_count_);
	memory::fill_with_zero_typed
		(xinput_joystick_connected_, xinput_joystick_count_);

	init_virtual_key_to_input_source_map();
	init_virtual_key_to_extended_event_map();
	init_virtual_key_to_char_proxy_map();

	update_mouse();
	update_joysticks_reconnect();
	update_joysticks();
}


ms_input_context::~ms_input_context() NOTHROW
{
	LOG_INFO("Destroying.");
	// Nothing to do here without DirectInput.
}


void ms_input_context::update()
{
	MUTEX_SCOPE(input_mutex_);

	reset_input();
	process_input_messages();
	update_mouse();
	update_joysticks();
}


float ms_input_context::value_of(input_source_type source) const
{
	MUTEX_SCOPE(input_mutex_);

	if (source <= input_source::none ||
		source >= input_source::count)
			return 0;

	return input_values_[source];
}


bool ms_input_context::is_inactive(input_source_type source) const
{
	MUTEX_SCOPE(input_mutex_);

	if (source <= input_source::none ||
		source >= input_source::count)
			return false;

	return input_values_[source] < 0.5f;
}


bool ms_input_context::is_active(input_source_type source) const
{
	MUTEX_SCOPE(input_mutex_);

	if (source <= input_source::none ||
		source >= input_source::count)
			return false;

	return input_values_[source] >= 0.5f;
}


bool ms_input_context::was_deactivated(input_source_type source) const
{
	MUTEX_SCOPE(input_mutex_);

	if (source <= input_source::none ||
		source >= input_source::count)
			return false;

	return input_deactivated_[source];
}


bool ms_input_context::was_activated(input_source_type source) const
{
	MUTEX_SCOPE(input_mutex_);

	if (source <= input_source::none ||
		source >= input_source::count)
			return false;

	return input_activated_[source];
}


void ms_input_context::get_text_input(text_input_buffer &text_input) const
{
	MUTEX_SCOPE(input_mutex_);
	text_input = text_input_;
}


void ms_input_context::freeze_mouse(bool freeze)
{
	MUTEX_SCOPE(input_mutex_);
	freeze_mouse_ = freeze;
}


void ms_input_context::reset_input()
{
	// Don't reset our current input values here. These remain
	// valid for message-based sources until a new value arrives.
	// Furthermore, we require cached values from the previous
	// update() to build certain input events.

	memory::fill_with_zero_typed
		(input_activated_, input_source::count);
	memory::fill_with_zero_typed
		(input_deactivated_, input_source::count);

	memory::fill_with_zero_typed
		(mouse_axis_accum_, mouse_axis_count_),

	text_input_.clear();
}


void ms_input_context::process_input_messages()
{
	window_.input_messages(input_messages_);

	for (int64 i = 0; i < input_messages_.size(); ++i)
	{
		UINT code = input_messages_[i].message_code;

		switch (code)
		{
			case WM_KEYDOWN: case WM_SYSKEYDOWN:
			case WM_KEYUP: case WM_SYSKEYUP:
				process_key_message(input_messages_[i]);
			break;

			case WM_CHAR:
				process_char_message(input_messages_[i]);
			break;

			case WM_USER_INPUT_MOUSE_BUTTON:
				process_raw_mouse_button_message(input_messages_[i]);
			break;

			case WM_USER_INPUT_MOUSE_AXIS:
				process_raw_mouse_axis_message(input_messages_[i]);
			break;

			case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
			case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: case WM_MBUTTONUP:
			case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: case WM_RBUTTONUP:
				process_win_mouse_button_message(input_messages_[i]);			
			break;

			case WM_MOUSEWHEEL:
				process_win_mouse_wheel_message(input_messages_[i]);
			break;

			case WM_DEVICECHANGE:
				update_joysticks_reconnect();
			break;
		}
	}
}


void ms_input_context::process_key_message(const ms_window_message& msg)
{
	/**
	 * Translate a given WM_KEYDOWN / KEYUP / SYSKEYDOWN / SYSKEYUP
	 * message into input values, input events, extended text input,
	 * and proxy characters for system keys or modifier input.
	 */

	UINT code = msg.message_code;
	WPARAM key = msg.word_param;
	LPARAM state = msg.long_param;

	bool keydown =
		(code == WM_KEYDOWN || code == WM_SYSKEYDOWN);

	static const LPARAM repeat_count_bits = 0x000000ff;
	uint8 repeat_count =
		static_cast<uint8>(state && repeat_count_bits);

	static const LPARAM extendedkeyflag = (1 << 24);
	bool isextendedkey =
		(state & extendedkeyflag) != 0;

	char modifiers =
		collect_modifiers_state();

	if (keydown)
	{
		text_input_event::extended_event_type extevent =
			map_virtual_key_to_extended_event(key);
	
		if (extevent != text_input_event::no_extended_event)
			add_text_input_event
				(text_input_event(extevent, repeat_count, modifiers));

		if ((modifiers & text_input_event::modifier_control) != 0 ||
			(modifiers & text_input_event::modifier_alt) != 0)
		{
			char char_proxy =
				map_virtual_key_to_char_proxy(key);

			if (char_proxy != '\0')
				add_text_input_event
					(text_input_event
						(char_proxy, repeat_count, modifiers));
		}
	}

	input_source_type source = 
		(isextendedkey && key == VK_RETURN) ? // Distinguish numpad return from enter key.
			input_source::k_num_enter :
			map_virtual_key_to_input_source(key);

	float value = keydown ? 1.f : 0.f;

	update_source(source, value);
}


void ms_input_context::process_char_message(const ms_window_message& msg)
{
	/**
	 * Add incoming character to our text input buffer.
	 *
	 * We ignore system keys and keys with control or alt modifiers,
	 * as these carry bogus character messages. Instead, we create
	 * proxy character events for these in process_key_message().
	 *
	 * We likewise ignore characters produced by extended_event_types. 
	 */

	char localized_char =
		static_cast<char>(msg.word_param);

	if (localized_char == '\0' || localized_char == ' ' ||
		localized_char == '\t' || localized_char == '\r' ||
		localized_char == '\n' || localized_char == '\b')
			return;

	static const LPARAM repeat_count_bits = 0x000000ff;
	uint8 repeat_count =
		static_cast<uint8>
			(msg.long_param && repeat_count_bits);

	char modifiers =
		collect_modifiers_state();

	if ((modifiers & text_input_event::modifier_control) != 0 ||
		(modifiers & text_input_event::modifier_alt) != 0)
			return;

	add_text_input_event
		(text_input_event(localized_char, repeat_count, 0));
}


void ms_input_context::process_raw_mouse_button_message
	(const ms_window_message& msg)
{
	/**
	 * Process a raw mouse button notification.
	 * There may be several incoming button clicks,
	 * which reflects both in multiple source updates
	 * and in multiple input event toggles.
	 */

	WPARAM button_index = msg.word_param;

	if (button_index < 0 ||
		button_index >= mouse_button_count_)
			return;

	LPARAM button_value = msg.long_param;

	static const input_source_type source_map[mouse_button_count_] =
		{ input_source::m_button0, input_source::m_button1,
		  input_source::m_button2, input_source::m_button3,
		  input_source::m_button4 };

	float value =
		(button_value != 0) ? 1.f : 0.f;

	update_source
		(source_map[button_index], value);
}


void ms_input_context::process_raw_mouse_axis_message
	(const ms_window_message& msg)
{
	/**
	 * Process an incoming, raw mouse axis notification.
	 * Notifications are additive, thus only accumulated
	 * and handled as a total on update_mouse().
	 */

	WPARAM axis_index = msg.word_param;

	if (axis_index < 0 ||
		axis_index >= mouse_axis_count_)
			return;

	LPARAM axis_value = msg.long_param;
	mouse_axis_accum_[axis_index] +=
		static_cast<int64>(axis_value);
}


void ms_input_context::process_win_mouse_button_message
	(const ms_window_message& msg)
{
	/**
	 * Process a traditional mouse button notification.
	 * There may be several incoming button clicks,
	 * which reflects both in multiple source updates
	 * and in multiple input event toggles.
	 */

	switch (msg.message_code)
	{
		case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK:
			update_source(input_source::m_button0, 1);
		break;
		case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK:
			update_source(input_source::m_button2, 1);
		break;
		case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK:
			update_source(input_source::m_button1, 1);
		break;

		case WM_LBUTTONUP:
			update_source(input_source::m_button0, 0);
		break;
		case WM_MBUTTONUP:
			update_source(input_source::m_button2, 0);
		break;
		case WM_RBUTTONUP:
			update_source(input_source::m_button1, 0);
		break;
	}
}


void ms_input_context::process_win_mouse_wheel_message
	(const ms_window_message& msg)
{
	/**
	 * Process a traditional mouse wheel notification.
	 * Wheel movements are additive, thus only accumulated
	 * and handled as a total on update_mouse().
	 */

	if (2 >= mouse_axis_count_)
		return;

	if (msg.message_code != WM_MOUSEWHEEL)
		return;

	mouse_axis_accum_[2] +=
		GET_WHEEL_DELTA_WPARAM(msg.word_param);
}


void ms_input_context::update_mouse()
{
	// First, handle absolute mouse position and mouse freezing
	// directly via the Windows API (i.e. without raw input).
	// We also accumulate a rather quirky relative mouse movement
	// from pre-freeze cursor positions if we have no raw input.
	bool foreground =
		(GetForegroundWindow() == window_.window_handle());

	POINT pending_cursor_position;
	GetCursorPos(&pending_cursor_position);

	if (!window_.use_raw_input)
	{
		mouse_axis_accum_[0] += 
			pending_cursor_position.x - current_cursor_position_.x;
		mouse_axis_accum_[1] += 
			pending_cursor_position.y - current_cursor_position_.y;
	}

	if (freeze_mouse_ && foreground)
	{
		SetCursorPos
			(current_cursor_position_.x, current_cursor_position_.y);
	}
	else
	{
		current_cursor_position_ = pending_cursor_position;

		input_values_[input_source::m_absolute_x] =
			static_cast<float>(current_cursor_position_.x);
		input_values_[input_source::m_absolute_y] =
			static_cast<float>(current_cursor_position_.y);
	}

	freeze_mouse_ = false;


	// Next, store the completely accumulated, relative mouse movement
	// resulting from raw input, wheel messages, and cursor positions.
	// We must flip the y axis to match framework conventions.
	static const input_source_type axistosource[mouse_axis_count_] =
		{ input_source::m_axis0, input_source::m_axis1,
		  input_source::m_axis2 };
	static const float axisscale[mouse_axis_count_] =
		{ 1, -1, 1 };

	for (int32 i = 0; i < mouse_axis_count_; ++i)
	{
		input_source_type source = axistosource[i];
		float value = mouse_axis_accum_[i] * axisscale[i];
		update_source(source, value);
	}
}


void ms_input_context::update_joysticks_reconnect()
{
	/**
	 * Recognize (re-)connected XInput joysticks
	 * on context construction or WM_DEVICECHANGE.
	 */

	for (uint32 i = 0; i < xinput_joystick_count_; ++i)
	{
		XINPUT_STATE dummy_state;
		memory::fill_with_zero_typed(&dummy_state, 1);

		xinput_joystick_connected_[i] =
			(XInputGetState(i, &dummy_state) == ERROR_SUCCESS);
	}
}


void ms_input_context::update_joysticks()
{
	/**
	 * Fetch input from currently connected XInput joysticks.
	 *
	 * Notes:
	 * - We only query joysticks that we believe to be connected
	 *		(i.e. from update_joysticks_reconnect()). This avoids
	 *		expensive (>> 1ms) device enumeration in XInputGetState.
	 *		Likewise, we instantly stop querying joysticks if we find
	 *		they have been disconnected.
	 * - We handle all disconnected joysticks as if they had returned
	 *		zero input, so we can cancel out any preceding valid input.
	 */

	for (uint32 i = 0; i < xinput_joystick_count_; ++i)
	{

		// Fetch current joystick state.
		XINPUT_STATE state;
		memory::fill_with_zero_typed(&state, 1);

		if (xinput_joystick_connected_[i])
		{
			xinput_joystick_connected_[i] =
				(XInputGetState(i, &state) == ERROR_SUCCESS);

			if (!xinput_joystick_connected_[i])
				memory::fill_with_zero_typed(&state, 1);
		}


		// Update joystick axes.
		update_joystick_axis_pair
			(state.Gamepad.sThumbLX, state.Gamepad.sThumbLY,
			 XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE, SHRT_MAX,
			 i, 0, 1);
		update_joystick_axis_pair
			(state.Gamepad.sThumbRX, state.Gamepad.sThumbRY,
			 XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE, SHRT_MAX,
			 i, 2, 3);

		update_joystick_axis
			(state.Gamepad.bLeftTrigger,
			 XINPUT_GAMEPAD_TRIGGER_THRESHOLD, UCHAR_MAX,
			 i, 4);
		update_joystick_axis
			(state.Gamepad.bRightTrigger,
			 XINPUT_GAMEPAD_TRIGGER_THRESHOLD, UCHAR_MAX,
			 i, 5);


		// Update joystick buttons.
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_A, i, 0);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_B, i, 1);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_X, i, 2);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_Y, i, 3);

		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_THUMB, i, 4);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_THUMB, i, 5);

		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER, i, 6);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER, i, 7);

		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_START, i, 8);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_BACK, i, 9);

		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_UP, i, 10);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN, i, 11);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT, i, 12);
		update_joystick_button
			(state.Gamepad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT, i, 13);
	}
}


void ms_input_context::update_joystick_axis
	(float value, float deadzone, float maximum,
	 uint32 joy_index, uint32 axis_index)
{
	/**
	 * Normalize incoming value to [-1, 1] range, 
	 * so that 0 corresponds to +-deadzone,
	 * and that +-1 corresponds to +-maximum.
	 * Finally, apply value to input source.
	 */

	if (value > deadzone)
	{
		value = min(value, maximum);
		value = (value - deadzone) / (maximum - deadzone);
	}
	else if (value < -deadzone)
	{
		value = max(value, -maximum);
		value = -((-value) - deadzone) / (maximum - deadzone);
	}
	else
		value = 0;

	static const input_source_type joyindex_to_axis_base[xinput_joystick_count_] =
		{ input_source::j0_axis0, input_source::j1_axis0,
		  input_source::j2_axis0, input_source::j3_axis0 };

	input_source_type axis_src =
		static_cast<input_source_type>
			(joyindex_to_axis_base[joy_index] + axis_index);

	update_source(axis_src, value);
}


void ms_input_context::update_joystick_axis_pair
	(float x, float y, float deadzone, float maximum,
	 uint32 joy_index, uint32 xaxis_index, uint32 yaxis_index)
{
	/**
	 * Normalize the magnitude of the input vector (x, y)
	 * to [0, 1] range, respective x and y to [-1, 1]:
	 *		magnitude <= deadzone -> magnitude 0,
	 *		magnitude >= maximum -> magnitude 1,
	 *		magnitude in between -> linear from 0 to 1.
	 * Finally, apply normalized (x, y) to input sources.
	 * We flip y here to match framework conventions.
	 */

	float magnitude = sqrtf(x * x + y * y);

	if (magnitude > deadzone)
	{
		x /= magnitude; // create unit vector
		y /= magnitude;

		if (magnitude < maximum) // rescale to correct magnitude
		{
			x *= (magnitude - deadzone) / (maximum - deadzone);
			y *= (magnitude - deadzone) / (maximum - deadzone);
		}
	}
	else
		x = 0, y = 0;

	static const input_source_type joyindex_to_axis_base[xinput_joystick_count_] =
		{ input_source::j0_axis0, input_source::j1_axis0,
		  input_source::j2_axis0, input_source::j3_axis0 };

	input_source_type xaxis_src =
		static_cast<input_source_type>
			(joyindex_to_axis_base[joy_index] + xaxis_index);
	update_source(xaxis_src, x);

	input_source_type yaxis_src =
		static_cast<input_source_type>
			(joyindex_to_axis_base[joy_index] + yaxis_index);
	update_source(yaxis_src, -y);
}


void ms_input_context::update_joystick_button
	(WORD button_flags, WORD button_flag,
	 uint32 joy_index, uint32 button_index)
{
	static const input_source_type joyindex_to_button_base[xinput_joystick_count_] =
		{ input_source::j0_button0, input_source::j1_button0,
		  input_source::j2_button0, input_source::j3_button0 };

	input_source_type button_src =
		static_cast<input_source_type>
			(joyindex_to_button_base[joy_index] + button_index);

	float button_value =
		((button_flags & button_flag) != 0) ? 1.f : 0.f;

	update_source(button_src, button_value);		
}


void ms_input_context::update_source
	(input_source_type source, float value)
{
	/**
	 * Apply incoming and not necessarily normalized input value
	 * to any keyboard, mouse, or joystick button or axis, and
	 * update input events accordingly.
	 *
	 * Notes:
	 * - We might have to introduce a different dead zone for
	 *		input events at some point (e.g. >= 0.5f).
	 * - We cross-check the source against valid ranges to avoid
	 *		any enum <-> integer conversion errors.
	 */

	if (source <= input_source::none ||
		source >= input_source::count)
			return;

	if (input_values_[source] == 0 && value != 0)
		input_activated_[source] = true;

	if (input_values_[source] != 0 && value == 0)
		input_deactivated_[source] = true;
	
	input_values_[source] = value;
}


char ms_input_context::collect_modifiers_state() const
{
	char result = 0;

	if (input_values_[input_source::k_shift] != 0 ||
		input_values_[input_source::k_left_shift] != 0 ||
		input_values_[input_source::k_right_shift] != 0)
			result |= text_input_event::modifier_shift;

	if (input_values_[input_source::k_control] != 0 ||
		input_values_[input_source::k_left_control] != 0 ||
		input_values_[input_source::k_right_control] != 0)
			result |= text_input_event::modifier_control;

	if (input_values_[input_source::k_alt] != 0 ||
		input_values_[input_source::k_left_alt] != 0 ||
		input_values_[input_source::k_right_alt] != 0)
			result |= text_input_event::modifier_alt;

	return result;
}


void ms_input_context::add_text_input_event(const text_input_event &event)
{
	/**
	 * Insert a new text input event.
	 * A maximum event restriction avoids excess buffer
	 * sizes on many incoming events.
	 */

	if (text_input_.size() >= max_text_input_events_) return;
	text_input_.push_back(event);
}


void ms_input_context::init_virtual_key_to_input_source_map()
{
	/**
	 * Initialize constant lookup table that provides, for a given
	 * Windows-side virtual key code VK_****, the corresponding
	 * input_source_type, if any.
	 */


	// For current definitions, 256 chars are enough.
	// We still check with at() to stay safe.
	virtual_key_to_input_source_.resize(256);
	for (int32 i = 0; i < 256; ++i)
		virtual_key_to_input_source_.at(i) = input_source::none;


	// F keys
	virtual_key_to_input_source_.at(VK_F1) = input_source::k_f1;
	virtual_key_to_input_source_.at(VK_F2) = input_source::k_f2;
	virtual_key_to_input_source_.at(VK_F3) = input_source::k_f3;
	virtual_key_to_input_source_.at(VK_F4) = input_source::k_f4;
	virtual_key_to_input_source_.at(VK_F5) = input_source::k_f5;
	virtual_key_to_input_source_.at(VK_F6) = input_source::k_f6;
	virtual_key_to_input_source_.at(VK_F7) = input_source::k_f7;
	virtual_key_to_input_source_.at(VK_F8) = input_source::k_f8;
	virtual_key_to_input_source_.at(VK_F9) = input_source::k_f9;
	virtual_key_to_input_source_.at(VK_F10) = input_source::k_f10;
	virtual_key_to_input_source_.at(VK_F11) = input_source::k_f11;
	virtual_key_to_input_source_.at(VK_F12) = input_source::k_f12;


	// Number keys
	virtual_key_to_input_source_.at('0') = input_source::k_0;
	virtual_key_to_input_source_.at('1') = input_source::k_1;
	virtual_key_to_input_source_.at('2') = input_source::k_2;
	virtual_key_to_input_source_.at('3') = input_source::k_3;
	virtual_key_to_input_source_.at('4') = input_source::k_4;
	virtual_key_to_input_source_.at('5') = input_source::k_5;
	virtual_key_to_input_source_.at('6') = input_source::k_6;
	virtual_key_to_input_source_.at('7') = input_source::k_7;
	virtual_key_to_input_source_.at('8') = input_source::k_8;
	virtual_key_to_input_source_.at('9') = input_source::k_9;


	// Letter keys
	virtual_key_to_input_source_.at('A') = input_source::k_a;
	virtual_key_to_input_source_.at('B') = input_source::k_b;
	virtual_key_to_input_source_.at('C') = input_source::k_c;
	virtual_key_to_input_source_.at('D') = input_source::k_d;
	virtual_key_to_input_source_.at('E') = input_source::k_e;
	virtual_key_to_input_source_.at('F') = input_source::k_f;
	virtual_key_to_input_source_.at('G') = input_source::k_g;
	virtual_key_to_input_source_.at('H') = input_source::k_h;
	virtual_key_to_input_source_.at('I') = input_source::k_i;
	virtual_key_to_input_source_.at('J') = input_source::k_j;
	virtual_key_to_input_source_.at('K') = input_source::k_k;
	virtual_key_to_input_source_.at('L') = input_source::k_l;
	virtual_key_to_input_source_.at('M') = input_source::k_m;
	virtual_key_to_input_source_.at('N') = input_source::k_n;
	virtual_key_to_input_source_.at('O') = input_source::k_o;
	virtual_key_to_input_source_.at('P') = input_source::k_p;
	virtual_key_to_input_source_.at('Q') = input_source::k_q;
	virtual_key_to_input_source_.at('R') = input_source::k_r;
	virtual_key_to_input_source_.at('S') = input_source::k_s;
	virtual_key_to_input_source_.at('T') = input_source::k_t;
	virtual_key_to_input_source_.at('U') = input_source::k_u;
	virtual_key_to_input_source_.at('V') = input_source::k_v;
	virtual_key_to_input_source_.at('W') = input_source::k_w;
	virtual_key_to_input_source_.at('X') = input_source::k_x;
	virtual_key_to_input_source_.at('Y') = input_source::k_y;
	virtual_key_to_input_source_.at('Z') = input_source::k_z;


	// Signs on main keyboard
	virtual_key_to_input_source_.at(VK_OEM_COMMA) = input_source::k_comma;
	virtual_key_to_input_source_.at(VK_OEM_PERIOD) = input_source::k_period;
	virtual_key_to_input_source_.at(VK_OEM_2) = input_source::k_slash;
	virtual_key_to_input_source_.at(VK_OEM_1) = input_source::k_semicolon;
	virtual_key_to_input_source_.at(VK_OEM_7) = input_source::k_apostrophe;
	virtual_key_to_input_source_.at(VK_OEM_4) = input_source::k_left_bracket;
	virtual_key_to_input_source_.at(VK_OEM_6) = input_source::k_right_bracket;
	virtual_key_to_input_source_.at(VK_OEM_MINUS) = input_source::k_minus;
	virtual_key_to_input_source_.at(VK_OEM_PLUS) = input_source::k_equals;
	virtual_key_to_input_source_.at(VK_OEM_5) = input_source::k_backslash;
	virtual_key_to_input_source_.at(VK_OEM_3) = input_source::k_grave;
	virtual_key_to_input_source_.at(VK_OEM_102) = input_source::k_angle_bracket;


	// Modifiers
	// input_source::k_caps_lock is not available on Win32 as a normal key.
	// Other modifiers are only supported in the side-agnostic versions.
	virtual_key_to_input_source_.at(VK_SHIFT) = input_source::k_shift;
	virtual_key_to_input_source_.at(VK_CONTROL) = input_source::k_control;
	virtual_key_to_input_source_.at(VK_MENU) = input_source::k_alt;


	// Editing keys
	virtual_key_to_input_source_.at(VK_ESCAPE) = input_source::k_escape;
	virtual_key_to_input_source_.at(VK_SPACE) = input_source::k_space;
	virtual_key_to_input_source_.at(VK_TAB) = input_source::k_tab;
	virtual_key_to_input_source_.at(VK_RETURN) = input_source::k_return;
	virtual_key_to_input_source_.at(VK_BACK) = input_source::k_backspace;

	virtual_key_to_input_source_.at(VK_INSERT) = input_source::k_insert;
	virtual_key_to_input_source_.at(VK_DELETE) = input_source::k_delete;
	virtual_key_to_input_source_.at(VK_HOME) = input_source::k_home;
	virtual_key_to_input_source_.at(VK_END) = input_source::k_end;
	virtual_key_to_input_source_.at(VK_PRIOR) = input_source::k_page_up;
	virtual_key_to_input_source_.at(VK_NEXT) = input_source::k_page_down;


	// Arrow keys
	virtual_key_to_input_source_.at(VK_UP) = input_source::k_up;
	virtual_key_to_input_source_.at(VK_DOWN) = input_source::k_down;
	virtual_key_to_input_source_.at(VK_LEFT) = input_source::k_left;
	virtual_key_to_input_source_.at(VK_RIGHT) = input_source::k_right;


	// Numpad numbers
	virtual_key_to_input_source_.at(VK_NUMPAD0) = input_source::k_num0;
	virtual_key_to_input_source_.at(VK_NUMPAD1) = input_source::k_num1;
	virtual_key_to_input_source_.at(VK_NUMPAD2) = input_source::k_num2;
	virtual_key_to_input_source_.at(VK_NUMPAD3) = input_source::k_num3;
	virtual_key_to_input_source_.at(VK_NUMPAD4) = input_source::k_num4;
	virtual_key_to_input_source_.at(VK_NUMPAD5) = input_source::k_num5;
	virtual_key_to_input_source_.at(VK_NUMPAD6) = input_source::k_num6;
	virtual_key_to_input_source_.at(VK_NUMPAD7) = input_source::k_num7;
	virtual_key_to_input_source_.at(VK_NUMPAD8) = input_source::k_num8;
	virtual_key_to_input_source_.at(VK_NUMPAD9) = input_source::k_num9;


	// Numpad signs
	// input_source::k_num_enter is reported as VK_RETURN, and can't be separated
	// from input_source::k_return at this level. This is done externally by
	// the extended keyboard flag.
	virtual_key_to_input_source_.at(VK_ADD) = input_source::k_num_plus;
	virtual_key_to_input_source_.at(VK_SUBTRACT) = input_source::k_num_minus;
	virtual_key_to_input_source_.at(VK_MULTIPLY) = input_source::k_num_multiply;
	virtual_key_to_input_source_.at(VK_DIVIDE) = input_source::k_num_divide;
	virtual_key_to_input_source_.at(VK_DECIMAL) = input_source::k_num_period;
}


input_source_type ms_input_context::map_virtual_key_to_input_source
	(WPARAM virtual_key) const
{
	int32 intkey = static_cast<int32>(virtual_key);

	// Check to catch non-standard (and thus unsupported)
	// virtual key codes that Windows may send to us.
	if (intkey < 0 ||
		intkey >= virtual_key_to_input_source_.size())
			return input_source::none;

	return virtual_key_to_input_source_[intkey];
}


void ms_input_context::init_virtual_key_to_extended_event_map()
{
	/**
	 * Initialize constant lookup table that provides, for a given
	 * Windows-side virtual key code VK_****, the corresponding
	 * extended_event_type, if any.
	 */


	// For current definitions, 256 chars are enough.
	// We still check with at() to stay safe.
	virtual_key_to_extended_event_.resize(256);
	for (int32 i = 0; i < 256; ++i)
		virtual_key_to_extended_event_.at(i) = text_input_event::no_extended_event;

	virtual_key_to_extended_event_.at(VK_LEFT) = text_input_event::cursor_left_event;
	virtual_key_to_extended_event_.at(VK_RIGHT) = text_input_event::cursor_right_event;
	virtual_key_to_extended_event_.at(VK_UP) = text_input_event::cursor_up_event;
	virtual_key_to_extended_event_.at(VK_DOWN) = text_input_event::cursor_down_event;

	virtual_key_to_extended_event_.at(VK_SPACE) = text_input_event::space_event;
	virtual_key_to_extended_event_.at(VK_TAB) = text_input_event::tab_event;
	virtual_key_to_extended_event_.at(VK_BACK) = text_input_event::backspace_event;
	virtual_key_to_extended_event_.at(VK_RETURN) = text_input_event::return_event;
	virtual_key_to_extended_event_.at(VK_ESCAPE) = text_input_event::escape_event;

	virtual_key_to_extended_event_.at(VK_INSERT) = text_input_event::insert_event;
	virtual_key_to_extended_event_.at(VK_DELETE) = text_input_event::delete_event;
	virtual_key_to_extended_event_.at(VK_HOME) = text_input_event::home_event;
	virtual_key_to_extended_event_.at(VK_END) = text_input_event::end_event;
	virtual_key_to_extended_event_.at(VK_PRIOR) = text_input_event::page_up_event;
	virtual_key_to_extended_event_.at(VK_NEXT) = text_input_event::page_down_event;
}


text_input_event::extended_event_type
	ms_input_context::map_virtual_key_to_extended_event
		(WPARAM virtual_key) const
{
	int32 intkey = static_cast<int32>(virtual_key);

	// Check to catch non-standard (and thus unsupported)
	// virtual key codes that Windows may send to us.
	if (intkey < 0 ||
		intkey >= virtual_key_to_extended_event_.size())
			return text_input_event::no_extended_event;

	return virtual_key_to_extended_event_[intkey];
}


void ms_input_context::init_virtual_key_to_char_proxy_map()
{
	/**
	 * Initialize constant lookup table that provides, for a given
	 * Windows-side virtual key code VK_****, a representable
	 * proxy character type, if any, to recognize hotkey strokes.
	 */


	// For current definitions, 256 chars are enough.
	// We still check with at() to stay safe.
	virtual_key_to_char_proxy_.resize(256);
	for (int32 i = 0; i < 256; ++i)
		virtual_key_to_char_proxy_.at(i) = '\0';


	// Numbers and letters.
	for (char c = '0'; c <= '9'; ++c)
		virtual_key_to_char_proxy_.at(c) = c;

	for (char c = 'A'; c <= 'Z'; ++c)
		virtual_key_to_char_proxy_.at(c) = c;


	// Signs on main keyboard.
	virtual_key_to_char_proxy_.at(VK_OEM_COMMA) = ',';
	virtual_key_to_char_proxy_.at(VK_OEM_PERIOD) = '.';
	virtual_key_to_char_proxy_.at(VK_OEM_2) = '/';
	virtual_key_to_char_proxy_.at(VK_OEM_1) = ';';
	virtual_key_to_char_proxy_.at(VK_OEM_7) = '\'';
	virtual_key_to_char_proxy_.at(VK_OEM_4) = '[';
	virtual_key_to_char_proxy_.at(VK_OEM_6) = ']';
	virtual_key_to_char_proxy_.at(VK_OEM_MINUS) = '-';
	virtual_key_to_char_proxy_.at(VK_OEM_PLUS) = '=';
	virtual_key_to_char_proxy_.at(VK_OEM_5) = '\\';
	virtual_key_to_char_proxy_.at(VK_OEM_3) = '^';
	virtual_key_to_char_proxy_.at(VK_OEM_102) = '<';
}


char ms_input_context::map_virtual_key_to_char_proxy
	(WPARAM virtual_key) const
{
	int32 intkey = static_cast<int32>(virtual_key);

	// Check to catch non-standard (and thus unsupported)
	// virtual key codes that Windows may send to us.
	if (intkey < 0 ||
		intkey >= virtual_key_to_char_proxy_.size())
			return '\0';

	return virtual_key_to_char_proxy_[intkey];
}




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_INPUT) */
