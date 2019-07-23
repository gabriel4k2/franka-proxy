/**
*************************************************************************
*
* @file network_task.cpp
*
* Threaded networking, implementation.
*
************************************************************************/


#include "network_task.hpp"


#include "log.hpp"
#include "network_stream.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// network_task
//
//////////////////////////////////////////////////////////////////////////


network_task::network_task
	(const string& network_task_name)
	:
	network_task_name(network_task_name)
{
	LOG_INFO("Creating: " + network_task_name);
	LOG_INFO("Creating and running internal thread.");
	start(); 
}


network_task::~network_task() NOTHROW
{
	LOG_INFO("Destroying.");
	LOG_INFO("Terminating internal thread.");

	try { join(); }
	catch (...)
	{
		LOG_CRITICAL
			("Internal thread has thrown an exception on joining.");
	}

	LOG_INFO("Destroyed: " + network_task_name);
}


void network_task::insert_stream
	(const shared_instance<network_stream>& stream)
{
	MUTEX_SCOPE(task_mutex_);

	if (streams_.contains(stream))
	{
		LOG_ERROR("Cannot insert stream, stream is already contained.");
		throw invalid_state();
	}

	streams_.insert(stream);
}


network_task::stream_set network_task::streams() const
{
	MUTEX_SCOPE(task_mutex_);
	return streams_;
}


void network_task::remove_stream
	(const shared_instance<network_stream>& stream) NOTHROW
{
	MUTEX_SCOPE(task_mutex_);
	streams_.remove(stream);
}


void network_task::purge() NOTHROW
{
	MUTEX_SCOPE(task_mutex_);

	for (stream_set::iterator it(streams_.first()); it;)
	{
		stream_set::iterator del(it);
		++it;

		if ((*del)().had_exception())
			streams_.remove(del);
	}
}


void network_task::clear() NOTHROW
{
	MUTEX_SCOPE(task_mutex_);
	streams_.clear();
}


void network_task::task_main()
{
	while (!join_now())
	{
		update();
		thread_util::sleep_seconds(thread_sleep_seconds_);
	}
}


void network_task::update()
{
	/**
	 * Send and receive on all failure-free network_stream%s.
	 * Ignores any exceptions from network_stream::update().
	 */

	MUTEX_SCOPE(task_mutex_);

	for (stream_set::iterator it
			(streams_.first()); it; ++it)
	{
		if ((*it)().had_exception())
			continue;

		try { (*it)().update(); }
			catch (...) { }
	}
}


const float network_task::thread_sleep_seconds_ = 0.005f;




} /* namespace viral_core */
