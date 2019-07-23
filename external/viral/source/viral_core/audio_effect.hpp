/**
 *************************************************************************
 *
 * @file audio_effect.hpp
 *
 * Audio effect algorithms.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_EFFECT_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_EFFECT_HPP


#include "build_config.hpp"


#include "audio_forward.hpp"


#include "auto_pointer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_effect_state
 * @ingroup audio_module
 *
 * Stores stream-specific, persistent state of an audio_effect.
 *
 * Effects and states:
 * - See audio_effect for the rationale of effects and states.
 *
 ************************************************************************/
class audio_effect_state
{

	public:

		virtual ~audio_effect_state() NOTHROW { }

		virtual auto_pointer<audio_effect_state> clone() const = 0;
};




/**
 *************************************************************************
 *
 * @class audio_effect
 * @ingroup audio_module
 *
 * Base class for audio effects.
 *
 * Effects and states:
 * - A single audio_effect instance may process() multiple audio streams
 *		(e.g. several playing waveforms, multiple inserts on a source).
 *		Therefore, an audio_effect must not cache stream-specific data,
 *		such as running averages or buffered sample values.
 * - An audio_effect instance only stores stream-independent parameters
 *		(e.g. gain, attack, release) and exposes these to clients.
 * - In contrast, an audio_effect_state holds all stream-specific data
 *		(e.g. buffered samples, running averages) for an audio_effect.
 * - The back-end supplies process() with an individual and persistent
 *		@bref{state} pointer that matches with the current audio stream.
 *		An audio_effect may store its audio_effect_state in this pointer
 *		and later process() on the same stream obtains the same pointer.
 *
 * Look-ahead buffering:
 * - If audio effects require look-ahead, then they may buffer samples
 *		while delaying output and storing silence. However, look-ahead
 *		of excessive samples should be avoided due to latency concerns.
 * - Alternatively, a priori look-ahead in waveforms reduces latency,
 *		but only for the first processed buffer of each audio_effect.
 *		Thus, a priori look-ahead has not been implemented.
 *
 ************************************************************************/
class audio_effect
{
	public:

		virtual ~audio_effect() NOTHROW { }

		virtual auto_pointer<audio_effect> clone() const = 0;


		/**
		 * Apply an audio effect onto given @bref{buffer}
		 * and write result back over the existing samples.
		 *
		 * Separately stores or reuses a stream @bref{state}.
		 * The @bref{state} may be @NULL and should be checked
		 * for the correct type of @bref{audio_effect_state}.
		 * See audio_effect class documentation for details.
		 *
		 * If @bref{restart}ing, disregard cached state,
		 * such as due to a pause in audio rendering.
		 */
		virtual void process
			(audio_buffer& buffer,
			 auto_pointer<audio_effect_state>& state,
			 bool restart) = 0;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_EFFECT_HPP) */
