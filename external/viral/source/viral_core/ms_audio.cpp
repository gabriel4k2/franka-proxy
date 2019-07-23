/**
 *************************************************************************
 *
 * @file ms_audio.cpp
 *
 * Audio on Microsoft Windows, implementation.
 *
 ************************************************************************/


#include "ms_audio.hpp"


#if defined(VIRAL__HAVE_MS_AUDIO)


#include "log.hpp"
#include "audio_buffer.hpp"
#include "audio_command.hpp"
#include "audio_graph.hpp"
#include "ms_audio_util.hpp"
#include "ms_audio_sink.hpp"
#include "ms_audio_graph.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_audio_context
//
//////////////////////////////////////////////////////////////////////////


ms_audio_context::ms_audio_context
	(const string& context_name)
	:
	audio_context(context_name)
{
	LOG_INFO("Using ms_audio_context implementation.");

	register_object_creators();

	// Explicitly create sink components here
	// for correct logging order.
	render_format_.reset(new ms_audio_render_format());
	buffer_queue_.reset(new ms_audio_buffer_queue());
	sink_.reset(new ms_audio_sink(*render_format_, *buffer_queue_));
}


ms_audio_context::~ms_audio_context() NOTHROW
{
	LOG_INFO("Destroying.");

	// Explicitly destroy covert objects before WASAPI.
	// As current implementations do not depend on WASAPI,
	// this is only a safety measure for future updates.
	clear_on_destruction();

	// Explicitly destroy sink components here
	// for correct logging order.
	sink_.reset();
	buffer_queue_.reset();
	render_format_.reset();
}


auto_pointer<covert_object> ms_audio_context::create_object
	(const shared_pointer<covert_id>& id)
{
	if (!id)
	{
		LOG_ERROR("Cannot create object for NULL ID.");
		throw null_pointer();
	}

	object_creator_map::iterator creator
		(object_creators_.find(typeid(*id)));

	if (!creator || !creator->value)
	{
		LOG_ERROR("No object implemented for ID: " + id->debug_name);
		throw unsupported_feature();
	}

	return (this->*(creator->value))();
}


void ms_audio_context::play
	(const audio_command_play& play_command)
{
	ms_audio_source& audio_source =
		object_manager().object_for_id<ms_audio_source>
			(play_command.source_id);

	audio_source.play();
}


void ms_audio_context::stop
	(const audio_command_stop& stop_command)
{
	ms_audio_source& audio_source =
		object_manager().object_for_id<ms_audio_source>
			(stop_command.source_id);

	audio_source.stop();
}


void ms_audio_context::set_master
	(const audio_command_master& master_command)
		{ master_id_ = master_command.master_id; }


void ms_audio_context::render
	(const audio_command_render& render_command)
{
	/**
	 * Fill up any missing buffers in ms_audio_buffer_queue
	 * by rendering corresponding audio. 
	 *
	 * Timing notes:
	 * - If clients render too often, then no buffers are missing.
	 *		In this case, we skip rendering, until either feeding
	 *		or discarding buffers left free space in the queue.
	 * - If clients miss @bref{render_time_seconds} past tolerance,
	 *		then the sink starves and plays intermediate silence.
	 * - Without render()ing, audio sources do not skip ahead.
	 * - See audio_source_data for further timing details.
	 *
	 * Implementation notes:
	 * - We separately submit buffers and query for existing buffers.
	 *		Thus, we might not reach the @bref{target_buffer_count_}
	 *		due to a race condition. This is not actually a problem:
	 *		The next render()ing call will insert any missed buffers,
	 *		and we already have tolerance in @bref{frame_count_total}
	 *		for any missed rendering timesteps.
	 * - On a rendering exception, we drop one (or more) audio buffers.
	 *		Audio sources may advance partially, see audio_context.
	 * - The alternative of resetting audio state (e.g. in audio scenes)
	 *		will still cause audio glitches and may even lock processing
	 *		to the current audio frame on repeated exceptions.
	 */


	// Notify clients if the ms_audio_sink thread failed.
	// In this case, no further audio playback is possible.
	sink_->throw_if_terminated();


	// Drop any stale buffers.
	// This causes later rendering to advance audio sources
	// even if the sink does not consume any pending buffers.
	// See audio_source_data playback and timing for details.
	buffer_queue_->discard_stale_buffers();


	// Fetch master bus as entry point into audio rendering.
	// There is nothing to render if there is no audio master.
	shared_pointer<audio_master_id> locked_master_id
		(master_id_.lock());

	if (!locked_master_id)
		return;

	ms_audio_master& audio_master =
		object_manager().object_for_id<ms_audio_master>
			(locked_master_id);


	// Fetch target rendering format for current pass.
	// Use defaults if no back-end device is connected.
	// If the sink should concurrently change the format,
	// later WASAPI feeding discards mismatching buffers.
	audio_channels_type render_channels =
		audio_channels::channels_unknown;
	audio_rate_type render_rate =
		audio_rate::rate_unknown;

	render_format_->get(render_channels, render_rate);

	if (render_channels == audio_channels::channels_unknown)
		render_channels = default_render_channels_;
	if (render_rate == audio_rate::rate_unknown)
		render_rate = default_render_rate_;


	// Determine how large audio buffers should be
	// for client-side estimates on audio timesteps.
	// Take timestep tolerance, target buffer count,
	// and small rounding epsilon into account.
	float buffer_seconds =
		render_command.render_time_seconds *
		render_time_tolerance_factor_;

	float frame_count_total = 
		buffer_seconds *
		audio_rate::to_frames_per_second_float(render_rate);

	int64 frame_count_per_buffer =
		1 + static_cast<int64>
				(frame_count_total / target_buffer_count_);

	if (frame_count_per_buffer <= 0 ||
		frame_count_per_buffer >= max_frame_count_per_buffer_)
	{
		LOG_ERROR("Client buffer estimate exceeds maximum frames.");
		throw bounds_exceeded();
	}


	// Render audio into buffer and submit to our queue
	// until there are (mostly) enough waiting buffers.
	// See timing and implementation notes above.
	int64 missing_buffer_count =
		target_buffer_count_ - buffer_queue_->buffer_count();

	for (int64 i = 0; i < missing_buffer_count; ++i)
	{
		auto_pointer<audio_buffer> buffer
			(new audio_buffer
				(render_channels, render_rate,
				 frame_count_per_buffer));
		
		audio_master.render(*buffer);
		buffer_queue_->submit_buffer(std::move(buffer));
	}
}


void ms_audio_context::register_object_creators()
{
	object_creators_.insert
		(typeid(audio_waveform_id),
		 &ms_audio_context::create_typed_object<ms_audio_waveform>);
	object_creators_.insert
		(typeid(audio_effect_id),
		 &ms_audio_context::create_typed_object<ms_audio_effect>);
	object_creators_.insert
		(typeid(audio_source_id),
		 &ms_audio_context::create_typed_object<ms_audio_source>);
	object_creators_.insert
		(typeid(audio_scene_id),
		 &ms_audio_context::create_typed_object<ms_audio_scene>);
	object_creators_.insert
		(typeid(audio_master_id),
		 &ms_audio_context::create_typed_object<ms_audio_master>);
}


template <typename T>
	auto_pointer<covert_object> ms_audio_context::create_typed_object()
		{ return auto_pointer<covert_object>(new T(object_manager())); }


/** Audio rendering parameters when no device is connected. */
//@{
const audio_channels_type ms_audio_context::default_render_channels_ =
	audio_channels::stereo;
const audio_rate_type ms_audio_context::default_render_rate_ =
	audio_rate::rate_44100hz;
//@}


/**
 * A factor that defines by how much clients may overshoot
 * their expected audio_command_render::render_time_seconds.
 */
const float ms_audio_context::render_time_tolerance_factor_ = 1.5f;


/**
 * Number of buffers that should be enqueued.
 * A larger buffer count causes some overhead,
 * but may avoid threading glitches.
 */
const int64 ms_audio_context::target_buffer_count_ = 4;


/**
 * Maximum frames allowed per audio_buffer
 * to avoid broken estimates for render times.
 */
const int64 ms_audio_context::max_frame_count_per_buffer_ = 16777216;




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_AUDIO) */
