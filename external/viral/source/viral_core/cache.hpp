/**
 *************************************************************************
 *
 * @file cache.hpp
 *
 * Provides various name-to-object bindings.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__CACHE_HPP)
#define INCLUDED__VIRAL_CORE__CACHE_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "string.hpp"
#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "owning_map.hpp"
#include "tree_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class cache_errors
 * @ingroup container_module
 *
 * Provides static error handlers for cache templates,
 * which avoids header dependencies on logging.
 *
 ************************************************************************/
class cache_errors :
	private non_constructable
{
	template <typename T> friend class owning_cache;
	template <typename T> friend class shared_cache;

	private:

		static void log_and_throw_registration_failed
			(const string& cache_name, const string& name);
		static void log_and_throw_not_found
			(const string& cache_name, const string& name);
		static void log_and_throw_null_pointer
			(const string& cache_name, const string& name);
		static void log_and_throw_type_mismatch
			(const string& cache_name, const string& name);
};




/**
 *************************************************************************
 *
 * @class owning_cache
 * @ingroup container_module
 *
 * Owns named objects at least of type @bref{T}.
 *
 ************************************************************************/
template <typename T>
	class owning_cache :
		private non_copyable
{

	public:

		typedef owning_map<string, T> entry_map;
		typedef typename entry_map::const_iterator const_iterator; 


		/**
		 * Creates a named cache.
		 * The provided name is only used for debugging,
		 * and need not be necessarily unique.
		 */
		owning_cache(const string& cache_name)
			: cache_name(cache_name) { }


		/**
		 * Add @bref{name}d @bref{entry} to cache.
		 * Optionally transfer entry instead of copy.
		 * Throws if a respective entry exists already.
		 */
		//@{
		void insert
			(const string& name, const T& entry)
		{
			if (entries_.contains(name))
				cache_errors::log_and_throw_registration_failed(cache_name, name);

			entries_.insert
				(name, entry.clone());
		}

		void insert
			(const string& name, auto_pointer<T> entry)
		{
			if (entries_.contains(name))
				cache_errors::log_and_throw_registration_failed(cache_name, name);

			entries_.insert
				(name, std::move(entry));
		}
		//@}


		int64 size() const NOTHROW { return entries_.size(); }
		bool empty() const NOTHROW { return entries_.empty(); }


		const_iterator first() const NOTHROW
			{ return entries_.first(); }
		const_iterator last() const NOTHROW
			{ return entries_.last(); }


		/**
		 * Retrieve existing entry with given @bref{name},
		 * optionally at least of or exactly of type @bref{T}.
		 */
		//@{
		T& entry(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));

			if (!find)
				cache_errors::log_and_throw_not_found(cache_name, name);
			if (!find->value)
				cache_errors::log_and_throw_null_pointer(cache_name, name);

			return *find->value;
		}

		template <typename U>
			U& entry(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));

			if (!find)
				cache_errors::log_and_throw_not_found(cache_name, name);
			if (!find->value)
				cache_errors::log_and_throw_null_pointer(cache_name, name);

			U* dyncast = dynamic_cast<U*>(find->value);
			if (!dyncast)
				cache_errors::log_and_throw_type_mismatch(cache_name, name);

			return *dyncast;
		}

		template <typename U>
			U& entry_exact(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));

			if (!find)
				cache_errors::log_and_throw_not_found(cache_name, name);
			if (!find->value)
				cache_errors::log_and_throw_null_pointer(cache_name, name);

			if (typeid(U) != typeid(*find->value))
				cache_errors::log_and_throw_type_mismatch(cache_name, name);

			return static_cast<U&>(*find->value);
		}
		//@}


		/**
		 * Retrieve existing entry with given @bref{name},
		 * optionally at least of or exactly of type @bref{T}.
		 * Return @NULL if no such entry exists.
		 */
		//@{
		T* find(const string& name) const NOTHROW
		{
			typename entry_map::const_iterator find(entries_.find(name));
			return find ? find->value : NULL;
		}

		template <typename U>
			U* find(const string& name) const NOTHROW
		{
			typename entry_map::const_iterator find(entries_.find(name));
			return find ? dynamic_cast<U*>(find->value) : NULL;
		}

		template <typename U>
			U* find_exact(const string& name) const NOTHROW
		{
			typename entry_map::const_iterator find(entries_.find(name));

			if (!find || !find->value)
				return NULL;

			if (typeid(U) != typeid(*find->value))
				return NULL;

			return static_cast<U*>(find->value);
		}
		//@}
		

		bool contains(const string& name) const
			{ return entries_.contains(name); }


		/**
		 * Release ownership over entry of given @bref{name},
		 * optionally at least of or exactly of type @bref{T}.
		 * The cache returns @NULL and keeps any ownership
		 * if no such entry exists.
		 */
		//@{
		auto_pointer<T> release(const string& name) NOTHROW
		{
			typename entry_map::iterator find(entries_.find(name));
			return find ?
					entries_.release(find) : auto_pointer<T>();
		}

		template <typename U>
			auto_pointer<U> release(const string& name) NOTHROW
		{
			typename entry_map::iterator find(entries_.find(name));
			if (!find || !find->value)
				return auto_pointer<U>();

			U* typed_ptr = dynamic_cast<U*>(find->value);
			if (!typed_ptr)
				return auto_pointer<U>();

			// NOTHROW: Pass on ownership.
			entries_.release(find).release_ownership();
			return auto_pointer<U>(typed_ptr);
		}

		template <typename U>
			auto_pointer<U> release_exact(const string& name) NOTHROW
		{
			typename entry_map::iterator find(entries_.find(name));
			if (!find || !find->value)
				return auto_pointer<U>();

			if (typeid(U) != typeid(*find->value))
				return auto_pointer<U>();

			// NOTHROW: Pass on ownership.
			T* ptr = entries_.release(find).release_ownership();
			return auto_pointer<U>(static_cast<U*>(ptr));
		}
		//@}


		void remove(const string& name) NOTHROW
			{ entries_.remove(name); }
		void clear() NOTHROW
			{ entries_.clear(); }


		const string cache_name;


	private:

		entry_map entries_;
};




/**
 *************************************************************************
 *
 * @class shared_cache
 * @ingroup container_module
 *
 * Binds names to shared_instance%s at least of type @bref{T}.
 *
 * Usage notes:
 * - Any shared_cache keeps contained instances alive, which prohibits
 *		deletion of associated objects. Hence, clients must manually
 *		remove instances of unwanted objects from the cache.
 *
 ************************************************************************/
template <typename T>
	class shared_cache :
		private non_copyable
{

	public:
		
		typedef tree_map<string, shared_instance<T>> entry_map;
		typedef typename entry_map::const_iterator const_iterator; 


		/**
		 * Creates a named cache.
		 * The provided name is only used for debugging,
		 * and need not be necessarily unique.
		 */
		shared_cache(const string& cache_name)
			: cache_name(cache_name) { }


		/**
		 * Add @bref{name}d @bref{entry} to cache.
		 * Throws if a respective entry exists already.
		 */
		void insert
			(const string& name, const shared_instance<T>& entry)
		{
			if (entries_.contains(name))
				cache_errors::log_and_throw_registration_failed(cache_name, name);

			entries_.insert(name, entry);
		}


		int64 size() const NOTHROW { return entries_.size(); }
		bool empty() const NOTHROW { return entries_.empty(); }


		const_iterator first() const NOTHROW
			{ return entries_.first(); }
		const_iterator last() const NOTHROW
			{ return entries_.last(); }


		/**
		 * Retrieve existing, not-@NULL entry with given @bref{name},
		 * optionally at least of or exactly of type @bref{U}.
		 */
		//@{
		shared_instance<T> entry(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));
			if (!find)
				cache_errors::log_and_throw_not_found(cache_name, name);

			return find->value;
		}

		template <typename U>
			shared_instance<U> entry(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));
			if (!find)
				cache_errors::log_and_throw_not_found(cache_name, name);

			shared_pointer<U> dyncast(find->value.template try_cast<U>());
			if (!dyncast)
				cache_errors::log_and_throw_type_mismatch(cache_name, name);

			return dyncast.instance_or_throw();
		}

		template <typename U>
			shared_instance<U> entry_exact(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));
			if (!find)
				cache_errors::log_and_throw_not_found(cache_name, name);

			shared_pointer<U> cast(find->value.template try_cast_exact<U>());
			if (!cast)
				cache_errors::log_and_throw_type_mismatch(cache_name, name);

			return cast.instance_or_throw();
		}
		//@}


		/**
		 * Retrieve existing entry with given @bref{name},
		 * optionally at least of or exactly of type @bref{T}.
		 * Return @NULL if no such entry exists.
		 */
		//@{
		shared_pointer<T> find(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));
			return find ? find->value.get() : shared_pointer<T>();
		}

		template <typename U>
			shared_pointer<U> find(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));
			return find ?
					find->value.template try_cast<U>() :
					shared_pointer<U>();
		}

		template <typename U>
			shared_pointer<U> find_exact(const string& name) const
		{
			typename entry_map::const_iterator find(entries_.find(name));
			return find ?
					find->value.template try_cast_exact<U>() :
					shared_pointer<U>();
		}
		//@}


		bool contains(const string& name) const
			{ return entries_.contains(name); }


		void remove(const string& name) NOTHROW
			{ entries_.remove(name); }
		void clear() NOTHROW
			{ entries_.clear(); }


		const string cache_name;


	private:

		entry_map entries_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__CACHE_HPP) */
