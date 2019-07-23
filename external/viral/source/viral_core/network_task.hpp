/**
 *************************************************************************
 *
 * @file network_task.hpp
 *
 * Threaded networking.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_TASK_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_TASK_HPP


#include "build_config.hpp"


#include "network_forward.hpp"


#include "shared_pointer.hpp"
#include "tree_set.hpp"
#include "string.hpp"
#include "thread.hpp"


namespace viral_core
{


/*
 *************************************************************************
 *
 * @class network_task
 * @ingroup network_module
 *
 * Executes multiple network_stream::update()s in a separate thread.
 *
 * Usage notes:
 * - The network_task only locks streams during network_stream::update().
 *		In particular, sending and receiving on a network_stream remains
 *		possible while other network_stream%s are updated.
 * - The network_task consumes exceptions from network_stream::update().
 *		To detect errors, clients must explicitly trigger error handling
 *		by sending, by receiving, or by network_stream::had_exception().
 * - The network_task ignores (i.e. does not update or remove) streams
 *		with network_stream::had_exception(). Clients must explicitly
 *		call purge() to remove failed network streams().
 *
 ************************************************************************/
class network_task :
	private threaded_task
{

	public:

		typedef tree_set<shared_instance<network_stream>> stream_set;


		network_task
			(const string& network_task_name);


		~network_task() NOTHROW;


		/**
		 * Store new @bref{stream} for automated network_stream::update().
		 * Throws on failure (e.g. if @bref{stream} is already contained).
		 */
		void insert_stream
			(const shared_instance<network_stream>& stream);


		/**
		 * Fetch a copy of the set of all registered network_stream%s.
		 * Due to thread safety, we cannot offer separate iterators.
		 */
		stream_set streams() const;
		

		/**
		 * Remove @bref{stream} from automated update()s.
		 * Logs and ignores errors for @ref NOTHROW behaviour.
		 */
		void remove_stream
			(const shared_instance<network_stream>& stream) NOTHROW;


		/**
		 * Remove all streams with network_stream::had_exception()
		 * from the internal streams() set.
		 */
		void purge() NOTHROW;


		/** Remove all streams from the network task. */
		void clear() NOTHROW;


		const string network_task_name;


	private:

		void task_main() override;
		void update();
		

		static const float thread_sleep_seconds_;

		mutable mutex task_mutex_;
		stream_set streams_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_TASK_HPP) */
