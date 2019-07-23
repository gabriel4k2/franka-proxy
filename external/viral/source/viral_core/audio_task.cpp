/**
 *************************************************************************
 *
 * @file audio_task.cpp
 *
 * Threaded audio rendering, implementation.
 *
 ************************************************************************/


#include "audio_task.hpp"


#include "log.hpp"
#include "timer.hpp"
#include "audio.hpp"
#include "audio_command.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// audio_task
//
//////////////////////////////////////////////////////////////////////////


audio_task::audio_task
	(const string& audio_task_name,
	 audio_context& audio)
	:
	audio_task_name(audio_task_name),
	audio(audio)
{
	LOG_INFO("Creating: " + audio_task_name);
	LOG_INFO("Creating and running internal thread.");
	start(); 
}


audio_task::~audio_task() NOTHROW
{
	LOG_INFO("Destroying.");
	LOG_INFO("Terminating internal thread.");

	try { join(); }
	catch (...)
	{
		LOG_CRITICAL
			("Internal thread has thrown an exception on joining.");
	}

	LOG_INFO("Destroyed: " + audio_task_name);
}


void audio_task::task_main()
{
	free_timer render_timer;

	while (!join_now())
	{
		// Timing in audio thread:
		// - Instead of accurate timing, we undershoot the target time.
		//		This is inexpensive (i.e. less work on OS-side buffers
		//		for very frequent updates) and hardens against stutter.
		// - As we cannot retroactively fix any audio buffer underrun,
		//		there is no need for any timestep catch-up.
		float thread_sleep_remaining =
			max(0.f, render_time_seconds_ - render_timer.seconds_passed());

		if (thread_sleep_remaining > render_time_tolerance_)
			thread_util::sleep_seconds(thread_sleep_remaining);

		render_timer.restart();


		// Render to OS audio buffers
		// from current audio master.
		audio_command_queue queue;
		queue.render(render_time_seconds_);
		audio.execute(std::move(queue));			
	}
}


const float audio_task::render_time_seconds_ = 0.010f;
const float audio_task::render_time_tolerance_ = 0.005f;




} /* namespace viral_core */
