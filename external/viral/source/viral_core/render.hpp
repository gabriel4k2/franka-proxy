/**
 *************************************************************************
 *
 * @file render.hpp
 *
 * Rendering module main header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"
#include "render_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "tree_map.hpp"
#include "string.hpp"
#include "thread_synch.hpp"
#include "thread_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup render_module Render Module
 *
 * Allows for API-independent 3D rendering and graphics object management
 * by means of a render_context.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class render_context
 * @ingroup render_module
 *
 * Interface for 3D rendering and management of graphics objects.
 *
 * Rendering overview:
 * - A render_context utilizes covert_object%s, covert_id%s,
 *		covert_command%s and covert_data for all its operations.
 *		See the @ref covert_module "Covert Module" for details.
 * - Clients control the renderer by passing a render_command_queue
 *		with various covert_command%s to render_context::execute().
 *		For instance, a render_command_render renders a new image
 *		and displays it in the application window. 
 *
 * Usage notes:
 * - See render_context implementations for individual features,
 *		such as shading setup and custom covert_data types.
 * - Helper classes support covert_data loading (render_loader)
 *		and multi-threaded rendering (render_task).
 * - A render_context does not provide strong exception safety.
 *		Notably, execute() may fail after partial object updates
 *		or after partial frame rendering, and the render_context
 *		may remain in a dead state. Thus, clients should recreate
 *		the render_context and recommit all data on any exception.
 * - A render_context is threadsafe for use by arbitrary clients.
 *		However, clients must be aware of performance implications
 *		caused by undesired blocking. See the below code example.
 *
 * Example for main application loop with fixed timestep,
 * multithreaded rendering, and input handling:
 * @code
 * using namespace viral_core;
 *
 * step_timer timer(0.01f, 0.1f);
 *
 * ms_window_context window(...);
 * ms_input_context input(..., window);
 * gl3_render_context renderer(..., window);
 * render_task rtask(..., renderer, ...);
 *
 * create_initial_covert_ids();
 * create_initial_covert_data();
 *
 * // Commit initial covert_data and covert_ids.
 * render_command_queue qinit;
 * commit_all_data(qinit);
 * rtask.execute(std::move(qinit));
 *
 * // Wait until all commands have been processed.
 * // This creates all initial covert_objects and
 * // avoids a delay on the first rendered frame.
 * rtask.finish();
 *
 * timer.restart();
 *
 * while (simulation_is_running)
 * {
 *		timer.try_sleep();
 *		timer.update();
 *
 *		if (!timer.has_next_timestep())
 *			continue;
 *
 *		while (timer.next_timestep())
 *		{
 *			window.update();
 *			input.update();
 *
 *			// Create, release, and update covert_data
 *			// and covert_ids in parallel to rendering
 *			// of another frame. Modifies no covert_object,
 *			// hence there are no race condititons.
 *			update_simulation(timer.timestep_seconds);
 *		}
 *
 *		if (!rtask.has_pending_queue())
 *		{
 *			// Build per-frame command queue,
 *			// including commits of preceding covert_data updates
 *			// and frame rendering for some render_process_id.
 *			render_command_queue qframe;
 *			commit_modified_data(qframe);
 *			qframe.purge();
 *			qframe.render(process_id);
 *
 *			// Concurrently execute queue in a separate thread.
 *			// Doesn't block, as no queue is currently pending.
 *			rtask.execute(std::move(q_frame));
 *		}
 * }
 * @endcode
 *
 * Implementation notes:
 * - The base render_context already handles most of the rendering
 *		infrastructure, including object management and thread safety.
 * - For thread safety reasons, renderer internals should be protected
 *		from client access by friending and restricted constructors.
 *
 ************************************************************************/
class render_context :
	private non_copyable
{
	friend class render_exclusive;


	public:

		virtual ~render_context() NOTHROW;


		/**
		 * Wait until the render_context is idle,
		 * then execute a rendering command @bref{queue}
		 * in FIFO order.
		 *
		 * Throws if another thread is render_exclusive,
		 * even if that thread is not rendering currently.
		 */
		void execute(render_command_queue&& queue);


		const string context_name;


	protected:

		render_context(const string& context_name);


		const covert_object_manager& object_manager() const;


		/**
		 * Acquire or release back-end rendering API for the calling thread.
		 * The render_context already handles mutexing, reference counting,
		 * and thread_id checks (e.g. for render_exclusive).
		 */
		//@{
		virtual void acquire_api() = 0;
		virtual void release_api() NOTHROW = 0;
		//@}


		/**
		 * Create a not-@NULL covert_object suited for given @bref{id}.
		 * The rendering mutex and API have already been acquired.
		 */
		virtual auto_pointer<covert_object> create_object
			(const shared_pointer<covert_id>& id) = 0;


		/**
		 * Perform rendering for the given rendering @bref{process_id}.
		 * The rendering mutex and API have already been acquired.
		 */
		virtual void render
			(const shared_pointer<render_process_id>& process_id) = 0;


		/**
		 * Release leaked covert_object%s from the covert_object_manager.
		 * Destructors of context implementations must explicitly call this,
		 * since ~covert_object() usually depends on a valid rendering API.
		 */
		void clear_on_destruction() NOTHROW;


	private:

		void acquire_api_exclusive();
		void release_api_exclusive() NOTHROW;

		void execute_commit(covert_command& command);
		void execute_query(covert_command& command);
		void execute_purge(covert_command& command);
		void execute_render(covert_command& command);
		

		mutex render_mutex_;

		thread_id exclusive_thread_;
		int64 exclusive_refcount_;

		auto_pointer<covert_object_manager> object_manager_;

		typedef void (render_context::*command_handler)(covert_command& command);
		typedef tree_map<type_info_ref, command_handler> command_handler_map;
		command_handler_map command_handlers_;
};




/**
 *************************************************************************
 *
 * @class render_exclusive
 * @ingroup render_module
 *
 * Exclusively binds rendering to the creating thread while alive.
 *
 * Usage notes:
 * - Without render_exclusive, each call to render_context::execute() 
 *		acquires and subsequently releases the back-end rendering API.
 *		This enables arbitrary threads to call render_context::execute(),
 *		but is detrimental to performance on certain implementations.
 * - In contrast, render_exclusive maintains the back-end rendering API
 *		for the creating thread over multiple render_context::execute()s.
 *		This reduces overhead, but causes other threads to fail and throw
 *		on render_exclusive() or render_context::execute().
 * - A render_exclusive must neither outlive the bound @bref{renderer}
 *		nor the creating thread.
 *
 * Implementation notes:
 * - This class fixes threaded optimization in 2017 NVIDIA OpenGL drivers:
 *		Without long-term binding of the OpenGL context to a single thread,
 *		each wglMakeCurrent(NULL, NULL) busy-waits for any vsync to finish,
 *		which fully occupies a CPU core. Alternative solutions (DwmFlush(),
 *		disabling vsync on any compositing desktop, sleep to next vsync),
 *		have downsides (e.g. stutter) and do not fully solve the problem.
 *
 ************************************************************************/
class render_exclusive :
	private non_copyable
{
	public:

		render_exclusive(render_context& renderer);
		~render_exclusive() NOTHROW;

		render_context& renderer;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_HPP) */
