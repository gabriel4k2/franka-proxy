/**
 *************************************************************************
 *
 * @file render_task.cpp
 *
 * Separate rendering thread implementation.
 *
 ************************************************************************/


#include "render_task.hpp"


#include "exception.hpp"
#include "scope_guard.hpp"
#include "log.hpp"
#include "render.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	render_task
//
//////////////////////////////////////////////////////////////////////////


render_task::render_task
	(const string& render_task_name,
	 render_context& renderer,
	 bool exclusive)
	:	
	render_task_name(render_task_name),
	renderer(renderer),
	exclusive(exclusive)
{
	LOG_INFO("Creating: " + render_task_name);
	LOG_INFO("Creating and running internal thread.");
	start(); 
}


render_task::~render_task() NOTHROW
{
	LOG_INFO("Destroying.");
	LOG_INFO("Terminating internal thread.");

	try { join(); }
	catch (...)
	{
		LOG_CRITICAL
			("Internal thread has thrown an exception on joining.");
	}

	LOG_INFO("Destroyed: " + render_task_name);
}


bool render_task::has_pending_queue()
{
	if (thread_has_terminated())
	{	
		LOG_ERROR("The internal thread has been terminated.");
		join();				   // Pass on any internal thread exception.
		throw invalid_state(); // Throw on further calls or on normal thread exit.
	}

	return has_pending_queue_signal_.get();
}


void render_task::execute(render_command_queue&& queue)
{
	// The client_mutex_ ensures that only a single client thread
	// may communicate with the internal thread at a time.
	// This avoids ugly race conditions in signaling logics.
	MUTEX_SCOPE(client_mutex_);

	while (true)
	{
 		if (thread_has_terminated())
		{
			LOG_ERROR("The internal thread has been terminated.");
			join();				   // Pass on any internal thread exception.
			throw invalid_state(); // Throw on further calls or on normal thread exit.
		}

		has_pending_queue_signal_.wait_for(false);

		if (!has_pending_queue_signal_.get())
		{
			// Not a timeout, so transfer queue to later execution.
			// Exception safety: NOTHROW in here.
			pending_queue_ = std::move(queue);
			has_pending_queue_signal_.set(true);
			return;
		}
	}
}


void render_task::finish()
{
	// The client_mutex_ ensures that only a single client thread
	// may communicate with the internal thread at a time.
	// This avoids ugly race conditions in signaling logics.
	MUTEX_SCOPE(client_mutex_);

	bool pending_queue_drained = false;

	while (true)
	{
 		if (thread_has_terminated())
		{
			LOG_ERROR("The internal thread has been terminated.");
			join();				   // Pass on any internal thread exception.
			throw invalid_state(); // Throw on further calls or on normal thread exit.
		}


		// First, drain any pending command queue.
		// The client mutex prohibits clients from passing in a new queue.
		// Then, wait until execution on the formerly pending queue finishes.
		// For this to work without a race, the renderer_is_executing_signal_
		// must be raised before lowering the has_pending_queue_signal_.
		if (!pending_queue_drained)
		{
			has_pending_queue_signal_.wait_for(false);

			if (!has_pending_queue_signal_.get())
				pending_queue_drained = true;
		}
		else
		{
			renderer_is_executing_signal_.wait_for(false);

			if (!renderer_is_executing_signal_.get())
				return;
		}
	}
}


void render_task::task_main()
{
	SCOPE_GUARD
		(thread_exit_guard,
			(*this, &render_task::disable_task_on_thread_exit));
		
	auto_pointer<render_exclusive> make_task_exclusive
		(exclusive ? new render_exclusive(renderer) : NULL);

	while (!join_now())
	{
		has_pending_queue_signal_.wait_for(true);

		if (has_pending_queue_signal_.get())
		{

			// Not a timeout, so fetch pending queue,
			// signal execution, and allow new queues.
			// The signal order is vital, see finish().
			// All preparation work is NOTHROW.
			render_command_queue transfer_queue
				(std::move(pending_queue_));

			renderer_is_executing_signal_.set(true);
			has_pending_queue_signal_.set(false);


			// Execute formerly pending queue (might throw).
			renderer.execute(std::move(transfer_queue));


			// Finished execution.
			// The queue has been deleted, so purging
			// after a finish() will work correctly.
			// NOTHROW from here.
			renderer_is_executing_signal_.set(false);
		}
	}
}


void render_task::disable_task_on_thread_exit()
{
	/**
	 * Clear pending queue and prohibit further incoming queues
	 * when the internal thread exits (e.g. due to an exception).
	 * Otherwise, we might maintain shared_pointers to render IDs,
	 * which disallows proper shutdown.
	 *
	 * To lock out future queues through render_task::execute(),
	 * we artifically raise and keep the has_pending_queue_signal_.
	 * This does not cause a deadlock, because all client threads
	 * (e.g. in concurrent has_pending_queue(), execute(), finish())
	 * exit their loop by exception on exit of the internal thread.
	 * 
     * We must lock against concurrent client-side threads here
	 * to avoid an incoming queue while we raise the signal.
	 */

	MUTEX_SCOPE(client_mutex_);
	pending_queue_.clear();
	has_pending_queue_signal_.set(true);
}




} /* namespace viral_core */
