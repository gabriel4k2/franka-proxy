/**
 *************************************************************************
 *
 * @file ms_audio_util.cpp
 *
 * Utilities for audio on Microsoft Windows, implementation.
 *
 ************************************************************************/


#include "ms_audio_util.hpp"


#include "log.hpp"
#include "integer_checks.hpp"
#include "audio_buffer.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_audio_render_format
//
//////////////////////////////////////////////////////////////////////////


ms_audio_render_format::ms_audio_render_format() NOTHROW
	:
	channels_(audio_channels::channels_unknown),
	rate_(audio_rate::rate_unknown)
{ }


void ms_audio_render_format::set
	(audio_channels_type channels,
	 audio_rate_type rate) NOTHROW
{
	MUTEX_SCOPE(render_format_mutex_);
	channels_ = channels;
	rate_ = rate;
}


void ms_audio_render_format::get
	(audio_channels_type& channels,
	 audio_rate_type& rate) const NOTHROW
{
	MUTEX_SCOPE(render_format_mutex_);
	channels = channels_;
	rate = rate_;
}




//////////////////////////////////////////////////////////////////////////
//
// ms_audio_buffer_queue
//
//////////////////////////////////////////////////////////////////////////


ms_audio_buffer_queue::ms_audio_buffer_queue()
	:
	first_buffer_completed_samples_(0)
{ }


void ms_audio_buffer_queue::submit_buffer
	(auto_pointer<audio_buffer> buffer)
{
	MUTEX_SCOPE(queue_mutex_);
	buffers_.push_back(std::move(buffer));
}


void ms_audio_buffer_queue::fetch_buffers
	(float* dest_buffer,
	 int64 dest_frame_count,
	 audio_channels_type channels,
	 audio_rate_type rate) NOTHROW
{
	MUTEX_SCOPE(queue_mutex_);


	// Check preconditions.
	if (dest_frame_count < 0)
	{
		LOG_ERROR("Cannot fetch less than zero frames.");
		return; // Exception safety: NOTHROW.
	}

	if (!dest_buffer && dest_frame_count > 0)
	{
		LOG_ERROR("Cannot fetch into NULL destination buffer.");
		return; // Exception safety: NOTHROW.
	}


	// Fetch samples into destination buffer.
	int64 dest_channel_count =
		audio_channels::to_channel_count(channels);

	if (!integer_checks::is_mult_safe
			(dest_frame_count, dest_channel_count))
	{
		LOG_ERROR("Requested too many target frames.");
		return; // Exception safety: NOTHROW;
	}

	int64 dest_sample_count =
		dest_channel_count * dest_frame_count;

	while (dest_sample_count > 0)
	{


		// No more source buffers remaining.
		// Client did not render audio often enough,
		// or no audio master currently is selected.
		if (buffers_.empty())
		{
			memory::fill_with_zero_typed
				(dest_buffer, dest_sample_count);

			return;
		}		

		
		// Fetch next source buffer.
		// Discard source buffer on mismatching format
		// and continue on another buffer.
		const audio_buffer& source_buffer =
			**buffers_.first();
		
		if (source_buffer.channels() != channels ||
			source_buffer.rate() != rate)
		{
			pop_front_buffer();
			continue;
		}


		// Copy over samples from source buffer.
		// Note source buffers may be copied partially.
		int64 buffer_sample_count =
			source_buffer.sample_count() -
			first_buffer_completed_samples_;

		int64 copy_sample_count =
			min(buffer_sample_count, dest_sample_count);

		memory::copy_block_typed
			(dest_buffer,
			 source_buffer.data() + first_buffer_completed_samples_,
			 copy_sample_count);


		// Update current copy state.
		// If finished, delete source buffer.
		dest_buffer += copy_sample_count;
		dest_sample_count -= copy_sample_count;

		first_buffer_completed_samples_ += copy_sample_count;
		if (first_buffer_completed_samples_ >= source_buffer.sample_count())
			pop_front_buffer();
	}	
}


int64 ms_audio_buffer_queue::buffer_count() const NOTHROW
{
	MUTEX_SCOPE(queue_mutex_);
	return buffers_.size();
}


void ms_audio_buffer_queue::discard_stale_buffers() NOTHROW
{
	MUTEX_SCOPE(queue_mutex_);	

	float front_buffer_playing_seconds =
		last_buffer_removed_timer_.seconds_passed();

	while (true)
	{
		buffer_list::iterator first(buffers_.first());
		if (!first) break;

		float front_buffer_finish_seconds =
			(*first)->approximate_length_seconds() *
			stale_buffer_tolerance_factor_;

		if (front_buffer_playing_seconds <
			front_buffer_finish_seconds)
				break;


		// The front buffer should have finished playback by now,
		// but has not (e.g. because no audio device is connected). 
		// Drop the stale buffer to allow audio sources to progress.
		// Assume that the buffer took time plus tolerance to play,
		// and under that assumption, check the next queued buffer.
		pop_front_buffer();
		front_buffer_playing_seconds -= front_buffer_finish_seconds;
	}
}


void ms_audio_buffer_queue::clear() NOTHROW
{
	MUTEX_SCOPE(queue_mutex_);

	while (!buffers_.empty())
		pop_front_buffer();
}


void ms_audio_buffer_queue::pop_front_buffer() NOTHROW
{
	// Remove front buffer from the queue
	// and reset completion for the next buffer.
	buffers_.pop_front();
	first_buffer_completed_samples_ = 0;


	// If any buffer was removed, the first remaining buffer
	// should now begin its playback. Thus, restart the timer
	// for the next discard_stale_buffers() pass.
	try { last_buffer_removed_timer_.restart(); }
	catch (...)
	{
		// Exception safety: NOTHROW.
		LOG_ERROR("Exception on restarting timer for stale buffers.");
	}
}


/**
 * Factor over original playback time
 * at which buffers are considered stale.
 */
const float ms_audio_buffer_queue::stale_buffer_tolerance_factor_ = 1.5f;




} /* namespace viral_core */
