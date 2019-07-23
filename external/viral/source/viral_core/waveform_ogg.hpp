/**
 *************************************************************************
 *
 * @file waveform_ogg.hpp
 *
 * Waveforms with OGG compression.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__WAVEFORM_OGG_HPP)
#define INCLUDED__VIRAL_CORE__WAVEFORM_OGG_HPP


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
 * @class waveform_ogg
 * @ingroup audio_module
 *
 * A compressed waveform in OGG format.
 *
 * Usage notes:
 * - The internal @bref{data_} vector holds the entire ogg stream.
 *
 ************************************************************************/
class waveform_ogg
{

	public:

		typedef dynamic_buffer<unsigned char> data_buffer;


		waveform_ogg() NOTHROW;
		waveform_ogg(const data_buffer& buffer);
		waveform_ogg(data_buffer&& buffer);


		/**
		 * Copy or move construct from @bref{other} waveform.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		waveform_ogg(const waveform_ogg& other);
		waveform_ogg(waveform_ogg&& other) NOTHROW;
		//@}


		/** Create not-@NULL duplicate of this waveform. */
		auto_pointer<waveform_ogg> clone() const;


		/**
		 * Copy or move assign of @bref{other} waveform.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		waveform_ogg& operator =(const waveform_ogg& other);
		waveform_ogg& operator =(waveform_ogg&& other) NOTHROW;
		//@}


		void swap_with(waveform_ogg& other) NOTHROW;


		/** Access waveform data and properties. */
		//@{
		const data_buffer& buffer() const NOTHROW;

		unsigned char* data() NOTHROW;
		const unsigned char* data() const NOTHROW;

		audio_channels_type channels() const NOTHROW;
		audio_rate_type rate() const NOTHROW;

		int64 channel_count() const NOTHROW;
		int64 frames_per_second() const NOTHROW;

		int64 sample_count() const NOTHROW;
		int64 frame_count() const NOTHROW;

		float approximate_length_seconds() const NOTHROW;
		//@}


		void clear() NOTHROW;


		/** Load waveform from some .ogg file. */
		//@{
		static waveform_ogg load
			(const string& path, file_context& files);
		static waveform_ogg load
			(file& input);
		//@}


	private:

		void validate();


		data_buffer buffer_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__WAVEFORM_OGG_HPP) */
