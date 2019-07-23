/**
 *************************************************************************
 *
 * @file null_window.hpp
 *
 * Null windowing system.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NULL_WINDOW_HPP)
#define INCLUDED__VIRAL_CORE__NULL_WINDOW_HPP


#include "build_config.hpp"


#include "window.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class null_window_context
 * @ingroup window_module
 *
 * A window_context implementation that does not show any window.
 *
 * Usage notes:
 * - The null_window_context always returns default caption(), position(),
 *		size(), and display_flags(). Conversely, the null_window_context
 *		ignores all setters and all update() calls.
 * - @attent The null_window_context never raises close_flag() to avoid
 *		premature program exit. Clients must thus implement another way
 *		for users to quit the program.
 *
 ************************************************************************/
class null_window_context :
	public window_context
{

	public:

		null_window_context(const string& context_name);
		~null_window_context() NOTHROW override;

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
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NULL_WINDOW_HPP) */
