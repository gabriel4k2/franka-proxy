/**
 *************************************************************************
 *
 * @file image_any.cpp
 *
 * Image hierarchy, implementation.
 *
 ************************************************************************/


#include "image_any.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// image_any
//
//////////////////////////////////////////////////////////////////////////


image_any::~image_any() NOTHROW { }


void image_any::swap_with(image_any& other) NOTHROW
{
	// No members, so nothing to swap.
	// Implemented for future-proof children swapping.
}




} /* namespace viral_core */
