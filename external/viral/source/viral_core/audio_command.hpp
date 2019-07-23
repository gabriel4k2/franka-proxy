/**
 *************************************************************************
 *
 * @file audio_command.hpp
 *
 * Covert commands specific to audio contexts.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_COMMAND_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_COMMAND_HPP


#include "build_config.hpp"


#include "covert_command.hpp"
#include "audio_graph.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_command_play
 * @ingroup audio_module
 *
 * Start playing on an audio_source_id.
 *
 * Usage notes:
 * - Playing sources keep playing when receiving another play command.
 * - If @bref{set_frame} is @true, then playback skips to @bref{frame},
 *		even if the @bref{source_id} is already playing. This enables
 *		resetting (i.e. by @bref{set_frame} @true, @bref{frame} zero)
 *		or resuming (i.e. by @bref{set_frame} @false) playback.
 * - By default, audio_command_play does not set a new source frame.
 * - See audio_source_data for negative or past-end @bref{frame}s.
 *
 ************************************************************************/
class audio_command_play :
	public covert_command
{

	public:

		audio_command_play
			(const shared_pointer<audio_source_id>& source_id);
		audio_command_play
			(const shared_pointer<audio_source_id>& source_id,
			 bool set_frame, int64 frame);
		audio_command_play
			(const audio_command_play& other);

		auto_pointer<covert_command> clone() const override;


		const shared_pointer<audio_source_id> source_id;

		const bool set_frame;
		const int64 frame;


	private:

		void validate();
};




/**
 *************************************************************************
 *
 * @class audio_command_stop
 * @ingroup audio_module
 *
 * Stop playing on an audio_source_id.
 * 
 * Usage notes:
 * - Audio sources ignore this command if they are not playing.
 *
 ************************************************************************/
class audio_command_stop :
	public covert_command
{

	public:

		audio_command_stop
			(const shared_pointer<audio_source_id>& source_id);
		audio_command_stop
			(const audio_command_stop& other);

		auto_pointer<covert_command> clone() const override;


		const shared_pointer<audio_source_id> source_id;


	private:

		void validate();
};




/**
 *************************************************************************
 *
 * @class audio_command_master
 * @ingroup audio_module
 *
 * Select an audio_master_id for subsequent audio rendering.
 *
 * Usage notes:
 * - A @NULL @bref{master_id} is valid and mutes all audio.
 *		See audio_command_render for details.
 *
 ************************************************************************/
class audio_command_master :
	public covert_command
{

	public:

		audio_command_master
			(const shared_pointer<audio_master_id>& master_id);
		audio_command_master
			(const audio_command_master& other);

		auto_pointer<covert_command> clone() const override;


		const shared_pointer<audio_master_id> master_id;
};




/**
 *************************************************************************
 *
 * @class audio_command_render
 * @ingroup audio_module
 *
 * Updates OS-side audio buffers from the current audio_master_id.
 *
 * Usage notes:
 * - If the client has not (yet) selected any audio_master_id,
 *		the audio_context silently ignores audio_command_render.
 *		This avoids a race condition with running audio_task%s
 *		during application startup.
 * - Clients must provide an approximated @bref{render_time_seconds}
 *		until the next audio_command_render is sent to audio_context.
 *		Implementations use this render time to select buffer sizes
 *		for internal buffering. Implementations must oversize buffers
 *		to take imprecise timing into account, but clients may still
 *		cause stuttering if they considerably miss their render time.
 *
 ************************************************************************/
class audio_command_render :
	public covert_command
{

	public:

		audio_command_render
			(float render_time_seconds);
		audio_command_render
			(const audio_command_render& other);

		auto_pointer<covert_command> clone() const override;


		const float render_time_seconds;


	private:

		void validate();
};




/**
 *************************************************************************
 *
 * @class audio_command_queue
 * @ingroup audio_module
 *
 * Offers audio-specific covert_command%s.
 *
 ************************************************************************/
class audio_command_queue :
	public covert_command_queue
{

	public:

		audio_command_queue() NOTHROW;


		/**
		 * Copy or move construct from @bref{other} command queue.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		audio_command_queue(const audio_command_queue& other);
		audio_command_queue(audio_command_queue&& other) NOTHROW;
		//@}


		/**
		 * Copy or move assign of @bref{other} command queue.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		audio_command_queue& operator =(const audio_command_queue& other);
		audio_command_queue& operator =(audio_command_queue&& other) NOTHROW;
		//@}


		void swap_with(audio_command_queue& other) NOTHROW;


		/**
		 * Start or stop playback on @bref{source_id}.
		 * See audio_command_play and audio_command_stop.
		 */
		//@{
		void play
			(const shared_pointer<audio_source_id>& source_id);
		void play
			(const shared_pointer<audio_source_id>& source_id,
			 bool set_frame, int64 frame);

		void stop
			(const shared_pointer<audio_source_id>& source_id);
		//@}


		/** Select a (possibly @NULL) audio master for playback. */
		void set_master
			(const shared_pointer<audio_master_id>& master_id);


		/**
		 * Render audio from current audio master.
		 *
		 * Audio rendering uses an approximate @bref{render_time_seconds}
		 * to select buffer sizes. See audio_render_command for details.
		 */
		void render(float render_time_seconds);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_COMMAND_HPP) */

