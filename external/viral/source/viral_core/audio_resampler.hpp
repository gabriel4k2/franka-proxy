/**
 *************************************************************************
 *
 * @file audio_resampler.hpp
 *
 * Utilities for audio resampling.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_RESAMPLER_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_RESAMPLER_HPP


#include "build_config.hpp"


#include "audio_forward.hpp"


#include "util.hpp"
#include "audio_sampling.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_channels_resampler
 * @ingroup audio_module
 *
 * Utility for changing the audio_channels_type of an audio_buffer.
 *
 ************************************************************************/
class audio_channels_resampler
{

	public:

		/**
		 * Resample @bref{source_buffer} to @bref{new_channels}
		 * and store new sample values in @bref{dest_buffer}.
		 *
		 * The @bref{dest_buffer} resizes to accomodate all frames
		 * of the @bref{source_buffer} for the @bref{new_channels}.
		 * Any allocated memory of @bref{dest_buffer} is reused.
		 *
		 * On any exception, clear @bref{dest_buffer}.
		 */
		void resample
				(const audio_buffer& source_buffer,
				 audio_channels_type new_channels,
				 audio_buffer& dest_buffer);
};




/**
 *************************************************************************
 *
 * @class audio_rate_resampler
 * @ingroup audio_module
 *
 * Utility for changing the audio_rate_type of an audio_buffer.
 *
 ************************************************************************/
class audio_rate_resampler
{

	public:

		/**
		 * Resample @bref{source_buffer} to @bref{new_rate}
		 * and store new sample values in @bref{dest_buffer}.
		 *
		 * The @bref{dest_buffer} resizes to accomodate all frames
		 * of the @bref{source_buffer} for the @bref{new_rate}.
		 * Any allocated memory of @bref{dest_buffer} is reused.
		 *
		 * On any exception, clear @bref{dest_buffer}.
		 */
		void resample
				(const audio_buffer& source_buffer,
				 audio_rate_type new_rate,
				 audio_buffer& dest_buffer);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_RESAMPLER_HPP) */

