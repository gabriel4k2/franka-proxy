/**
 *************************************************************************
 *
 * @file ms_input.hpp
 * 
 * Microsoft Windows-specific input system.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_INPUT_HPP)
#define INCLUDED__VIRAL_CORE__MS_INPUT_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_INPUT)


#include "thread_util.hpp"
#include "ms_window.hpp"
#include "input.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_input_context
 * @ingroup input_module
 *
 * An input_context implementation for use with Microsoft Windows.
 *
 * Implementation notes:
 * - Input on Microsoft Windows employs the Win32 API and the XInput API.
 * - Keyboard and mouse input is mostly handled through window messages
 *		that have accumulated on a construction-bound ms_window_context
 *		in between the last two calls to window_context::update().
 *
 * Known quirks:
 * - Left and right modifiers (shift, control, alt) are not distinguished
 *		correctly, and produce small glitches if pressed at the same time.
 * - The AltGr key translates into a simultaneus event on control and alt.
 * - If numlock is off, numpad keys report as their non-numpad counterpart.
 * - WM_*KEYUP events are lost if a key is held down and the user alt-tabs
 *		at the same time. The input source is not updated, but returns to
 *		working order once the user hits the key again.
 * - Mouse relative axi are denormalized, since the device-specific maximum
 *		values can't be queried.
 * - XInput uses polling instead of events, and thus might miss events
 *		at low update rates.
 *
 ************************************************************************/
class ms_input_context :
	public input_context

{
	public:

		/**
		 * Create input context that collaborates with some @bref{window}.
		 * The window must remain valid for the entire input_context lifetime.
		 * At most a single ms_input_context may be bound to a ms_window_context.
		 * Otherwise, input may not be recognized correctly.
		 */
		ms_input_context
			(const string& context_name,
			 ms_window_context& window);


		~ms_input_context() NOTHROW override;


		void update() override;

		float value_of(input_source_type source) const override;
		bool is_inactive(input_source_type source) const override;
		bool is_active(input_source_type source) const override;

		bool was_deactivated(input_source_type source) const override;
		bool was_activated(input_source_type source) const override;

		void get_text_input(text_input_buffer &text_input) const override;

		void freeze_mouse(bool freeze) override;


	private:

		void reset_input();

		void process_input_messages();
		void process_key_message(const ms_window_message& msg);
		void process_char_message(const ms_window_message& msg);
		void process_raw_mouse_button_message(const ms_window_message& msg);
		void process_raw_mouse_axis_message(const ms_window_message& msg);
		void process_win_mouse_button_message(const ms_window_message& msg);
		void process_win_mouse_wheel_message(const ms_window_message& msg);

		void update_mouse();

		void update_joysticks_reconnect();
		void update_joysticks();
		void update_joystick_axis
			(float value, float deadzone, float maximum,
			 uint32 joy_index, uint32 axis_index);
		void update_joystick_axis_pair
			(float x, float y, float deadzone, float maximum,
			 uint32 joy_index, uint32 xaxis_index, uint32 yaxis_index);
		void update_joystick_button
			(WORD button_flags, WORD button_flag,
			 uint32 joy_index, uint32 button_index);

		void update_source(input_source_type source, float value);

		char collect_modifiers_state() const;
		void add_text_input_event(const text_input_event &event);

		void init_virtual_key_to_input_source_map();
		input_source_type map_virtual_key_to_input_source(WPARAM virtual_key) const;

		void init_virtual_key_to_extended_event_map();
		text_input_event::extended_event_type
			map_virtual_key_to_extended_event(WPARAM virtual_key) const;

		void init_virtual_key_to_char_proxy_map();
		char map_virtual_key_to_char_proxy(WPARAM virtual_key) const;


		mutable mutex input_mutex_;

		ms_window_context& window_;
		HWND window_handle_;

		float input_values_[input_source::count];
		bool input_activated_[input_source::count];
		bool input_deactivated_[input_source::count];

		ms_window_context::input_message_buffer input_messages_;

		bool freeze_mouse_;
		POINT current_cursor_position_;	

		static const int32 mouse_button_count_ = 5;
		static const int32 mouse_axis_count_ = 3;
		int64 mouse_axis_accum_[mouse_axis_count_];

		static const int32 xinput_joystick_count_ = 4;
		bool xinput_joystick_connected_[xinput_joystick_count_];

		static const int64 max_text_input_events_ = 1024;
		text_input_buffer text_input_;

		dynamic_buffer<input_source_type> virtual_key_to_input_source_;
		dynamic_buffer<text_input_event::extended_event_type> virtual_key_to_extended_event_;
		dynamic_buffer<char> virtual_key_to_char_proxy_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_INPUT) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_INPUT_HPP) */
