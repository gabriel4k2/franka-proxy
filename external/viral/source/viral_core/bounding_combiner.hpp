/**
 *************************************************************************
 *
 * @file bounding_combiner.hpp
 *
 * Coupling of two bounding hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_COMBINER_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_COMBINER_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_combiner_iterator
 * @ingroup bounding_module
 *
 * Iterates over a @bref{C}oarse and multiple @bref{D}etail hierarchies
 * up to the leaves of each detail hierarchy.
 *
 ************************************************************************/
template <typename C, typename D>
	class bounding_combiner_iterator :
		private bool_convert<bounding_combiner_iterator<C, D>>
{
	friend class bounding_combiner<C, D>;


	public:

		typedef typename C::const_node_iterator coarse_const_node_iterator;
		typedef typename D::const_node_iterator detail_const_node_iterator;


		bounding_combiner_iterator() NOTHROW { }


		bounding_combiner_iterator
			(const bounding_combiner_iterator& other) NOTHROW
			:
			coarse_node_(other.coarse_node_),
			detail_node_(other.detail_node_)
		{ }


		bounding_combiner_iterator&
			operator =(const bounding_combiner_iterator& other) NOTHROW
		{
			coarse_node_ = other.coarse_node_;
			detail_node_ = other.detail_node_;
			return *this;
		}


		/**
		 * Determine if the iterator is not past-end yet.
		 * Note valid iterators may still have a past-end detail_node().
		 */
		operator bool_conversion_type() const NOTHROW
		{
			return coarse_node_ ?
						&bounding_combiner_iterator::bool_conversion_is_true :
						NULL;
		}


		void swap_with
			(bounding_combiner_iterator& other) NOTHROW
		{
			swap(coarse_node_, other.coarse_node_);
			swap(detail_node_, other.detail_node_);
		}


		/** Iterator comparison. */
		//@{
		bool operator ==
			(const bounding_combiner_iterator& other) const NOTHROW
		{
			return
				coarse_node_ == other.coarse_node_ &&
				detail_node_ == other.detail_node_;
		}

		bool operator !=
			(const bounding_combiner_iterator& other) const NOTHROW
		{
			return
				coarse_node_ != other.coarse_node_ ||
				detail_node_ != other.detail_node_;
		}
		//@}


		const coarse_const_node_iterator& coarse_node() const NOTHROW
			{ return coarse_node_; }
		const detail_const_node_iterator& detail_node() const NOTHROW
			{ return detail_node_; }


	private:

		/**
		 * Wrap coarse and detail iterators into a combined iterator.
		 * Private to prohibit clients from creating broken iterators.
		 */
		bounding_combiner_iterator
			(const coarse_const_node_iterator& coarse_node,
			 const detail_const_node_iterator& detail_node) NOTHROW
			:
			coarse_node_(coarse_node),
			detail_node_(detail_node)
		{ }


		coarse_const_node_iterator coarse_node_;
		detail_const_node_iterator detail_node_;
};




/**
 *************************************************************************
 *
 * @class bounding_combiner
 * @ingroup bounding_module
 *
 * Combines an existing @bref{C}oarse bounding volume hierarchy
 * with leaf-level @bref{D}etail bounding volume hierarchies
 * into a unified, fully traversable bounding volume hierarchy.
 *
 * Usage notes:
 * - Leaves of the @bref{coarse_hierarchy} hold geometry objects
 *		that are instances of the @bref{D}etail hierarchy type.
 * - The bounding_combiner_iterator%s of bounding_combiner traverse
 *		both the coarse and detail hierarchies up to detail leaves,
 *		where final geometry objects are stored.
 * - Iterators over a bounding_combiner may at times become invalid
 *		(e.g. due to operations that modify @bref{coarse_hierarchy}).
 *		Clients thus should prefer to keep a bounding_combiner alive
 *		but over respective uses (e.g. one bounding_distance call).
 *
 * Hierarchy notes:
 * - See the @ref bounding_module "Bounding Module" for requirements
 *		of bounding volume hierarchy types for @bref{C} and @bref{D}.
 * - The @bref{coarse_hierarchy} must have an additional method
 *			const D& detail_hierarchy
 *				(const bounding_combiner_iterator<C, D>& node)
 *					const NOTHROW
 *		to fetch the detail hierarchy in a not past-end @bref{node},
 *		where node.coarse_node() is a leaf of the coarse hierarchy,
 *		and where node.detail_node() is past-end. Multiple combiners
 *		can act on a single type of coarse hierarchy by overloading
 *		detail_hierarchy() for suitable bounding_combiner_iterator%s
 *		(e.g. to use one of multiple detail hierarchies).
 * - A bounding_combiner itself is another bounding volume hierarchy.
 *		For example, clients can stack multiple bounding combiners.
 * 
 * Implementation notes:
 * - Even with overhead owing to the larger bounding_combiner_iterator,
 *		geometry-level operations through bounding_combiner outperform
 *		two-staged variants (e.g. bounding_distance on coarse hierarchy
 *		calling bounding_distance on detail hierarchy) for enough nodes.
 *		Notably, a bounding_combiner can maintain a mean-case O(log n),
 *		while alternatives degrade to a less efficient O(log n * log n).
 *
 ************************************************************************/
template <typename C, typename D>
	class bounding_combiner :
		private non_copyable
{

	public:

		typedef C coarse_hierarchy_type;
		typedef typename C::const_node_iterator coarse_const_node_iterator;

		typedef D detail_hierarchy_type;
		typedef typename D::const_node_iterator detail_const_node_iterator;

		typedef bounding_combiner_iterator<C, D> const_node_iterator;


		/**
		 * Create a combined hierarchy over a @bref{coarse_hierarchy},
		 * each leaf of which contains a @bref{D}etail hierarchy.
		 *
		 * @attent
		 * The coarse hierarchy must outlive the bounding_combiner.
		 */
		bounding_combiner
			(const coarse_hierarchy_type& coarse_hierarchy) NOTHROW
			:
			coarse_hierarchy(coarse_hierarchy)
		{ }


		/**
		 * Bounding volume hierarchy interface for traversing
		 * the @bref{coarse_hierarchy} up to leaves of the detail hierarchy.
		 */
		//@{
		const_node_iterator root_node() const NOTHROW
		{
			return
				const_node_iterator
					(coarse_hierarchy.root_node(),
					 detail_const_node_iterator());
		}


		const_node_iterator parent_node
			(const const_node_iterator& node) const NOTHROW
		{
			if (!node.coarse_node())
				return const_node_iterator();


			// If the iterator points to a bounding node in a detail hierarchy,
			// ascend to the next higher, possibly past-end detail bounding node.
			// If the detail node becomes past-end, the coarse node remains valid
			// and we arrive at a leaf of the coarse hierarchy.
			if (node.detail_node())
			{
				const detail_hierarchy_type& detail_hierarchy =
					coarse_hierarchy.detail_hierarchy(node);

				return
					const_node_iterator
						(node.coarse_node(),
						 detail_hierarchy.parent(node.detail_node()));
			}


			// The iterator points to a node of the coarse hierarchy,
			// therefore ascend to the next higher coarse node.
			return
				const_node_iterator
					(coarse_hierarchy.parent_node(node.coarse_node()),
					 detail_const_node_iterator());
		}


		const_node_iterator left_node
			(const const_node_iterator& node) const NOTHROW
		{
			if (!node.coarse_node())
				return const_node_iterator();


			// If the iterator points to a node in a detail hierarchy,
			// descend to the next lower, left-hand detail bounding node.
			// If this node is past-end (e.g. outer node, possibly a leaf),
			// then return a past-end iterator to disable further descent.
			if (node.detail_node())
			{
				const detail_hierarchy_type& detail_hierarchy =
					coarse_hierarchy.detail_hierarchy(node);

				detail_const_node_iterator detail_left_node
					(detail_hierarchy.left_node(node.detail_node()));

				if (!detail_left_node)
					return const_node_iterator();

				return
					const_node_iterator
						(node.coarse_node(), detail_left_node);	
			}

	
			// We are not inside a detail node yet.
			// If we are at a leaf in the coarse hierarchy,
			// continue to the root of the detail hierarchy.
			coarse_const_node_iterator coarse_left_node
				(coarse_hierarchy.left_node(node.coarse_node()));	
			coarse_const_node_iterator coarse_right_node
				(coarse_hierarchy.right_node(node.coarse_node()));	

			if (!coarse_left_node && !coarse_right_node)
			{
				const detail_hierarchy_type& detail_hierarchy =
					coarse_hierarchy.detail_hierarchy(node);
	
				return const_node_iterator
							(node.coarse_node(),
							 detail_hierarchy.root_node());
			}


			// Not at a leaf of the coarse hierarchy yet,
			// so continue on (possibly past-end) left child.
			return const_node_iterator
						(coarse_left_node,
						 detail_const_node_iterator());
		}


		const_node_iterator right_node
			(const const_node_iterator& node) const NOTHROW
		{
			if (!node.coarse_node())
				return const_node_iterator();


			// If the iterator points to a node in a detail hierarchy,
			// descend to the next lower, right-hand detail bounding node.
			// If this node is past-end (e.g. outer node, possibly a leaf),
			// then return a past-end iterator to disable further descent.
			if (node.detail_node())
			{
				const detail_hierarchy_type& detail_hierarchy =
					coarse_hierarchy.detail_hierarchy(node);

				detail_const_node_iterator detail_right_node
					(detail_hierarchy.right_node(node.detail_node()));

				if (!detail_right_node)
					return const_node_iterator();

				return
					const_node_iterator
						(node.coarse_node(), detail_right_node);	
			}


			// We are not inside a detail node yet.
			// If we are at a leaf in the coarse hierarchy,
			// do not pass through to the detail hierarchy,
			// bounding_combiner::left_node() already does this.
			// Instead, stop iterating here.
			coarse_const_node_iterator coarse_left_node
				(coarse_hierarchy.left_node(node.coarse_node()));	
			coarse_const_node_iterator coarse_right_node
				(coarse_hierarchy.right_node(node.coarse_node()));	

			if (!coarse_left_node && !coarse_right_node)
				return const_node_iterator();


			// Not at a leaf of the coarse hierarchy yet,
			// so continue on (possibly past-end) right child.
			return const_node_iterator
						(coarse_right_node,
						 detail_const_node_iterator());
		}
		//@}


		const C& coarse_hierarchy;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_COMBINER_HPP) */
