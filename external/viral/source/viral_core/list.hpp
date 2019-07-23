/**
 *************************************************************************
 *
 * @file list.hpp
 *
 * Linked list header.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__LIST_HPP)
#define INCLUDED__VIRAL_CORE__LIST_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "integer_checks.hpp"
#include "scope_guard.hpp"
#include "allocator_heap.hpp"
#include "allocator_util.hpp"
#include "container_limits.hpp"
#include "iterator.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class list_node
 * @ingroup container_module
 *
 * A double-linked entry within a @ref list.
 *
 ************************************************************************/
template <typename T, typename A>
	class list_node
{
	friend class list<T, A>;

	friend class iterator<list<T, A>>;
	friend class const_iterator<list<T, A>>;


	public:

		list_node(const T& value)
			: value_(value), next_(NULL), previous_(NULL) { }


	private:

		T& content_for_iterator() NOTHROW
			{ return value_; }


		T value_;
		list_node* next_;
		list_node* previous_;
};




/**
 *************************************************************************
 *
 * @class list
 * @ingroup container_module
 *
 * Stores a dynamic, ordered series of arbitrary values of type @bref{T},
 * allocated through an allocator of type @bref{A}.
 *
 * Notes:
 * - A single call to the copy constructor of @bref{T} is used to insert
 *		values into list entries. No default constructor is required.
 * - list::find() and consorts require a const, non-throwing equals
 *		operator (==) on @bref{T}. 
 *
 ************************************************************************/
template <typename T, typename A>
	class list
{
	friend class iterator<list<T, A>>;
	friend class const_iterator<list<T, A>>;


	public:

		typedef T content_type;
		typedef list_node<T, A> node_type;

		typedef A allocator_type;

		typedef viral_core::iterator<list<T, A>> iterator;
		typedef viral_core::const_iterator<list<T, A>> const_iterator;


		list() NOTHROW
			:
			first_(NULL), last_(NULL), size_(0)
		{ }
		

		explicit list(const allocator_type& allocator) NOTHROW
			:
			allocator_(allocator),
			first_(NULL), last_(NULL), size_(0)
		{ }


		list(std::initializer_list<T> initializer)
			:
			first_(NULL), last_(NULL), size_(0)
		{
			// ~list() won't run on any exception,
			// thus partially inserted nodes may leak.
			// Hence, we must clear explicitly here.
			SCOPE_GUARD
				(mem_guard, (*this, &list::clear));


			// Avoid overflow in cast of initializer.size() to int64,
			// as an initializer list has (unsigned) size_t elements.
			// Auto promotion to unsigned and larger of both types.
			if (initializer.size() > container_limits<T>::max_size)
				throw bounds_exceeded();

			for (auto it(initializer.begin());
				 it != initializer.end(); ++it)
					insert(*it);


			mem_guard.dismiss();
		}


		/**
		 * Copy or move construct from @bref{other} list.
		 * Move construction leaves a valid, empty() source.
		 */
		//@{		
		list(const list& other)
			:
			allocator_(other.allocator_),
			first_(NULL), last_(NULL), size_(0)
		{
			// ~list() won't run on any exception,
			// thus partially inserted nodes may leak.
			// Hence, we must clear explicitly here.
			SCOPE_GUARD
				(mem_guard, (*this, &list::clear));


			// Copy nodes of source list.
			const node_type* copy = other.first_;
			while (copy)
			{
				insert(copy->value_);
				copy = copy->next_;
			}


			mem_guard.dismiss();
		}

		list(list&& other) NOTHROW
			:
			allocator_(other.allocator_),
			first_(NULL), last_(NULL), size_(0)
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		~list() NOTHROW { clear(); }


		/** Create not-@NULL copy of list on heap. */
		auto_pointer<list> clone() const
			{ return auto_pointer<list>(new list(*this)); }


		/**
		 * Copy or move assign of @bref{other} list.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty() source.
		 */
		//@{
		list& operator =(const list& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			list offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		list& operator =(list&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			list offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(list& other) NOTHROW
		{
			swap(allocator_, other.allocator_);
			swap(first_, other.first_);
			swap(last_, other.last_);
			swap(size_, other.size_);
		}


		/**
		 * Insert copy of @bref{value} at the end of the list,
		 * and return an iterator to the new entry.
		 */
		iterator insert(const T& value)
			{ return insert_after(iterator(last_), value); }


		/**
		 * Add copy of @bref{value} at the begin or end of the list,
		 * and return an iterator to the inserted entry.
		 */
		//@{
		iterator push_front(const T& value)
			{ return insert_before(iterator(first_), value); }
		iterator push_back(const T& value)
			{ return insert_after(iterator(last_), value); }
		//@}


		/**
		 * Add copy of @bref{value} before or after @bref{it}erator,
		 * and return an iterator to the new entry.
		 *
		 * If the provided iterator is past-end, @bref{insert_before}
		 * inserts as the first entry, and @bref{insert_after} inserts
		 * as the last entry.
		 *
		 * @attent The @bref{it}erator must belong to this list.
		 */
		//@{
		iterator insert_before(iterator it, const T& value)
		{
			if (!integer_checks::is_add_safe(size_, 1) ||
				size_ + 1 > container_limits<T>::max_size)
					throw bounds_exceeded();


			node_type *new_node =
				allocator_util::allocate_and_construct<node_type>
					(allocator_, value);


			// NOTHROW after here!
			++size_;

			node_type *insert_spot =
				it.node_ ? it.node_ : first_;
		
			if (insert_spot)
			{
				new_node->next_ = insert_spot;
				new_node->previous_ = insert_spot->previous_; 

				insert_spot->previous_ = new_node;

				if (new_node->previous_)
					new_node->previous_->next_ = new_node;
				else
					first_ = new_node;
			}
			else
			{				
				first_ = new_node;
				last_ = new_node;
			}

			return iterator(new_node);
		}

		iterator insert_after(iterator it, const T& value)
		{
			if (!integer_checks::is_add_safe(size_, 1) ||
				size_ + 1 > container_limits<T>::max_size)
					throw bounds_exceeded();


			node_type *new_node =
				allocator_util::allocate_and_construct<node_type>
					(allocator_, value);


			// NOTHROW after here!
			++size_;

			node_type *insert_spot =
				it.node_ ? it.node_ : last_;

			if (insert_spot)
			{
				new_node->next_ = insert_spot->next_;
				new_node->previous_ = insert_spot;

				insert_spot->next_ = new_node;

				if (new_node->next_)
					new_node->next_->previous_ = new_node;
				else
					last_ = new_node;
			}
			else
			{
				first_ = new_node;
				last_ = new_node;
			}

			return iterator(new_node);
		}
		//@}


		/**
		 * Transfer all entries from @bref{other} list to this list
		 * and insert the entries after the indicated @bref{it}erator.
		 * If the @bref{it}erator is past-end, insert at the start.
		 *
		 * Usage notes:
		 * - No copies occur, thus any node iterators remain valid.
		 * - Strong exception safety, adds either all or no entries.
		 * - Throws on identical lists or incompatible allocators.
		 */
		void splice(const iterator& it, list&& other)
		{
			if (!integer_checks::is_add_safe(size_, other.size_) ||
				size_ + other.size_ > container_limits<T>::max_size)
					throw bounds_exceeded();

			if (this == &other)
				throw invalid_parameter();

			if (!allocator_.can_free_for(other.allocator_))
				throw invalid_state();


			// Exception safety:
			// NOTHROW from here.


			// Ensure source list has at least one entry
			// so we need not check for NULL pointers.
			if (other.empty())
				return;


			// If the target list has no entries yet,
			// we can simply swap.
			if (empty())
			{
				swap_with(other);
				return;
			}


			// Neither list is empty.
			// Move entries from source list to target list,
			// depending on whether we insert at the start,
			// at the end, or somewhere in-between.
			size_ += other.size_;
			
			if (it.node_ == last_)
			{
				last_->next_ = other.first_;
				other.first_->previous_ = last_;
				last_ = other.last_;
			}
			else if (it.node_)
			{
				other.first_->previous_ = it.node_;
				other.last_->next_ = it.node_->next_;
				it.node_->next_->previous_ = other.last_;
				it.node_->next_ = other.first_;
			}
			else
			{
				other.last_->next_ = first_;
				first_->previous_ = other.last_;
				first_ = other.first_;						
			}


			// Reset source list.
			other.first_ = NULL;
			other.last_ = NULL;
			other.size_ = 0;
		}


		/**
		 * Fetch a copy of the allocator
		 * currently in use by this list.
		 */
		allocator_type allocator() const NOTHROW
			{ return allocator_; }


		/**
		 * Checked entry access, throws on invalid @bref{index}.
		 * O(n) complexity, do not use in tight loops.
		 */
		//@{
		const T& at(int64 index) const
		{
			if (index < 0 || index >= size_) throw invalid_parameter();
			return (*this)[index];
		}
		T& at(int64 index)
		{
			if (index < 0 || index >= size_) throw invalid_parameter();
			return (*this)[index];
		}
		//@}


		/**
		 * Unchecked entry access.
		 * O(n) complexity, do not use in tight loops.
		 */
		//@{
		const T& operator[](int64 index) const NOTHROW
		{
			const node_type *n = first_;
			for (int64 i = 0; i != index; ++i) n = n->next_;
			return n->value_;
		}
		T& operator[](int64 index) NOTHROW
		{
			node_type *n = first_;
			for (int64 i = 0; i != index; ++i) n = n->next_;
			return n->value_;
		}	
		//@}


		int64 size() const NOTHROW { return size_; }
		bool empty() const NOTHROW { return size_ == 0; }


		/**
		 * Retrieve iterator to first or last entry,
		 * or past-end iterator if the list is empty.
		 */
		//@{
		iterator first() NOTHROW { return iterator(first_); }	
		iterator last() NOTHROW { return iterator(last_); }
		const_iterator first() const NOTHROW { return const_iterator(first_); }	
		const_iterator last() const NOTHROW { return const_iterator(last_); }
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
		 * Find @bref{value} in list and return iterator
		 * to the first match in search direction.
		 *
		 * A past-end result indicates the provided value
		 * is not contained in the list.
		 */
		//@{
		iterator find(const T& value) NOTHROW
			{ return iterator(find_first_helper(value)); }
		iterator find_first(const T& value) NOTHROW
			{ return iterator(find_first_helper(value)); }
		iterator find_last(const T& value) NOTHROW
			{ return iterator(find_last_helper(value)); }

		const_iterator find(const T& value) const NOTHROW
			{ return const_iterator(find_first_helper(value)); }
		const_iterator find_first(const T& value) const NOTHROW
			{ return const_iterator(find_first_helper(value)); }
		const_iterator find_last(const T& value) const NOTHROW
			{ return const_iterator(find_last_helper(value)); }
		//@}


		bool contains(const T& value) const NOTHROW
			{ return find_first_helper(value) != NULL; }


		/** Remove first or last entry of list. */
		//@{
		void pop_front() NOTHROW { remove(iterator(first_)); }
		void pop_back() NOTHROW { remove(iterator(last_)); }
		//@}


		/**
		 * Remove first entry in list with given @bref{value}
		 * or entry at given @bref{it}erator.
		 *
		 * Usage notes:
		 * - All pointers and iterators to the deleted entry
		 *		become invalid.
		 * - The call is ignored if the value is not contained,
		 *		or if the iterator is past end.
		 * -  @attent The iterator must belong to this list
		 *		and becomes invalid after the call.
		 */
		//@{
		void remove(const T& value) NOTHROW
			{ remove(find(value)); }

		void remove(const iterator& it) NOTHROW
		{
			node_type* del_node = it.node_;

			if (!del_node) return;

			if (del_node->previous_)
				del_node->previous_->next_ = del_node->next_;
			else
				first_ = del_node->next_;

			if (del_node->next_)
				del_node->next_->previous_ = del_node->previous_;
			else
				last_ = del_node->previous_;

			allocator_util::destroy_and_free
				(allocator_, del_node);

			--size_;
		}
		//@}

		
		void clear() NOTHROW
		{
			node_type* node = first_;
	
			while (node)
			{
				node_type* del_node = node;
				node = node->next_;

				allocator_util::destroy_and_free
					(allocator_, del_node);
			}

			first_ = NULL;
			last_ = NULL;
			size_ = 0;
		}		


	private:

		node_type* find_first_helper(const T& value) const NOTHROW
		{
			node_type* node = first_;

			while (node)
			{
				if (node->value_ == value) return node;
				node = node->next_;
			}

			return NULL;
		}		

		node_type* find_last_helper(const T& value) const NOTHROW
		{
			node_type* node = last_;

			while (node)
			{
				if (node->value_ == value) return node;
				node = node->previous_;
			}

			return NULL;
		}


		static node_type* next(node_type* node) NOTHROW
			{ return node ? node->next_ : NULL; }
		static node_type* previous(node_type* node) NOTHROW
			{ return node ? node->previous_ : NULL; }


		allocator_type allocator_;

		node_type *first_;
		node_type *last_;
		int64 size_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__LIST_HPP) */
