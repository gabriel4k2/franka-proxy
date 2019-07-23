/**
 *************************************************************************
 *
 * @file audio_buffer.cpp
 *
 * Software audio buffers, implementation.
 *
 ************************************************************************/


#include "audio_buffer.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// audio_buffer
//
//////////////////////////////////////////////////////////////////////////


audio_buffer::audio_buffer() NOTHROW
	:
	channels_(audio_channels::channels_unknown),
	rate_(audio_rate::rate_unknown)
{
	// No need to validate().
	// Default audio_buffer is always valid.
}


audio_buffer::audio_buffer
	(audio_channels_type channels,
	 audio_rate_type rate)
	:
	channels_(channels),
	rate_(rate)
{
	validate();
}


audio_buffer::audio_buffer
	(audio_channels_type channels,
	 audio_rate_type rate,
	 int64 frame_count)
	:
	channels_(channels),
	rate_(rate)
{
	buffer_.resize
		(frame_count * audio_channels::to_channel_count(channels));

	validate();
}
		

audio_buffer::audio_buffer
	(audio_channels_type channels,
	 audio_rate_type rate,
	 const data_buffer& buffer)
	:
	buffer_(buffer),
	channels_(channels),
	rate_(rate)
{
	validate();	
}


audio_buffer::audio_buffer
	(audio_channels_type channels,
	 audio_rate_type rate,
	 data_buffer&& buffer)
	:
	buffer_(std::move(buffer)),
	channels_(channels),
	rate_(rate)
{
	validate();	
}


audio_buffer::audio_buffer
	(const audio_buffer& other)
	:
	buffer_(other.buffer_),
	channels_(other.channels_),
	rate_(other.rate_)
{
	// No need to validate().
	// Original must have been valid.
}


audio_buffer::audio_buffer
	(audio_buffer&& other) NOTHROW
	:
	channels_(audio_channels_type::channels_unknown),
	rate_(audio_rate_type::rate_unknown)
{
	// Swap ensures defaulted other.
	swap(*this, other);

	// No need to validate().
	// Original must have been valid,
	// and default is valid as well.
}


auto_pointer<audio_buffer> audio_buffer::clone() const
	{ return auto_pointer<audio_buffer>(new audio_buffer(*this)); }


audio_buffer&
	audio_buffer::operator =(const audio_buffer& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	audio_buffer offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


audio_buffer&
	audio_buffer::operator =(audio_buffer&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	audio_buffer offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void audio_buffer::swap_with(audio_buffer& other) NOTHROW
{
	swap(buffer_, other.buffer_);
	swap_assign(channels_, other.channels_);
	swap_assign(rate_, other.rate_);
}


const audio_buffer::data_buffer&
	audio_buffer::buffer() const NOTHROW
		{ return buffer_; }
float* audio_buffer::data() NOTHROW
	{ return buffer_.data(); }
const float* audio_buffer::data() const NOTHROW
	{ return buffer_.data(); }


audio_channels_type audio_buffer::channels() const NOTHROW
	{ return channels_; }
audio_rate_type audio_buffer::rate() const NOTHROW
	{ return rate_; }


int64 audio_buffer::channel_count() const NOTHROW
	{ return audio_channels::to_channel_count(channels_); }
int64 audio_buffer::frames_per_second() const NOTHROW
	{ return audio_rate::to_frames_per_second_int64(rate_); }


int64 audio_buffer::sample_count() const NOTHROW
	{ return buffer_.size(); }

int64 audio_buffer::frame_count() const NOTHROW
{
	if (channels_ == audio_channels::channels_unknown ||
		rate_ == audio_rate::rate_unknown)
			return 0;

	// Division has no remainder, see validate().
	return buffer_.size() / channel_count();
}


float audio_buffer::approximate_length_seconds() const NOTHROW
{
	if (channels_ == audio_channels::channels_unknown ||
		rate_ == audio_rate::rate_unknown)
			return 0.f;

	return static_cast<float>(frame_count()) /
		   static_cast<float>(frames_per_second());
}


void audio_buffer::clear() NOTHROW
{
	buffer_.clear();
	channels_ = audio_channels::channels_unknown;
	rate_ = audio_rate::rate_unknown;
}


void audio_buffer::validate()
{
	if (channels_ < 0 || channels_ >= audio_channels::channels_count)
	{
		LOG_ERROR("Invalid channels.");
		throw invalid_parameter();
	}

	if (rate_ < 0 || rate_ >= audio_rate::rate_count)
	{
		LOG_ERROR("Invalid rate.");
		throw invalid_parameter();
	}

	if (channels_ == audio_channels::channels_unknown ||
		rate_ == audio_rate::rate_unknown)
	{
		if (!buffer_.empty())
		{
			LOG_ERROR("Audio buffers with unknown channels or rate must be empty.");
			throw invalid_parameter();
		}
	}
	else
	{
		if (channel_count() == 0)
		{
			LOG_ERROR("Known channels constant has zero channel count.");
			throw assert_failed();
		}

		if (frames_per_second() == 0)
		{
			LOG_ERROR("Known rate constant has zero frames per second.");
			throw assert_failed();
		}

		if (buffer_.size() > max_sample_count)
		{
			LOG_ERROR("Too many samples.");
			throw invalid_parameter();
		}

		if (buffer_.size() % channel_count() != 0)
		{
			LOG_ERROR("Not a sample per channel and frame.");
			throw invalid_parameter();
		}
	}
}


/** Current limit is 10 minutes of 7.1 channel audio at 96khz. */
const int64 audio_buffer::max_sample_count =
	10 * 60 * 8 * 96000;




} /* namespace viral_core */
