/**
 *************************************************************************
 *
 * @file audio.hpp
 *
 * Device-independent audio layer.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "audio_forward.hpp"


#include "auto_pointer.hpp"
#include "tree_map.hpp"
#include "thread_synch.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup audio_module Audio Module
 *
 * Realizes device-independent audio output via an audio_context.
 * 
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class audio_context
 * @ingroup audio_module
 *
 * Base class for device-independent audio output.
 *
 * Audio overview:
 * - An audio_context requires covert_object%s, covert_id%s,
 *		covert_command%s and covert_data for all its operations.
 *		See the @ref covert_module "Covert Module" for details.
 * - Clients control playback by passing an audio_command_queue
 *		with various covert_command%s to audio_context::execute().
 *		For instance, an audio_command_play starts audio playback
 *		on some given audio source. 
 * - To render audio output into the OS-side buffers, clients must
 *		regularly send audio_command_render to their audio_context
 *		(e.g. through audio_command_queue::render() or audio_task).
 *		Otherwise, buffer underruns may cause severe stuttering.
 * - Audio rendering requires a single, enabled audio master bus
 *		(see audio_command_queue::set_master()). The audio master
 *		links to multiple audio scenes, which in turn are targets
 *		for individual audio sources that play waveforms.
 *
 * Usage notes:
 * - To avoid stutter, clients should split commits into small batches.
 *		Furthermore, clients may mute all audio playback during commits,
 *		or clients may manually add intermediate audio_command_render%s
 *		to excessive audio_command_queue%s.
 * - An audio_context does not provide strong exception safety.
 *		Notably, execute() may fail after partial object updates
 *		or after partial audio rendering, and the audio_context
 *		may remain in a dead state. Thus, clients should recreate
 *		the audio_context and recommit all data on any exception.
 * - An audio_context is threadsafe for use by arbitrary clients.
 *		Since processing of audio commands in general is efficient,
 *		clients usually need not consider potential blocking.
 *
 * Implementation notes:
 * - See audio_buffer for audio concepts and generic nomenclature.
 * - The base audio_context already handles most of the playback
 *		infrastructure, including object management and thread safety.
 * - For thread safety reasons, audio internals should be protected
 *		from client access by friending and restricted constructors.
 *
 ************************************************************************/
class audio_context :
	private non_copyable
{

	public:

		virtual ~audio_context() NOTHROW;


		/**
		 * Wait until the audio_context is idle,
		 * then perform all commands of incoming
		 * command @bref{queue} in FIFO order.
		 */
		void execute(audio_command_queue&& queue);


		const string context_name;


	protected:

		audio_context(const string& context_name);


		const covert_object_manager& object_manager() const;


		/**
		 * Create a not-@NULL covert_object suited for given @bref{id}.
		 * The @bref{audio_mutex_} has already been acquired.
		 */
		virtual auto_pointer<covert_object> create_object
			(const shared_pointer<covert_id>& id) = 0;


		/**
		 * Start or stop playback on some audio source.
		 * The @bref{audio_mutex_} has already been acquired.
		 */
		//@{
		virtual void play(const audio_command_play& play_command) = 0;
		virtual void stop(const audio_command_stop& stop_command) = 0;
		//@}


		/**
		 * Set a new audio master.
		 * The @bref{audio_mutex_} has already been acquired.
		 */
		virtual void set_master
			(const audio_command_master& master_command) = 0;


		/**
		 * Render to audio buffers.
		 * The @bref{audio_mutex_} has already been acquired.
		 */
		virtual void render
			(const audio_command_render& render_command) = 0;


		/**
		 * Release leaked covert_object%s from the covert_object_manager.
		 * Destructors of context implementations must explicitly call this,
		 * since ~covert_object() usually depends on a valid audio API.
		 */
		void clear_on_destruction();


	private:

		void execute_commit(covert_command& command);
		void execute_query(covert_command& command);
		void execute_purge(covert_command& command);

		void execute_play(covert_command& command);
		void execute_stop(covert_command& command);
		void execute_master(covert_command& command);
		void execute_render(covert_command& command);


		mutex audio_mutex_;

		const auto_pointer<covert_object_manager> object_manager_;

		typedef void (audio_context::*command_handler)(covert_command& command);
		typedef tree_map<type_info_ref, command_handler> command_handler_map;
		command_handler_map command_handlers_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_HPP) */
