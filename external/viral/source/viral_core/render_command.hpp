/**
 *************************************************************************
 *
 * @file render_command.hpp
 *
 * Rendering-specific commands.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__RENDER_COMMAND_HPP)
#define INCLUDED__VIRAL_CORE__RENDER_COMMAND_HPP


#include "build_config.hpp"


#include "render_forward.hpp"
#include "covert_command.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class render_command_render
 * @ingroup render_module
 *
 * Performs rendering for a not-@NULL render_process_id.
 *
 ************************************************************************/
class render_command_render : public covert_command
{

	public:

		render_command_render
			(const shared_pointer<render_process_id>& process_id);
		render_command_render
			(const render_command_render& other);

		auto_pointer<covert_command> clone() const override;


		const shared_pointer<render_process_id> process_id;


	private:

		void validate();
};




/**
 *************************************************************************
 *
 * @class render_command_queue
 * @ingroup render_module
 *
 * Exposes rendering-specific covert_command%s.
 *
 ************************************************************************/
class render_command_queue : public covert_command_queue
{

	public:

		render_command_queue() NOTHROW;


		/**
		 * Copy or move construct from @bref{other} command queue.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		render_command_queue(const render_command_queue& other);
		render_command_queue(render_command_queue&& other) NOTHROW;
		//@}


		/**
		 * Copy or move assign of @bref{other} command queue.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		render_command_queue& operator =(const render_command_queue& other);
		render_command_queue& operator =(render_command_queue&& other) NOTHROW;
		//@}


		void swap_with(render_command_queue& other) NOTHROW;


		/**
		 * Perform rendering for the indicated
		 * not-@NULL @bref{process_id}.
		 */
		void render
			(const shared_pointer<render_process_id>& process_id);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__RENDER_COMMAND_HPP) */
