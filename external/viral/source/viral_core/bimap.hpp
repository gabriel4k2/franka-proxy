/**
 *************************************************************************
 *
 * @file bimap.hpp
 *
 * Bijective mapping container.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BIMAP_HPP)
#define INCLUDED__VIRAL_CORE__BIMAP_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "exception.hpp"
#include "scope_guard.hpp"
#include "allocator_heap.hpp"
#include "tree_set.hpp"
#include "tree_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bimap
 * @ingroup container_module
 *
 * Identifies keys of type @bref{L} with keys of type @bref{R},
 * and allocates internal structures through allocator @bref{A}.
 *
 * Usage notes:
 * - Requirements for the key types @bref{L} and @bref{R}
 *		are described in tree_set and tree_map.
 * - Bimaps always maintain valid pairs, even under exceptions.
 *		Unless the client breaks any entries within a bimap,
 *		the bimap always holds an @bref{L} for each @bref{R}
 *		and vice versa.
 *
 ************************************************************************/
template <typename L, typename R, typename A>
	class bimap
{

	public:

		typedef L left_type;
		typedef R right_type;

		typedef A allocator_type;

		typedef tree_set<L, A> l_set;
		typedef typename l_set::iterator l_iterator;
		typedef typename l_set::const_iterator const_l_iterator;

		typedef tree_set<R, A> r_set;
		typedef typename r_set::iterator r_iterator;
		typedef typename r_set::const_iterator const_r_iterator;


		bimap() NOTHROW { }

	
		explicit bimap(const allocator_type& allocator) NOTHROW
			:
			ls_(allocator),
			rs_(allocator),
			l_to_r_(allocator),
			r_to_l_(allocator)
		{ }


		/**
		 * Copy or move construct from an@bref{other} bimap.
		 * Move construction leaves a valid, empty source bimap.
		 */
		//@{
		bimap(const bimap& other)
			:
			ls_(other.ls_.allocator()),
			rs_(other.rs_.allocator()),
			l_to_r_(other.l_to_r_.allocator()),
			r_to_l_(other.r_to_l_.allocator())
		{
			for (const_l_iterator l_it(other.ls_.first()); l_it; ++l_it)
			{
				const_r_iterator r_it(other.l_to_r_.find(l_it)->value);
				insert(*l_it, *r_it); // find(l_it) and r_it are always valid.
			}
		}

		bimap(bimap&& other) NOTHROW
			:
			ls_(other.ls_.allocator()),
			rs_(other.rs_.allocator()),
			l_to_r_(other.l_to_r_.allocator()),
			r_to_l_(other.r_to_l_.allocator())
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		/** Create copy of bimap on heap. */
		auto_pointer<bimap> clone() const
			{ return auto_pointer<bimap>(new bimap(*this)); }


		/**
		 * Copy or move assign from @bref{other} bimap.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		bimap& operator =(const bimap& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			bimap offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		bimap& operator =(bimap&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			bimap offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		/**
		 * Identify @bref{l} with @bref{r} and vice versa.
		 * The map must not contain either @bref{l} or @bref{r} yet,
		 * otherwise inserting throws an exception.
		 */
		void insert(const L& l, const R& r)
		{
			if (contains_l(l) || contains_r(r))
				throw invalid_state();

			l_iterator l_it(ls_.insert(l));
			SCOPE_GUARD
				(remove_l, (ls_, &l_set::remove_by_iterator, l_it));

			r_iterator r_it(rs_.insert(r));
			SCOPE_GUARD
				(remove_r, (rs_, &r_set::remove_by_iterator, r_it));

			typename l_to_r_map::iterator l_to_r_it
				(l_to_r_.insert(l_it, r_it));
			SCOPE_GUARD
				(remove_lr,
					(l_to_r_, &l_to_r_map::remove_by_iterator, l_to_r_it));

			typename r_to_l_map::iterator r_to_l_it
				(r_to_l_.insert(r_it, l_it));
			SCOPE_GUARD
				(remove_rl,
					(r_to_l_, &r_to_l_map::remove_by_iterator, r_to_l_it));

			remove_rl.dismiss();
			remove_lr.dismiss();
			remove_r.dismiss();
			remove_l.dismiss();
		}


		l_iterator first_l() NOTHROW { return ls_.first(); }
		const_l_iterator first_l() const NOTHROW { return ls_.first(); }
		r_iterator first_r() NOTHROW { return rs_.first(); }
		const_r_iterator first_r() const NOTHROW { return rs_.first(); }

		l_iterator last_l() NOTHROW { return ls_.last(); }
		const_l_iterator last_l() const NOTHROW { return ls_.last(); }
		r_iterator last_r() NOTHROW { return rs_.last(); }
		const_r_iterator last_r() const NOTHROW { return rs_.last(); }


		int64 size() const NOTHROW
			{ return ls_.size(); } // == rs_.size()
		bool empty() const NOTHROW
			{ return ls_.empty(); } // == rs_.empty()


		/**
		 * Convert incoming key to opposite key of pair.
		 * Throws if the map does not contain the pair.
		 */
		//@{
		L& l_for_r(const R& r)
		{
			l_iterator l_it(find_l_for_r(r));
			if (!l_it) throw invalid_parameter();
			return *l_it;
		}

		const L& l_for_r(const R& r) const
		{
			const_l_iterator l_it(find_l_for_r(r));
			if (!l_it) throw invalid_parameter();
			return *l_it;
		}

		R& r_for_l(const L& l)
		{
			r_iterator r_it(find_r_for_l(l));
			if (!r_it) throw invalid_parameter();
			return *r_it;
		}

		const R& r_for_l(const L& l) const
		{
			const_r_iterator r_it(find_r_for_l(l));
			if (!r_it) throw invalid_parameter();
			return *r_it;
		}
		//@}


		/**
		 * Retrieve iterator to existing key within bimap,
		 * or return past-end iterator if key is not found.
		 */
		//@{
		l_iterator find_l(const L& l) NOTHROW
			{ return ls_.find(l); }
		const_l_iterator find_l(const L& l) const NOTHROW
			{ return ls_.find(l); }
		r_iterator find_r(const R& r) NOTHROW
			{ return rs_.find(r); }
		const_r_iterator find_r(const R& r) const NOTHROW
			{ return rs_.find(r); }
		//@}


		/**
		 * Find other part of pair, or return past-end iterator
		 * if the bimap does not contain the pair.
		 */
		//@{
		l_iterator find_l_for_r(const R& r) NOTHROW
		{
			r_iterator r_it(find_r(r)); // r_it valid => find(r_it) valid
			return r_it ? r_to_l_.find(r_it)->value : l_iterator();
		}

		const_l_iterator find_l_for_r(const R& r) const NOTHROW
		{
			const_r_iterator r_it(find_r(r)); // r_it valid => find(r_it) valid
			return r_it ? r_to_l_.find(r_it)->value : const_l_iterator();
		}

		r_iterator find_r_for_l(const L& l) NOTHROW
		{
			l_iterator l_it(find_l(l)); // l_it valid => find(l_it) valid
			return l_it ? l_to_r_.find(l_it)->value : r_iterator();
		}

		const_r_iterator find_r_for_l(const L& l) const NOTHROW
		{
			const_l_iterator l_it(find_l(l)); // l_it valid => find(l_it) valid
			return l_it ? l_to_r_.find(l_it)->value : const_r_iterator();
		}
		//@}


		/**
		 * Remove a pair @bref{l}, @bref{r} from the bimap.
		 * Ignored if the bimap does not contain @bref{l} or @bref{r}.
		 */
		//@{
		void remove_l(const L& l) NOTHROW
			{ remove_pair(find_l(l), find_r_for_l(l)); }
		void remove_r(const R& r) NOTHROW
			{ remove_pair(find_l_for_r(r), find_r(r)); }
		//@}


		bool contains_l(const L& l) const NOTHROW
			{ return find_l(l); }
		bool contains_r(const R& r) const NOTHROW
			{ return find_r(r); }


		void swap_with(bimap& other) NOTHROW
		{
			swap(ls_, other.ls_);
			swap(rs_, other.rs_);
			swap(l_to_r_, other.l_to_r_);
			swap(r_to_l_, other.r_to_l_);
		}


		void clear() NOTHROW
		{
			ls_.clear();
			rs_.clear();
			l_to_r_.clear();
			r_to_l_.clear();
		}


	private:

		void remove_pair
			(l_iterator l_it, r_iterator r_it) NOTHROW
		{
			if (!l_it || !r_it) return; // NOTHROW

			l_to_r_.remove(l_it);
			r_to_l_.remove(r_it);

			ls_.remove(l_it);
			rs_.remove(r_it);
		}


		l_set ls_;
		r_set rs_;

		typedef tree_map<const_l_iterator, r_iterator, allocator_type>
			l_to_r_map;
		l_to_r_map l_to_r_;

		typedef tree_map<const_r_iterator, l_iterator, allocator_type>
			r_to_l_map;
		r_to_l_map r_to_l_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BIMAP_HPP) */
