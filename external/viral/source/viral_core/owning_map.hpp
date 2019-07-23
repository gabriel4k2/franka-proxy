/**
 *************************************************************************
 *
 * @file owning_map.hpp
 *
 * Owning map template class.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__OWNING_MAP_HPP)
#define INCLUDED__VIRAL_CORE__OWNING_MAP_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "scope_guard.hpp"
#include "tree_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class owning_map
 * @ingroup owning_module
 *
 * Holds exclusive ownership over unique objects at least of type @bref{V},
 * each of which is identified by an unique key of type @bref{K}.
 *
 * Notes:
 * - See the @ref owning_module "Owning Module" for general usage.
 * - Requirements for the key type @bref{K} are described in tree_map.
 *
 ************************************************************************/
template <typename K, typename V>
	class owning_map
{
	public:

		typedef tree_map<K, V*> pointer_map;

		typedef typename pointer_map::content_type content_type;
		typedef typename pointer_map::node_type node_type;

		typedef typename pointer_map::iterator iterator;
		typedef typename pointer_map::const_iterator const_iterator;


		owning_map() NOTHROW { }


		/**
		 * Copy or move construct from @bref{other} owning_map.
		 * Move construction leaves a valid, empty source.
		 */
		//@{	
		owning_map(const owning_map& other)
		{
			// On any exception, ~owning_map() will not run.
			// While ~pointer_map() releases any pointer memory,
			// we still must explicitly free any owned objects.
			SCOPE_GUARD
				(clear_on_failure, (*this, &owning_map::clear));

			for (const_iterator it(other.first()); it; ++it)
			{
				auto_pointer<V> value_clone
					(it->value ?
						viral_core::clone(*it->value) :
						auto_pointer<V>(NULL));

				insert(it->key, std::move(value_clone));
			}

			clear_on_failure.dismiss();
		}

		owning_map(owning_map&& other) NOTHROW
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		~owning_map() NOTHROW { clear(); }
		

		/** Create copy of owning_map on heap. */
		auto_pointer<owning_map> clone() const
			{ return auto_pointer<owning_map>(new owning_map(*this)); }


		/**
		 * Copy or move assign from @bref{other} owning_map.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		owning_map& operator =(const owning_map& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			owning_map offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		owning_map& operator =(owning_map&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			owning_map offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(owning_map& other) NOTHROW
			{ swap(pointers_, other.pointers_); }


		/**
		 * Take ownership over potentially @NULL incoming @bref{object},
		 * insert pointer into the map under provided @bref{key},
		 * and return iterator to the respective map entry.
		 *
		 * On duplicate keys, the object stored within the map is deleted
		 * and replaced.
		 *
		 * In case of any exception, the incoming @bref{object} is
		 * deleted and the map is not modified.
		 *
		 * @attent One must be wary of potential execution order.
		 * See the @ref exception_module "Exception Module" for details.
		 */
		iterator insert(const K& key, auto_pointer<V> object)
		{
			// Create new node with NULL pointer or find existing
			// node under strong exception safety. Thereafter, delete
			// any existing object, and store actual new object,
			// both NOTHROW actions.
			iterator exists(pointers_.try_insert(key, NULL));
			delete exists->value;
			exists->value = object.release_ownership();
			return exists;
		}


		int64 size() const NOTHROW { return pointers_.size(); }
		bool empty() const NOTHROW { return pointers_.empty(); }


		/**
		 * Return iterator to first or last entry,
		 * or past-end iterator for empty maps.
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
		 * Return an iterator to the entry for given @bref{key},
		 * or a past-end iterator if the map does not contain the key.
		 */
		//@{
		iterator find(const K& key) NOTHROW
			{ return pointers_.find(key); }
		const_iterator find(const K& key) const NOTHROW
			{ return pointers_.find(key); }
		//@}


		bool contains(const K& key) const NOTHROW
			{ return pointers_.contains(key); }


		/**
		 * Release ownership over object for given @bref{key} or
		 * at indicated iterator @bref{it}.
		 * 
		 * The call returns @NULL if the indicated @bref{key}
		 * is not contained or if the iterator is past-end.
		 *
		 * @attent The iterator must belong to the map, and becomes
		 * invalid after the call.
		 */
		//@{		
		auto_pointer<V> release(const K& key) NOTHROW
			{ return release(find(key)); }
		auto_pointer<V> release(const iterator& it) NOTHROW
		{
			if (!it) return auto_pointer<V>();
			auto_pointer<V> result(it->value);
			pointers_.remove(it); // Does not call delete.
			return result;
		}
		//@}


		/**
		 * Remove entry at @bref{key} or at @bref{it}erator
		 * and delete any associated object.
		 * Any iterators or pointers to the removed entry
		 * and the deleted object become invalid.
		 *
		 * The call is ignored if @bref{it} is past-end
		 * or if the map does not contain @bref{key}. 
		 *
		 * @attent The iterator must belong to the map,
		 * and becomes invalid after the call.
		 */
		//@{		
		void remove(const K& key) NOTHROW
			{ remove(find(key)); }
		void remove(const iterator& it) NOTHROW
		{
			if (!it) return;
			delete it->value;
			pointers_.remove(it);
		}
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


		/** Remove all entries and delete any objects. */
		void clear() NOTHROW
		{
			for (iterator it(first()); it; ++it)
				delete it->value;
			pointers_.clear();
		}


	private:

		pointer_map pointers_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__OWNING_MAP_HPP) */
