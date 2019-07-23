/**
 *************************************************************************
 *
 * @file bounding_iterator.hpp
 *
 * Iterator over bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_ITERATOR_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_ITERATOR_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_const_node_iterator
 * @ingroup bounding_module
 *
 * Iterator over nodes in a bounding volume hierarchy of type @bref{H}.
 *
 * Usage notes:
 * - This iterator unifies iterating through bounding_tree, bounding_scene,
 *		bounding_single, and bounding_implicit. See the @ref bounding_module
 *		"Bounding Module" for hierarchy traversal with node iterators.
 * - @attent Bounding volume hierarchy types determine individually
 *		whether iterators become invalid after hierarchy operations.
 *
 * Implementation notes:
 * - There is no non-const bounding_node_iterator to prohibit clients
 *		from externally updating (and breaking) bounding hierarchies.
 *		Modifiable hierarchies instead expose suitable member methods
 *		to update bounding shapes or geometry objects.
 *
 ************************************************************************/
template <typename H>
	class bounding_const_node_iterator :
		private bool_convert<bounding_const_node_iterator<H>>
{

	public:

		typedef H hierarchy_type;
		typedef typename H::node_type node_type;


		bounding_const_node_iterator() NOTHROW
			: node_(NULL) { }

		bounding_const_node_iterator(const node_type* node) NOTHROW 
			: node_(node) { }

		bounding_const_node_iterator
			(const bounding_const_node_iterator& other) NOTHROW 
				: node_(other.node_) { }


		bounding_const_node_iterator&
			operator =(const bounding_const_node_iterator& other) NOTHROW
				{ node_ = other.node_; return *this; }


		/** Determine if the iterator is not past-end yet. */
		operator bool_conversion_type() const NOTHROW
		{
			return node_ ?
					&bounding_const_node_iterator<H>::bool_conversion_is_true :
					NULL;
		}


		/** Retrieve constant content of target node. */
		//@{
		const node_type& operator *() const NOTHROW { return *node_; }		
		const node_type* operator ->() const NOTHROW { return node_; }
		//@}


		void swap_with(bounding_const_node_iterator& other) NOTHROW
			{ swap(node_, other.node_); }


		/** Iterator comparison. */
		//@{
		bool operator ==(const bounding_const_node_iterator& other) const NOTHROW
			{ return node_ == other.node_; }
		bool operator !=(const bounding_const_node_iterator& other) const NOTHROW
			{ return node_ != other.node_; }

		bool operator <(const bounding_const_node_iterator& other) const NOTHROW
			{ return node_ < other.node_; }
		bool operator <=(const bounding_const_node_iterator& other) const NOTHROW
			{ return node_ <= other.node_; }
		//@}


	private:

		const node_type* node_;		
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_ITERATOR_HPP) */
