/**
 *************************************************************************
 *
 * @file tree_map.hpp
 *
 * Tree map and supporting template classes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__TREE_MAP_HPP)
#define INCLUDED__VIRAL_CORE__TREE_MAP_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "allocator_heap.hpp"
#include "allocator_util.hpp"
#include "auto_pointer.hpp"
#include "iterator.hpp"
#include "tree_base.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class tree_map_pair
 * @ingroup container_module
 *
 * A key-value pair as stored in a tree_map.
 *
 ************************************************************************/
template <typename K, typename V, typename A>
	class tree_map_pair
{
	friend class tree_map_node<K, V, A>;
	friend class tree_map<K, V, A>;


	public:

		tree_map_pair(const K& key, const V& value)
			: key(key), value(value) { }


		K key;
		V value;
};




/**
 *************************************************************************
 *
 * @class tree_map_node
 * @ingroup container_module
 *
 * An entry within a tree_map, holds a single tree_map_pair.
 *
 * Implementation notes:
 * - A tree_map_node does neither own nor delete its children nodes,
 *		because nodes do not know the tree_map @bref{allocator_}.
 *
 ************************************************************************/
template <typename K, typename V, typename A>
	class tree_map_node
{
	friend class tree_base<tree_map_node<K, V, A>, A>;
	friend class tree_map<K, V, A>;

	friend class iterator<tree_map<K, V, A>>;
	friend class const_iterator<tree_map<K, V, A>>;


	public:

		tree_map_node(const K& key, const V& value)
			:
			key_value_(key, value),
			less_(NULL), greater_(NULL),
			parent_(NULL), balance_(0)
		{ }

		tree_map_node(const tree_map_node& other)
			:
			key_value_(other.key_value_),
			less_(NULL), greater_(NULL),
			parent_(NULL), balance_(other.balance_)
		{ }


	private:

		bool operator <(const K& key) const NOTHROW
			{ return key_value_.key < key; }
		bool operator >(const K& key) const NOTHROW
			{ return key < key_value_.key; }
		bool operator <(const tree_map_node& other) const NOTHROW
			{ return key_value_.key < other.key_value_.key; }
		bool operator >(const tree_map_node& other) const NOTHROW
			{ return other.key_value_.key < key_value_.key; }

		tree_map_pair<K, V, A>& content_for_iterator() NOTHROW
			{ return key_value_; }


		tree_map_pair<K, V, A> key_value_;
		tree_map_node *less_;
		tree_map_node *greater_;
		tree_map_node *parent_;
		char balance_;
};




/**
 *************************************************************************
 *
 * @class tree_map
 * @ingroup container_module
 *
 * Stores a mapping of sorted, unique keys @bref{K}
 * to corresponding values @bref{V}, allocated with
 * an allocator of type @bref{A}.
 *
 * Notes:
 * - Single calls to copy constructors of @bref{K} and of @bref{V}
 *		are used to insert keys and values into tree_map entries.
 *		No default constructors are required.
 * - Key types @bref{K} must define a strict total order through
 *		bool operator <(const K& other) const NOTHROW:
 *		- @bref{a} less than @bref{b} xor @bref{b} less than @bref{a}
 *			xor @bref{a} equal to @bref{b} must always be true, and
 *		- @bref{a} less than @bref{b}, @bref{b} less than @bref{c}
 *			must imply @bref{a} less than @bref{c}.
 * - A tree_map can only store one of multiple equal keys.
 *		If required, clients must explicitly handle duplicates.
 * - tree_map::insert() requires an assignment operator on @bref{V}
 *		to replace an existing mapping for some key. This is no
 *		requirement for tree_map::try_insert().
 * - @attent Clients must not change key order when modifying keys
 *		in a tree map. Otherwise, behaviour becomes undefined.
 *
 ************************************************************************/
template <typename K, typename V, typename A>
	class tree_map
{
	friend class iterator<tree_map<K, V, A>>;
	friend class const_iterator<tree_map<K, V, A>>;


	public:

		typedef K key_type;
		typedef V value_type;

		typedef tree_map_pair<K, V, A> content_type;
		typedef tree_map_node<K, V, A> node_type;

		typedef A allocator_type;

		typedef typename viral_core::iterator<tree_map<K, V, A>> iterator;
		typedef typename viral_core::const_iterator<tree_map<K, V, A>> const_iterator;


		tree_map() NOTHROW { }


		explicit tree_map
			(const allocator_type& allocator) NOTHROW 
				: tree_(allocator) { }


		/**
		 * Copy or move construct from @bref{other} tree_map.
		 * Move construction leaves a valid, empty source map.
		 */
		//@{		
		tree_map(const tree_map& other)
			: tree_(other.tree_) { }
		tree_map(tree_map&& other) NOTHROW
			: tree_(std::move(other.tree_)) { }
		//@}	


		/** Create copy of tree_map on heap. */
		auto_pointer<tree_map> clone() const
			{ return auto_pointer<tree_map>(new tree_map(*this)); }


		/**
		 * Copy or move assign from @bref{other} tree_map.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		tree_map& operator =(const tree_map& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			tree_map offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		tree_map& operator =(tree_map&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			tree_map offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(tree_map& other) NOTHROW
			{ swap(tree_, other.tree_); }


		/**
		 * Insert a @bref{key}-@bref{value} pair into the map.
		 * Using insert() replaces values on existing entries,
		 * while try_insert() keeps any existing key mapping.
		 * Both variants return an iterator to the map entry
		 * for the @bref{key}.
		 *
		 * On any exception, the map remains unchanged.
		 */
		//@{
		iterator insert(const K& key, const V& value)
		{
			node_type **spot;
			node_type *parent;

			tree_.find_insertion_spot(key, spot, parent);

			if (!spot)
			{
				parent->key_value_.value = value;
				return iterator(parent);
			}

			node_type* new_node =
				allocator_util::allocate_and_construct<node_type>
					(tree_.allocator_, key, value);

			// Pass ownership over new node to tree_base,
			// which deletes new_node on any exception.
			tree_.insert(new_node, spot, parent);

			return iterator(new_node);
		}

		iterator try_insert(const K& key, const V& value)
		{
			node_type **spot;
			node_type *parent;

			tree_.find_insertion_spot(key, spot, parent);
			if (!spot) return iterator(parent);

			node_type* new_node =
				allocator_util::allocate_and_construct<node_type>
					(tree_.allocator_, key, value);

			// Pass ownership over new node to tree_base,
			// which deletes new_node on any exception.
			tree_.insert(new_node, spot, parent);

			return iterator(new_node);
		}	
		//@}


		/**
		 * Fetch a copy of the allocator
		 * currently in use by the tree_map.
		 */
		allocator_type allocator() const NOTHROW
			{ return tree_.allocator_; }


		/**
		 * Checked entry access.
		 * Throws if @bref{key} is not contained.
		 */
		//@{
		const tree_map_pair<K, V, A>& at(const K& key) const
		{
			const_iterator it(find(key));
			if (!it) throw invalid_parameter();
			return *it;
		}

		tree_map_pair<K, V, A>& at(const K& key)
		{
			iterator it(find(key));
			if (!it) throw invalid_parameter();
			return *it;
		}
		//@}


		int64 size() const NOTHROW { return tree_.size_; }
		bool empty() const NOTHROW { return tree_.size_ == 0; }


		/**
		 * Return iterator to first or last entry,
		 * or past-end iterator for empty maps.
		 */
		//@{
		iterator first() NOTHROW
			{ return iterator(tree_.find_smallest()); }
		iterator last() NOTHROW
			{ return iterator(tree_.find_largest()); }
		const_iterator first() const NOTHROW 
			{ return const_iterator(tree_.find_smallest()); }
		const_iterator last() const NOTHROW 
			{ return const_iterator(tree_.find_largest()); }
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
		 * or a past-end iterator if the map does not contain the key.
		 */
		//@{
		iterator find(const K& key) NOTHROW
			{ return iterator(tree_.find(key)); }
		const_iterator find(const K& key) const NOTHROW
			{ return const_iterator(tree_.find(key)); }
		//@}


		bool contains(const K& key) const NOTHROW
			{ return tree_.find(key) != NULL; }


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
		 */
		//@{
		void remove(const K& key) NOTHROW
			{ tree_.remove(find(key).node_); }
		void remove(const iterator& it) NOTHROW
			{ tree_.remove(it.node_); }
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


		void clear() NOTHROW { tree_.clear(); }


		/**
		 * Validate tree structure for testing purposes.
		 * Clients never have to call this method.
		 */
		void validate() const { tree_.validate(); }


	private:

		static node_type* next(node_type* node) NOTHROW
			{ return tree_base<node_type, allocator_type>::next(node); }
		static node_type* previous(node_type* node) NOTHROW
			{ return tree_base<node_type, allocator_type>::previous(node); }


		tree_base<node_type, allocator_type> tree_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__TREE_MAP_HPP) */
