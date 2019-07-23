/**
 *************************************************************************
 *
 * @file waveform_raw.hpp
 *
 * Header for uncompressed host-memory waveforms.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__WAVEFORM_RAW_HPP)
#define INCLUDED__VIRAL_CORE__WAVEFORM_RAW_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "string_forward.hpp"
#include "file_forward.hpp"


#include "buffer.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class waveform_raw
 * @ingroup audio_module
 *
 * Uncompressed in-memory waveform.
 *
 * Implementation notes:
 * - To keep usage simple, waveform_raw only stores 16 bit PCM data.
 *		This resolution should be sufficient for most mastered audio.
 *		Load()ing silently converts other resolutions as required.
 * - Implementations should still render audio at higher bit depths
 *		for improved headroom (e.g. for overlapping audio sources).
 *
 ************************************************************************/
class waveform_raw
{

	public:

		typedef dynamic_buffer<int16> data_buffer;


		/**
		 * Create empty waveform with unknown channels and rate.
		 * Never throws on construction.
		 */
		waveform_raw() NOTHROW;


		/**
		 * Create waveform with @bref{channels} and @bref{rate}.
		 * Optionally, initialize the waveform with @bref{data}
		 * or with @bref{frame_count} audio frames, the latter
		 * with uninitialized sample values.
		 *
		 * On any failure (e.g. due to invalid frame count),
		 * construction throws an exception.
		 */
		//@{
		waveform_raw
			(audio_channels_type channels,
			 audio_rate_type rate);
		waveform_raw
			(audio_channels_type channels,
			 audio_rate_type rate,
			 int64 frame_count);
		waveform_raw
			(const data_buffer& buffer,
			 audio_channels_type channels,
			 audio_rate_type rate);
		waveform_raw
			(data_buffer&& buffer,
			 audio_channels_type channels,
			 audio_rate_type rate);
		//@}


		/**
		 * Copy or move construct from @bref{other} waveform.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		waveform_raw(const waveform_raw& other);
		waveform_raw(waveform_raw&& other) NOTHROW;
		//@}


		/** Create not-@NULL duplicate of this waveform. */
		auto_pointer<waveform_raw> clone() const;


		/**
		 * Copy or move assign of @bref{other} waveform.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		waveform_raw& operator =(const waveform_raw& other);
		waveform_raw& operator =(waveform_raw&& other) NOTHROW;
		//@}


		void swap_with(waveform_raw& other) NOTHROW;


		/** Access waveform data and properties. */
		//@{
		const data_buffer& buffer() const NOTHROW;

		int16* data() NOTHROW;
		const int16* data() const NOTHROW;

		audio_channels_type channels() const NOTHROW;
		audio_rate_type rate() const NOTHROW;

		int64 channel_count() const NOTHROW;
		int64 frames_per_second() const NOTHROW;

		int64 sample_count() const NOTHROW;
		int64 frame_count() const NOTHROW;

		float approximate_length_seconds() const NOTHROW;
		//@}


		void clear() NOTHROW;


		/** Load waveform from some .wav file. */
		//@{
		static waveform_raw load
			(const string& path, file_context& files);
		static waveform_raw load
			(file& input);
		//@}


		/** Implementation-specific limit to sample count. */
		static const int64 max_sample_count;


	private:

		void validate();


		data_buffer buffer_;
		audio_channels_type channels_;
		audio_rate_type rate_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__WAVEFORM_RAW_HPP) */
