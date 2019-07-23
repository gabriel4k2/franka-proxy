/**
 *************************************************************************
 *
 * @file covert_command.hpp
 *
 * Commands for modifying covert objects.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COVERT_COMMAND_HPP)
#define INCLUDED__VIRAL_CORE__COVERT_COMMAND_HPP


#include "build_config.hpp"


#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "owning_list.hpp"
#include "thread_synch.hpp"
#include "covert_data.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class covert_command
 * @ingroup covert_module
 *
 * A single command within a covert_command_queue.
 *
 * Usage notes:
 * - Implementations are free to provide their own command subclasses.
 *		This allows more intricate operations than commits or queries.
 *
 ************************************************************************/
class covert_command :
	private non_copyable
{

	public:

		virtual ~covert_command() NOTHROW;

		virtual auto_pointer<covert_command> clone() const = 0;


	protected:

		covert_command() NOTHROW;
};




/**
 *************************************************************************
 *
 * @class covert_command_commit
 * @ingroup covert_module
 *
 * Applies not-@NULL covert_data to a covert_object
 * with a specific not-@NULL covert_id.
 *
 ************************************************************************/
class covert_command_commit :
	public covert_command
{
	friend class covert_command_queue;


	public:

		covert_command_commit
			(auto_pointer<covert_data> data,
			 const shared_pointer<covert_id>& id);
		covert_command_commit
			(const covert_command_commit& other);

		auto_pointer<covert_command> clone() const override;

		auto_pointer<covert_data> release_data() NOTHROW;

		const shared_pointer<covert_id> id;


	private:

		void validate();

		auto_pointer<covert_data> data_;
};




/**
 *************************************************************************
 *
 * @class covert_query_result
 * @ingroup covert_module
 *
 * Holds the result of an asynchronous query
 * for the covert_data of some covert_object.
 *
 * Usage notes:
 * - Clients usually place a covert_command_query with a preallocated
 *		covert_query_result into a covert_command_queue.
 * - Once the subsystem has (concurrently) processed the query,
 *		finished() signals waiting data(). Clients should reset()
 *		the query result before reuse.
 * - Queries are asynchronous since a separate thread might process
 *		the incoming covert_commands. Hence, results are safe for use
 *		by arbitrary client threads. Lifetime of covert_query_result%s
 *		over thread boundaries must be tracked by shared_pointer.
 *
 ************************************************************************/
class covert_query_result :
	private non_copyable
{

	public:

		covert_query_result();


		void finish(auto_pointer<covert_data> data);


		/**
		 * Fetch resulting data.
		 *
		 * Returns @NULL if data is not yet ready,
		 * or if data has been fetched already.
		 */
		auto_pointer<covert_data> data();


		bool finished() const;
		void reset();


	private:

		mutable mutex mutex_;
		auto_pointer<covert_data> data_;		
		bool finished_;
};




/**
 *************************************************************************
 *
 * @class covert_command_query
 * @ingroup covert_module
 *
 * Retrieves the current covert_data of some covert_object
 * into a covert_query_result.
 *
 ************************************************************************/
class covert_command_query : public covert_command
{

	public:

		covert_command_query
			(const shared_pointer<covert_id>& id,
			 const shared_pointer<covert_query_result>& result);
		covert_command_query
			(const covert_command_query& other);

		auto_pointer<covert_command> clone() const override;
	
		const shared_pointer<covert_id> id;
		const shared_pointer<covert_query_result> result;


	private:

		void validate();
};




/**
 *************************************************************************
 *
 * @class covert_command_purge
 * @ingroup covert_module
 *
 * Deletes any covert_object whose covert_id has been destroyed.
 *
 ************************************************************************/
class covert_command_purge : public covert_command
{

	public:

		covert_command_purge();
		covert_command_purge
			(const covert_command_purge& other);

		auto_pointer<covert_command> clone() const override;
};




/**
 *************************************************************************
 *
 * @class covert_command_queue
 * @ingroup covert_module
 *
 * A FIFO queue of covert_command%s for one-time processing
 * within some subsystem.
 *
 * Usage notes:
 * - Command queues are order-sensitive. For instance, a query()
 *		considers all earlier commit()s. Likewise, an individual
 *		commit might require preceding commits on its dependencies.
 *
 * Implementation notes:
 * - Command queues, owned covert_commands and owned covert_data
 *		are not threadsafe. However, queues usually are moved
 *		by auto_pointer. This ensures access by at most a single
 *		owning thread. Shared components (e.g. covert_query_result)
 *		must still be protected adequately.
 * - Subsystems may derive from covert_command_queue in order to
 *		provide additional convenience functions.
 *
 ************************************************************************/
class covert_command_queue
{
	public:

		typedef owning_list<covert_command> command_list;
		typedef command_list::iterator iterator;
		typedef command_list::const_iterator const_iterator;


		covert_command_queue() NOTHROW;


		/**
		 * Copy or move construct from @bref{other} command queue.
		 * Move construction leaves a valid, empty source.
		 */
		//@{		
		covert_command_queue(const covert_command_queue& other);
		covert_command_queue(covert_command_queue&& other) NOTHROW;
		//@}


		virtual ~covert_command_queue() NOTHROW;


		/**
		 * Copy or move assign of @bref{other} command queue.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		covert_command_queue& operator =(const covert_command_queue& other);
		covert_command_queue& operator =(covert_command_queue&& other) NOTHROW;
		//@}


		void swap_with(covert_command_queue& other) NOTHROW;


		/** Insert an arbitrary, not-@NULL @bref{command}. */
		void insert(auto_pointer<covert_command> command);


		/**
		 * Commit some @bref{data} to a covert_object
		 * of given not-@NULL @bref{id}.
		 */
		//@{
		template <typename T>
			void commit
				(const T& data,
				 const shared_pointer<typename T::covert_id_type>& id)
		{
			auto_pointer<covert_command> command
				(new covert_command_commit(data.clone(), id));

			insert(std::move(command));
		}

		template <typename T>
			void commit
				(T&& data,
				 const shared_pointer<typename T::covert_id_type>& id)
		{
			auto_pointer<covert_data> data_ptr
				(new T(std::move(data)));

			auto_pointer<covert_command> command
				(new covert_command_commit
					(std::move(data_ptr), id));

			insert(std::move(command));
		}

		template <typename T>
			void commit
				(auto_pointer<T> data,
				 const shared_pointer<typename T::covert_id_type>& id)
		{
			auto_pointer<covert_command> command
				(new covert_command_commit
					(std::move(data), id));

			insert(std::move(command));
		}
		//@}


		/**
		 * A commit() that allows for arbitrary covert_id
		 * and covert_data types. Useful for data without
		 * compile-time type, e.g. from covert_loader.
		 */
		//@{
		void commit_untyped
			(const covert_data& data,
			 const shared_pointer<covert_id>& id);
		void commit_untyped
			(auto_pointer<covert_data> data,
			 const shared_pointer<covert_id>& id);
		//@}


		/**
		 * Query the covert_data of a covert_object with some
		 * not-@NULL @bref{id} into a not-@NULL @bref{result}.
		 */
		void query
			(const shared_pointer<covert_id>& id,
			 const shared_pointer<covert_query_result>& result);


		/**
		 * Delete any covert_object whose covert_id
		 * has been deleted.
		 */
		void purge();


		iterator first() NOTHROW;
		const_iterator first() const NOTHROW;
		iterator last() NOTHROW;
		const_iterator last() const NOTHROW;

		int64 size() const NOTHROW;
		bool empty() const NOTHROW;


		/** Clear all commands from the queue. */
		void clear() NOTHROW;


	private:

		command_list commands_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COVERT_COMMAND_HPP) */
