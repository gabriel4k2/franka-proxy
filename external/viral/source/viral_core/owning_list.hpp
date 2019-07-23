/**
 *************************************************************************
 *
 * @file owning_list.hpp
 *
 * A list that owns externally ordered objects.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__OWNING_LIST_HPP)
#define INCLUDED__VIRAL_CORE__OWNING_LIST_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "scope_guard.hpp"
#include "list.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class owning_list
 * @ingroup owning_module
 *
 * Owns an ordered list of objects, all at least of type @bref{T}.
 *
 * Notes:
 * - See the @ref owning_module "Owning Module" for general usage.
 *
 ************************************************************************/
template <typename T>
	class owning_list
{
	public:

		typedef list<T*> pointer_list;

		typedef typename pointer_list::content_type content_type;
		typedef typename pointer_list::node_type node_type;

		typedef typename pointer_list::iterator iterator;
		typedef typename pointer_list::const_iterator const_iterator;


		owning_list() NOTHROW { }


		/**
		 * Copy or move construct from @bref{other} owning_list.
		 * Move construction leaves a valid, empty source.
		 */
		//@{	
		owning_list(const owning_list& other)
		{
			// On any exception, ~owning_list() will not run.
			// While ~pointer_list() releases any pointer memory,
			// we still must explicitly free any owned objects.
			SCOPE_GUARD
				(clear_on_failure, (*this, &owning_list::clear));

			for (const_iterator it(other.first()); it; ++it)
			{
				auto_pointer<T> object_clone
					(*it ?
						viral_core::clone(**it) :
						auto_pointer<T>(NULL));

				insert(std::move(object_clone));
			}

			clear_on_failure.dismiss();
		}

		owning_list(owning_list&& other) NOTHROW
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		~owning_list() NOTHROW { clear(); }


		/** Create copy of owning_list and contents on heap. */
		auto_pointer<owning_list> clone() const
			{ return auto_pointer<owning_list>(new owning_list(*this)); }


		/**
		 * Copy or move assign from @bref{other} owning_list.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		owning_list& operator =(const owning_list& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			owning_list offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		owning_list& operator =(owning_list&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			owning_list offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(owning_list& other) NOTHROW
			{ swap(pointers_, other.pointers_); }


		/**
		 * Take ownership over potentially @NULL incoming @bref{object},
		 * insert it into the list, and return iterator to the respective
		 * list entry. On any exception, the list remains unchanged,
		 * and the incoming object is deleted.
		 */
		iterator insert(auto_pointer<T> object)
			{ return push_back(std::move(object)); }


		/** Invoke insert() for various places within the list. */
		//@{
		iterator push_front(auto_pointer<T> object)
			{ return insert_before(first(), std::move(object)); }
		iterator push_back(auto_pointer<T> object)
			{ return insert_after(last(), std::move(object)); }

		iterator insert_before(iterator it, auto_pointer<T> object)
		{
			// Create appropriate node with strong exception safety,
			// thereafter NOTHROW release of original ownership.
			iterator result(pointers_.insert_before(it, object.get()));
			object.release_ownership();
			return result;
		}

		iterator insert_after(iterator it, auto_pointer<T> object)
		{
			// See above.
			iterator result(pointers_.insert_after(it, object.get()));
			object.release_ownership();
			return result;
		}
		//@}


		int64 size() const NOTHROW { return pointers_.size(); }
		bool empty() const NOTHROW { return pointers_.empty(); }


		/**
		 * Return iterator to first or last entry,
		 * or past-end iterator for empty lists.
		 */
		//@{
		iterator first() NOTHROW { return pointers_.first(); }
		iterator last() NOTHROW { return pointers_.last(); }
		const_iterator first() const NOTHROW { return pointers_.first(); }
		const_iterator last() const NOTHROW { return pointers_.last(); }
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
		 * Return an iterator to the entry for the indicated
		 * @bref{object}, or a past-end iterator if the object
		 * is not contained.
		 */
		//@{
		iterator find(T* object) NOTHROW
			{ return pointers_.find(object); }
		const_iterator find(const T* object) const NOTHROW
			{ return pointers_.find(object); }
		//@}


		bool contains(T* object) const NOTHROW
			{ return pointers_.contains(object); }


		/**
		 * Release ownership over object at indicated @bref{it}erator
		 * or at some @bref{object} pointer. The call returns @NULL
		 * if the iterator is past-end or if the object pointer
		 * is @NULL or not contained.
		 *
		 * @attent The iterator must belong to the list, and becomes
		 * invalid after the call.
		 */
		//@{
		auto_pointer<T> release(const iterator& it) NOTHROW
		{
			if (!it) return auto_pointer<T>();
			auto_pointer<T> result(*it);
			pointers_.remove(it); // Does not call delete.
			return result;
		}

		auto_pointer<T> release(T* object) NOTHROW
			{ return release(find(object)); }
		//@}


		/**  Invoke release() on first or last object. */
		//@{
		auto_pointer<T> release_first() NOTHROW
			{ return release(first()); }
		auto_pointer<T> release_last() NOTHROW
			{ return release(last()); }
		//@}


		/**  Remove and delete first or last object. */
		//@{
		void pop_front() NOTHROW { remove(first()); }
		void pop_back() NOTHROW { remove(last()); }
		//@}


		/**
		 * Remove entry for indicated @bref{key} or at indicated
		 * iterator @bref{it} and delete any associated object.
		 * Any iterators or pointers to the removed entry and
		 * the deleted object become invalid.
		 *
		 * The call is ignored if the indicated @bref{object}
		 * is not contained or if the iterator is past-end. 
		 *
		 * @attent The iterator must belong to the list,
		 * and becomes invalid after the call.
		 *
		 * @attent Removing @NULL from the list removes
		 * only the first @NULL pointer, not all occurrences.
		 */
		//@{
		void remove(T* object) NOTHROW
			{ remove(find(object)); }
		void remove(const iterator& it) NOTHROW
		{
			if (!it) return;
			if (*it) delete *it;
			pointers_.remove(it);
		}
		//@}


		/** Remove all entries and delete any objects. */
		void clear() NOTHROW
		{
			for (iterator it(first()); it; ++it)
				if (*it) delete *it;

			pointers_.clear();
		}


	private:

		pointer_list pointers_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__OWNING_LIST_HPP) */

