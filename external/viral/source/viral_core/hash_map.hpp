/**
 *************************************************************************
 *
 * @file hash_map.hpp
 *
 * Header for hash_map and helper templates.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__HASH_MAP_HPP)
#define INCLUDED__VIRAL_CORE__HASH_MAP_HPP


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
 * @class hash_map_pair
 * @ingroup container_module
 *
 * A key-value pair, as stored in a hash_map.
 *
 ************************************************************************/
template <typename K, typename V, typename A>
	class hash_map_pair
{
	friend class hash_map_node<K, V, A>;
	friend class hash_map<K, V, A>;


	private:

		hash_map_pair(const K& key, const V& value)
			: key(key), value(value) { }


	public:

		K key;
		V value;
};




/**
 *************************************************************************
 *
 * @class hash_map_node
 * @ingroup container_module
 *
 * An entry within a hash_map, holds a single hash_map_pair.
 *
 ************************************************************************/
template <typename K, typename V, typename A>
	class hash_map_node
{
	friend class hash_base<hash_map_node<K, V, A>, A>;
	friend class hash_map<K, V, A>;

	friend class iterator<hash_map<K, V, A>>;
	friend class const_iterator<hash_map<K, V, A>>;


	public:

		hash_map_node(const K& key, const V& value)
			:
			key_value_(key, value),
			previous_(NULL), next_(NULL),
			bucket_(NULL)
		{ }

		hash_map_node(const hash_map_node& other)
			:
			key_value_(other.key_value_),
			previous_(NULL), next_(NULL),
			bucket_(NULL)
		{ }


	private:

		bool operator ==(const K& other) const NOTHROW
			{ return key_value_.key == other; }
		bool operator ==(const hash_map_node& other) const NOTHROW
			{ return key_value_.key == other.key_value_.key; }

		uint64 hash_content() const NOTHROW
			{ return hash(key_value_.key); }

		hash_map_pair<K, V, A>& content_for_iterator() NOTHROW
			{ return key_value_; }


		hash_map_pair<K, V, A> key_value_;
		hash_map_node *previous_;
		hash_map_node *next_;
		hash_bucket<hash_map_node<K, V, A>, A> *bucket_;
};




/**
 *************************************************************************
 *
 * @class hash_map
 * @ingroup container_module
 *
 * Stores a mapping of unsorted, unique keys @bref{K}
 * to corresponding values @bref{V}, allocated with
 * an allocator of type @bref{A}.
 *
 * Notes:
 * - Single calls to the copy constructors of @bref{K} and of @bref{V}
 *		are used to insert keys and values into hash_map entries.
 *		No default constructors are required.
 * - The key type @bref{K} of a hash_map must support the global
 *		hash utility function viral_core::hash().
 * - hash_map::insert() requires an assignment operator on @bref{V}
 *		to replace an existing mapping for some key. This is no
 *		requirement for hash_map::try_insert().
 * - @attent Keys already stored within a hash map must not be modified
 *		in a way that changes hashes. Otherwise, behaviour is undefined.
 *
 ************************************************************************/
template <typename K, typename V, typename A>
	class hash_map
{
	friend class iterator<hash_map<K, V>>;
	friend class const_iterator<hash_map<K, V>>;


	public:

		typedef K key_type;
		typedef V value_type;

		typedef hash_map_pair<K, V, A> content_type;
		typedef hash_map_node<K, V, A> node_type;

		typedef A allocator_type;

		typedef typename viral_core::iterator<hash_map<K, V, A>> iterator;
		typedef typename viral_core::const_iterator<hash_map<K, V, A>> const_iterator;


		hash_map() NOTHROW { }


		explicit hash_map
			(const allocator_type& allocator) NOTHROW
				: hash_(allocator) { }


		/**
		 * Copy or move construct from @bref{other} hash_map.
		 * Move construction leaves a valid, empty source map.
		 */
		//@{		
		hash_map(const hash_map& other)
			: hash_(other.hash_) { }
		hash_map(hash_map&& other) NOTHROW
			: hash_(std::move(other.hash_)) { }
		//@}	


		/** Create copy of hash_map on heap. */
		auto_pointer<hash_map> clone() const
			{ return auto_pointer<hash_map>(new hash_map(*this)); }


		/**
		 * Copy or move assign from @bref{other} hash_map.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		hash_map& operator =(const hash_map& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			hash_map offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		hash_map& operator =(hash_map&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			hash_map offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(hash_map& other) NOTHROW
			{ swap(hash_, other.hash_); }


		/**
		 * Insert a @bref{key}-@bref{value} pair into the map.
		 * Using insert() replaces values on existing entries,
		 * while try_insert() keeps any existing key mapping.
		 * Both variants return an iterator to the map entry
		 * for the @bref{key}.
		 *
		 * On any exception, the map might already
		 * have been rehashed.
		 *
		 * @attent The entire map might be rehashed
		 * if max_load_hint() is exceeded, which may
		 * change entry order. Any open iterators
		 * still remain valid.
		 */
		//@{
		iterator insert(const K& key, const V& value)
		{			
			uint64 hash_code = hash(key);

			node_type *existing_node =
				hash_.find(key, hash_code);

			if (existing_node)
			{
				existing_node->key_value_.value = value;
				return iterator(existing_node);
			}

			node_type* new_node =
				allocator_util::allocate_and_construct<node_type>
					(hash_.allocator_, key, value);

			// Pass ownership over new node to hash_base,
			// which deletes new_node on any exception.
			hash_.insert(new_node, hash_code);

			return iterator(new_node);
		}
	
		iterator try_insert(const K& key, const V& value)
		{
			uint64 hash_code = hash(key);

			node_type *existing_node =
				hash_.find(key, hash_code);

			if (existing_node)
				return iterator(existing_node);

			node_type* new_node =
				allocator_util::allocate_and_construct<node_type>
					(hash_.allocator_, key, value);

			// Pass ownership over new node to hash_base,
			// which deletes new_node on any exception.
			hash_.insert(new_node, hash_code);

			return iterator(new_node);
		}
		//@}


		/**
		 * Fetch a copy of the allocator
		 * currently in use by the hash_map.
		 */
		allocator_type allocator() const NOTHROW
			{ return hash_.allocator_; }


		/**
		 * Checked entry access.
		 * Throws if @bref{key} is not contained.
		 */
		//@{
		const hash_map_pair<K, V, A>& at(const K& key) const
		{
			const_iterator it(find(key));
			if (!it) throw invalid_parameter();
			return *it;
		}

		hash_map_pair<K, V, A>& at(const K& key)
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
		 * or past-end iterator for empty maps.
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
		 * in the map.
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
		 * or if the map does not contain @bref{key}. 
		 *
		 * @attent The iterator must belong to the map,
		 * and becomes invalid after the call.
		 *
		 * @attent The map is not rehashed after removal,
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
		 * Validate hash map consistency for debugging purposes.
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


#endif /* !defined(INCLUDED__VIRAL_CORE__HASH_MAP_HPP) */
