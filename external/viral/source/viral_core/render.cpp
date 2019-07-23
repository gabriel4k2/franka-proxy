/**
 *************************************************************************
 *
 * @file render.cpp
 *
 * Rendering module main implementation.
 *
 ************************************************************************/


#include "render.hpp"


#include "log.hpp"
#include "scope_guard.hpp"
#include "covert_object.hpp"
#include "render_command.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// render_context
//
//////////////////////////////////////////////////////////////////////////


render_context::~render_context() NOTHROW
	{ LOG_INFO("Destroyed: " + context_name); }


void render_context::execute
	(render_command_queue&& queue)
{
	MUTEX_SCOPE(render_mutex_);

	acquire_api_exclusive();
	SCOPE_GUARD
		(release_api_on_exit,
			(*this, &render_context::release_api_exclusive));
	
	for (render_command_queue::iterator it(queue.first()); it; ++it)
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


render_context::render_context(const string& context_name)
	:
	context_name(context_name),
	exclusive_thread_(thread_id::invalid_thread()),
	exclusive_refcount_(0),
	object_manager_(new covert_object_manager())
{
	LOG_INFO("Creating: " + context_name);

	command_handlers_.insert
		(typeid(covert_command_commit), &render_context::execute_commit);
	command_handlers_.insert
		(typeid(covert_command_query), &render_context::execute_query);
	command_handlers_.insert
		(typeid(covert_command_purge), &render_context::execute_purge);
	command_handlers_.insert
		(typeid(render_command_render), &render_context::execute_render);
}


const covert_object_manager& render_context::object_manager() const
{
	// Object manager PIMPL removes dependency
	// on backend covert_object.hpp from render.hpp.
	return *object_manager_;
}


void render_context::clear_on_destruction() NOTHROW
{
	// Only called on destruction,
	// no rendering mutex required.

	try	{ acquire_api_exclusive(); }
	catch (...)
	{
		LOG_CRITICAL("Failed to acquire rendering API "
					 "for render_context destruction.\n"
					 "A render_exclusive is still alive, "
					 "which implies a race condition.\n"
					 "Program will possibly crash.");

		// Still release_api_on_exit, which only prints another warning.
		// Clearing covert_object%s may crash, but omitting is pointless,
		// because the object_manager_ clears automatically on destruction.
	}

	SCOPE_GUARD
		(release_api_on_exit,
			(*this, &render_context::release_api_exclusive));

	object_manager_->clear();
}


void render_context::acquire_api_exclusive()
{
	// Mutexing for concurrent calls through render_exclusive.
	MUTEX_SCOPE(render_mutex_);

	if (exclusive_thread_ != thread_id::this_thread() &&
		exclusive_thread_ != thread_id::invalid_thread())
	{
		LOG_ERROR("Cannot acquire back-end rendering API for current thread,\n"
				  "because this API is already exclusive to another thread.\n"
				  "See render_context::execute() and render_exclusive.");

		throw invalid_state();
	}


	// Possibly throws.	
	if (exclusive_refcount_ == 0)
		acquire_api();


	// Exception safety: NOTHROW from here.
	exclusive_thread_ = thread_id::this_thread();
	++exclusive_refcount_;
}


void render_context::release_api_exclusive() NOTHROW
{
	// Mutexing for concurrent calls through render_exclusive.
	MUTEX_SCOPE(render_mutex_);

	if (exclusive_thread_ != thread_id::this_thread())
	{
		LOG_CRITICAL("Cannot release back-end rendering API from current thread,\n"
					 "because the thread has not acquired this API.");

		return; // No exception due to NOTHROW.
	}

	--exclusive_refcount_;

	if (exclusive_refcount_ == 0)
	{
		release_api();
		exclusive_thread_ = thread_id::invalid_thread();
	}
}
	

void render_context::execute_commit(covert_command& command)
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


void render_context::execute_query(covert_command& command)
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


void render_context::execute_purge(covert_command& command)
	{ object_manager_->purge(); }


void render_context::execute_render(covert_command& command)
{
	render_command_render& cmd =
		static_cast<render_command_render&>(command);

	render(cmd.process_id);
}




//////////////////////////////////////////////////////////////////////////
//
// render_exclusive
//
//////////////////////////////////////////////////////////////////////////


render_exclusive::render_exclusive
	(render_context& renderer)
	:
	renderer(renderer)
{
	renderer.acquire_api_exclusive();
}


render_exclusive::~render_exclusive() NOTHROW
	{ renderer.release_api_exclusive(); }




} /* namespace viral_core */
