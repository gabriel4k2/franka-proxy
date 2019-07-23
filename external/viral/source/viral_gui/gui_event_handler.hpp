/**
 *************************************************************************
 *
 * @file gui_event_handler.hpp
 *
 * Event handling for GUI applications.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_GUI__GUI_EVENT_HANDLER_HPP)
#define INCLUDED__VIRAL_GUI__GUI_EVENT_HANDLER_HPP


#include <viral_core/build_config.hpp>


#include <viral_core/util.hpp>


#include "gui_forward.hpp"


namespace viral_gui
{


/**
 *************************************************************************
 *
 * @class gui_event_handler
 * @ingroup gui_module
 *
 * Handles generic gui_output_event%s from gui_element%s in a gui_app.
 *
 ************************************************************************/
class gui_event_handler :
	private non_copyable
{
	friend class gui_app;

	public:

		virtual ~gui_event_handler() NOTHROW { }

	private:

		virtual void handle_event
			(const gui_output_event& event) = 0;
};




/**
 *************************************************************************
 *
 * @class gui_event_handler_callback
 * @ingroup gui_module
 *
 * A gui_event_handler that calls a member function
 * on a generic @bref{object} of type @bref{O}
 * for all events at least of a base type @bref{T}.
 *
 ************************************************************************/
template <typename T, typename O>
	class gui_event_handler_callback :
		public gui_event_handler
{
	public:

		typedef void (O::*callback_type)(const T& event);

		gui_event_handler_callback
			(O& object, callback_type callback)
			:
			object_(object),
			callback_(callback)
		{
			if (!callback_)
				throw null_pointer();
		}


	private:

		void handle_event
			(const gui_output_event& event) override
		{
			const T* filtered_event =
				dynamic_cast<const T*>(&event);

			if (!filtered_event) return;

			(object_.*callback_)(*filtered_event);
		}


		O& object_;
		const callback_type callback_;
};




} /* namespace viral_gui */


#endif /* !defined(INCLUDED__VIRAL_GUI__GUI_EVENT_HANDLER_HPP) */