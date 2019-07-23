/**
 *************************************************************************
 *
 * @file tree_set.hpp
 *
 * Tree set and supporting template classes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__TREE_SET_HPP)
#define INCLUDED__VIRAL_CORE__TREE_SET_HPP


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
 * @class tree_set_node
 * @ingroup container_module
 *
 * An entry within a tree_set, holds a single key.
 *
 * Implementation notes:
 * - A tree_set_node does neither own nor delete its children nodes,
 *		because nodes do not know the tree_set @bref{allocator_}.
 *
 ************************************************************************/
template <typename K, typename A>
	class tree_set_node
{
	friend class tree_base<tree_set_node<K, A>, A>;
	friend class tree_set<K, A>;

	friend class iterator<tree_set<K, A>>;
	friend class const_iterator<tree_set<K, A>>;


	public:

		tree_set_node(const K& key)
			:
			key_(key),
			less_(NULL), greater_(NULL),
			parent_(NULL), balance_(0)
		{ }

		tree_set_node(const tree_set_node& other)
			:
			key_(other.key_),
			less_(NULL), greater_(NULL),
			parent_(NULL), balance_(other.balance_)
		{ }


	private:

		bool operator <(const K& key) const NOTHROW
			{ return key_ < key; }
		bool operator >(const K& key) const NOTHROW
			{ return key < key_; }
		bool operator <(const tree_set_node& other) const NOTHROW
			{ return key_ < other.key_; }
		bool operator >(const tree_set_node& other) const NOTHROW
			{ return other.key_ < key_; }

		K& content_for_iterator() NOTHROW
			{ return key_; }


		K key_;
		tree_set_node *less_;
		tree_set_node *greater_;
		tree_set_node *parent_;
		char balance_;
};




/**
 *************************************************************************
 *
 * @class tree_set
 * @ingroup container_module
 *
 * Stores a sorted series of unique keys with a given type @bref{K},
 * allocated through an allocator of type @bref{A}.
 *
 * Notes:
 * - A single call to the copy constructor of @bref{K} is used to insert
 *		keys into tree_set entries. No default constructor is required.
 * - Key types @bref{K} must define a strict total order through
 *		bool operator <(const K& other) const NOTHROW:
 *		- @bref{a} less than @bref{b} xor @bref{b} less than @bref{a}
 *			xor @bref{a} equal to @bref{b} must always be true, and
 *		- @bref{a} less than @bref{b}, @bref{b} less than @bref{c}
 *			must imply @bref{a} less than @bref{c}.
 * - A tree_set can only store one of multiple equal keys.
 *		If required, clients must explicitly handle duplicates.
 * - @attent Clients must not change key order when modifying keys
 *		in a tree set. Otherwise, behaviour becomes undefined.
 *
 ************************************************************************/
template <typename K, typename A>
	class tree_set
{
	friend class iterator<tree_set<K, A>>;
	friend class const_iterator<tree_set<K, A>>;


	public:

		typedef K key_type;

		typedef K content_type;
		typedef tree_set_node<K, A> node_type;

		typedef A allocator_type;

		typedef typename viral_core::iterator<tree_set<K, A>> iterator;
		typedef typename viral_core::const_iterator<tree_set<K, A>> const_iterator;


		tree_set() NOTHROW { }


		explicit tree_set
			(const allocator_type& allocator) NOTHROW 
				: tree_(allocator) { }


		/**
		 * Copy or move construct from @bref{other} tree_set.
		 * Move construction leaves a valid, empty source set.
		 */
		//@{		
		tree_set(const tree_set& other)
			: tree_(other.tree_) { }
		tree_set(tree_set&& other) NOTHROW
			: tree_(std::move(other.tree_)) { }
		//@}


		/** Create copy of tree_set on heap. */
		auto_pointer<tree_set> clone() const
			{ return auto_pointer<tree_set>(new tree_set(*this)); }


		/**
		 * Copy or move assign from @bref{other} tree_set.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		tree_set& operator =(const tree_set& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			tree_set offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		tree_set& operator =(tree_set&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			tree_set offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(tree_set& other) NOTHROW
			{ swap(tree_, other.tree_); }


		/**
		 * Insert a @bref{key} into the set.
		 * If the set already contains the @bref{key},
		 * return an iterator to the existing entry.
		 * Otherwise, return an iterator to the new
		 * entry for @bref{key}.
		 *
		 * On any exception, the set remains unchanged.
		 */
		iterator insert(const K& key)
		{
			node_type **spot;
			node_type *parent;

			tree_.find_insertion_spot(key, spot, parent);

			if (!spot) return iterator(parent);

			node_type* new_node =
				allocator_util::allocate_and_construct<node_type>
					(tree_.allocator_, key);

			// Pass ownership over new node to tree_base,
			// which deletes new_node on any exception.
			tree_.insert(new_node, spot, parent);

			return iterator(new_node);
		}


		/**
		 * Fetch a copy of the allocator
		 * currently in use by the tree_set.
		 */
		allocator_type allocator() const NOTHROW
			{ return tree_.allocator_; }


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


		int64 size() const NOTHROW { return tree_.size_; }
		bool empty() const NOTHROW { return tree_.size_ == 0; }


		/**
		 * Return iterator to first or last entry,
		 * or past-end iterator for empty sets.
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
		 * or a past-end iterator if the set does not contain the key.
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
		 * or if the set does not contain @bref{key}. 
		 *
		 * @attent The iterator must belong to the set,
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


#endif /* !defined(INCLUDED__VIRAL_CORE__TREE_SET_HPP) */
