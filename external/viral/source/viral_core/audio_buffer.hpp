/**
 *************************************************************************
 *
 * @file audio_buffer.hpp
 *
 * Software audio buffers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_BUFFER_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_BUFFER_HPP


#include "build_config.hpp"


#include "buffer.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_buffer
 * @ingroup audio_module
 *
 * Stores temporary, uncompressed audio data for internal mixing.
 *
 * Channels, frames, and samples:
 * - Audio buffers contain a series of channels over a shared number
 *		of audio frames, with one sample value per channel and frame.
 *		Samples are interleaved over channels, with contiguous memory
 *		for the samples of one frame. See audio_channels for ordering.
 * - Both channels() and rate() are restricted to select enum values.
 *		See audio_channels and audio_rate for details.
 * - Specific enum values denote unsupported channels() and rates().
 *		Audio buffers with unsupported channels or rate are empty.
 *
 * Implementation notes:
 * - Internal audio processing uses 32bit float for additional headroom.
 *		For integer conversions (e.g. as required by some back-end APIs),
 *		all float values should still be normalized to the [0, 1] range.
 * - Due to higher precision, audio_buffer%s carry some memory overhead.
 *		Thus, audio_buffer%s are not suitable for static or long audio.
 *		Both waveform_raw and waveform_ogg offer alternatives here.
 *
 ************************************************************************/
class audio_buffer
{

	public:

		typedef dynamic_buffer<float> data_buffer;


		/**
		 * Create empty audio_buffer with unknown rate and channels.
		 * Never throws an exception.
		 */
		audio_buffer() NOTHROW;


		/**
		 * Create buffer with @bref{channels} and @bref{rate}.
		 * Optionally, initialize the buffer with @bref{buffer}
		 * or with @bref{frame_count} audio frames, the latter
		 * with uninitialized sample values.
		 *
		 * On any failure (e.g. due to invalid frame count),
		 * construction throws an exception.
		 */
		//@{
		audio_buffer
			(audio_channels_type channels,
			 audio_rate_type rate);
		audio_buffer
			(audio_channels_type channels,
			 audio_rate_type rate,
			 int64 frame_count);	
		audio_buffer
			(audio_channels_type channels,
			 audio_rate_type rate,
			 const data_buffer& buffer);
		audio_buffer
			(audio_channels_type channels,
			 audio_rate_type rate,
			 data_buffer&& buffer);
		//@} 


		/**
		 * Copy or move construct from @bref{other} audio buffer.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		audio_buffer(const audio_buffer& other);
		audio_buffer(audio_buffer&& other) NOTHROW;
		//@}


		/** Create not-@NULL duplicate of this audio buffer. */
		auto_pointer<audio_buffer> clone() const;


		/**
		 * Copy or move assign of @bref{other} audio buffer.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		audio_buffer& operator =(const audio_buffer& other);
		audio_buffer& operator =(audio_buffer&& other) NOTHROW;
		//@}


		void swap_with(audio_buffer& other) NOTHROW;


		/** Fetch internal data. */
		//@{
		const data_buffer& buffer() const NOTHROW;
		float* data() NOTHROW;
		const float* data() const NOTHROW;

		audio_channels_type channels() const NOTHROW;
		audio_rate_type rate() const NOTHROW;
		//@}


		/** Calculate buffer details. */
		//@{
		int64 channel_count() const NOTHROW;
		int64 frames_per_second() const NOTHROW;
		int64 sample_count() const NOTHROW;
		int64 frame_count() const NOTHROW;
		//@}


		/** Approximate play time in (fractional) seconds. */
		float approximate_length_seconds() const NOTHROW;


		void clear() NOTHROW;


		/** Maximum number of samples allowed in an audio_buffer. */
		static const int64 max_sample_count;


	private:

		void validate();


		data_buffer buffer_;
		audio_channels_type channels_;
		audio_rate_type rate_;
};


 

} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_BUFFER_HPP) */
