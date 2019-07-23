/**
 *************************************************************************
 *
 * @file ms_window.cpp
 * 
 * Microsoft Windows-specific application window, implementation.
 *
 ************************************************************************/


#include "ms_window.hpp"


#if defined(VIRAL__HAVE_MS_WINDOW)


#include "log.hpp"
#include "integer_checks.hpp"
#include "memory.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//  ms_window_context
//
//////////////////////////////////////////////////////////////////////////


ms_window_context::ms_window_context
	(const string& context_name,
	 const string& caption,
	 const string& resource_base_name,
	 bool use_raw_input)
	:
	window_context(context_name),

	resource_base_name(resource_base_name),
	use_raw_input(use_raw_input),

	icon_handle_(NULL),
	backdrop_brush_(NULL),
	cursor_handle_(NULL),

	window_class_name_(context_name + window_class_base_name_),
	window_class_registered_(false),
	window_handle_(NULL),

	render_area_class_name_(context_name + render_area_class_base_name_),
	render_area_class_registered_(false),
	render_area_handle_(NULL),
	render_device_context_(NULL),

	query_state_(caption, default_position_, default_size_),
	thread_state_(caption, default_position_, default_size_),
	pending_caption_(false),
	pending_display_(false)
{
	LOG_INFO("Using ms_window_context implementation.");


	/**
	 * Start window thread, then wait for initialization.
	 *
	 * The window thread internally creates the window and associated
	 * objects before entering the message pump. Once window creation
	 * has succeeded, the thread raises a success signal.
	 *
	 * If window creation within the thread fails, the thread clears
	 * any resources, and exits with an exception. We check for this
	 * exception in the default timeout of the success signal.
	 */
	LOG_INFO("Starting window thread.");

	start();

	while (true)
	{
		init_finished_signal_.wait_for(true);

		if (init_finished_signal_.get())
			break;

		if (thread_has_terminated())
		{
			LOG_ERROR("Window thread failed to create the window.");
			join(); // should throw unhandled_exception

			LOG_CRITICAL("Window thread terminated without exception.");
			throw assert_failed();
		}
	}
}


ms_window_context::~ms_window_context() NOTHROW
{
	/**
	 * Terminate the window thread on shutdown.
	 * The thread destroys the OS window and performs
	 * proper resource cleanup before terminating.
	 */

	LOG_INFO("Destroying.");
	LOG_INFO("Terminating window thread.");

	try { join(); }
	catch (...)
		{ LOG_CRITICAL("Window thread has thrown on destruction."); }
}


void ms_window_context::set_caption(const string& caption)
{
	MUTEX_SCOPE(window_mutex_);
	thread_state_.caption_ = caption;
	pending_caption_ = true;
}

string ms_window_context::caption() const
	{ MUTEX_SCOPE(window_mutex_); return query_state_.caption_; }


void ms_window_context::set_display
	(const vector2f& position, const vector2f& size,
	 uint32 display_flags)
{
	/**
	 * Apply entire display settings in a single atomic operation.
	 * Otherwise, the window thread could apply a partial state
	 * to the OS-side window.
	 */

	MUTEX_SCOPE(window_mutex_);
	thread_state_.position_ = position;
	thread_state_.size_ = size;
	thread_state_.display_flags_ = display_flags;
	pending_display_ = true;
}

vector2f ms_window_context::position() const
	{ MUTEX_SCOPE(window_mutex_); return query_state_.position_; }
vector2f ms_window_context::size() const
	{ MUTEX_SCOPE(window_mutex_); return query_state_.size_; }
uint32 ms_window_context::display_flags() const
	{ MUTEX_SCOPE(window_mutex_); return query_state_.display_flags_; }


uint32 ms_window_context::notify_flags() const
	{ MUTEX_SCOPE(window_mutex_); return query_state_.notify_flags_; }

bool ms_window_context::close_flag() const
{
	MUTEX_SCOPE(window_mutex_);
	return (query_state_.notify_flags_ & close) != 0;
}


void ms_window_context::update()
{
	MUTEX_SCOPE(window_mutex_);


	// Exception monitoring: If the window-internal thread stops
	// because of any exception, our client thread must be notified.
	if (thread_has_terminated())
	{
		LOG_ERROR("Window thread is not running anymore.");
		join();
		throw invalid_state(); // join()ed previously
	}


	// Always reset close flag from previous update.
	if ((query_state_.notify_flags_ & close) != 0)
		thread_state_.notify_flags_ &= ~close;


	// Update query_state_ to match current thread_state_.
	// Assignment of caption_ and input_ are not expensive,
	// and in general reuse existing memory.
	query_state_ = thread_state_;


	// Accumulated messages have been delivered to client.
	// Clear and restart accumulation in internal thread.
	thread_state_.input_.clear();
}


HWND ms_window_context::window_handle() const
	{ MUTEX_SCOPE(window_mutex_); return window_handle_; }
HDC ms_window_context::render_device_context() const
	{ MUTEX_SCOPE(window_mutex_); return render_device_context_; }


void ms_window_context::input_messages
	(input_message_buffer &input_messages) const
{
	MUTEX_SCOPE(window_mutex_);
	input_messages = query_state_.input_;
}


void ms_window_context::apply_hide()
{
	LOG_INFO("Changing display mode: hidden");

	SetWindowLong(window_handle_, GWL_STYLE,
				  static_cast<LONG>(windowed_style_));
	SetWindowPos(window_handle_, NULL, 0, 0, 0, 0,
				 SWP_FRAMECHANGED | SWP_HIDEWINDOW |
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	ShowWindow(window_handle_, SW_HIDE);
}


void ms_window_context::apply_minimized()
{
	LOG_INFO("Changing display mode: minimized");

	SetWindowLong(window_handle_, GWL_STYLE,
				  static_cast<LONG>(windowed_style_));
	SetWindowPos(window_handle_, NULL, 0, 0, 0, 0,
				 SWP_FRAMECHANGED | SWP_SHOWWINDOW |
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	ShowWindow(window_handle_, SW_SHOWMINIMIZED);
}


void ms_window_context::apply_fullscreen()
{
	LOG_INFO("Changing display mode: fullscreen");	

	HMONITOR hmon =
		MonitorFromWindow(window_handle_, MONITOR_DEFAULTTOPRIMARY);

	MONITORINFO hmoninfo;
	hmoninfo.cbSize = sizeof(MONITORINFO);

	RECT fullscreen_rect;
	if (GetMonitorInfo(hmon, &hmoninfo) != 0)
	{
		fullscreen_rect = hmoninfo.rcMonitor;
	}
	else
	{
		fullscreen_rect.left = fullscreen_rect.top = 0;
		fullscreen_rect.right = GetSystemMetrics(SM_CXSCREEN);
		fullscreen_rect.bottom = GetSystemMetrics(SM_CYSCREEN);
	}

	AdjustWindowRect
		(&fullscreen_rect, fullscreen_style_, false);

	SetWindowLong(window_handle_, GWL_STYLE,
				  static_cast<LONG>(fullscreen_style_));
	SetWindowPos(window_handle_, NULL,
				 fullscreen_rect.left, fullscreen_rect.top,
				 fullscreen_rect.right - fullscreen_rect.left,
				 fullscreen_rect.bottom - fullscreen_rect.top,
				 SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOZORDER);
	ShowWindow(window_handle_, SW_SHOW);
}


void ms_window_context::apply_maximized()
{
	LOG_INFO("Changing display mode: maximized");	

	SetWindowLong(window_handle_, GWL_STYLE,
				  static_cast<LONG>(windowed_style_));
	SetWindowPos(window_handle_, NULL, 0, 0, 0, 0,
				 SWP_FRAMECHANGED | SWP_SHOWWINDOW |
				 SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);
	ShowWindow(window_handle_, SW_SHOWMAXIMIZED);
}


void ms_window_context::apply_windowed
	(const vector2f& position, const vector2f& size)
{
	LOG_INFO("Changing display mode: windowed");

	RECT wndrect =
		{
			integer_operations::cast_or_throw<int32>(position.x),
			integer_operations::cast_or_throw<int32>(position.y),
			integer_operations::cast_or_throw<int32>
				(position.x + max(size.x, window_size_min_)),
			integer_operations::cast_or_throw<int32>
				(position.y + max(size.y, window_size_min_))
		};

	AdjustWindowRect(&wndrect, windowed_style_, false);

	SetWindowLong(window_handle_, GWL_STYLE,
				  static_cast<LONG>(windowed_style_));
	SetWindowPos(window_handle_, NULL,
					wndrect.left, wndrect.top,
					wndrect.right - wndrect.left,
					wndrect.bottom - wndrect.top,
			 		SWP_FRAMECHANGED | SWP_SHOWWINDOW | SWP_NOZORDER);
	ShowWindow(window_handle_, SW_SHOW);
}


void ms_window_context::synch_caption()
{
	if (!pending_caption_) return;	
	pending_caption_ = false;
	SetWindowText(window_handle_, thread_state_.caption_.data());
}


void ms_window_context::synch_display()
{
	/**
	 * Apply an incoming (usually code based) request
	 * to change the window placement or display flags.
	 * Any contradictory flags are removed, based on
	 * a reasonable flag priority.
	 */

	if (!pending_display_) return;
	pending_display_ = false;


	// Accept cursor flag.
	LONG_PTR cursor =
		(thread_state_.display_flags_ & no_cursor) != 0 ?
			reinterpret_cast<LONG_PTR>(cursor_handle_) :
			reinterpret_cast<LONG_PTR>(LoadCursor(NULL, IDC_ARROW));

	SetClassLongPtr(window_handle_, GCLP_HCURSOR, cursor);
	SetClassLongPtr(render_area_handle_, GCLP_HCURSOR, cursor);


	// Accept display mode flags.
	uint32 disable_flags =
		~(static_cast<uint32>
			(hide | fullscreen | maximized | minimized));

	if ((thread_state_.display_flags_ & hide) != 0)
	{
		thread_state_.display_flags_ &= disable_flags;
		thread_state_.display_flags_ |= hide;
		apply_hide();		
	}
	else if ((thread_state_.display_flags_ & minimized) != 0)
	{
		thread_state_.display_flags_ &= disable_flags;
		thread_state_.display_flags_ |= minimized;
		apply_minimized();
	}
	else if ((thread_state_.display_flags_ & maximized) != 0)
	{
		thread_state_.display_flags_ &= disable_flags;
		thread_state_.display_flags_ |= maximized;
		apply_maximized();
	}
	else if ((thread_state_.display_flags_ & fullscreen) != 0)
	{
		thread_state_.display_flags_ &= disable_flags;
		thread_state_.display_flags_ |= fullscreen;
		apply_fullscreen();
	}
	else
	{
		thread_state_.display_flags_ &= disable_flags;
		apply_windowed(thread_state_.position_, thread_state_.size_);
	}


	// Move window to foreground after code-provoked change.
	// This works around a bug where an application window
	// does not show up on the taskbar if the user changed
	// to another window during display synchronization.
	if ((thread_state_.display_flags_ & hide) == 0 &&
		(thread_state_.display_flags_ & minimized) == 0)
	{
		SetForegroundWindow(window_handle_);
	}
}


void ms_window_context::synch_window_state()
{
	/**
	 * Synch thread state to current window minimization,
	 * maximization and active state. This considers
	 * both code based settings, and GUI interactions.
	 */

	if (IsWindowVisible(window_handle_))
		thread_state_.display_flags_ &= ~hide;
	else
		thread_state_.display_flags_ |= hide;

	if (IsIconic(window_handle_))
		thread_state_.display_flags_ |= minimized;
	else
		thread_state_.display_flags_ &= ~minimized;

	if (IsZoomed(window_handle_))
		thread_state_.display_flags_ |= maximized;
	else
		thread_state_.display_flags_ &= ~maximized;

	if (GetActiveWindow() == window_handle_)
		thread_state_.notify_flags_ |= active;
	else
		thread_state_.notify_flags_ &= ~active;
}


void ms_window_context::synch_window_placement()
{
	/**
	 * Synchronize thread_state_ and render area
	 * to the actual window position and size.
	 * Hidden or minimized windows sometimes return
	 * corrupt sizes, so we maintain any preceding
	 * sizes for these.
	 */

	if (IsIconic(window_handle_) || !IsWindowVisible(window_handle_))
		return;
	
	RECT client_size;
	GetClientRect(window_handle_, &client_size);


	// Update thread state to screen-space client area of window.
	POINT upper_left = { client_size.left, client_size.top };
	POINT lower_right = { client_size.right, client_size.bottom };

	ClientToScreen(window_handle_, &upper_left);
	ClientToScreen(window_handle_, &lower_right);

	thread_state_.position_ =
		vector2f(static_cast<float>(upper_left.x),
				 static_cast<float>(upper_left.y));
	thread_state_.size_ =
		vector2f(static_cast<float>(lower_right.x - upper_left.x),
				 static_cast<float>(lower_right.y - upper_left.y));


	// Update render area to entire client area of window.
	// Note SetWindowPos is expensive compared to GetWindowRect(),
	// so we countercheck before we resize the render area.
	RECT render_area_size;
	GetWindowRect(render_area_handle_, &render_area_size);

	if (render_area_size.right - render_area_size.left !=
			client_size.right - client_size.left ||
		render_area_size.bottom - render_area_size.top !=
			client_size.bottom - client_size.top)
	{
		SetWindowPos
			(render_area_handle_, NULL, 0, 0,
				client_size.right - client_size.left,
				client_size.bottom - client_size.top,
				SWP_SHOWWINDOW | SWP_NOZORDER);
	}
}


void ms_window_context::synch_auto_exit_fullscreen()
{
	/**
	 * If a window is fullscreen but currently not active,
	 * restore the default size and position on next synch.
	 * This avoids occluding the entire desktop.
	 */

	if ((thread_state_.display_flags_ & fullscreen) == 0) return;
	if (GetActiveWindow() == window_handle_) return;

	thread_state_.display_flags_ &= ~fullscreen;
	thread_state_.position_ = default_position_;
	thread_state_.size_ = default_size_;
	pending_display_ = true;
}


void ms_window_context::synch_window_and_thread_state()
{
	/**
	 * Apply pending modifications to the window, and fetch
	 * resulting or current window properties into thread_state_.
	 * Frequently called from within the window-internal thread
	 * so that we can apply pending data without any message.
	 * Also invoked immediately by the message pump once we
	 * get a notification that our window size changed.
	 */

	MUTEX_SCOPE(window_mutex_);

	synch_caption();
	synch_display();
	synch_window_state();
	synch_window_placement();
	synch_auto_exit_fullscreen();
}


void ms_window_context::notify_close()
{
	MUTEX_SCOPE(window_mutex_);
	thread_state_.notify_flags_ |= close;
}


void ms_window_context::push_back_input_message
	(HWND target_window, UINT message_code,
	 WPARAM word_param, LPARAM long_param)
{
	/**
	 * Store normal KEY_DOWN / KEY_UP / ... message or custom
	 * WM_USER_MOUSE* message for later use in input context.
	 *
	 * An upper bound on the number of messages avoids flooded
	 * message buffers for slow update()s.
	 */

	MUTEX_SCOPE(window_mutex_);

	if (thread_state_.input_.size() >= max_input_messages_)
		return;

	thread_state_.input_.push_back
		(ms_window_message(target_window, message_code, word_param, long_param));
}


void ms_window_context::handle_raw_mouse_input
	(HWND target_window, UINT message_code,
	 WPARAM word_param, LPARAM long_param)
{
	/**
	 * Translate a single WM_INPUT message into helper messages
	 * for ms_input_context. We cannot pass on WM_INPUT directly,
	 * since raw input must be evaluated within the window procedure.
	 * The helper push_back_input_message() performs mutexing.
	 */


	// First, extract raw input data packet
	// into our statically sized buffer.
	// We use a pre-determined buffer size,
	// as we only request mouse messages.
	HRAWINPUT handle =
		reinterpret_cast<HRAWINPUT>(long_param);
	UINT datasize = 0;

	if (GetRawInputData
			(handle, RID_INPUT, NULL,
			 &datasize, sizeof(RAWINPUTHEADER)) != 0)
	{
		LOG_WARN("Failed to query size of a raw input message.");
		return;
	}

	if (datasize > raw_input_buffer_size_)
	{
		LOG_WARN("Raw input size exceeds the input buffer size.");
		return;
	}

	_declspec(align(16)) // Required for x64
		unsigned char raw_input_buffer_[raw_input_buffer_size_];

	RAWINPUT *rawinput =
		reinterpret_cast<RAWINPUT*>(raw_input_buffer_);

	if (GetRawInputData
			(handle, RID_INPUT, rawinput,
			 &datasize, sizeof(RAWINPUTHEADER)) != datasize)
	{
		LOG_WARN("Failed to retrieve raw input data.");
		return;
	}

	if (rawinput->header.dwType != RIM_TYPEMOUSE)
	{
		LOG_WARN("Raw input for unexpected device.");
		return;
	}


	// Translate relative mouse messages to input messages.
	// We ignore absolute devices (mouse position, touchscreens),
	// since these are separately handled by ms_input_context.
	// Note the mouse wheel comes in as unsigned, but actually
	// holds signed data.
	RAWMOUSE &mouse = rawinput->data.mouse;

	if ((mouse.usFlags & MOUSE_MOVE_ABSOLUTE) != 0)
		return;

	static const USHORT buttonflags[10] =
		{ RI_MOUSE_BUTTON_1_DOWN, RI_MOUSE_BUTTON_1_UP, 
		  RI_MOUSE_BUTTON_2_DOWN, RI_MOUSE_BUTTON_2_UP, 
		  RI_MOUSE_BUTTON_3_DOWN, RI_MOUSE_BUTTON_3_UP, 
		  RI_MOUSE_BUTTON_4_DOWN, RI_MOUSE_BUTTON_4_UP, 
		  RI_MOUSE_BUTTON_5_DOWN, RI_MOUSE_BUTTON_5_UP };

	for (uint32 i = 0; i < 5; ++i)
	{
		if ((mouse.usButtonFlags & buttonflags[i * 2]) != 0)
		{
			push_back_input_message
				(target_window, WM_USER_INPUT_MOUSE_BUTTON, i, 1);
		}

		if ((mouse.usButtonFlags & buttonflags[i * 2 + 1]) != 0)
		{
			push_back_input_message
				(target_window, WM_USER_INPUT_MOUSE_BUTTON, i, 0);
		}
	}

	push_back_input_message
		(target_window, WM_USER_INPUT_MOUSE_AXIS, 0, mouse.lLastX);
	push_back_input_message
		(target_window, WM_USER_INPUT_MOUSE_AXIS, 1, mouse.lLastY);

	if ((mouse.usFlags & RI_MOUSE_WHEEL) != 0)
	{
		push_back_input_message
			(target_window, WM_USER_INPUT_MOUSE_AXIS, 2,
				static_cast<SHORT>(mouse.usButtonData));
	}
}


LRESULT ms_window_context::wndproc_window
	(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/**
	 * Handle some message within our window thread,
	 * either for our window or for no window at all.
	 *
	 * @attent We don't lock here, so that clients which read
	 * the query_state_ won't be locked for each message we get.
	 * We need to lock within some message handlers though.
	 */

	switch (uMsg)
	{

		case WM_SYSCOMMAND:
		{
			// Don't allow screen saver or energy saver, and don't
			// allow those ugly context menus. Lower 4 bits contain
			// irrelevant system internal values.
			WPARAM mparam = (wParam & 0xFFF0);

			if (mparam == SC_SCREENSAVE || mparam == SC_MONITORPOWER ||
				mparam == SC_KEYMENU || mparam == SC_MOUSEMENU)
					return 0;
		}
		break; // DefWindowProc() for other system commands.


		case WM_GETMINMAXINFO:
		{
			// Restrict minimal and maximal window size
			// when dragging the window frames via the GUI.
			// Do not adjust position / size of maximized windows.
			// The OS anyways knows better for these.
			MINMAXINFO *minmax = (MINMAXINFO*)lParam;
			minmax->ptMinTrackSize.x = static_cast<LONG>(window_size_min_);
			minmax->ptMinTrackSize.y = static_cast<LONG>(window_size_min_);
			minmax->ptMaxTrackSize.x = GetSystemMetrics(SM_CXSCREEN);
			minmax->ptMaxTrackSize.y = GetSystemMetrics(SM_CYSCREEN);
		}
		return 0; // Provided new minmax data.


		case WM_WINDOWPOSCHANGED:
		{
			synch_window_and_thread_state();
		}
		break; // DefWindowProc() generates WM_MOVE / WM_SIZE.


		case WM_KEYDOWN: case WM_KEYUP:
		case WM_SYSKEYDOWN: case WM_SYSKEYUP:
		case WM_CHAR:
		{
			push_back_input_message(hWnd, uMsg, wParam, lParam);
		}
		break; // DefWindowProc() required for OS shortcuts.


		case WM_DEVICECHANGE:
		{
			// XInput requires WM_DEVICECHANGE
			// to recognize joystick reconnect.
			push_back_input_message(hWnd, uMsg, wParam, lParam);
		}
		break;


		case WM_INPUT:
		{
			if (use_raw_input)
			{
				try { handle_raw_mouse_input(hWnd, uMsg, wParam, lParam); }
					catch (...) { LOG_WARN("Exception in handle_raw_input."); }
			}

			// DefWindowProc() frees raw memory of WM_INPUT message,
			// and thus must always run after the break statement.
		}
		break;


		case WM_MOUSEWHEEL:
		{
			// Implementation notes:
			// - To avoid conflicts with WM_INPUT for the mouse wheel,
			//		we only store WM_MOUSEWHEEL if not using raw input.
			// - Mouse wheel events arrive only at the topmost window,
			//		while other mouse events end up in the render area.

			if (!use_raw_input)
				push_back_input_message(hWnd, uMsg, wParam, lParam);
		}
		break;


		case WM_CLOSE:
		{
			notify_close();			 
		}
		return 0; // Allow application to react to close flag.
	}


	return
		DefWindowProc(hWnd, uMsg, wParam, lParam);
}


LRESULT ms_window_context::wndproc_render_area
	(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/**
	 * Handle messages specifically sent to the render area.
	 * We must handle these separately from wndproc_window()
	 * to avoid processing of redundant or unrelated messages
	 * (e.g. of WM_CHAR or WM_GETMINMAXINFO).
	 *
	 * @attent We don't lock here, so that clients which read
	 * the query_state_ won't be locked for each message we get.
	 * We need to lock within some message handlers though.
	 */

	switch (uMsg)
	{
		case WM_LBUTTONDOWN: case WM_LBUTTONDBLCLK: case WM_LBUTTONUP:
		case WM_MBUTTONDOWN: case WM_MBUTTONDBLCLK: case WM_MBUTTONUP:
		case WM_RBUTTONDOWN: case WM_RBUTTONDBLCLK: case WM_RBUTTONUP:
		{
			// Implementation notes:
			// - To avoid conflicts with WM_INPUT for the mouse buttons,
			//		we only forward WM_*BUTTON* if not using raw input.
			// - Client-area mouse messages apply only to topmost windows,
			//		which in this case is the render area child window.
			// - We do not forward WM_MOUSEMOVE, as this conflicts with
			//		input_context::freeze_mouse() in a separate thread.
			//		Instead, ms_input_context derives mouse movements
			//		through GetCursorPos().

			if (!use_raw_input)
				push_back_input_message(hWnd, uMsg, wParam, lParam);
		}
		break;
	}

	return
		DefWindowProc(hWnd, uMsg, wParam, lParam);
}


LRESULT CALLBACK ms_window_context::wndproc_static
	(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	/**
	 * Retrieve context instance pointer from window extra data,
	 * and pass on message to appropriate window message handler.
	 * If a message comes in very early during window creation,
	 * no instance pointer is set yet, so only use DefWindowProc().
	 */

	ms_window_context* context =
		reinterpret_cast<ms_window_context*>
			(GetWindowLongPtr(hWnd, 0));

	if (!context)
		return DefWindowProc(hWnd, uMsg, wParam, lParam);

	if (hWnd == context->render_area_handle_)
		return context->wndproc_render_area(hWnd, uMsg, wParam, lParam);

	return context->wndproc_window(hWnd, uMsg, wParam, lParam);
}


void ms_window_context::message_pump()
{
	MSG message;

	// Dispatch all messages on the thread message queue,
	// regardless of target window, to avoid queue overflow.
	// The window procedure registered on the WNDCLASSEX
	// ensures that we process messages for our window only.

	while (PeekMessage(&message, NULL, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&message);
		DispatchMessage(&message);
	}
}


void ms_window_context::task_main()
{
	/**
	 * Main method of the window-internal thread.
	 *
	 * Outline:
	 * - Create window.
	 * - Run synch and message pump up to ~ms_window_context().
	 * - Cleanup.
	 *
	 * Cleanup via scope guard on:
	 * - Normal exit by ~ms_window_context and thread::terminate().
	 * - Exceptions in initialization.
	 * - Exceptions in the main thread loop.
	 */

	SCOPE_GUARD
		(destroy_window, (*this, &ms_window_context::destroy));

	init_thread_internal();

	while (!join_now())
	{
		synch_window_and_thread_state();
		message_pump();
		MsgWaitForMultipleObjects
			(0, NULL, false, window_thread_sleep_time_msecs_, QS_ALLINPUT);
	}
}


void ms_window_context::init_resources()
{
	HINSTANCE inst = GetModuleHandle(NULL);


	LOG_INFO("Loading application icon.");
	icon_handle_ = LoadIcon
		(inst, (resource_base_name + icon_resource_appendix_).data());

	if (!icon_handle_)
		LOG_WARN("Couldn't find application icon.");


	LOG_INFO("Creating window backdrop brush.");

	backdrop_brush_ = CreateSolidBrush(0);

	if (!backdrop_brush_)
		LOG_WARN("Failed to create window backdrop brush.");


	LOG_INFO("Creating invisible window cursor.");

	int cursor_x = GetSystemMetrics(SM_CXCURSOR);
	int cursor_y = GetSystemMetrics(SM_CYCURSOR);

	if (cursor_x <= 0 || cursor_y <= 0 ||
		!integer_checks::is_mult_safe(cursor_x, cursor_y))
	{
		LOG_ERROR("Invalid system cursor metrics.");
		throw api_call_failed();
	}

	int cursor_size = cursor_x * cursor_y;

	dynamic_buffer<char> and_plane, or_plane;
	and_plane.resize(cursor_size);
	or_plane.resize(cursor_size);

	for (int i = 0; i < cursor_size; ++i)
		{ and_plane[i] = -1; or_plane[i] = 0; }

	cursor_handle_ =
		CreateCursor(inst, 0, 0, cursor_x, cursor_y,
					 and_plane.data(), or_plane.data());

	if (!cursor_handle_)
		LOG_WARN("Failed to create invisible window cursor.");
}


void ms_window_context::init_window()
{
	HINSTANCE inst = GetModuleHandle(NULL);


	LOG_INFO("Registering window class.");

	WNDCLASSEX wc =
		{ sizeof(WNDCLASSEX),
		  CS_HREDRAW | CS_VREDRAW,
		  &wndproc_static, 0, sizeof(ms_window_context*), inst,
		  icon_handle_, cursor_handle_, backdrop_brush_,
		  NULL, window_class_name_.data(), NULL };

	if (!RegisterClassEx(&wc))
	{
		LOG_CRITICAL("Failed to register window class.");
		throw api_call_failed();
	}

	window_class_registered_ = true;
	backdrop_brush_ = NULL; // Window class owns brush now.


	LOG_INFO("Creating window.");

	window_handle_ =
		CreateWindow(window_class_name_.data(),
					 query_state_.caption_.data(),
					 0, 0, 0, 0, 0,
					 NULL, NULL, inst, NULL);

	if (!window_handle_)
	{
		LOG_CRITICAL("Failed to create window.");
		throw api_call_failed();
	}


	LOG_INFO("Binding ms_window_context to user data segment.");

	SetWindowLongPtr
		(window_handle_, 0, reinterpret_cast<LONG_PTR>(this));
}


void ms_window_context::init_render_area()
{
	HINSTANCE inst = GetModuleHandle(NULL);


	/**
	 * Implementation notes:
	 * - We must render into a separate child window. Otherwise,
	 *		returning from OpenGL fullscreen mode breaks composition
	 *		on Win7/8 and leaves artifacts all over the desktop.
	 * - We need a unique device context for threaded rendering,
	 *		as device contexts in general are not threadsafe.
	 * - If present, concurrent redraws from within the window thread
	 *		share the popular CS_OWNDC. Hence, we must not use it.
	 * - We can't create a new DC in the context of varying rendering
	 *		threads, as the creating thread must also release the DC.
	 *		So, let the window-internal thread do all the work.
	 */

	LOG_INFO("Registering window class for render area.");

	WNDCLASSEX wc =
		{ sizeof(WNDCLASSEX), 0,
		  &wndproc_static, 0, sizeof(ms_window_context*), inst,
		  NULL, NULL, NULL,
		  NULL, render_area_class_name_.data(), NULL };

	if (!RegisterClassEx(&wc))
	{
		LOG_CRITICAL("Failed to register render area class.");
		throw api_call_failed();
	}

	render_area_class_registered_ = true;


	LOG_INFO("Creating render area.");

	render_area_handle_ =
		CreateWindow(render_area_class_name_.data(),
					 NULL, WS_VISIBLE | WS_CHILD,
					 0, 0, 0, 0,
					 window_handle_, NULL, inst, NULL);

	if (!render_area_handle_)
	{
		LOG_CRITICAL("Failed to create render area.");
		throw api_call_failed();
	}


	LOG_INFO("Binding ms_window_context to user data segment of render area.");

	SetWindowLongPtr
		(render_area_handle_, 0, reinterpret_cast<LONG_PTR>(this));


	LOG_INFO("Retrieving device context from render area.");

	render_device_context_ = GetDC(render_area_handle_);

	if (!render_device_context_)
	{
		LOG_CRITICAL("Failed to get device context.");
		throw api_call_failed();
	}
}


void ms_window_context::init_input_registration()
{
	/**
	 * Tell Windows that we want raw mouse input.
	 * Magic numbers for devices are TLCs for the system mouse.
	 * Ignored when raw input is not allowed on this window context.
	 * See ms_window_context class documentation for details.
	 */

	if (!use_raw_input) return;

	RAWINPUTDEVICE mouse_device =
		{ 0x01, 0x02, 0, window_handle_ };

	if (RegisterRawInputDevices
			(&mouse_device, 1, sizeof(RAWINPUTDEVICE)) == FALSE)
	{
		LOG_CRITICAL("Failed to register for mouse input.");
		throw api_call_failed();
	}
}


void ms_window_context::init_thread_internal()
{
	/**
	 * Initialize the actual window and its resources from within the
	 * window thread. Then signal the main program thread to continue.
	 * No mutex here, since the creating thread is only waiting for
	 * our signal.
	 */

	init_resources();
	init_window();
	init_render_area();
	init_input_registration();

	init_finished_signal_.set(true);
}


void ms_window_context::destroy_render_area() NOTHROW
{
	if (render_device_context_)
	{
		LOG_INFO("Releasing device context of render area.");

		if (ReleaseDC(render_area_handle_, render_device_context_) != 1) // 1 is success.
			LOG_ERROR("Failed to release device context.");

		render_device_context_ = NULL;
	}

	if (render_area_handle_)
	{
		LOG_INFO("Destroying render area.");

		if (DestroyWindow(render_area_handle_) == 0)
			LOG_ERROR("Failed to destroy render area.");

		render_area_handle_ = NULL;
	}

	if (render_area_class_registered_)
	{
		LOG_INFO("Unregistering window class of render area.");

		if (UnregisterClass(render_area_class_name_.data(), GetModuleHandle(NULL)) == 0)
			LOG_ERROR("Failed to unregister window class.");

		render_area_class_registered_ = false;	
	}
}


void ms_window_context::destroy_window() NOTHROW
{
	if (window_handle_)
	{
		LOG_INFO("Destroying window.");

		if (DestroyWindow(window_handle_) == 0)
			LOG_ERROR("Failed to destroy window.");

		window_handle_ = NULL;
	}

	if (window_class_registered_)
	{
		LOG_INFO("Unregistering window class.");

		if (UnregisterClass(window_class_name_.data(), GetModuleHandle(NULL)) == 0)
			LOG_ERROR("Failed to unregister window class.");

		window_class_registered_ = false;
	}
}


void ms_window_context::destroy_resources() NOTHROW
{
	/**
	 * Implementation notes:
	 * - We must reset the current cursor to NULL before
	 *		deletion, since the (invisible) cursor from
	 *		destroyed classes may still linger.
	 * - Ownership over the backdrop brush usually transfers
	 *		to the main window class. In this case, the handle
	 *		of the brush is NULL.
	 * - The app icon must not be freed here, since it resides
	 *		directly within the executable file.
	 */

	icon_handle_ = NULL;

	if (cursor_handle_)
	{
		LOG_INFO("Deleting (invisible) window cursor.");

		SetCursor(NULL);

		if (DestroyCursor(cursor_handle_) == 0)
			LOG_ERROR("Failed to delete cursor.");

		cursor_handle_ = NULL;
	}

	if (backdrop_brush_)
	{
		LOG_INFO("Deleting backdrop brush.");

		if (DeleteObject(backdrop_brush_) == 0)
			LOG_ERROR("Failed to delete backdrop brush.");

		backdrop_brush_ = NULL;
	}
}


void ms_window_context::destroy() NOTHROW
{
	/**
	 * Destroy the window from within the window thread.
	 *
	 * Implementation notes:
	 * - We need a mutex here: destroy() may also be called because of
	 *		an exception in the window thread, in which case there might
	 *		be some concurrent client reading from our query_state_.
	 * - destroy() also may be called to clean up a partially successful
	 *		initialization, so we need to check for the validity of each
	 *		object.
	 */

	MUTEX_SCOPE(window_mutex_);

	destroy_render_area();
	destroy_window();
	destroy_resources();
}


const char* const ms_window_context::icon_resource_appendix_ = "_icon";
const char* const ms_window_context::window_class_base_name_ = "_viral_window";
const char* const ms_window_context::render_area_class_base_name_ = "_viral_render_area";

const DWORD ms_window_context::window_thread_sleep_time_msecs_ = 100;

const vector2f ms_window_context::default_position_(50, 50);
const vector2f ms_window_context::default_size_(320, 240);

const DWORD ms_window_context::fullscreen_style_ =
	WS_VISIBLE | WS_POPUP;
const DWORD ms_window_context::windowed_style_ =
	WS_VISIBLE | WS_POPUP | WS_CAPTION | WS_SYSMENU |
	WS_SIZEBOX | WS_MAXIMIZEBOX | WS_MINIMIZEBOX;

const float ms_window_context::window_size_min_ = 128;




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_WINDOW) */
