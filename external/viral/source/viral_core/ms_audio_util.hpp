/**
 *************************************************************************
 *
 * @file ms_audio_util.hpp
 *
 * Utilities for audio on Microsoft Windows.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__MS_AUDIO_UTIL_HPP


#include "build_config.hpp"


#include "audio_forward.hpp"


#include "util.hpp"
#include "owning_list.hpp"
#include "timer.hpp"
#include "thread_synch.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_audio_render_format
 * @ingroup audio_module
 *
 * Format in which ms_audio_sink expects the ms_audio_buffer_queue.
 *
 * Threading notes:
 * - The internal ms_audio_sink thread may set() another render format
 *		when users connect, disconnect or change default audio devices.
 * - An ms_audio_render_format is threadsafe for use by arbitrary threads.
 *		To avoid race conditions, the ms_audio_render_format only allows
 *		to set() or get() all parameters at once.
 *
 ************************************************************************/
class ms_audio_render_format :
	private non_copyable
{

	public:

		ms_audio_render_format() NOTHROW;

		void set
			(audio_channels_type channels,
			 audio_rate_type rate) NOTHROW;

		void get
			(audio_channels_type& channels,
			 audio_rate_type& rate) const NOTHROW;


	private:

		mutable mutex render_format_mutex_;

		audio_channels_type channels_;
		audio_rate_type rate_;
};




/**
 *************************************************************************
 *
 * @class ms_audio_buffer_queue
 * @ingroup audio_module
 *
 * Threadsafe storage for pending audio_buffer%s.
 *
 * Threading notes:
 * - Rendering from client threads (e.g. audio_task%s) enqueues buffers,
 *		while the internal ms_audio_sink thread feeds buffers to WASAPI.
 * - An ms_audio_buffer_queue is threadsafe for use by arbitrary threads.
 *		However, only the ms_audio_sink thread should fetch_buffers().
 *
 ************************************************************************/
class ms_audio_buffer_queue :
	private non_copyable
{

	public:

		ms_audio_buffer_queue();


		/** Submit a @bref{buffer} to the queue. */
		void submit_buffer
			(auto_pointer<audio_buffer> buffer);


		/**
		 * Take @bref{dest_frame_count} from queued @bref{buffers_},
		 * fill these frames into the @bref{dest_buffer}, and finally
		 * remove finished or invalid @bref{buffers_} from the queue.
		 *
		 * If less than @bref{dest_frame_count} frames are pending,
		 * fill the remaining @bref{dest_buffer} frames with zero.
		 *
		 * Silently discards queued and touched buffers that mismatch
		 * @bref{rate} or @bref{channels} (e.g. due to device change).
		 */
		void fetch_buffers
			(float* dest_buffer,
			 int64 dest_frame_count,
			 audio_channels_type channels,
			 audio_rate_type rate) NOTHROW;


		/** Number of currently enqueued buffers. */
		int64 buffer_count() const NOTHROW;


		/**
		 * Discard pending buffers that fetch_buffers() should,
		 * with some tolerance, already have removed.
		 */
		void discard_stale_buffers() NOTHROW;


		/** Discard all enqueued buffers. */
		void clear() NOTHROW;


	private:

		void pop_front_buffer() NOTHROW;


		static const float stale_buffer_tolerance_factor_;

		mutable mutex queue_mutex_;
		
		typedef owning_list<audio_buffer> buffer_list;
		buffer_list buffers_;

		int64 first_buffer_completed_samples_;
		free_timer last_buffer_removed_timer_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_AUDIO_UTIL_HPP) */
