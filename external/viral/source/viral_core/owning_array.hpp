/**
 *************************************************************************
 *
 * @file owning_array.hpp
 *
 * Heap-owning array.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__OWNING_ARRAY_HPP)
#define INCLUDED__VIRAL_CORE__OWNING_ARRAY_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "scope_guard.hpp"
#include "buffer.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup owning_module Owning Module
 * @ingroup container_module
 *
 * Owning containers store pointers to heap objects and delete these
 * automatically once the container is destroyed.
 *
 * Owning container usage notes:
 * - The contained object type @bref{T} must support viral_core::clone()
 *		to copy-construct or clone respective owning containers.
 *		There are no further requirements on @bref{T}.
 * - It is explicitly allowed to replace pointers already stored within
 *		an owning container, for example via data pointers or iterators.
 *		However, clients must be aware of ownership issues and must
 *		delete or transfer old pointers before overwriting.
 * - Constant owning containers prevent clients from changing pointers,
 *		but do not propagate constness to contained objects. Notably,
 *		clients can still call non-const methods on objects inside
 *		a constant owning container. If required, clients must hide
 *		the entire container to achieve const-correctness.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class owning_array
 * @ingroup owning_module
 *
 * Owns indexed heap objects, all at least of type @bref{T},
 * via tightly-packed pointers.
 *
 * Notes:
 * - See the @ref owning_module "Owning Module" for general usage.
 * - The pointers() within the owning_array may move in memory
 *		whenever size() increases, even in case of an exception.
 *		Therefore, all pointers to stored pointers become invalid.
 *
 ************************************************************************/
template <typename T>
	class owning_array
{

	public:

		owning_array() NOTHROW { }


		/**
		 * Copy or move construct from @bref{other} owning_array.
		 * Move construction leaves a valid, empty source.
		 */
		//@{	
		owning_array(const owning_array& other)
		{
			// On any exception, ~owning_array() will not run.
			// While ~dynamic_buffer() releases any pointer memory,
			// we still must explicitly free any owned objects.
			SCOPE_GUARD
				(mem_guard, (*this, &owning_array::free));	
	
			resize(other.size());

			for (int64 i = 0; i < size(); ++i)
			{
				if (other[i])
				{
					pointers_[i] =
						other[i]->clone().release_ownership();
				}
			}


			// NOTHROW from here.
			mem_guard.dismiss();
		}

		owning_array(owning_array&& other) NOTHROW
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		~owning_array() NOTHROW { free(); }


		/** Create copy of owning_array and contents on heap. */
		auto_pointer<owning_array> clone() const
			{ return auto_pointer<owning_array>(new owning_array(*this)); }


		/**
		 * Copy or move assign from @bref{other} owning_array.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty source.
		 */
		//@{
		owning_array& operator =(const owning_array& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			owning_array offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		owning_array& operator =(owning_array&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			owning_array offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		void swap_with(owning_array& other) NOTHROW
			{ swap(pointers_, other.pointers_); }


		/**
		 * Reserve enough storage space so that at least @bref{new_capacity}
		 * pointers fit into the array without later dynamic allocation.
		 * Provides strong exception safety.
		 */
		void reserve(int64 new_capacity)
			{ pointers_.reserve(new_capacity); }


		/**
		 * Add @NULL pointers or destroy objects at the end of the array
		 * up to the given @bref{new_size}. Does not throw if at least
		 * @bref{new_size} pointers have already been reserve()d successfully.
		 */
		void resize(int64 new_size)
		{
			int64 old_size = size();


			// Shrink array and delete objects.
			// Need to check for invalid new_size here,
			// just in case user tries to shrink past zero.
			if (new_size < old_size)
			{
				if (new_size < 0)
					throw invalid_parameter();

				for (int64 i = new_size; i < old_size; ++i)
				{
					if (pointers_[i])
					{
						delete pointers_[i];
						pointers_[i] = NULL;
					}
				}
				
				pointers_.resize(new_size); // NOTHROW here.
			}


			// Grow array with new NULL pointers.
			// dynamic_buffer::resize() validates new_size.
			if (old_size < new_size)
			{
				pointers_.resize(new_size);

				// Reset pointers to NULL.
				// The dynamic_buffer won't do this for us.
				// NOTHROW from here.
				for (int64 i = old_size; i < new_size; ++i)
					pointers_[i] = NULL;
			}
		}

	
		/**
		 * Take ownership over (potentially @NULL) object
		 * and insert at the end of the array.
		 * Destroys the object on any exception.
		 */
		//@{
		void insert(auto_pointer<T> object)
		{
			resize(size() + 1);

			// NOTHROW from here.
			pointers_[size() - 1] = object.release_ownership();
		}
		
		void push_back(auto_pointer<T> object)
			{ insert(std::move(object)); }
		//@}


		/**
		 * Destroy a single object at the end of the array.
		 * pop_back() is ignored if the array is empty().
		 */
		void pop_back() NOTHROW
			{ if (size() > 0) resize(size() - 1); }


		/**
		 * Pointer to the array of pointers, or undefined result
		 * if the owning_array is empty().
		 */
		//@{
		T* const* data() const NOTHROW { return pointers_.data(); }
		T** data() NOTHROW { return pointers_.data(); }
		//@}


		/** Checked pointer access, throws on invalid @bref{index}. */
		//@{
		T* const& at(int64 index) const { return pointers_.at(index); }
		T*& at(int64 index) { return pointers_.at(index); }
		//@}


		/** Unchecked pointer access. */
		//@{
		T* const& operator[](int64 index) const NOTHROW { return pointers_[index]; }
		T*& operator[](int64 index) NOTHROW { return pointers_[index]; }	
		//@}


		int64 size() const NOTHROW { return pointers_.size(); }
		bool empty() const NOTHROW { return pointers_.empty(); }
		int64 capacity() const NOTHROW { return pointers_.capacity(); }


		/**
		 * Release or delete single object at given @bref{index}.
		 * Ordered variant requires O(n), unordered variant is O(1).
		 * Calls silently ignore invalid @bref{indices} for @ref NOTHROW.
		 * Release variants return @NULL on invalid indices.
		 */
		//@{
		auto_pointer<T> release_ordered(int64 index) NOTHROW
		{
			if (index < 0 || index >= pointers_.size())
				return auto_pointer<T>();

			for (int64 i = index + 1; i < pointers_.size(); ++i)
				swap(pointers_[i - 1], pointers_[i]);
	
			auto_pointer<T> result(pointers_[pointers_.size() - 1]);
			pointers_.resize(pointers_.size() - 1); // NOTHROW here.
			return result;
		}

		auto_pointer<T> release_unordered(int64 index) NOTHROW
		{
			if (index < 0 || index >= pointers_.size())
				return auto_pointer<T>();

			swap(pointers_[index], pointers_[pointers_.size() - 1]);

			auto_pointer<T> result(pointers_[pointers_.size() - 1]);
			pointers_.resize(pointers_.size() - 1); // NOTHROW here.
			return result;
		}

		void remove_ordered(int64 index) NOTHROW
			{ release_ordered(index); }
		void remove_unordered(int64 index) NOTHROW
			{ release_unordered(index); }
		//@}


		/**
		 * Destroy owned objects and reset size,
		 * but keep pointer memory.
		 */
		void clear() NOTHROW
		{
			for (int64 i = 0; i < pointers_.size(); ++i)
				if (pointers_[i]) delete pointers_[i];

			pointers_.clear();
		}


		/** Destroy owned objects and free pointer memory. */
		void free() NOTHROW
		{
			clear();
			pointers_.free();
		}


	private:

		dynamic_buffer<T*> pointers_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__OWNING_ARRAY_HPP) */
