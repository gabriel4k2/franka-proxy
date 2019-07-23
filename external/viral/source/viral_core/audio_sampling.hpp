/**
 *************************************************************************
 *
 * @file audio_sampling.hpp
 *
 * Sampling parameters for audio.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_SAMPLING_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_SAMPLING_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_channels
 * @ingroup audio_module
 *
 * Defines device-independent constants for audio channel setups.
 *
 * Usage notes:
 * - @attent Enum constants (e.g. @bref{surround_5_1}) do not match
 *		with channel counts (e.g. 6). Use to_channel_count() to get
 *		the number of channels for an enum constant.
 *
 * Implementation notes:
 * - Integer counts for audio channels do not specify speaker positions,
 *		and separate channel maps (e.g. as in wave files) are cumbersome.
 *		Instead, we internally use a fixed enumeration of audio channels
 *		with predefined speaker_position()s.
 * - We only support common channel constellations. If required, back-ends
 *		can upmix any missing channels (e.g. for uncommon 5.2 speaker sets)
 *		from available channels without much loss of fidelity.
 *
 ************************************************************************/
class audio_channels :
	private non_constructable
{

	public:

		/** See audio_channels class documentation. */
		enum type
		{

			/** Unknown speaker setup, no channels. */
			channels_unknown,


			/**
			 * One speaker directly in front of the listener
			 * and at ear distance.
			 */
			mono,


			/**
			 * Traditional stereo with two speakers.
			 * Ear distance is the distance to either speaker.
			 *
			 * Sample order: (left, right).
			 */
			stereo,


			/**
			 * Four speakers in a square around the listener. 
			 * Ear distance is the distance to either speaker.
			 *
			 * Sample order:
			 * (front-left, front-right, back-left, back-right).
			 */
			surround_4,


			/**
			 * A variant of @bref{surround_4} with another speaker
			 * in the middle of both front speakers.
			 *
			 * Sample order:
			 * (front-left, front-right, center, back-left, back-right).
			 */
			surround_5,


			/**
			 * Adds a low-frequency speaker to @bref{surround_5}
			 * at the same place as the center speaker.
			 *
			 * Sample order:
			 * (front-left, front-right, center,
			 * subwoofer, back-left, back-right).
			 */
			surround_5_1,


			channels_count
		};


		/**
		 * Return the number of channels for a @bref{channels} constant,
		 * or zero if @bref{channels} is not a defined enum value.
		 */
		static int64 to_channel_count(type channels) NOTHROW;


		/**
		 * Return an audio_channels_type with given @bref{channel_count},
		 * or @bref{channels_unknown} if there is no matching enum value.
		 * If there are multiple, matching audio_channels_type constants
		 * with given @bref{channel_count}, then any of these may result.
		 */
		static type to_channels(int64 channel_count) NOTHROW;


		/**
		 * Fetch 3D position for @bref{speaker_index} in @bref{channels}
		 * at @bref{ear_distance} in generic units, or an origin vector()
		 * for invalid parameters (e.g. invalid index, negative distance).
		 */
		static vector speaker_position
			(int64 speaker_index, type channels, float ear_distance) NOTHROW;


	private:

		static const int64 channel_count_for_channels[channels_count];
};


typedef audio_channels::type audio_channels_type;




/**
 *************************************************************************
 *
 * @class audio_rate
 * @ingroup audio_module
 *
 * Defines device-independent constants for supported audio rates.
 *
 * Usage notes:
 * - Back-ends should run at @bref{rate_44100hz} or @bref{rate_48000hz},
 *		with a final conversion to the rate	of the audio device.
 * - Higher rates (i.e. @bref{rate_96000hz}) do not improve audio quality,
 *		but cost memory and performance. Certain audio_effect%s may still
 *		require oversampling (e.g. to detect inter-sample peaks).
 * - Lower rates (e.g. @bref{rate_22050hz}) help conserve system memory
 *		on less important audio samples.
 * - @attent Enum constants (e.g. @bref{rate_48000hz}) do not match
 *		with numeric values for respective frequencies (e.g. 48000).
 *		Use to_frames_per_second_int() or to_frames_per_second_float()
 *		to get numeric frequencies.
 *
 * Implementation notes:
 * - Arbitrary integers as sampling rates can potentially cause issues
 *		(e.g. accidential sampling at 1hz), and are usually not needed.
 *		Thus, we fix sampling rates to a few compile-time constants.
 *
 ************************************************************************/
class audio_rate :
	private non_constructable
{

	public:

		/** See audio_rate class documentation. */
		enum type
		{
			rate_unknown,
			rate_96000hz,
			rate_48000hz,
			rate_44100hz,
			rate_22050hz,
			rate_16000hz,
			rate_8000hz,
			rate_count
		};


		/**
		 * Convert between rate enums and numeric frequency.
		 * Undefined enum constants return a zero frequency,
		 * while frequencies without matching enum constant
		 * return @bref{rate_unknown}.
		 */
		//@{
		static int64 to_frames_per_second_int64(type rate) NOTHROW;
		static float to_frames_per_second_float(type rate) NOTHROW;

		static type to_rate(int64 frames_per_second) NOTHROW;
		//@}


	private:

		static const int64 frames_per_second_for_rate_int64[rate_count];
		static const float frames_per_second_for_rate_float[rate_count];
};


typedef audio_rate::type audio_rate_type;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_SAMPLING_HPP) */
