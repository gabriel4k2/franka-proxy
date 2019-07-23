/**
 *************************************************************************
 *
 * @file iterator.hpp
 *
 * Iterator base classes for sets, maps and lists.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ITERATOR_HPP)
#define INCLUDED__VIRAL_CORE__ITERATOR_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class iterator
 * @ingroup container_module
 *
 * Provides read/write access to the content of entries within any
 * node-based container of type @bref{C}.
 *
 ************************************************************************/
template <typename C>
	class iterator :
		private bool_convert<iterator<C>>
{
	friend C;
	friend class const_iterator<C>;


	public:

		typedef C container_type;
		typedef typename C::content_type content_type;


		iterator() NOTHROW
			: node_(NULL) { }


		/** Determine if the iterator is not past-end yet. */
		operator bool_conversion_type() const NOTHROW
			{ return node_ ? &iterator<C>::bool_conversion_is_true : NULL; }


		/** Retrieve content of target entry. */
		//@{
		content_type& operator *() const NOTHROW
			{ return node_->content_for_iterator(); }		
		content_type* operator ->() const NOTHROW
			{ return &node_->content_for_iterator(); }
		//@}


		/** Move to next or previous entry, ignored if past-end. */
		//@{
		iterator& operator ++() NOTHROW
			{ node_ = C::next(node_); return *this; }
		iterator& operator --() NOTHROW
			{ node_ = C::previous(node_); return *this; }
		//@}


		/** Swap two iterators. */
		void swap_with(iterator& other) NOTHROW
			{ swap(node_, other.node_); }


		/** Test if iterators point to exactly the same entry. */
		//@{
		bool operator ==(const iterator& other) const NOTHROW
			{ return node_ == other.node_; }
		bool operator !=(const iterator& other) const NOTHROW
			{ return node_ != other.node_; }
		bool operator ==(const const_iterator<C>& other) const NOTHROW
			{ return node_ == other.node_; }
		bool operator !=(const const_iterator<C>& other) const NOTHROW
			{ return node_ != other.node_; }
		//@}


		/** Compare iterators based on the address of their entry. */
		//@{
		bool operator <(const iterator& other) const NOTHROW
			{ return node_ < other.node_; }
		bool operator <=(const iterator& other) const NOTHROW
			{ return node_ <= other.node_; }
		bool operator >(const iterator& other) const NOTHROW
			{ return node_ > other.node_; }
		bool operator >=(const iterator& other) const NOTHROW
			{ return node_ >= other.node_; }
		//@}


	private:

		typedef typename C::node_type node_type;

		iterator(node_type *node) NOTHROW
			: node_(node) { }

		node_type *node_;
};




/**
 *************************************************************************
 *
 * @class const_iterator
 * @ingroup container_module
 *
 * Provides read-only access to the content of entries within any
 * node-based container of type @bref{C}.
 *
 ************************************************************************/
template <typename C>
	class const_iterator :
		private bool_convert<const_iterator<C>>
{
	friend C;
	friend class iterator<C>;


	public:

		typedef C container_type;
		typedef typename C::content_type content_type;


		const_iterator() NOTHROW
			: node_(NULL) { }


		/** Conversion from non-const iterator is possible. */
		//@{
		const_iterator(const iterator<C>& source) NOTHROW
			: node_(source.node_) { }
		const_iterator& operator =(const iterator<C>& source) NOTHROW
			{ node_ = source.node_; return *this; }
		//@}


		/** Determine if the iterator is not past-end yet. */
		operator bool_conversion_type() const NOTHROW
			{ return node_ ? &const_iterator<C>::bool_conversion_is_true : NULL; }


		/** Retrieve content of target entry. */
		//@{
		const content_type& operator *() const NOTHROW
			{ return node_->content_for_iterator(); }
		const content_type* operator ->() const NOTHROW
			{ return &node_->content_for_iterator(); }		
		//@}


		/** Move to next or previous entry, ignored if past-end. */
		//@{
		const_iterator& operator ++() NOTHROW
			{ node_ = C::next(node_); return *this; }
		const_iterator& operator --() NOTHROW
			{ node_ = C::previous(node_); return *this; }
		//@}


		/** Swap two iterators. */
		void swap_with(const_iterator& other) NOTHROW
			{ swap(node_, other.node_); }


		/** Test if iterators point to exactly the same entry. */
		//@{
		bool operator ==(const iterator<C>& other) const NOTHROW
			{ return node_ == other.node_; }
		bool operator !=(const iterator<C>& other) const NOTHROW
			{ return node_ != other.node_; }
		bool operator ==(const const_iterator& other) const NOTHROW
			{ return node_ == other.node_; }
		bool operator !=(const const_iterator& other) const NOTHROW
			{ return node_ != other.node_; }
		//@}


		/** Compare iterators based on the address of their entry. */
		//@{
		bool operator <(const const_iterator& other) const NOTHROW
			{ return node_ < other.node_; }
		bool operator <=(const const_iterator& other) const NOTHROW
			{ return node_ <= other.node_; }
		bool operator >(const const_iterator& other) const NOTHROW
			{ return node_ > other.node_; }
		bool operator >=(const const_iterator& other) const NOTHROW
			{ return node_ >= other.node_; }
		//@}


	private:

		typedef typename C::node_type node_type;

		const_iterator(node_type *node_) NOTHROW
			: node_(node_) { }

		node_type *node_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ITERATOR_HPP) */


