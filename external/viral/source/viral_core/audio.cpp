/**
 *************************************************************************
 *
 * @file audio.cpp
 *
 * Device-independent audio layer, implementation.
 *
 ************************************************************************/


#include "audio.hpp"


#include "log.hpp"
#include "covert_object.hpp"
#include "audio_command.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	audio_context
//
//////////////////////////////////////////////////////////////////////////


audio_context::~audio_context() NOTHROW
	{ LOG_INFO("Destroyed: " + context_name); }


void audio_context::execute
	(audio_command_queue&& queue)
{
	MUTEX_SCOPE(audio_mutex_);

	for (audio_command_queue::iterator it(queue.first()); it; ++it)
	{
		if (!*it)
		{
			LOG_ERROR("Encountered NULL command.");
			throw null_pointer();
		}

		command_handler_map::const_iterator handler
			(command_handlers_.find(typeid(**it)));

		if (!handler)
		{
			LOG_ERROR("Encountered unknown command.");
			throw unsupported_feature();
		}

		(this->*(handler->value))(**it);
	}
}


audio_context::audio_context(const string& context_name)
	:
	context_name(context_name),
	object_manager_(new covert_object_manager())
{
	LOG_INFO("Creating: " + context_name);

	command_handlers_.insert
		(typeid(covert_command_commit), &audio_context::execute_commit);
	command_handlers_.insert
		(typeid(covert_command_query), &audio_context::execute_query);
	command_handlers_.insert
		(typeid(covert_command_purge), &audio_context::execute_purge);

	command_handlers_.insert
		(typeid(audio_command_play), &audio_context::execute_play);
	command_handlers_.insert
		(typeid(audio_command_stop), &audio_context::execute_stop);
	command_handlers_.insert
		(typeid(audio_command_master), &audio_context::execute_master);
	command_handlers_.insert
		(typeid(audio_command_render), &audio_context::execute_render);
}


const covert_object_manager& audio_context::object_manager() const
{
	// Object manager PIMPL removes dependency
	// on backend covert_object.hpp from audio.hpp.
	return *object_manager_;
}


void audio_context::clear_on_destruction()
{
	// Only called on destruction, no mutex required.
	object_manager_->clear();
}


void audio_context::execute_commit(covert_command& command)
{
	covert_command_commit& commit=
		static_cast<covert_command_commit&>(command);

	auto_pointer<covert_data> data(commit.release_data());

	if (!data || !commit.id)
	{
		LOG_ERROR("Cannot commit NULL data or to NULL ID.");
		throw null_pointer();
	}
	
	covert_object *find =
		object_manager_->find_object(commit.id);

	if (!find)
	{
		// Exception safety:
		// Implementation object may be deleted on exception.
		// This poses no threat, as we hold the API context.
		auto_pointer<covert_object> object(create_object(commit.id));
		find = object.get();
		object_manager_->insert
			(commit.id, std::move(object));
	}

	find->set_data
		(std::move(data));
}


void audio_context::execute_query(covert_command& command)
{
	covert_command_query& query =
		static_cast<covert_command_query&>(command);

	if (!query.id || !query.result)
	{
		LOG_ERROR("Cannot query NULL ID or into NULL query result.");
		throw null_pointer();
	}

	covert_object* object =
		object_manager_->find_object(query.id);

	query.result->finish
		(object ? object->data() : auto_pointer<covert_data>());
}


void audio_context::execute_purge(covert_command& command)
	{ object_manager_->purge(); }


void audio_context::execute_play(covert_command& command)
{
	audio_command_play& play_command =
		static_cast<audio_command_play&>(command);

	play(play_command);
}

void audio_context::execute_stop(covert_command& command)
{
	audio_command_stop& stop_command =
		static_cast<audio_command_stop&>(command);

	stop(stop_command);
}

void audio_context::execute_master(covert_command& command)
{
	audio_command_master& master_command =
		static_cast<audio_command_master&>(command);

	set_master(master_command);
}

void audio_context::execute_render(covert_command& command)
{
	audio_command_render& render_command =
		static_cast<audio_command_render&>(command);

	render(render_command);
}




} /* namespace viral_core */

