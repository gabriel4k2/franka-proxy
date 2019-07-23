/**
 *************************************************************************
 *
 * @file audio_sampling.cpp
 *
 * Sampling parameters for audio, implementation.
 *
 ************************************************************************/


#include "audio_sampling.hpp"


#include "log.hpp"
#include "exception.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// audio_channels
//
//////////////////////////////////////////////////////////////////////////


int64 audio_channels::to_channel_count(type channels) NOTHROW
{
	if (channels < 0 || channels >= channels_count)
		return 0; // Exception safety: NOTHROW

	return channel_count_for_channels[channels];
}


audio_channels::type
	audio_channels::to_channels(int64 channel_count) NOTHROW
{
	if (channel_count == 0)
		return channels_unknown;
	else if (channel_count == 1)
		return mono;
	else if (channel_count == 2)
		return stereo;
	else if (channel_count == 4)
		return surround_4;
	else if (channel_count == 5)
		return surround_5;
	else if (channel_count == 6)
		return surround_5_1;

	// Exception safety: NOTHROW
	return channels_unknown;
}


const int64 audio_channels::channel_count_for_channels[channels_count] =
	{ 0, 1, 2, 4, 5, 6 };




//////////////////////////////////////////////////////////////////////////
//
// audio_rate
//
//////////////////////////////////////////////////////////////////////////


int64 audio_rate::to_frames_per_second_int64(type rate) NOTHROW
{
	if (rate < 0 || rate >= rate_count)
		return 0; // Exception safety: NOTHROW

	return frames_per_second_for_rate_int64[rate];
}


float audio_rate::to_frames_per_second_float(type rate) NOTHROW
{
	if (rate < 0 || rate >= rate_count)
		return 0.f; // Exception safety: NOTHROW

	return frames_per_second_for_rate_float[rate];
}


audio_rate::type audio_rate::to_rate(int64 frames_per_second) NOTHROW
{
	if (frames_per_second == 96000)
		return rate_96000hz;
	else if (frames_per_second == 48000)
		return rate_48000hz;
	else if (frames_per_second == 44100)
		return rate_44100hz;
	else if (frames_per_second == 22050)
		return rate_22050hz;
	else if (frames_per_second == 16000)
		return rate_16000hz;
	else if (frames_per_second == 8000)
		return rate_8000hz;

	// Exception safety: NOTHROW
	return rate_unknown;
}


const int64 audio_rate::frames_per_second_for_rate_int64[rate_count] =
	{ 0, 96000, 48000, 44100, 22050, 16000, 8000 };
const float audio_rate::frames_per_second_for_rate_float[rate_count] =
	{ 0.f, 96000.f, 48000.f, 44100.f, 22050.f, 16000.f, 8000.f };




} /* namespace viral_core */
