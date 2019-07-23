/**
 *************************************************************************
 *
 * @file heap.hpp
 *
 * Heap data structure.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__HEAP_HPP)
#define INCLUDED__VIRAL_CORE__HEAP_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "array.hpp"
#include "buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class heap
 * @ingroup container_module
 *
 * A heap supports efficient access to the smallest of multiple keys.
 *
 * Usage notes:
 * - A single call to the copy constructor of @bref{K} is used to insert
 *		keys into heap entries. No default constructor is required.
 * - Key types @bref{K} must define a strict total order through
 *		bool operator <(const K& other) const NOTHROW:
 *		- @bref{a} less than @bref{b} xor @bref{b} less than @bref{a}
 *			xor @bref{a} equal to @bref{b} must always be true, and
 *		- @bref{a} less than @bref{b}, @bref{b} less than @bref{c}
 *			must imply @bref{a} less than @bref{c}.
 * - To achieve strong exception safety, the heap sorts keys by swapping.
 *		Key types @bref{K} must implement a @ref NOTHROW swap_with() method
 *		that is compatible with the @ref swap() utility.
 * - Unlike tree_set and tree_map, a heap can store multiple equal keys.
 *		However, the heap sorts equal keys in arbitrary order. 
 * - @attent Clients must not change key order when modifying keys
 *		in a heap. Otherwise, behaviour becomes undefined.
 *
 * Implementation notes:
 * - The fundamental @ref array checks maximum size and most overflows.
 *
 ************************************************************************/
template <typename K>
	class heap
{

	public:

		/** Create empty() heap. */
		heap() NOTHROW { }


		/**
		 * Copy or move construct from an@bref{other} heap.
		 * Move construction leaves a valid, empty source heap.
		 */
		//@{		
		heap(const heap& other)
			: array_(other.array_) { }
		heap(heap&& other) NOTHROW
			: array_(std::move(other.array_)) { }
		//@}


		/** Sort given @bref{data} into a new heap. */
		//@{
		heap(const K* data, int64 count)
			: array_(data, count) { build_heap(); }
		heap(const dynamic_buffer<K>& data)
			: array_(data.data(), data.size()) { build_heap(); }
		heap(const array<K>& data)
			: array_(data) { build_heap(); }
		//@}


		/**
		 * Copy or move assign from @bref{other} heap.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		heap& operator =(const heap& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			heap offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		heap& operator =(heap&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			heap offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		/** Reserve memory for entries, see array::reserve(). */
		void reserve(int64 new_capacity)
			{ array_.reserve(new_capacity); }


		/** Sort new entry into heap. */
		void insert(const K& key)
		{
			array_.push_back(key);
			pull_up(array_.size() - 1);
		}


		/**
		 * Remove minimum from heap.
		 * Ignored on empty() heaps.
		 */
		void pop_minimum() NOTHROW
		{
			// Not much to do on empty or one-entry heaps.
			// We avoid self-swaps for heaps with one entry.
			if (size() <= 1)
			{
				clear();
				return;
			}

			// Swap first entry to back and remove there,
			// then restore heap by pushing root element.
			swap(array_[0], array_[array_.size() - 1]);
			array_.pop_back();
			push_down(0);
		}


		/**
		 * Fetch minimum within non-empty() heap.
		 * Undefined behaviour (probably crash) on empty() heaps.
		 */
		//@{
		const K& minimum() const NOTHROW { return array_[0]; }
		K& minimum() NOTHROW { return array_[0]; }
		//@}


		/**
		 * Pointer to internal data, undefined if empty(),
		 * and invalidated on each insert().
		 */
		//@{
		const K* data() const NOTHROW { return array_.data(); }
		K* data() NOTHROW { return array_.data(); }
		//@}


		/**
		 * Checked and unchecked entry access.
		 * Entry references are invalidated on insert().
		 */
		//@{
		const K& at(int64 index) const { return array_.at(index); }
		K& at(int64 index) { return array_.at(index); }
		const K& operator[](int64 index) const NOTHROW { return array_[index]; }
		K& operator[](int64 index) NOTHROW { return array_[index]; }	
		//@}


		/** Index of root node, or invalid_index for empty heaps. */
		int64 root() const NOTHROW
			{ return array_.empty() ? invalid_index : 0; }


		/**
		 * Find parent or children for node at @bref{index}.
		 * Returns invalid_index if @bref{index} is invalid,
		 * or if there is no such child or parent.
		 */
		//@{
		int64 parent(int64 index) const NOTHROW
		{
			// Implementation notes:
			// - We specifically consider root nodes.
			// - Rounding down makes the formula for left children 
			//		work for right children as well.
			return (index <= 0 || index >= size()) ?
					invalid_index : ((index - 1) / 2);
		}

		int64 left(int64 index) const NOTHROW
		{
			// Rationale:
			// Let index = sum(2^depth) + offset, then with 2^n sums:
			// left_child = sum(2^depth) + 2^(depth + 1) + 2 offset
			//			  = sum(2^depth) + offset + 2^(depth + 1) + offset
			//			  = index + 2^(depth + 1) + offset + 1 - 1
			//			  = index + sum(2 ^ depth) + offset + 1
			//			  = 2 index + 1.
			if (index < 0 || index > max_parent_index_with_left_child_)
				return invalid_index;

			int64 left_child = 2 * index + 1;
			if (left_child >= size()) return invalid_index;

			return left_child;
		}

		int64 right(int64 index) const NOTHROW
		{
			if (index < 0 || index > max_parent_index_with_right_child_)
				return invalid_index;

			// Use next place after left_child, see left().
			int64 right_child = 2 * index + 2;
			if (right_child >= size()) return invalid_index;

			return right_child;
		}
		//@}


		int64 size() const NOTHROW { return array_.size(); }
		bool empty() const NOTHROW { return array_.empty(); }
		int64 capacity() const NOTHROW { return array_.capacity(); }


		void swap_with(heap& other) NOTHROW
			{ swap(array_, other.array_); }


		/** Destroy contents but retain memory. */
		void clear() NOTHROW { array_.clear(); }

		/** Destroy contents and release memory. */
		void free() NOTHROW { array_.free(); }


		/** Tag for non-existing nodes. */
		static const int64 invalid_index = -1;


	private:

		void build_heap() NOTHROW
		{
			/**
			 * Assume an unsorted array and restore heap sorting.
			 * We start from the leaves and push_down() each node
			 * to achieve O(n) instead of O(n log n) complexity.
			 */

			for (int64 i = array_.size() - 1; i >= 0; --i)
				push_down(i);
		}


		void pull_up(int64 index) NOTHROW
		{
			/**
			 * Pull leaf node at @bref{index} up through heap
			 * to restore heap conditions in a single branch.
			 * Does not consider any children of @bref{index}.
			 */
			
			while (true)
			{
				// Abort at root, on invalid nodes,
				// or if heap sorting is still OK.
				// Avoid swaps on equal values.
				int64 p = parent(index);
				if (p == invalid_index) return;
				if (!(array_[index] < array_[p])) return;


				// Correct heap condition on parent node,
				// then iterate on next higher heap level.
				swap(array_[p], array_[index]);
				index = p;
			}
		}


		void push_down(int64 index) NOTHROW
		{
			/**
			 * Push node at @bref{index} down through heap
			 * to restore heap conditions in a single branch.
			 * Does not consider any parents of @bref{index}.
			 */

			while (true)
			{
				int64 l = left(index);
				int64 r = right(index);


				// Abort if we have arrived at a leaf.
				// Also works as failsafe if index is invalid.
				if (l == invalid_index &&
					r == invalid_index)
						return;


				// Early out if subtree heap sorting is OK already.
				// Otherwise, swap with smaller child and continue.
				// The specific comparison order avoids swaps
				// with children of equal value.			
				int64 smaller_child =
					(l == invalid_index) ? r :
					(r == invalid_index) ? l :
					(array_[r] < array_[l]) ? r : l;

				if (!(array_[smaller_child] < array_[index]))
					return;

				swap(array_[index], array_[smaller_child]);
				index = smaller_child;
			}
		}


		/**
		 * Maximum indices of parent nodes so that their children
		 * can have valid indices (i.e. < max_size, no overflow).
		 *
		 * Rationale:
		 * - Left child:
		 *		parent_index <= truncate((max_size - 1 + 1) / 2) - 1
		 *		iff parent_index < truncate((max_size - 1 + 1) / 2)
		 *		iff parent_index < (max_size - 1) / 2
		 *		iff left_child = 2 * parent_index + 1 < max_size.
		 * - Right child:
		 *		parent_index <= truncate((max_size - 2 + 1) / 2) - 1
		 *		iff parent_index < truncate((max_size - 2 + 1) / 2)
		 *		iff parent_index < (max_size - 2) / 2
		 *		iff right_child = 2 * parent_index + 2 < max_size.
		 * - Proof: Adding +1 before truncate(x) does not change
		 *		the result on even x and rounds uneven x to ceil.
		 *		Comparison then matches with using untruncated x.
		 */
		//@{
		static const int64 max_parent_index_with_left_child_ =
			container_limits<K>::max_size / 2 - 1;
		static const int64 max_parent_index_with_right_child_ =
			(container_limits<K>::max_size - 1) / 2 - 1;
		//@}


		array<K> array_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__HEAP_HPP) */
