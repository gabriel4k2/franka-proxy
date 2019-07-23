/**
 *************************************************************************
 *
 * @file audio_command.cpp
 *
 * Covert commands specific to audio contexts, implementation.
 *
 ************************************************************************/


#include "audio_command.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	audio_command_play
//
//////////////////////////////////////////////////////////////////////////


audio_command_play::audio_command_play
	(const shared_pointer<audio_source_id>& source_id)
	:
	source_id(source_id),
	set_frame(false),
	frame(0)
{
	validate();
}


audio_command_play::audio_command_play
	(const shared_pointer<audio_source_id>& source_id,
	 bool set_frame, int64 frame)
	:
	source_id(source_id),
	set_frame(set_frame),
	frame(frame)
{
	validate();
}


audio_command_play::audio_command_play
	(const audio_command_play& other)
	:
	source_id(other.source_id),
	set_frame(other.set_frame),
	frame(other.frame)
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	audio_command_play::clone() const
{
	return auto_pointer<covert_command>
				(new audio_command_play(*this));
}


void audio_command_play::validate()
{
	if (!source_id)
	{
		LOG_ERROR("Cannot play a NULL audio_source_id.");
		throw null_pointer();
	}

	// Negative frames are valid.
	// See audio_source_data.
}




//////////////////////////////////////////////////////////////////////////
//
//	audio_command_stop
//
//////////////////////////////////////////////////////////////////////////


audio_command_stop::audio_command_stop
	(const shared_pointer<audio_source_id>& source_id)
	:
	source_id(source_id)
{
	validate();
}


audio_command_stop::audio_command_stop
	(const audio_command_stop& other)
	:
	source_id(other.source_id)
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	audio_command_stop::clone() const
{
	return auto_pointer<covert_command>
				(new audio_command_stop(*this));
}


void audio_command_stop::validate()
{
	if (!source_id)
	{
		LOG_ERROR("Cannot stop a NULL audio_source_id.");
		throw null_pointer();
	}
}




//////////////////////////////////////////////////////////////////////////
//
//	audio_command_master
//
//////////////////////////////////////////////////////////////////////////


audio_command_master::audio_command_master
	(const shared_pointer<audio_master_id>& master_id)
	:
	master_id(master_id)
{
	// No validate(), a NULL master_id is valid.
}


audio_command_master::audio_command_master
	(const audio_command_master& other)
	:
	master_id(other.master_id)
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	audio_command_master::clone() const
{
	return auto_pointer<covert_command>
				(new audio_command_master(*this));
}




//////////////////////////////////////////////////////////////////////////
//
//	audio_command_render
//
//////////////////////////////////////////////////////////////////////////


audio_command_render::audio_command_render
	(float render_time_seconds)
	:
	render_time_seconds(render_time_seconds)
{
	validate();
}


audio_command_render::audio_command_render
	(const audio_command_render& other)
	:
	render_time_seconds(other.render_time_seconds)
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	audio_command_render::clone() const
{
	return auto_pointer<covert_command>
				(new audio_command_render(*this));
}


void audio_command_render::validate()
{
	if (render_time_seconds <= 0)
	{
		LOG_ERROR("Audio rendering time must be greater than zero.");
		throw invalid_parameter();
	}
}




//////////////////////////////////////////////////////////////////////////
//
//	audio_command_queue
//
//////////////////////////////////////////////////////////////////////////


audio_command_queue::audio_command_queue() NOTHROW { }


audio_command_queue::audio_command_queue
	(const audio_command_queue& other)
	:
	covert_command_queue(other)
{ }


audio_command_queue::audio_command_queue
	(audio_command_queue&& other) NOTHROW
{
	// Swap ensures defaulted other.
	swap(*this, other);
}


audio_command_queue&
	audio_command_queue::operator =(const audio_command_queue& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	audio_command_queue offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


audio_command_queue&
	audio_command_queue::operator =(audio_command_queue&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	audio_command_queue offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void audio_command_queue::swap_with
	(audio_command_queue& other) NOTHROW
{
	swap<covert_command_queue>(*this, other);
}


void audio_command_queue::play
	(const shared_pointer<audio_source_id>& source_id)
{
	auto_pointer<covert_command> command
		(new audio_command_play(source_id));

	insert(std::move(command));
}


void audio_command_queue::play
	(const shared_pointer<audio_source_id>& source_id,
	 bool set_frame, int64 frame)
{
	auto_pointer<covert_command> command
		(new audio_command_play
			(source_id, set_frame, frame));

	insert(std::move(command));
}


void audio_command_queue::stop
	(const shared_pointer<audio_source_id>& source_id)
{
	auto_pointer<covert_command> command
		(new audio_command_stop(source_id));

	insert(std::move(command));
}


void audio_command_queue::set_master
	(const shared_pointer<audio_master_id>& master_id)
{
	auto_pointer<covert_command> command
		(new audio_command_master(master_id));

	insert(std::move(command));
}


void audio_command_queue::render
	(float render_time_seconds)
{
	auto_pointer<covert_command> command
		(new audio_command_render(render_time_seconds));

	insert(std::move(command));
}




} /* namespace viral_core */
