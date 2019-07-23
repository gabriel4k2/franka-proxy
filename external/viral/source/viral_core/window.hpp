/**
 *************************************************************************
 *
 * @file window.hpp
 *
 * Application window header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__WINDOW_HPP)
#define INCLUDED__VIRAL_CORE__WINDOW_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup window_module Window Module
 *
 * Supports management of application windows
 * through the window_context class.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class window_context
 * @ingroup window_module
 *
 * Potentially threaded application window.
 *
 * Window lifetime overview:
 * - An application must select (set_display()) and apply (update())
 *		initial window display options. By default, windows are hidden
 *		and use implementation-specific positions and sizes.
 * - Thereafter, the application calls update() in regular intervals,
 *		for example within some main logics loop. This synchronizes
 *		all queries (e.g. position() or size()) to the OS-side window.
 *		In-between update()s, all queries return constant values.
 *		This implies a small lag between application and window.
 * - The window context raises a close_flag() once the user tries
 *		to close the window. The application should catch this flag,
 *		and react accordingly. If ignored, the close flag is reset
 *		on the next update().
 *
 * General usage notes:
 * - Applications must tolerate implementation-specific glitches.
 *		Notably, the OS-side window may perform arbitrary actions
 *		(e.g. change window size or display flags) that propagate
 *		to client-side queries on some update(). Likewise, client
 *		requests may take some time to apply to the OS-side window
 *		and need not be realized on the next client-side update().
 *		Finally, the window may even ignore client-side requests.
 * - A window context is threadsafe for an arbitrary number of threads.
 *		Yet, there only should be one thread calling update() at a time
 *		to enforce consistency in between updates.
 *
 * Implementation notes:
 * - A window context provides several query methods for collaboration
 *		with input_context, render_context, and audio_context classes.
 *		As with state queries, respective data changes only on updates.
 *
 ************************************************************************/
class window_context :
	private non_copyable
{

	public:

		enum display_flag
		{
			/**
			 * The window currently is hidden, minimized, maximized,
			 * or covers the entire screen. Both position() and size()
			 * might require a few update()s to reflect these flags.
			 * If contradictory flags are set, the backend decides
			 * on which flags to apply.
			 */
			//@{
			hide = 0x1,
			minimized = 0x2,
			maximized = 0x4,
			fullscreen = 0x8,
			//@}


			/** Hide the cursor while the mouse is over the window. */
			no_cursor = 0x10
		};


		enum notify_flag
		{
			/** Raised once the user tries to close the OS window. */
			close = 0x1,

			/** The window currently is the active foreground window. */
			active = 0x2
		};
	

		virtual ~window_context() NOTHROW;


		/** Caption in window title bar. */
		//@{
		virtual void set_caption(const string& caption) = 0;
		virtual string caption() const = 0;
		//@}


		/**
		 * Access window position, size, and display_flag%s.
		 * Size is given in pixels and potentially subpixels.
		 * Positive x points right, positive y points down.
		 */
		//@{
		virtual void set_display
			(const vector2f& position, const vector2f& size,
			 uint32 display_flags) = 0;

		virtual vector2f position() const = 0;
		virtual vector2f size() const = 0;
		virtual uint32 display_flags() const = 0;
		//@}


		/** A read-only combination of notify_flag%s. */
		virtual uint32 notify_flags() const = 0;


		/** Whether notify_flag::close is set. */
		virtual bool close_flag() const = 0;


		/** Make all queries return updated values. */
		virtual void update() = 0;


		const string context_name;


	protected:

		window_context(const string& context_name);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__WINDOW_HPP) */
