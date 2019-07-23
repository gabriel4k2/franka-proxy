/**
 *************************************************************************
 *
 * @file x11_window.hpp
 *
 * Application windows on X11 Window Server.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__X11_WINDOW_HPP)
#define INCLUDED__VIRAL_CORE__X11_WINDOW_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_X11_WINDOW)


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class x11_window_context
 * @ingroup window_module
 *
 * A window_context implementation on the X11 Window Server.
 *
 ************************************************************************/
class x11_window_context :
	public window_context
{
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_X11_WINDOW) */


#endif /* !defined(INCLUDED__VIRAL_CORE__X11_WINDOW_HPP) */