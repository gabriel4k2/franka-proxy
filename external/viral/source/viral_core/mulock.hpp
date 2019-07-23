/**
 *************************************************************************
 *
 * @file mulock.hpp
 *
 * Multi-lock with reader-writer support.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MULOCK_HPP)
#define INCLUDED__VIRAL_CORE__MULOCK_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "shared_pointer.hpp"
#include "tree_set.hpp"
#include "tree_map.hpp"
#include "owning_map.hpp"
#include "thread_synch.hpp"
#include "thread_util.hpp"
#include "mulock_data.hpp"
#include "mulock_access.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup mulock_module Multi-Lock Module
 * @ingroup thread_module
 *
 * Implements safe multi-locks with reader-writer support.
 *
 * Client usage overview:
 * - Clients store custom mulock_data in a mulock_context
 *		to protect this data against concurrent read/write
 *		or write/write operations.
 * - Clients identify unique mulock_data within a mulock_context
 *		through shared_instance%s to unique mulock_id%s.
 * - Clients must associate initial mulock_data with a mulock_id
 *		through thread-safe mulock_context::register_data().
 * - Clients subsequently must acquire a @ref mulock to access
 *		the current mulock_data for any registered mulock_id.
 *		To this end, clients must compile a mulock_request_list
 *		that specifies all relevant mulock_id%s and corresponding
 *		mulock_request::lock_type%s (e.g. @bref{read_write}).
 * - After successful locking, the @ref mulock exposes mulock_data
 *		through mulock_const_access for lock_request::read_only
 *		or through both mulock_access and mulock_const_access
 *		for lock_request::read_write.
 *
 * Data deletion:
 * - On purge(), the mulock_context deletes all mulock_data
 *		without a living mulock_id. Clients must explicitly
 *		purge() the mulock_context at reasonable intervals.
 * - A mulock_context must outlive all bound @ref mulock%s
 *		and all mulock_id%s for mulock_data inside the context.
 *		Notably, clients must ensure that no ownership cycles
 *		prohibit final purging of mulock_id%s and mulock_data.
 *
 * Blocking behaviour:
 * - Conflicting mulock%s block until all their mulock_request%s
 *		are conflict-free: lock_request::read_write blocks until
 *		the respective mulock_data is used by no other mulock,
 *		while lock_request::read_only allows concurrent readers.
 * - @attent In most cases, mulock acquiration is dead-lock free:
 *		A rollback-and-retry strategy ensures atomic multilocks,
 *		where locking is atomic per thread and per mulock_context.
 *		Deadlocks still remain possible when concurrent threads work
 *		on multiple mulock_context%s or acquire additional resources.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class mulock
 * @ingroup mulock_module
 *
 * Executes a mulock_request_list and exposes mulock_access.
 *
 ************************************************************************/
class mulock :
	private non_copyable
{
	public:

		/**
		 * Acquire a lock on the data of @bref{request}
		 * or @bref{requests} within given @bref{context}.
		 * Throws on failure, e.g. on duplicate locking.
		 */
		//@{
		mulock(mulock_context& context,
			   const mulock_request& request);

		mulock(mulock_context& context,
			   const mulock_request_list& requests);
		//@}


		~mulock() NOTHROW;


		/**
		 * Fetch mulock_data associated with given @bref{id}.
		 * Throws if the corresponding ID was not locked.
		 *
		 * Clients do not directly use the mulock_result,
		 * but instead wrap the result into mulock_access
		 * or mulock_const_access.
		 */
		mulock_result at
			(const shared_instance<mulock_id>& id);


	private:

		static mulock_request_list to_request_list
			(const mulock_request& request);

		void acquire_lock();
		void release_lock() NOTHROW;


		mulock_context& context_;
		const mulock_request_list requests_;
};




/**
 *************************************************************************
 *
 * @class mulock_state
 * @ingroup mulock_module
 *
 * Utility that stores internal mulock_context state for mulock_id%s.
 *
 ************************************************************************/
class mulock_state :
	private non_copyable
{
	friend class mulock_context;

	private:

		mulock_state
			(auto_pointer<mulock_data> data);


		auto_pointer<mulock_data> data_;

		mulock_request::lock_type lock_type_;
		int64 lock_count_;
};




/**
 *************************************************************************
 *
 * @class mulock_context
 * @ingroup mulock_module
 *
 * Stores mulock_data for mulock_id%s.
 *
 ************************************************************************/
class mulock_context :
	private non_copyable
{
	friend class mulock;


	public:

		mulock_context();
		~mulock_context() NOTHROW;


		/**
		 * Register a not-@NULL mulock_data under the given mulock_id.
		 * After registration, access data only through a @ref mulock!
		 */
		void register_data
			(const shared_instance<mulock_id>& id,
			 auto_pointer<mulock_data> data);


		/**
		 * Delete any unreferenced mulock_data objects.
		 *
		 * Optionally, test only @bref{purge_limit} objects,
		 * starting one past the last tested object.
		 */
		//@{
		void purge() NOTHROW;
		void purge(int64 purge_limit) NOTHROW;
		//@}


	private:
		
		/**
		 * Utilities for locking through a @ref mulock.
		 * Must be internal to context since data structures
		 * are shared over all @ref mulock instances.
		 */
		//@{
		void acquire_lock
			(const mulock_request_list& requests);
		void release_lock() NOTHROW;

		mulock_result fetch_lock_result
			(const shared_instance<mulock_id>& id);
		//@}


		mutable mutex mutex_;


		/**
		 * Map from mulock_id%s to mulock_data
		 * and current locking state.
		 */
		//@{
		typedef owning_map<weak_pointer<mulock_id>,
						   mulock_state> id_to_state_map;

		id_to_state_map id_to_state_;
		id_to_state_map::iterator id_to_state_next_purge_;
		//@}


		/**
		 * Per-thread set of currently locked mulock_id%s.
		 * Required for @ref mulock tracking.
		 *
		 * We use shared_instance%s to keep locked IDs alive
		 * over the entire @ref mulock lifetime.
		 */
		//@{
		typedef tree_set<shared_instance<mulock_id>> locked_set;
		typedef tree_map<thread_id, locked_set> thread_to_locked_map;
		thread_to_locked_map thread_to_locked_;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MULOCK_HPP) */