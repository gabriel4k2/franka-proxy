/**
 *************************************************************************
 *
 * @file mulock_access.hpp
 *
 * Access to multilock results.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MULOCK_ACCESS_HPP)
#define INCLUDED__VIRAL_CORE__MULOCK_ACCESS_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "shared_pointer.hpp"
#include "list.hpp"
#include "mulock_data.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mulock_access_util
 * @ingroup mulock_module
 *
 * Static utilities for @ref mulock_access and @ref mulock_const_access.
 *
 ************************************************************************/
class mulock_access_util :
	private non_constructable
{
	template <typename D> friend class mulock_access;
	template <typename D> friend class mulock_const_access;


	private:

		static void throw_const_access_only();
		static void throw_data_type_mismatch();
};




/**
 *************************************************************************
 *
 * @class mulock_request
 * @ingroup mulock_module
 *
 * Requests a @ref mulock on the mulock_data of a mulock_id.
 *
 ************************************************************************/
class mulock_request
{
	public:

		enum lock_type
		{
			/** Read-only access. */
			read = 0,  

			/** Read-write access. */
			write = 1
		};


		mulock_request
			(const shared_instance<mulock_id>& id,
			 lock_type lock)
			:
			id(id), lock(lock)
		{ }


		const shared_instance<mulock_id> id;
		const lock_type lock;
};


/**
 * A list of lock requests.
 * @ingroup mulock_module
 */
typedef list<mulock_request> mulock_request_list;




/**
 *************************************************************************
 *
 * @class mulock_result
 * @ingroup mulock_module
 *
 * Carries immutable information about a successful mulock_request.
 *
 * Usage notes:
 * - Lock results only transfer @bref{locked_data_} from a @ref mulock
 *		to a @ref mulock_access or a @ref mulock_const_access object.
 *		This allows to prohibit client-side access copies.
 *
 ************************************************************************/
class mulock_result
{
	friend class mulock;
	friend class mulock_context;

	template <typename T> friend class mulock_access;
	template <typename T> friend class mulock_const_access;

	private:

		mulock_result
			(const mulock_request& request,
			 mulock_data* locked_data)
			:
			request_(request),
			locked_data_(locked_data)
		{ }


		const mulock_request request_;
		mulock_data* const locked_data_;
};




/**
 *************************************************************************
 *
 * @class mulock_access
 * @ingroup mulock_module
 *
 * Realizes read-write access to a mulock_data of type @bref{T}.
 *
 * Usage notes:
 * - A mulock_access keeps its @bref{locked_data_} alive
 *		through a strong @bref{locked_id_} reference.
 *
 ************************************************************************/
template <typename T>
	class mulock_access :
		private non_copyable
{

	public:

		mulock_access(const mulock_result& result)
			:
			locked_id_(result.request_.id),
			locked_data_(dynamic_cast<T*>(result.locked_data_))
		{
			if (result.request_.lock != mulock_request::write)
				mulock_access_util::throw_const_access_only();

			if (!locked_data_)
				mulock_access_util::throw_data_type_mismatch();
		}


		/** Access stored mulock_data. */
		//@{
		T& operator *() const { return *locked_data_; }
		T* operator ->() const { return locked_data_; }
		//@}

		
	private:
	
		const shared_instance<mulock_id> locked_id_;
		T* const locked_data_;
};




/**
 *************************************************************************
 *
 * @class mulock_const_access
 * @ingroup mulock_module
 *
 * Realizes read-only access to a mulock_data of type @bref{T}.
 *
 * Usage notes:
 * - A mulock_const_access keeps its @bref{locked_data_} alive
 *		through a strong @bref{locked_id_} reference.
 *
 ************************************************************************/
template <typename T>
	class mulock_const_access :
		private non_copyable
{

	public:

		mulock_const_access(const mulock_result& result)
			:
			locked_id_(result.request_.id),
			locked_data_(dynamic_cast<const T*>(result.locked_data_))
		{
			if (!locked_data_)
				mulock_access_util::throw_data_type_mismatch();
		}


		/** Access stored mulock_data. */
		//@{
		const T& operator *() const { return *locked_data_; }
		const T* operator ->() const { return locked_data_; }
		//@}


	private:

		const shared_instance<mulock_id> locked_id_;
		const T* const locked_data_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MULOCK_ACCESS_HPP) */