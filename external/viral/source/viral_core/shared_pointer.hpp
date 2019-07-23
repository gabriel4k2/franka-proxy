/**
 *************************************************************************
 *
 * @file shared_pointer.hpp
 *
 * Header for shared and weak pointers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__SHARED_POINTER_HPP)
#define INCLUDED__VIRAL_CORE__SHARED_POINTER_HPP


#include "build_config.hpp"


#if defined(VIRAL__USE_CPP11_THREAD)
	#include <atomic>
#endif


#include <new>
#include <typeinfo>


#include "container_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class shared_life_manager_base
 * @ingroup container_module
 *
 * Helper for shared_pointer and weak_pointer
 * that is used for managing object lifetimes,
 * including thread-safe reference counting,
 * object deletion and self-deletion.
 *
 * Overflow on reference counters is not possible:
 * - On x32 systems, counter ranges exceed the available address space,
 *		thus exceed the maximum possible number of shared or weak pointers.
 * - On x64 systems, counter ranges cover but half of the addressable space.
 *		However, each shared or weak pointer also requires additional memory
 *		(in excess of two bytes), which drops the maximum number of pointers
 *		in the addressable space to covered range.
 *
 ************************************************************************/
class shared_life_manager_base :
	private non_copyable
{
	template <typename T> friend class shared_life_manager;
	template <typename T> friend class shared_pointer;
	template <typename T> friend class weak_pointer;


	private:

		void add_shared_pointer() NOTHROW;
		void add_weak_pointer() NOTHROW;
		void remove_shared_pointer() NOTHROW;
		void remove_weak_pointer() NOTHROW;
		bool try_lock_weak_pointer() NOTHROW;
		bool try_destroy_if_unique() NOTHROW;

		shared_life_manager_base() NOTHROW;
		virtual ~shared_life_manager_base() NOTHROW { };
		virtual void delete_object() NOTHROW = 0;


		#if defined(VIRAL__USE_MS_THREAD)
			volatile int64 shared_pointer_count_;
			volatile int64 total_pointer_count_;
		#endif

		#if defined(VIRAL__USE_CPP11_THREAD)
			std::atomic<int64> shared_pointer_count_;
			std::atomic<int64> total_pointer_count_;
		#endif

		#if !defined(VIRAL__USE_MS_THREAD) && \
			!defined(VIRAL__USE_CPP11_THREAD)
				#error "Not implemented for current thread back-end."
		#endif
};




/**
 *************************************************************************
 *
 * @class shared_life_manager
 * @ingroup container_module
 *
 * Implementation of shared_life_manager_base destruction
 * for the original pointer class @bref{T}, independent of
 * the type of any remaining shared_pointers.
 *
 ************************************************************************/
template <typename T>
	class shared_life_manager : public shared_life_manager_base
{
	template <typename U> friend class shared_life_manager;
	template <typename U> friend class shared_pointer;
	template <typename U> friend class weak_pointer;


	private:

		shared_life_manager(T* original_object) NOTHROW
			: original_object_(original_object) { }


		static shared_life_manager* create(T* object)
		{
			// Exception safety: Delete the object
			// if construction of life manager fails.
			SCOPE_GUARD
				(creation_guard, (&guard_delete<T>, object));

			shared_life_manager *life_manager =
				new shared_life_manager(object);

			creation_guard.dismiss();
			return life_manager;
		}


		void delete_object() NOTHROW override
			{ delete original_object_; }


		T* original_object_;
};




/**
 *************************************************************************
 *
 * @class shared_pointer
 * @ingroup container_module
 *
 * A series of strong, shared pointers share ownership
 * over the same object of base type @bref{T}.
 *
 * Notes:
 * - If a call does not need long-time shared ownership over an object,
 *		pass the actual object by reference or by pointer instead.
 * - Copying shared or weak pointers is costy, thus pass these pointers
 *		by constant reference or move where possible.
 * - Circular dependencies in shared pointers prohibit object deletion.
 *		A weak_pointer should be used to break any such cycle.
 * - Multiple pointers to the same shared instance of some object
 *		are threadsafe for multiple threads, but each shared pointer
 *		instance is safe for use by only a single thread at a time.
 * - The stored object is deleted in the context of the thread
 *		that releases the last shared pointer. The object destructor
 *		needs to be particularly aware of thread safety guidelines.
 *
 ************************************************************************/
template <typename T>
	class shared_pointer :
		private bool_convert<shared_pointer<T>>
{
	template <typename U> friend class shared_pointer;
	template <typename U> friend class weak_pointer;


	public:

		shared_pointer() NOTHROW
			: object_(NULL), life_manager_(NULL) { }


		/**
		 * Create new shared pointer from @bref{object},
		 * and delete the object on exceptional failure
		 * (e.g. out_of_memory for a reference counter).
		 */
		template <typename D>
			explicit shared_pointer(D* object)
			:
			object_(object),
			life_manager_(object ? shared_life_manager<D>::create(object) : NULL)
		{ }


		/**
		 * Copy or move construct from @bref{other} shared pointer.
		 * Move construction leaves a valid, @NULL @bref{other}.
		 */
		//@{		
		shared_pointer(const shared_pointer& other) NOTHROW
			: object_(other.object_), life_manager_(other.life_manager_)
				{ if (life_manager_) life_manager_->add_shared_pointer(); }
		
		shared_pointer(shared_pointer&& other) NOTHROW
			: object_(other.object_), life_manager_(other.life_manager_)
				{ other.object_ = NULL; other.life_manager_ = NULL; }

		template <typename D>
			shared_pointer(const shared_pointer<D>& other) NOTHROW
				: object_(other.object_), life_manager_(other.life_manager_)
					{ if (life_manager_) life_manager_->add_shared_pointer(); }

		template <typename D>
			shared_pointer(shared_pointer<D>&& other) NOTHROW
				: object_(other.object_), life_manager_(other.life_manager_)
					{ other.object_ = NULL; other.life_manager_ = NULL; }
		//@}


		template <typename D>
			shared_pointer(const shared_instance<D>& instance) NOTHROW
				: object_(instance.get().object_),
				  life_manager_(instance.get().life_manager_)
					{ if (life_manager_) life_manager_->add_shared_pointer(); }


		~shared_pointer() NOTHROW
			{ if (life_manager_) life_manager_->remove_shared_pointer(); }


		/** Reset to @NULL pointer. */
		void reset() NOTHROW
		{
			if (life_manager_)
				life_manager_->remove_shared_pointer();

			object_ = NULL;
			life_manager_ = NULL;
		}


		/**
		 * Reset to new, currently unshared @bref{object}, or delete
		 * this object if shared counter allocation fails exceptionally.
		 * In the latter case, the shared_pointer remains unchanged.
		 */
		void reset(T *object)
		{
			shared_life_manager<T> *life_manager =
				object ? shared_life_manager<T>::create(object) : NULL;

			// Exception safety: NOTHROW from here.
			reset();
			object_ = object;
			life_manager_ = life_manager;
		}


		/**
		 * Copy or move assign of @bref{other} shared pointer.
		 * Move construction leaves a valid, @NULL @bref{other}.
		 */
		//@{
		shared_pointer& operator =(const shared_pointer& other) NOTHROW
			{ assign_new_target(other.object_, other.life_manager_); return *this; }

		shared_pointer& operator =(shared_pointer&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			reset();
			object_ = other.object_;
			life_manager_ = other.life_manager_;
			other.object_ = NULL;
			other.life_manager_ = NULL;
			return *this;
		}

		template <typename D>
			shared_pointer& operator =(const shared_pointer<D>& other) NOTHROW
				{ assign_new_target(other.object_, other.life_manager_); return *this; }

		template <typename D>
			shared_pointer& operator =(shared_pointer<D>&& other) NOTHROW
		{
			// No test for self-identity,
			// as other has different type.

			// Exception safety: NOTHROW.
			reset();
			object_ = other.object_;
			life_manager_ = other.life_manager_;
			other.object_ = NULL;
			other.life_manager_ = NULL;
			return *this;
		}
		//@}


		template <typename D>
			shared_pointer& operator =(const shared_instance<D>& instance) NOTHROW
		{
			assign_new_target(instance.get().object_,
							  instance.get().life_manager_);
			return *this;
		}


		void swap_with(shared_pointer& other) NOTHROW
		{
			swap(object_, other.object_);
			swap(life_manager_, other.life_manager_);
		}


		/**
		 * Try to cast to another pointer type @bref{D}.
		 * Return @NULL if types are incompatible or
		 * if the initial pointer is @NULL.
		 *
		 * In general, prefer dynamic over exact casts.
		 * Exact casts are much faster, but only work
		 * for exact object types.
		 */
		//@{
		template <typename D>
			shared_pointer<D> try_cast() const NOTHROW
		{
			D* dyncast = dynamic_cast<D*>(object_);
			if (!dyncast) return shared_pointer<D>();

			life_manager_->add_shared_pointer();
			return shared_pointer<D>(dyncast, life_manager_);
		}

		template <typename D>		
			shared_pointer<D> try_cast_exact() const NOTHROW
		{
			if (!object_) return shared_pointer<D>();
			if (typeid(*object_) != typeid(D)) return shared_pointer<D>();

			D* exact = static_cast<D*>(object_);
	
			life_manager_->add_shared_pointer();
			return shared_pointer<D>(exact, life_manager_);
		} 
		//@}


		/**
		 * Dereference target object, or throw on @NULL pointer.
		 * Optionally cast to type @bref{T} and throw on failure.
		 * See try_cast() and try_cast_exact().
		 */
		//@{		
		T& object() const
		{
			if (!object_) throw null_pointer();
			return *object_;
		}

		template <typename D>
			D& object() const
		{
			if (!object_) throw null_pointer();
			D* dyncast = dynamic_cast<D*>(object_);
			if (!dyncast) throw type_mismatch();
			return *dyncast;
		}

		template <typename D>
			D& object_exact() const
		{
			if (!object_) throw null_pointer();
			if (typeid(*object_) != typeid(D)) throw type_mismatch();
			return static_cast<D&>(*object_);
		}
		//@}


		/**
		 * Internal pointer access.
		 *
		 * @attent Dereferencing does not check for @NULL pointers.
		 * Use object() for automatic null_pointer() exceptions.
		 */
		//@{
		T* get() const NOTHROW { return object_; }
		T& operator *() const NOTHROW { return *object_; }
		T* operator ->() const NOTHROW { return object_; }
		//@}


		/**
		 * If this shared pointer is unique, both reset the pointer and 
		 * destroy any attached object in a single atomic operation.
		 * Returns @true if an attached object was destroyed, @false if
		 * the pointer was @NULL or not unique.
		 * 
		 * @attent Explicit, separate calls to unique() and reset()
		 * must not be used to avoid a potential race condition.
		 */
		bool try_destroy() NOTHROW
		{
			if (!life_manager_ ||
				!life_manager_->try_destroy_if_unique()) return false;

			object_ = NULL;
			life_manager_ = NULL;

			return true;
		}


		/**
		 * Convert a not-@NULL shared_pointer to a shared_instance.
		 * By default, throw if and only if the shared_pointer is @NULL.
		 * Optionally, terminate the program for @ref NOTHROW guarantee.
		 * The latter variant expects client code to explicitly check
		 * for @NULL pointers.
		 */
		//@{
		shared_instance<T> instance_or_throw() const
		{
			if (!object_) throw null_pointer();
			return shared_instance<T>(*this);
		}

		shared_instance<T> instance_or_terminate() const NOTHROW
		{
			if (!object_) std::terminate();
			return shared_instance<T>(*this);
		}
		//@}


		/** Test if this pointer is the only one keeping an object alive. */
		bool unique() const NOTHROW
			{ return life_manager_->shared_pointer_count_ == 1; }

		/** Test if an internal object currently is assigned. */
		operator bool_conversion_type() const NOTHROW
			{ return object_ ? &shared_pointer<T>::bool_conversion_is_true : NULL; }


		template <typename D>
			bool operator ==(const shared_pointer<D>& other) const NOTHROW
				{ return life_manager_ == other.life_manager_; }
		template <typename D>
			bool operator ==(const weak_pointer<D>& other) const NOTHROW
				{ return life_manager_ == other.life_manager_; }
		template <typename D>
			bool operator ==(const shared_instance<D>& instance) const NOTHROW
				{ return life_manager_ == instance.get().life_manager_; }

		template <typename D>
			bool operator !=(const shared_pointer<D>& other) const NOTHROW
				{ return life_manager_ != other.life_manager_; }
		template <typename D>
			bool operator !=(const weak_pointer<D>& other) const NOTHROW
				{ return life_manager_ != other.life_manager_; }
		template <typename D>
			bool operator !=(const shared_instance<D>& instance) const NOTHROW
				{ return life_manager_ != instance.get().life_manager_; }

		bool operator <(const shared_pointer& other) const NOTHROW
			{ return life_manager_ < other.life_manager_; }


	private:

		/**
		 * Used for sharing ownership with an existing pointer
		 * of incompatible type, such as on weak_pointer::lock,
		 * or on shared_pointer::cast.
		 *
		 * @attent Previous code already updated counters.
		 */
		shared_pointer(T *object, shared_life_manager_base *life_manager) NOTHROW
			: object_(object), life_manager_(life_manager) { }


		/**
		 * Share ownership with an existing pointer
		 * of compatible type, such as on operator =.
		 * Counters have not been taken care of here.
		 */
		template <typename D>
			void assign_new_target
				(D* new_object, shared_life_manager_base *new_life_manager) NOTHROW
		{
			if (life_manager_ == new_life_manager) return;

			if (life_manager_) life_manager_->remove_shared_pointer();

			object_ = new_object;
			life_manager_ = new_life_manager;
			if (life_manager_) life_manager_->add_shared_pointer();
		}
		

		T *object_;
		shared_life_manager_base *life_manager_;
};




/**
 *************************************************************************
 *
 * @class weak_pointer
 * @ingroup container_module
 *
 * Holds a non-owning reference to a shared_pointer target object,
 * and must be locked back into a shared_pointer instance before use.
 *
 ************************************************************************/
template <typename T>
	class weak_pointer :
		private bool_convert<weak_pointer<T>>
{
	template <typename U> friend class shared_pointer;
	template <typename U> friend class weak_pointer;


	public:

		weak_pointer() NOTHROW
			: object_(NULL), life_manager_(NULL) { }


		/**
		 * Copy or move construct from @bref{other} weak pointer.
		 * Move construction leaves a valid, @NULL @bref{other}.
		 */
		//@{
		weak_pointer(const weak_pointer& other) NOTHROW
			: object_(other.object_), life_manager_(other.life_manager_)
				{ if (life_manager_) life_manager_->add_weak_pointer(); }

		weak_pointer(weak_pointer&& other) NOTHROW
			: object_(other.object_), life_manager_(other.life_manager_)
				{ other.object_ = NULL; other.life_manager_ = NULL; }

		template <typename D>
			weak_pointer(const weak_pointer<D>& other) NOTHROW
				: object_(other.object_), life_manager_(other.life_manager_)
					{ if (life_manager_) life_manager_->add_weak_pointer(); }

		template <typename D>
			weak_pointer(weak_pointer<D>&& other) NOTHROW
				: object_(other.object_), life_manager_(other.life_manager_)
					{ other.object_ = NULL; other.life_manager_ = NULL; }
		//@}


		template <typename D>
			weak_pointer(const shared_pointer<D>& pointer) NOTHROW
				: object_(pointer.object_), life_manager_(pointer.life_manager_)
					{ if (life_manager_) life_manager_->add_weak_pointer(); }

		template <typename D>
			weak_pointer(const shared_instance<D>& instance) NOTHROW
				: object_(instance.get().object_),
				  life_manager_(instance.get().life_manager_)
					{ if (life_manager_) life_manager_->add_weak_pointer(); }


		~weak_pointer() NOTHROW
			{ if (life_manager_) life_manager_->remove_weak_pointer(); }


		/** Reset to @NULL pointer. */
		void reset() NOTHROW
		{
			if (life_manager_) life_manager_->remove_weak_pointer();
			object_ = NULL;
			life_manager_ = NULL;
		}


		/**
		 * Copy or move assign of @bref{other} weak pointer.
		 * Move construction leaves a valid, @NULL @bref{other}.
		 */
		//@{
		weak_pointer& operator =(const weak_pointer& other) NOTHROW
			{ assign_new_target(other.object_, other.life_manager_); return *this; }

		weak_pointer& operator =(weak_pointer&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			reset();
			object_ = other.object_;
			life_manager_ = other.life_manager_;
			other.object_ = NULL;
			other.life_manager_ = NULL;
			return *this;
		}

		template <typename D>
			weak_pointer& operator =(const weak_pointer<D>& other) NOTHROW
				{ assign_new_target(other.object_, other.life_manager_); return *this; }

		template <typename D>
			weak_pointer& operator =(weak_pointer<D>&& other) NOTHROW
		{
			// No test for self-identity,
			// as other has different type.

			// Exception safety: NOTHROW.
			reset();
			object_ = other.object_;
			life_manager_ = other.life_manager_;
			other.object_ = NULL;
			other.life_manager_ = NULL;
			return *this;
		}
		//@}


		template <typename D>
			weak_pointer& operator =(const shared_pointer<D>& pointer) NOTHROW
				{ assign_new_target(pointer.object_, pointer.life_manager_); return *this; }

		template <typename D>
			weak_pointer& operator =(const shared_instance<D>& instance) NOTHROW
		{
			assign_new_target(instance.get().object_,
							  instance.get().life_manager_);
			return *this;
		}


		void swap_with(weak_pointer& other) NOTHROW
		{
			swap(object_, other.object_);
			swap(life_manager_, other.life_manager_);
		}


		/**
		 * Create a shared pointer that owns the weak pointer target,
		 * or a @NULL shared pointer if the weak pointer is @NULL,
		 * or a @NULL shared pointer if the target object has already
		 * been deleted (i.e. when there are no more shared pointers).
		 */
		shared_pointer<T> lock() const NOTHROW
		{
			if (!life_manager_ || !life_manager_->try_lock_weak_pointer())
				return shared_pointer<T>();

			return shared_pointer<T>(object_, life_manager_);
		}


		/** Test if the weak pointer is not @NULL. */
		operator bool_conversion_type() const NOTHROW
			{ return object_ ? &weak_pointer<T>::bool_conversion_is_true : NULL; }


		/**
		 * Compare weak pointers based on associated objects.
		 * Correctly discerns deleted objects even if their
		 * addresses have been reused.
		 */
		//@{
		template <typename D>
			bool operator ==(const weak_pointer<D>& other) const NOTHROW
				{ return life_manager_ == other.life_manager_; }
		template <typename D>
			bool operator ==(const shared_pointer<D>& other) const NOTHROW
				{ return life_manager_ == other.life_manager_; }
		template <typename D>
			bool operator ==(const shared_instance<D>& instance) const NOTHROW
				{ return life_manager_ == instance.get().life_manager_; }

		template <typename D>
			bool operator !=(const weak_pointer<D>& other) const NOTHROW
				{ return life_manager_ != other.life_manager_; }
		template <typename D>
			bool operator !=(const shared_pointer<D>& other) const NOTHROW
				{ return life_manager_ != other.life_manager_; }
		template <typename D>
			bool operator !=(const shared_instance<D>& instance) const NOTHROW
				{ return life_manager_ != instance.get().life_manager_; }

		bool operator <(const weak_pointer& other) const NOTHROW
			{ return life_manager_ < other.life_manager_; }
		//@}


	private:

		template <typename D>
			void assign_new_target
				(D* new_object, shared_life_manager_base *new_life_manager) NOTHROW
		{
			if (life_manager_ == new_life_manager) return;

			if (life_manager_) life_manager_->remove_weak_pointer();

			object_ = new_object;
			life_manager_ = new_life_manager;
			if (life_manager_) life_manager_->add_weak_pointer();
		}


		T *object_;
		shared_life_manager_base *life_manager_;
};




/**
 *************************************************************************
 *
 * @class shared_instance
 * @ingroup container_module
 *
 * Shares ownership over a not-@NULL heap object of type @bref{T}
 * with other shared_instance%s and shared_pointer%s.
 *
 * Usage notes:
 * - All operations that would result in a @NULL shared_instance
 *		(e.g. construction from a @NULL native or shared_pointer)
 *		throw a null_pointer exception instead.
 * - Shared instances do not mimic shared_pointer%s or native pointers.
 *		In particular, shared instances have no dereferencing operators,
 *		as dereferencing always is safe and need not stand out in code.
 *		Instead, clients should access the internal object with operator().
 *
 * Implementation notes:
 * - Shared instances do not implement move constructors or move assign,
 *		because for defined behaviour shared instances may not be @NULL.
 *
 ************************************************************************/
template <typename T>
	class shared_instance
{
	template <typename U> friend class shared_pointer;
	template <typename U> friend class shared_instance;


	public:

		/**
		 * Accept shared ownership over not-@NULL @bref{object} pointer.
		 * Throws null_pointer if @bref{object} is @NULL.
		 */
		explicit shared_instance(T* object)
			: pointer_(object)
				{ if (!pointer_) throw null_pointer(); }


		shared_instance(const shared_instance& other) NOTHROW
			: pointer_(other.pointer_) { }

		template <typename D>
			shared_instance(const shared_instance<D>& other) NOTHROW
				: pointer_(other.pointer_) { }


		/**
		 * Reset to new, currently unshared not-@NULL @bref{object}.
		 * Throws null_pointer if @bref{object} is @NULL.
		 * Deletes @bref{object} and remains unchanged on other errors.
		 */
		void reset(T *object)
		{
			if (!object) throw null_pointer();
			pointer_ = shared_pointer<T>(object);
		}


		shared_instance& operator =(const shared_instance& other) NOTHROW
			{ pointer_ = other.pointer_; return *this; }


		void swap_with(shared_instance& other) NOTHROW
			{ swap(pointer_, other.pointer_); }


		/**
		 * Convert to shared_instance exactly of
		 * or at least of another type @bref{T}.
		 * Throws type_mismatch on failure.
		 * See try_cast() and try_cast_exact().
		 */
		//@{
		template <typename D>
			shared_instance<D> cast() const
		{
			shared_pointer<D> pointer_cast(pointer_.try_cast<D>());
			if (!pointer_cast) throw type_mismatch();
			return shared_instance<D>(pointer_cast);
		}

		template <typename D>
			shared_instance<D> cast_exact() const
		{
			shared_pointer<D> pointer_cast(pointer_.try_cast_exact<D>());
			if (!pointer_cast) throw type_mismatch();
			return shared_instance<D>(pointer_cast);
		}
		//@}


		/**
		 * Cast contained pointer to different type.
		 * Returns a @NULL shared_pointer on failure.
		 * See shared_pointer::try_cast() and
		 * shared_pointer::try_cast_exact().
		 */
		//@{
		template <typename D>
			shared_pointer<D> try_cast() const NOTHROW
				{ return pointer_.try_cast<D>(); }
		template <typename D>		
			shared_pointer<D> try_cast_exact() const NOTHROW
				{ return pointer_.try_cast_exact<D>(); }
		//@}


		/**
		 * Fetch internal object.
		 * Optionally cast to type @bref{T} and throw on failure.
		 * See try_cast() and try_cast_exact().
		 */
		//@{
		T& operator()() const NOTHROW
			{ return *pointer_; }
		T& object() const NOTHROW
			{ return *pointer_; }

		template <typename D>
			D& object() const
		{
			D* dyncast = dynamic_cast<D*>(pointer_.get());
			if (!dyncast) throw type_mismatch();
			return *dyncast;
		}

		template <typename D>
			D& object_exact() const
		{
			if (typeid(*pointer_.get()) != typeid(D))
				throw type_mismatch();

			return static_cast<D&>(*pointer_.get());
		}
		//@}


		/** Fetch internal not-@NULL shared_pointer. */
		const shared_pointer<T>& get() const NOTHROW
			{ return pointer_; }


		template <typename D>
			bool operator ==(const shared_instance<D>& other) const NOTHROW
				{ return pointer_ == other.pointer_; }
		template <typename D>
			bool operator ==(const shared_pointer<D>& pointer) const NOTHROW
				{ return pointer_ == pointer; }
		template <typename D>
			bool operator ==(const weak_pointer<D>& pointer) const NOTHROW
				{ return pointer_ == pointer; }

		template <typename D>
			bool operator !=(const shared_instance<D>& other) const NOTHROW
				{ return pointer_ != other.pointer_; }
		template <typename D>
			bool operator !=(const shared_pointer<D>& pointer) const NOTHROW
				{ return pointer_ != pointer; }
		template <typename D>
			bool operator !=(const weak_pointer<D>& pointer) const NOTHROW
				{ return pointer_ != pointer; }

		bool operator <(const shared_instance& other) const NOTHROW
			{ return pointer_ < other.pointer_; }


	private:

		/**
		 * Only used through shared_pointer::instance_or_throw()
		 * and through instance_or_exit().
		 */
		template <typename D>
			shared_instance(const shared_pointer<D>& pointer) NOTHROW
				: pointer_(pointer) { }


		shared_pointer<T> pointer_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__SHARED_POINTER_HPP) */
