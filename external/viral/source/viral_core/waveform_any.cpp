/**
 *************************************************************************
 *
 * @file waveform_any.cpp
 *
 * Polymorphic waveforms, implementation.
 *
 ************************************************************************/


#include "waveform_any.hpp"


#include "log.hpp"
#include "string.hpp"
#include "file_util.hpp"
#include "waveform_raw.hpp"
#include "waveform_ogg.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// waveform_any
//
//////////////////////////////////////////////////////////////////////////


waveform_any::~waveform_any() NOTHROW { }


void waveform_any::swap_with(waveform_any& other) NOTHROW
{
	// No members, so nothing to swap.
	// Implemented for future-proof children swapping.
}


auto_pointer<waveform_any> waveform_any::load
	(const string& path, file_context& files)
{
	string ext(virtual_path::extract_extension(path));

	if (ext == "wav")
	{
		return auto_pointer<waveform_any>
					(new waveform_any_raw
							(waveform_raw::load(path, files)));
	}
	else if (ext == "ogg")
	{
		return auto_pointer<waveform_any>
					(new waveform_any_ogg
							(waveform_ogg::load(path, files)));
	}
	else
	{
		LOG_ERROR("Waveform extension unknown: " + path);
		throw invalid_parameter();
	}		
}




} /* namespace viral_core */