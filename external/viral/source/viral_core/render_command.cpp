/**
 *************************************************************************
 *
 * @file render_command.cpp
 *
 * Renderer-specific commands, implementation.
 *
 ************************************************************************/


#include "render_command.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// render_command_render
//
//////////////////////////////////////////////////////////////////////////


render_command_render::render_command_render
	(const shared_pointer<render_process_id>& process_id)
	:
	process_id(process_id)
{
	validate();
}


render_command_render::render_command_render
	(const render_command_render& other)
	:
	process_id(other.process_id)
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	render_command_render::clone() const
{
	return auto_pointer<covert_command>
				(new render_command_render(*this));
}


void render_command_render::validate()
{
	if (!process_id)
	{
		LOG_ERROR("Cannot render with NULL render_process_id.");
		throw null_pointer();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// render_command_queue
//
//////////////////////////////////////////////////////////////////////////


render_command_queue::render_command_queue() NOTHROW { }


render_command_queue::render_command_queue
	(const render_command_queue& other)
	:
	covert_command_queue(other)
{ }


render_command_queue::render_command_queue
	(render_command_queue&& other) NOTHROW
{
	// Swap ensures defaulted other.
	swap(*this, other);
}


render_command_queue&
	render_command_queue::operator =(const render_command_queue& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	render_command_queue offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


render_command_queue&
	render_command_queue::operator =(render_command_queue&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	render_command_queue offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void render_command_queue::swap_with
	(render_command_queue& other) NOTHROW
{
	swap<covert_command_queue>(*this, other);
}


void render_command_queue::render
	(const shared_pointer<render_process_id>& process_id)
{
	auto_pointer<covert_command> command
		(new render_command_render(process_id));
		
	insert(std::move(command));
}




} /* namespace viral_core */
