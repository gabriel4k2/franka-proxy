/**
 *************************************************************************
 *
 * @file image_raw.cpp
 *
 * Raw images, implementation.
 *
 ************************************************************************/


#include "image_raw.hpp"


#include "log.hpp"
#include "exception.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// image_raw_errors
//
//////////////////////////////////////////////////////////////////////////


void image_raw_errors::log_and_throw_max_size_exceeded()
{
	LOG_ERROR("Image exceeded minimal or maximal dimensions on creation.");
	throw bounds_exceeded();
}


void image_raw_errors::log_and_throw_invalid_buffer_size()
{
	LOG_ERROR("Image created with invalid-sized image data buffer.");
	throw invalid_parameter();
}


void image_raw_errors::log_and_throw_invalid_pixel_access()
{
	LOG_ERROR("Access to invalid pixel.");
	throw invalid_parameter();
}




} /* namespace viral_core */
