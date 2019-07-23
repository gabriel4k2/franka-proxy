/**
 *************************************************************************
 *
 * @file tree_base.hpp
 *
 * Base class for a self-balancing AVL tree.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__TREE_BASE_HPP)
#define INCLUDED__VIRAL_CORE__TREE_BASE_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "integer_checks.hpp"
#include "scope_guard.hpp"
#include "allocator_util.hpp"
#include "container_limits.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class tree_base
 * @ingroup container_module
 *
 * Models an AVL tree for nodes of type @bref{N}
 * and is used internally by tree_set and tree_map.
 *
 * Implementation notes:
 * - tree_base does not directly know of the content stored in @bref{N}.
 *		Instead, all key comparisons accept a general key type @bref{K}
 *		and are delegated to the container-specific node type @bref{N}.
 * - There is no explicit limit to depth (e.g. to avoid stack overflow)
 *		for tree_base, as AVL rebalancing and container_limits::max_size
 *		ensure a limited tree depth already.
 *
 ************************************************************************/
template <typename N, typename A>
	class tree_base
{
	template <typename K, typename A>
		friend class tree_set;
	template <typename K, typename V, typename A>
		friend class tree_map;


	public:

		~tree_base() NOTHROW { clear(); }


		void swap_with(tree_base& other) NOTHROW
		{
			swap(allocator_, other.allocator_);
			swap(root_node_, other.root_node_);
			swap(size_, other.size_);
		}


	private:

		typedef N node_type;
		typedef A allocator_type;


		tree_base() NOTHROW
			:
			root_node_(NULL), size_(0)
		{ }


		explicit tree_base(const allocator_type& allocator) NOTHROW
			:
			allocator_(allocator),
			root_node_(NULL), size_(0)
		{ }


		/**
		 * Copy or move construct from @bref{other} tree.
		 * Move construction leaves a valid, empty() source.
		 */
		//@{		
		tree_base(const tree_base& other)
			:
			allocator_(other.allocator_),
			root_node_(NULL), size_(0)
		{
			root_node_ = copy_recursive(other.root_node_);
			size_ = other.size_;
		}

		tree_base(tree_base&& other) NOTHROW
			:
			allocator_(other.allocator_),
			root_node_(NULL), size_(0) 
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		/**
		 * Copy or move assign of @bref{other} tree.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty() source.
		 */
		//@{
		tree_base& operator =(const tree_base& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			tree_base offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		tree_base& operator =(tree_base&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			tree_base offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}

	
		node_type* copy_recursive(node_type* subtree)
		{
			/**
			 * Create deep copy of @bref{subtree}
			 * using the current @bref{allocator_}.
			 *
			 * As the subtree is part of an AVL tree_base,
			 * there is no risk of stack overflow here.
			 */

			if (!subtree) return NULL;


			// Create recursive copy
			// of the entire subtree.
			node_type *new_node =
				allocator_util::allocate_and_construct<node_type>
					(allocator_, *subtree);
			SCOPE_GUARD
				(new_node_guard,
					(*this,
					 &tree_base::destroy_and_free_recursive,
					 new_node));

			node_type *greater =
				copy_recursive(subtree->greater_);
			SCOPE_GUARD
				(greater_guard,
					(*this,
					 &tree_base::destroy_and_free_recursive,
					 greater));

			node_type *less =
				copy_recursive(subtree->less_);
			SCOPE_GUARD
				(less_guard,
					(*this,
					 &tree_base::destroy_and_free_recursive,
					 less));


			// Succeeded with copying the subtree,
			// so fix up parent and child pointers.
			// Exception safety: NOTHROW from here.
			new_node_guard.dismiss();
			greater_guard.dismiss();
			less_guard.dismiss();

			new_node->greater_ = greater;
			new_node->less_ = less;

			if (greater) greater->parent_ = new_node;
			if (less) less->parent_ = new_node;

			return new_node;
		}


		void destroy_and_free_recursive
			(node_type* subtree) NOTHROW
		{
			/**
			 * Recursively destroy an entire @bref{subtree}
			 * allocated through the @bref{allocator_}.
			 *
			 * As the subtree is part of an AVL tree_base,
			 * there is no risk of stack overflow here.
			 */

			if (!subtree) return;

			destroy_and_free_recursive(subtree->less_);
			destroy_and_free_recursive(subtree->greater_);

			allocator_util::destroy_and_free
				(allocator_, subtree);
		}


		template <typename K>
			node_type* find(const K& key) const NOTHROW
		{
			/** Iteratively find node for given key in tree. */

			if (!root_node_) return NULL;

			node_type *find_node = root_node_;

			while (true)
			{
				if (*find_node < key)
				{
					if (!find_node->greater_) return NULL;
					find_node = find_node->greater_;
				}
				else if (*find_node > key)
				{
					if (!find_node->less_) return NULL;
					find_node = find_node->less_;
				}
				else
				{
					return find_node;
				}
			}
		}


		static node_type* previous
			(node_type* node) NOTHROW
		{
			/**
			 * Find next smaller node in entire tree,
			 * or return @NULL if input is @NULL or
			 * the smallest node.
			 */

			if (!node) return NULL;

			// Either pick largest in smaller subtree,
			// or next-smaller parent for leaf nodes
			// without a smaller subtree.
			if (node->less_)
				return find_largest_below(node->less_);

			while (node->parent_)
			{
				if (node->parent_->greater_ == node)
					return node->parent_;

				node = node->parent_;
			}

			return NULL;
		}


		static node_type* next
			(node_type* node) NOTHROW
		{
			/**
			 * Find next greater node in entire tree,
			 * or return @NULL if input is @NULL or
			 * the greatest node.
			 */

			if (!node) return NULL;

			// Either pick smallest in larger subtree,
			// or next-larger parent for leaf nodes
			// without a larger subtree.
			if (node->greater_)
				return find_smallest_below(node->greater_);

			while (node->parent_)
			{
				if (node->parent_->less_ == node)
					return node->parent_;

				node = node->parent_;
			}			

			return NULL;
		}


		static node_type* find_smallest_below
			(node_type* node) NOTHROW
		{	
			/**
			 * Find smallest node in subtree below @bref{node},
			 * or @NULL if input is @NULL already.
			 */

			if (!node) return NULL;		
			while (node->less_) node = node->less_;
			return node;
		}


		static node_type* find_largest_below
			(node_type* node) NOTHROW
		{
			/**
			 * Find largest node in subtree below @bref{node},
			 * or @NULL if input is @NULL already.
			 */

			if (!node) return NULL;
			while (node->greater_) node = node->greater_;
			return node;
		}


		node_type* find_smallest() const NOTHROW
			{ return find_smallest_below(root_node_); }
		node_type* find_largest() const NOTHROW
			{ return find_largest_below(root_node_); }


		node_type* rotate_left(node_type* node) NOTHROW
		{
			/**
			 * Requirement:
			 *  node->balance_ == +2, node->greater_->balance_ != -1
			 *
			 * Result:
			 *  level(node->less_) += 1, level(node->greater_->greater_) -= 1
			 *
			 * Does:
			 * @verbatim
			 *        N               S    
			 *    a/     \S   =>   N/   \c
			 *         b/  \c    a/ \b
			 * @endverbatim
			 */

			node_type* swap_node = node->greater_;
			
			swap_node->parent_ = node->parent_;
			if (!swap_node->parent_)
				root_node_ = swap_node;
			else if (swap_node->parent_->less_ == node)
				swap_node->parent_->less_ = swap_node;
			else
				swap_node->parent_->greater_ = swap_node;

			node->greater_ = swap_node->less_;
			if (node->greater_)
				node->greater_->parent_ = node;

			swap_node->less_ = node;
			node->parent_ = swap_node;

			if (swap_node->balance_ == 0)
			{
				node->balance_ = +1;
				swap_node->balance_ = -1;
			}
			else // swap_node->balance_ == +1
			{
				node->balance_ = 0;
				swap_node->balance_ = 0;
			}
			
			return swap_node;
		}
	
		node_type* rotate_right(node_type* node) NOTHROW
		{
			/**
			 * Requirement:
			 *  node->balance_ == -2, node->less_->balance_ != +1
			 *
			 * Result:
			 *  level(node->greater_) += 1, level(node->less_->less_) -= 1
			 *
			 * Does:
			 * @verbatim
			 *        N               S    
			 *    S/     \c   =>  a/     \N
			 *  a/ \b                  b/  \c
			 * @endverbatim
			 */

			node_type* swap_node = node->less_;

			swap_node->parent_ = node->parent_;
			if (!swap_node->parent_)
				root_node_ = swap_node;
			else if (swap_node->parent_->less_ == node)
				swap_node->parent_->less_ = swap_node;
			else
				swap_node->parent_->greater_ = swap_node;

			node->less_ = swap_node->greater_;
			if (node->less_)
				node->less_->parent_ = node;

			swap_node->greater_ = node;
			node->parent_ = swap_node;

			if (swap_node->balance_ == 0)
			{
				node->balance_ = -1;
				swap_node->balance_ = +1;
			}
			else // swap_node->balance_ == -1
			{
				node->balance_ = 0;
				swap_node->balance_ = 0;
			}

			return swap_node;
		}

		node_type* rotate_right_left(node_type* node) NOTHROW
		{
			/**
			 * Requirement:
			 *  node->balance_ == +2, node->greater_->balance_ == -1
			 *
			 * Result:
			 *  level(node->less_) += 1, level(node->greater_->less_) -= 1
			 *
			 * Does:
			 * @verbatim
			 *          N                  S    
			 *    a/        \I   =>   N/       \I
			 *            S/  \d    a/  \b   c/  \d
			 *          b/ \c
			 * @endverbatim
			 */

			node_type* intermediate_node = node->greater_;
			node_type* swap_node = intermediate_node->less_;

			swap_node->parent_ = node->parent_;
			if (!swap_node->parent_)
				root_node_ = swap_node;
			else if (swap_node->parent_->less_ == node)
				swap_node->parent_->less_ = swap_node;
			else
				swap_node->parent_->greater_ = swap_node;

			node->greater_ = swap_node->less_;
			if (node->greater_)
				node->greater_->parent_ = node;
			intermediate_node->less_ = swap_node->greater_;
			if (intermediate_node->less_)
				intermediate_node->less_->parent_ = intermediate_node;

			swap_node->less_ = node;
			swap_node->greater_ = intermediate_node;
			node->parent_ = swap_node;
			intermediate_node->parent_ = swap_node;

			if (swap_node->balance_ == -1)
			{
				node->balance_ = 0;
				intermediate_node->balance_ = +1;
			}
			else if (swap_node->balance_ == +1)
			{
				node->balance_ = -1;
				intermediate_node->balance_ = 0;
			}
			else // swap_node->balance_ == 0
			{
				node->balance_ = 0;
				intermediate_node->balance_ = 0;
			}

			swap_node->balance_ = 0;

			return swap_node;
		}

		node_type* rotate_left_right(node_type* node) NOTHROW
		{
			/**
			 * Requirement:
			 *  node->balance_ == -2, node->less_->balance_ == +1
			 *
			 * Result:
			 *  level(node->greater_) += 1, level(node->less_->greater_) -= 1
			 *
			 * Does:
			 * @verbatim
			 *             N                  S
			 *      I/         \d  =>    I/        \N
			 *  a/     \S              a/  \b    c/  \d
			 *       b/  \c  
			 * @endverbatim
			 */

			node_type* intermediate_node = node->less_;
			node_type* swap_node = intermediate_node->greater_;

			swap_node->parent_ = node->parent_;
			if (!swap_node->parent_)
				root_node_ = swap_node;
			else if (swap_node->parent_->less_ == node)
				swap_node->parent_->less_ = swap_node;
			else
				swap_node->parent_->greater_ = swap_node;

			node->less_ = swap_node->greater_;
			if (node->less_)
				node->less_->parent_ = node;
			intermediate_node->greater_ = swap_node->less_;
			if (intermediate_node->greater_)
				intermediate_node->greater_->parent_ = intermediate_node;

			swap_node->less_ = intermediate_node;
			swap_node->greater_ = node;
			node->parent_ = swap_node;
			intermediate_node->parent_ = swap_node;

			if (swap_node->balance_ == -1)
			{
				node->balance_ = +1;
				intermediate_node->balance_ = 0;
			}
			else if (swap_node->balance_ == +1)
			{
				node->balance_ = 0;
				intermediate_node->balance_ = -1;
			}
			else // swap_node->balance_ == 0
			{
				node->balance_ = 0;
				intermediate_node->balance_ = 0;
			}

			swap_node->balance_ = 0;

			return swap_node;
		}


		template <typename K>
			void find_insertion_spot
				(const K& key,
				 node_type**& spot,
				 node_type*& parent) NOTHROW
		{
			/**
			 * Find spot within tree where key can be attached:
			 * - Empty tree => parent == NULL, spot == &root_node_.
			 * - Key in tree already => spot == NULL, parent == existing node.
			 * - Key not in (non-empty) tree => parent == parent node,
			 *		spot == &parent_->less_ or &parent_->greater_.
			 */

			if (!root_node_)
			{
				spot = &root_node_;
				parent = NULL;
				return;
			}
			
			parent = root_node_;
			
			while (true)
			{		
				if (*parent < key)
				{
					if (parent->greater_)
					{
						parent = parent->greater_;
						continue;
					}

					spot = &parent->greater_;
					return;
				}
				else if (*parent > key)
				{
					if (parent->less_)
					{
						parent = parent->less_;
						continue;
					}

					spot = &parent->less_;
					return;
				}
				else
				{
					spot = NULL;
					return;
				}
			}	
		}


		void rebalance_insert
			(node_type* node, node_type* subtree) NOTHROW
		{
			/**
			 * Iteratively rebalance, starting at the @bref{node}
			 * whose provided @bref{subtree} increased in height.
			 */

			while (node)
			{
				if (subtree == node->greater_) ++node->balance_;
					else --node->balance_;


				// Either subtree grew to balance out with the other.
				// Node total height didn't change, so abort.
				if (node->balance_ == 0) return;


				// AVL condition not satisfied: Rotate to fix.
				// Rotations on an insertion always result in
				// a pre-insertion subtree height, so we need
				// not continue balance updates.
				if (node->balance_ == -2)
				{
					if (node->less_->balance_ == +1)
						rotate_left_right(node);
					else					
						rotate_right(node);
					return;
				}
				else if (node->balance_ == +2)
				{
					if (node->greater_->balance_ == -1)
						rotate_right_left(node);
					else
						rotate_left(node);
					return;
				}

				
				// Either subtree outgrew the other with a tolerable
				// imbalance +1 or -1, thus the entire node tree grew,
				// and we need to continue updates on the parent.
				subtree = node;
				node = node->parent_;
			}			
		}


		void insert
			(node_type* new_node,
			 node_type** spot,
			 node_type* parent)
		{
			/**
			 * Insert not-@NULL @bref{new_node} at a previously found,
			 * not-@NULL insertion @bref{spot} in a @bref{parent} node.
			 * Take ownership over (and possibly delete) @bref{new_node},
			 * in particular in the case of any exception.
			 *
			 * Requirements:
			 * - The tree must not yet contain the key of the node
			 *		(i.e. a not-@NULL spot from find_insertion_spot()).
			 * - The @bref{new_node} must have been allocated with
			 *		the tree_base @bref{allocator_} and must not
			 *		have any children.
			 */


			SCOPE_GUARD
				(new_node_guard,
					(*this,
					 &tree_base::destroy_and_free_recursive,
					 new_node));


			// We cannot insert entire subtrees.
			if (new_node->less_ || new_node->greater_)
				throw assert_failed();


			// Shared container size check
			// for all tree variants.
			if (!integer_checks::is_add_safe(size_, 1) ||
				size_ + 1 > container_limits<node_type>::max_size)
					throw bounds_exceeded();


			// Attach node and rebalance AVL tree.
			// Exception safety: NOTHROW from here.	
			new_node_guard.dismiss();

			*spot = new_node;
			new_node->parent_ = parent;

			++size_;

			rebalance_insert(parent, new_node);
		}
	

		void rebalance_remove
			(node_type* node, bool greater) NOTHROW
		{
			/**
			 * Rebalance an AVL tree because the @bref{greater}
			 * or smaller subtree of a not-@NULL @bref{node}
			 * decreased in height.
			 */

			while (true)
			{
				if (greater) --node->balance_;
					else ++node->balance_;


				// Either of our subtrees shrunk below the other
				// without creating an AVL violation. Therefore,
				// the complete node tree height didn't change,
				// and we can abort.
				if (node->balance_ == -1 || node->balance_ == +1) return;


				// AVL violation: Rotate to fix.
				// Unlike insertions, just certain one-way rotations
				// keep the original tree height and enable early-out.
				// Respective rotations are recognized by the balances
				// resulting from rotation. All other rotations still
				// reduce subtree height and require further processing
				// on parent nodes.
				if (node->balance_ == -2)
				{
					if (node->less_->balance_ == +1)
					{
						node = rotate_left_right(node);
					}
					else		
					{			
						node = rotate_right(node);
						if (node->balance_ == +1) return;
					}
				}
				else if (node->balance_ == +2)
				{
					if (node->greater_->balance_ == -1)
					{
						node = rotate_right_left(node);
					}
					else
					{
						node = rotate_left(node);
						if (node->balance_ == -1) return;
					}
				}
			

				// Our entire node tree shrunk, either because a subtree
				// shrunk back to balance == 0, or because of a rotation.
				// Thus, continue on parent node.
				if (!node->parent_) return;
				greater = (node->parent_->greater_ == node);
				node = node->parent_;
			}
		}

		
		void remove
			(node_type *node) NOTHROW
		{
			if (!node) return;


			// We can't efficiently delete a complete inner node,
			// thus we exchange it with its successor before deletion.
			// As the deletion node is complete inner, this successor
			// always exists and has no further left-hand child node.
			// Finally, we need a special case if we switch with the
			// direct child of the deletion node.	
			if (node->less_ && node->greater_)
			{
				node_type *successor =
					find_smallest_below(node->greater_);

				successor->less_ = node->less_;
				successor->less_->parent_ = successor;
				node->less_ = NULL;			

				swap(successor->balance_, node->balance_);	

				if (successor == node->greater_)
				{
					successor->parent_ = node->parent_;
					if (!successor->parent_)
						root_node_ = successor;
					else if (successor->parent_->less_ == node)
						successor->parent_->less_ = successor;
					else
						successor->parent_->greater_ = successor;

					node->greater_ = successor->greater_;
					if (node->greater_)
						node->greater_->parent_ = node;

					successor->greater_ = node;
					node->parent_ = successor;					
				}
				else
				{
					swap(successor->parent_, node->parent_);
					swap(successor->greater_, node->greater_);

					if (!successor->parent_)
						root_node_ = successor;
					else if (successor->parent_->less_ == node)
						successor->parent_->less_ = successor;
					else
						successor->parent_->greater_ = successor;

					if (node->parent_->less_ == successor)
						node->parent_->less_ = node;
					else
						node->parent_->greater_ = node;

					successor->greater_->parent_ = successor;
					if (node->greater_)
						node->greater_->parent_ = node;
				}		
			}


			// Disconnect node:
			// Either disconnect leaf from its parent, or replace
			// non-leaf with its single child. We also must handle
			// a non-existent parent on root nodes.
			node_type *parent = node->parent_;
			node_type **spot;
			
			if (!parent)
				spot = &root_node_;
			else if (parent->less_ == node)
				spot = &parent->less_;
			else
				spot = &parent->greater_;

			if (node->less_) *spot = node->less_;
				else *spot = node->greater_;

			if (*spot)
				(*spot)->parent_ = parent;

			node->less_ = NULL;
			node->greater_ = NULL;


			// Iteratively rebalance our tree,
			// unless we deleted the last node.
			if (parent)
				rebalance_remove(parent, spot == &parent->greater_);


			// Delete detached node.
			// No recursion required.
			allocator_util::destroy_and_free
				(allocator_, node);

			--size_;
		}


		int64 tree_height(node_type* node) const NOTHROW
		{
			if (!node) return 0;

			// No overflow due to container_limits::max_size
			// and AVL rebalancing.
			int64 left_height = tree_height(node->less_);
			int64 right_height = tree_height(node->greater_);

			return (left_height > right_height) ?
						(left_height + 1) : (right_height + 1);
		}


		void clear() NOTHROW
		{
			destroy_and_free_recursive(root_node_);
			root_node_ = NULL;
			size_ = 0;
		}


		void validate(node_type* node) const
		{
			/**
			 * Testing helper: Validate that a @bref{node}
			 * satisfies AVL and general tree properties.
			 */

			if (!node) return;


			// Pointers.
			if (!node->parent_ && node != root_node_)
				throw validation_failed();

			if (node->parent_ &&
				node != node->parent_->less_ &&
				node != node->parent_->greater_)
					throw validation_failed();

			if (node->less_ && node->less_->parent_ != node)
				throw validation_failed();
			if (node->greater_ && node->greater_->parent_ != node)
				throw validation_failed();
			if (node->less_ != NULL && node->less_ == node->greater_)
				throw validation_failed();


			// AVL properties.
			int64 left_height = tree_height(node->less_);
			int64 right_height = tree_height(node->greater_);

			if (right_height - left_height != node->balance_)
				throw validation_failed();


			// Ordering.
			if (node->greater_ && node->less_)
			{
				if (*node->greater_ < *node->less_)
					throw validation_failed();
			}
			

			// Recurse.
			validate(node->less_);
			validate(node->greater_);
		}

		void validate() const
			{ validate(root_node_); }


		allocator_type allocator_;

		node_type *root_node_;
		int64 size_;
};




} /* namespace viral_core */


#endif /* defined(INCLUDED__VIRAL_CORE__TREE_BASE_HPP) */
