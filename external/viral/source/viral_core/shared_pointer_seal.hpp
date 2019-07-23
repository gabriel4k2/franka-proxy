/**
 *************************************************************************
 *
 * @file shared_pointer_seal.hpp
 * 
 * Constant, shared, and threadsafe data storage.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SHARED_POINTER_SEAL_HPP)
#define INCLUDED__VIRAL_CORE__SHARED_POINTER_SEAL_HPP


#include "build_config.hpp"


#include "shared_pointer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class shared_pointer_seal
 * @ingroup container_module
 *
 * Shared pointer to internal, read-only and thus threadsafe data
 * of some data type @bref{T}.
 *
 * Usage notes:
 * - A shared_pointer_seal internally creates a new data instance
 *		by copying or moving client-provided initialization data.
 * - Several shared_pointer_seal%s may share their data instance
 *		in the same way as traditional shared_pointer%s.
 * - Each shared_pointer_seal only exposes constant data access.
 *		Multiple shared_pointer_seal%s to the same data instance
 *		thus are threadsafe if constness of @bref{T} is absolute.
 * - @attent If @bref{T} foregoes constness (e.g. mutable states,
 *		pointers to non-const objects), then shared_pointer_seal
 *		does not ensure thread safety for the target data.
 *
 * Implementation notes:
 * - When using traditional shared_pointer%s with a const target,
 *		clients may still keep non-const references to the data
 *		(e.g. from before initializing the first shared_pointer)
 *		and thus compromise thread safety. This is not possible
 *		with shared_pointer_seal due to its internal data.
 *
 ************************************************************************/
template <typename T>
	class shared_pointer_seal :
		private bool_convert<shared_pointer_seal<T>>
{

	public:

		/** Create pointer without assigned data. */
		shared_pointer_seal() NOTHROW { }


		/** Create pointer by copying or moving client @bref{data}. */
		//@{
		shared_pointer_seal(const T& data)
			: pointer_(new T(data)) { }
		shared_pointer_seal(T && data)
			: pointer_(new T(std::move(data))) { }
		//@}


		/** Create another pointer to existing, read-only data. */
		shared_pointer_seal
			(const shared_pointer_seal& other) NOTHROW
				: pointer_(other.pointer_) { }


		/** Copy assign pointer to existing, read-only data. */
		shared_pointer_seal& operator =
			(const shared_pointer_seal& other) NOTHROW
		{
			if (this == &other) return *this;
			pointer_ = other.pointer_;
			return *this;
		}	


		/** Dereference target data, or throw on @NULL pointer. */
		const T& data() const
		{
			if (!pointer_) throw null_pointer();
			return *pointer_;
		}


		/** Unchecked, but const, access to target data. */
		//@{
		const T* get() const NOTHROW { return pointer_; }
		const T& operator *() const NOTHROW { return *pointer_; }
		const T* operator ->() const NOTHROW { return pointer_; }
		//@}


		/** Test if any data currently is assigned. */
		operator bool_conversion_type() const NOTHROW
		{
			return pointer_ ?
				&shared_pointer_seal<T>::bool_conversion_is_true : NULL;
		}


	private:

		shared_pointer<const T> pointer_;
};




/**
 *************************************************************************
 *
 * @class shared_instance_seal
 * @ingroup container_module
 *
 * A shared_instance variant of shared_pointer_seal.
 *
 ************************************************************************/
template <typename T>
	class shared_instance_seal
{

	public:

		/** Create instance by copying or moving client @bref{data}. */
		//@{
		shared_instance_seal(const T& data)
			: pointer_(data) { }
		shared_instance_seal(T && data)
			: pointer_(std::move(data)) { }
		//@}


		/** Create another reference to existing, read-only data. */
		shared_instance_seal
			(const shared_instance_seal& other) NOTHROW
				: pointer_(other.pointer_) { }


		/** Copy assign reference to existing, read-only data. */
		shared_instance_seal& operator =
			(const shared_instance_seal& other) NOTHROW
		{
			if (this == &other) return *this;
			pointer_ = other.pointer_;
			return *this;
		}	


		/** Dereference target data. */
		//@{
		const T& operator()() const NOTHROW
			{ return *pointer_; }
		const T& data() const NOTHROW
			{ return *pointer_; }
		//@}


		/** Fetch internal not-@NULL shared_pointer_seal. */
		const shared_pointer_seal<T>& get() const NOTHROW
			{ return pointer_; }


	private:

		shared_pointer_seal<T> pointer_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SHARED_POINTER_SEAL_HPP) */
