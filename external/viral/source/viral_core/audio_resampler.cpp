/**
 *************************************************************************
 *
 * @file audio_resampler.cpp
 *
 * Utilities for audio resampling, implementation.
 *
 ************************************************************************/


#include "audio_resampler.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// audio_channels_resampler
//
//////////////////////////////////////////////////////////////////////////


void audio_channels_resampler::resample
	(const audio_buffer& source_buffer,
	 audio_channels_type new_channels,
	 audio_buffer& dest_buffer)
{ }




//////////////////////////////////////////////////////////////////////////
//
// audio_rate_resampler
//
//////////////////////////////////////////////////////////////////////////


void audio_rate_resampler::resample
	(const audio_buffer& source_buffer,
	 audio_rate_type new_rate,
	 audio_buffer& dest_buffer)
{ }




} /* namespace viral_core */
