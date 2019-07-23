/**
 *************************************************************************
 *
 * @file render_task.hpp
 *
 * Task that runs a separate rendering thread.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_TASK_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_TASK_HPP


#include "build_config.hpp"


#include "thread.hpp"
#include "thread_util.hpp"
#include "string.hpp"
#include "render_command.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class render_task
 * @ingroup render_module
 *
 * Avoids waiting for an idle render_context in a single client thread
 * by executing render_command_queue%s in a separate, internal thread.
 *
 * General usage notes:
 * - A render_task can detach back-end rendering from a main logics thread.
 *		This main logics thread can then sustain responsive update rates
 *		even for high rendering load or low vsync rates. See render_context
 *		for an appropriate usage example. 
 * - Render tasks double-buffer command queues to keep rendering busy.
 *		Otherwise, logic timesteps could slightly miss rendering rates,
 *		which would causes noticeable vsync stutter.
 *
 * Error handling:
 * - @attent If the internal thread has been terminated unexpectedly
 *		(e.g. because of an exception in render_context::execute()),
 *		then all subsequent, potentially blocking render_task calls
 *		(i.e. has_pending_queue(), execute(), and finish()) throw.
 * - After any exception, the render_task cannot resume its work.
 *		Clients instead must destroy and recreate the render_task.
 *		In this case, the renderer may already have (partially)
 *		processed the last two provided command queues.
 *
 * Threading notes:
 * - One need not start() or join() a render_task manually. Instead,
 *		the render task automatically starts on construction and
 *		automatically joins on destruction.
 * - A render_task is threadsafe for any number of client threads.
 *		However, access blocks again for more than a single client,
 *		as has_pending_queue() and execute() are separate calls.
 *
 ************************************************************************/
class render_task :
	private threaded_task
{

	public:

		/**
		 * Create a separate thread for parallel command execution
		 * on the provided @bref{renderer}. The @bref{renderer}
		 * must remain valid for the entire render_task lifetime.
		 *
		 * If enabled, render @bref{exclusive}ly on this render_task.
		 * Clients must then destroy the render_task before rendering
		 * on the renderer in another thread. See render_exclusive.
		 */
		render_task
			(const string& render_task_name,
			 render_context& renderer,
			 bool exclusive);


		~render_task() NOTHROW override;


		/**
		 * Test, without blocking, whether a queue is waiting
		 * for its execution through the internal thread.
		 */
		bool has_pending_queue();


		/**
		 * Block while the task still has_pending_queue(),
		 * then transfer a @bref{queue} of rendering commands
		 * for later processing by the internal thread.
		 *
		 * The render_task maintains ownership over the command queue
		 * (and in turn keeps all relevant covert_id%s and associated
		 * covert_object%s alive) until processing has finish()ed.
		 */
		void execute(render_command_queue&& queue);


		/**
		 * Block until the @bref{renderer} has fully processed
		 * all queues that were execute()d on this render_task.
		 */
		void finish();


		const string render_task_name;
		render_context& renderer;
		const bool exclusive;


	private:

		void task_main() override;

		void disable_task_on_thread_exit();


		mutex client_mutex_;

		signal has_pending_queue_signal_;
		render_command_queue pending_queue_;

		signal renderer_is_executing_signal_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_TASK_HPP) */
