/**
 *************************************************************************
 *
 * @file x11_input.hpp
 *
 * Input handling on X11 Window Server.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__X11_INPUT_HPP)
#define INCLUDED__VIRAL_CORE__X11_INPUT_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_X11_INPUT)


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class x11_input_context
 * @ingroup input_module
 *
 * An input_context implementation on the X11 Window Server.
 *
 ************************************************************************/
class x11_input_context :
	public input_context
{
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_X11_INPUT) */


#endif /* !defined(INCLUDED__VIRAL_CORE__X11_INPUT_HPP) */

