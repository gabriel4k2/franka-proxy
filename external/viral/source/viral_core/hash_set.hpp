/**
 *************************************************************************
 *
 * @file hash_set.hpp
 *
 * Header for hash_set and helper templates.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__HASH_SET_HPP)
#define INCLUDED__VIRAL_CORE__HASH_SET_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "allocator_heap.hpp"
#include "allocator_util.hpp"
#include "auto_pointer.hpp"
#include "iterator.hpp"
#include "hash_base.hpp"
#include "hash_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class hash_set_node
 * @ingroup container_module
 *
 * An entry within a hash_set, holds a single key.
 *
 ************************************************************************/
template <typename K, typename A>
	class hash_set_node
{
	friend class hash_base<hash_set_node<K, A>, A>;
	friend class hash_set<K, A>;

	friend class iterator<hash_set<K, A>>;
	friend class const_iterator<hash_set<K, A>>;


	public:

		hash_set_node(const K& key)
			:
			key_(key),
			previous_(NULL), next_(NULL),
			bucket_(NULL)
		{ }

		hash_set_node(const hash_set_node& other)
			:
			key_(other.key_),
			previous_(NULL), next_(NULL),
			bucket_(NULL)
		{ }


	private:

		bool operator ==(const K& other) const NOTHROW
			{ return key_ == other; }
		bool operator ==(const hash_set_node& other) const NOTHROW
			{ return key_ == other.key_; }

		uint64 hash_content() const NOTHROW
			{ return hash(key_); }

		K& content_for_iterator() NOTHROW
			{ return key_; }


		K key_;
		hash_set_node *previous_;
		hash_set_node *next_;
		hash_bucket<hash_set_node<K, A>, A> *bucket_;
};




/**
 *************************************************************************
 *
 * @class hash_set
 * @ingroup container_module
 *
 * Stores an unsorted sequence of unique keys of type @bref{K},
 * allocated through an allocator of type @bref{A}.
 *
 * Notes:
 * - A single call to the copy constructor of @bref{K} is used to insert
 *		keys into hash_set entries. No default constructor is required.
 * - The key type @bref{K} of a hash_set must support the global
 *		hash utility function viral_core::hash().
 * - @attent Keys already stored within a hash set must not be modified
 *		in a way that changes hashes. Otherwise, behaviour is undefined.
 *
 ************************************************************************/
template <typename K, typename A>
	class hash_set
{
	friend class iterator<hash_set<K, A>>;
	friend class const_iterator<hash_set<K, A>>;


	public:

		typedef K key_type;

		typedef K content_type;
		typedef hash_set_node<K, A> node_type;

		typedef A allocator_type;

		typedef typename viral_core::iterator<hash_set<K, A>> iterator;
		typedef typename viral_core::const_iterator<hash_set<K, A>> const_iterator;


		hash_set() NOTHROW { }


		explicit hash_set
			(const allocator_type& allocator) NOTHROW
				: hash_(allocator) { }


		/**
		 * Copy or move construct from @bref{other} hash_set.
		 * Move construction leaves a valid, empty source set.
		 */
		//@{		
		hash_set(const hash_set& other)
			: hash_(other.hash_) { }
		hash_set(hash_set&& other) NOTHROW
			: hash_(std::move(other.hash_)) { }
		//@}


		/** Create copy of hash_set on heap. */
		auto_pointer<hash_set> clone() const
			{ return auto_pointer<hash_set>(new hash_set(*this)); }


		/**
		 * Copy or move assign from @bref{other} hash_set.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		hash_set& operator =(const hash_set& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			hash_set offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		hash_set& operator =(hash_set&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			hash_set offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(hash_set& other) NOTHROW
			{ swap(hash_, other.hash_); }

		
		/**
		 * Insert a @bref{key} into the set.
		 * If the set already contains the @bref{key},
		 * return an iterator to the existing entry.
		 * Otherwise, return an iterator to the new
		 * entry for @bref{key}.
		 *
		 * On any exception, the set might already
		 * have been rehashed.
		 *
		 * @attent The entire set might be rehashed
		 * if max_load_hint() is exceeded, which may
		 * change entry order. Any open iterators
		 * still remain valid.
		 */
		iterator insert(const K& key)
		{			
			uint64 hash_code = hash(key);

			node_type *existing_node =
				hash_.find(key, hash_code);

			if (existing_node)
				return iterator(existing_node);

			node_type* new_node =
				allocator_util::allocate_and_construct<node_type>
					(hash_.allocator_, key);

			// Pass ownership over new node to hash_base,
			// which deletes new_node on any exception.
			hash_.insert(new_node, hash_code);

			return iterator(new_node);
		}


		/**
		 * Fetch a copy of the allocator
		 * currently in use by the hash_set.
		 */
		allocator_type allocator() const NOTHROW
			{ return hash_.allocator_; }


		/**
		 * Checked entry access.
		 * Throws if @bref{key} is not contained.
		 *
		 * @attent
		 * The returned @bref{K}ey instance is equal
		 * but usually not identical to @bref{key}.
		 */
		//@{
		const K& at(const K& key) const
		{
			const_iterator it(find(key));
			if (!it) throw invalid_parameter();
			return *it;
		}

		K& at(const K& key)
		{
			iterator it(find(key));
			if (!it) throw invalid_parameter();
			return *it;
		}
		//@}


		int64 size() const NOTHROW { return hash_.size_; }
		bool empty() const NOTHROW { return hash_.size_ == 0; }
		float load_factor() const NOTHROW { return hash_.load_factor(); }
		float max_load_hint() const NOTHROW { return hash_.max_load_hint_; }


		/**
		 * Return iterator to first or last entry,
		 * or past-end iterator for empty sets.
		 */
		//@{
		iterator first() NOTHROW
			{ return iterator(hash_.first()); }
		iterator last() NOTHROW
			{ return iterator(hash_.last()); }
		const_iterator first() const NOTHROW 
			{ return const_iterator(hash_.first()); }
		const_iterator last() const NOTHROW 
			{ return const_iterator(hash_.last()); }
		//@}


		/**
		 * Iterators for limited STL compatibility.
		 * @attent See @ref container_module for limitations.
		 */
		//@{
		iterator begin() NOTHROW { return first(); }
		iterator end() NOTHROW { return iterator(); }
		const_iterator begin() const NOTHROW { return first(); }
		const_iterator end() const NOTHROW { return iterator(); }
		//@}
		

		/**
		 * Return an iterator to the entry for given @bref{key},
		 * or a past-end iterator if the key is not contained
		 * in the set.
		 */
		//@{
		iterator find(const K& key) NOTHROW
			{ return iterator(hash_.find(key, hash(key))); }
		const_iterator find(const K& key) const NOTHROW
			{ return const_iterator(hash_.find(key, hash(key))); }
		//@}


		bool contains(const K& key) const NOTHROW
			{ return hash_.find(key, hash(key)) != NULL; }


		/**
		 * Remove entry corresponding to @bref{key}
		 * or entry at indicated @bref{it}erator.
		 * Any iterators or pointers to the removed
		 * entry become invalid.
		 *
		 * The call is ignored if @bref{it} is past-end
		 * or if the set does not contain @bref{key}. 
		 *
		 * @attent The iterator must belong to the set,
		 * and becomes invalid after the call.
		 *
		 * @attent The set is not rehashed after removal,
		 * thus entry order is maintained.
		 */
		//@{
		void remove(const K& key) NOTHROW
			{ hash_.remove(hash_.find(key, hash(key))); }
		void remove(const iterator& it) NOTHROW
			{ hash_.remove(it.node_); }
		//@}


		/**
		 * Named remove() variants avoid bloated casts
		 * on function selection (e.g. with scope guard).
		 */
		//@{
		void remove_by_key(const K& key) NOTHROW
			{ remove(key); }
		void remove_by_iterator(const iterator& it) NOTHROW
			{ remove(it); }
		//@}


		/**
		 * Set a new maximum load factor hint, and rehash
		 * (with tolerance) when exceeding the new factor.
		 * Rehashing has strong exception safety.
		 */
		void apply_max_load_hint(float max_load_hint)
			{ hash_.apply_max_load_hint(max_load_hint); }


		void clear() NOTHROW { hash_.clear(); }


		/**
		 * Validate hash set consistency for debugging purposes.
		 * Clients never have to call this method.
		 */
		void validate() const { hash_.validate(); }


	private:

		static node_type* next(node_type* node) NOTHROW
			{ return hash_base<node_type, allocator_type>::next(node); }
		static node_type* previous(node_type* node) NOTHROW
			{ return hash_base<node_type, allocator_type>::previous(node); }


		hash_base<node_type, allocator_type> hash_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__HASH_SET_HPP) */
