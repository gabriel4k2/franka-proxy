/**
 *************************************************************************
 *
 * @file audio_task.hpp
 *
 * Threaded audio rendering.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__AUDIO_TASK_HPP)
#define INCLUDED__VIRAL_CORE__AUDIO_TASK_HPP


#include "build_config.hpp"


#include "audio_forward.hpp"


#include "string.hpp"
#include "thread.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class audio_task
 * @ingroup audio_module
 *
 * Regularly sends audio_command_render in a separate thread.
 *
 * Usage notes:
 * - Clients must explicitly and separately select an audio master bus
 *		from which to render audio sources (see audio_command_master).
 * - Under heavy audio_context load (e.g. when committing many waveforms),
 *		the audio thread may block before sending an audio_command_render.
 *		Resulting buffer underflows may cause severe audio stuttering.
 *		See the audio_context documentation for solutions to stutter.
 *
 ************************************************************************/
class audio_task :
	private threaded_task
{

	public:

		audio_task
			(const string& audio_task_name,
			 audio_context& audio);


		~audio_task() NOTHROW;


		const string audio_task_name;
		audio_context& audio;


	private:

		void task_main() override;

		static const float render_time_seconds_;
		static const float render_time_tolerance_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__AUDIO_TASK_HPP) */
