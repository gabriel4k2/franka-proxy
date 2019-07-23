/**
 *************************************************************************
 *
 * @file ms_window.hpp
 * 
 * Microsoft Windows-specific application window.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_WINDOW_HPP)
#define INCLUDED__VIRAL_CORE__MS_WINDOW_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_WINDOW)


#include <windows.h>


#include "buffer.hpp"
#include "geo_2d.hpp"
#include "thread.hpp"
#include "window.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_window_message
 * @ingroup window_module
 *
 * Stores a Microsoft Windows message, as accepted by a window procedure.
 *
 ************************************************************************/
class ms_window_message
{

	public:

		ms_window_message
			(HWND target_window, UINT message_code,
			 WPARAM word_param, LPARAM long_param)
			:
			target_window(target_window), message_code(message_code),
			word_param(word_param), long_param(long_param)
		{ }

		HWND target_window;
		UINT message_code;
		WPARAM word_param;
		LPARAM long_param;
};


/**
 * Special user defined messages that are forwarded to ms_input_context
 * instead of WM_INPUT messages from the mouse.
 *
 * Parameter usage:
 * - @bref{WPARAM} provides the index of the axis or button.
 * - @bref{LPARAM} provides the relative mouse movement,
 *		or the input value if a button event occurred.
 */
//@{
#define WM_USER_INPUT_MOUSE_AXIS		WM_USER + 1
#define WM_USER_INPUT_MOUSE_BUTTON		WM_USER + 2
//@}




/**
 *************************************************************************
 *
 * @class ms_window_state
 * @ingroup window_module
 *
 * Utility class that holds the state of a ms_window_context.
 *
 ************************************************************************/
class ms_window_state
{
	friend class ms_window_context;

	private:

		typedef dynamic_buffer<ms_window_message> input_message_buffer;

		ms_window_state
			(const string& caption,
			 const vector2f& position,
			 const vector2f& size)
			:
			caption_(caption), position_(position), size_(size),
	 		display_flags_(window_context::hide),
			notify_flags_(0)
		{ }

		string caption_;

		vector2f position_;
		vector2f size_;
		uint32 display_flags_;

		uint32 notify_flags_;
		input_message_buffer input_;
};




/**
 *************************************************************************
 *
 * @class ms_window_context
 * @ingroup window_module
 *
 * Threaded window_context for use on Microsoft Windows.
 *
 * Fullscreen windows and display resolutions:
 * - An ms_window_context does not implicitly change display resolutions
 *		if display_flag::fullscreen is set. Instead, fullscreen windows
 *		fill the entire screen at the current resolution. This avoids
 *		certain problems with multiple windows, multiple displays, and
 *		badly-scaling devices (e.g. Intel GPUs or low-end displays).
 * - Clients should perform offscreen rendering on custom resolutions
 *		and finally blit to a fullscreen window. If required, clients
 *		must change resolutions via Win32 API utilities.
 *
 * Raw input considerations:
 * - Usually, clients should specify @bref{use_raw_input} as @true
 *		to enable the modern raw input API (i.e. WM_INPUT messages)
 *		for glitchless mouse handling.
 * - Only a single ms_window_context at a time may @bref{use_raw_input}.
 *		Otherwise, new window contexts steal any raw input registration.
 * - Raw input fails for certain virtual devices (e.g. remote desktops).
 *		Those devices send no WM_INPUT, but traditional mouse messages.
 * - To solve above problems, clients set @bref{use_raw_input} to @false.
 *		In this case, ms_window_context and ms_input_context synthesize
 *		mouse input from traditional mouse messages and cursor positions.
 *		This may cause various input quirks (e.g. with high-DPI mouses).
 *
 * Implementation-specific notes:
 * - @attent The construction-provided context name is used for various
 *		internal window properties and must be unique over all current
 *		ms_window_context instances.
 * - The window icon must reside within "<resource_base_name>_icon.ico"
 *		stored as a resource in the program executable.
 * - In the Windows API, only the creator thread can communicate with
 *		a window and process its messages. Thus, we use an internal thread
 *		to initialize and create the application window. Then, we allow
 *		general threads to control the window by replicating any operations
 *		within the internal window thread.
 *
 ************************************************************************/
class ms_window_context :
	public window_context,
	private threaded_task
{

	public:

		typedef ms_window_state::input_message_buffer input_message_buffer;


		/**
		 * Create a new window with an unique @bref{context_name},
		 * an initial @bref{caption}, and a @bref{resource_base_name}
		 * for the window icon.
		 */
		ms_window_context
			(const string& context_name,
			 const string& caption,
			 const string& resource_base_name,
			 bool use_raw_input);


		~ms_window_context() NOTHROW override;


		void set_caption(const string& caption) override;
		string caption() const override;

		void set_display
			(const vector2f& position, const vector2f& size,
			 uint32 display_flags) override;

		vector2f position() const override;
		vector2f size() const override;
		uint32 display_flags() const override;

		uint32 notify_flags() const override;
		bool close_flag() const override;

		void update() override;


		/**
		 * Retrieve Windows API specific handles.
		 *
		 * @attent
		 * These handles follow unique threading rules which may lead
		 * to random, weird bugs. When in doubt, check with the MSDN.
		 */
		//@{
		HWND window_handle() const;
		HDC render_device_context() const;
		//@}


		/**
		 * Retrieve any input messages (e.g. WM_USER_INPUT_MOUSE_AXIS)
		 * that have accumulated since the last update().
		 *
		 * Implementation notes:
		 * - Only ms_input_context should call input_messages().
		 * - The resulting buffer does not contain conflicting messages
		 *		from raw input and traditional mouse messages.
		 */
		void input_messages
			(input_message_buffer& input_messages) const;


		const string resource_base_name;
		const bool use_raw_input;


	private:

		void apply_hide();
		void apply_minimized();
		void apply_fullscreen();
		void apply_maximized();
		void apply_windowed
				(const vector2f& position, const vector2f& size);

		void synch_caption();
		void synch_display();

		void synch_window_state();
		void synch_window_placement();
		void synch_auto_exit_fullscreen();

		void synch_window_and_thread_state();


		void notify_close();

		void push_back_input_message
			(HWND target_window, UINT message_code,
			 WPARAM word_param, LPARAM long_param);
		void handle_raw_mouse_input
			(HWND target_window, UINT message_code,
			 WPARAM word_param, LPARAM long_param);

		LRESULT wndproc_window
			(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		LRESULT wndproc_render_area
			(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK wndproc_static
			(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

		void message_pump();

		void task_main() override;


		void init_resources();
		void init_window();
		void init_render_area();
		void init_input_registration();
		void init_thread_internal();

		void destroy_render_area() NOTHROW;
		void destroy_window() NOTHROW;
		void destroy_resources() NOTHROW;
		void destroy() NOTHROW;


		static const char* const icon_resource_appendix_;
		static const char* const window_class_base_name_;
		static const char* const render_area_class_base_name_;

		static const DWORD window_thread_sleep_time_msecs_;

		static const vector2f default_position_;
		static const vector2f default_size_;

		static const DWORD fullscreen_style_;
		static const DWORD windowed_style_;
		static const float window_size_min_;

		static const int64 raw_input_buffer_size_ = 512;
		static const int64 max_input_messages_ = 1024;


		mutable mutex window_mutex_;
		signal init_finished_signal_;


		HICON icon_handle_;
		HBRUSH backdrop_brush_;
		HCURSOR cursor_handle_;

		string window_class_name_;
		bool window_class_registered_;
		HWND window_handle_;

		string render_area_class_name_;
		bool render_area_class_registered_;
		HWND render_area_handle_;
		HDC render_device_context_;

		
		/**
		 * State visible to client-side queries.
		 * An update() sets this to thread_state_.
		 */
		ms_window_state query_state_;
		

		/**
		 * Thread-internal state of the OS-side window.
		 * Regularly synchronized to the current window.
		 * Modified instantly by client code and GUI events.
		 */
		ms_window_state thread_state_;


		/**
		 * Whether thread_state_ holds incoming modifications
		 * that synch_window_and_thread_state() should apply.
		 */
		//@{
		bool pending_caption_;
		bool pending_display_;
		//@}
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_WINDOW) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_WINDOW_HPP) */