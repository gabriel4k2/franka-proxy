/**
 *************************************************************************
 *
 * @file array.hpp
 *
 * Array containers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__ARRAY_HPP)
#define INCLUDED__VIRAL_CORE__ARRAY_HPP


#include "build_config.hpp"


#include <new>


#include "util.hpp"
#include "memory.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_checks.hpp"
#include "integer_operations.hpp"
#include "auto_pointer.hpp"
#include "container_limits.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class array
 * @ingroup container_module
 *
 * Stores a tightly packed sequence of an arbitrary type @bref{T}
 * in dynamically allocated memory.
 *
 * Usage notes:
 * - An array properly constructs and destroys all contained objects.
 *		Hence, the array works with an arbitrary object type @bref{T}.
 *		In constrast, a dynamic_buffer provides improved performance,
 *		but only supports intrinsic or POD types.
 * - Internally, the array may arbitrarily invoke copy constructors
 *		and destructors of type @bref{T}. For instance, when an array
 *		exceeds its capacity(), it transfers contained objects by copy,
 *		and then destroys the originals.
 * - Arrays support types @bref{T} without a default constructor,
 *		but then calls to some methods (e.g. resize()) do not compile.
 * - Even on an exception, capacity() and entry pointers may change.
 *		Apart from this, all array calls have strong exception safety.
 *
 ************************************************************************/
template <typename T>
	class array
{

	public:

		array() NOTHROW
			: data_(NULL), size_(0), capacity_(0) { }


		array(const T* data, int64 count)
			: data_(NULL), size_(0), capacity_(0)
		{
			// Other validation (e.g. for maximum size)
			// is performed by reserve() and push_back().
			if (count > 0 && !data)
				throw null_pointer();


			array offhand_copy;
			offhand_copy.reserve(count);

			for (int64 i = 0; i < count; ++i)
				offhand_copy.push_back(data[i]);

			swap(*this, offhand_copy);
		}


		array(std::initializer_list<T> initializer)
			: data_(NULL), size_(0), capacity_(0)
		{
			array offhand_copy;
			offhand_copy.reserve
				(integer_operations::cast_or_throw<int64>
					(initializer.size()));

			for (auto it(initializer.begin()); it != initializer.end(); ++it)
				offhand_copy.push_back(std::move(*it));

			swap(*this, offhand_copy);
		}


		/**
		 * Copy or move construct from @bref{other} array.
		 * Move construction leaves a valid, empty() source.
		 */
		//@{		
		array(const array& other)
			: data_(NULL), size_(0), capacity_(0)
		{
			array offhand_copy(other.data_, other.size_);
			swap(*this, offhand_copy);
		}

		array(array&& other) NOTHROW
			: data_(NULL), size_(0), capacity_(0)
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		~array() NOTHROW { free(); }


		/** Create not-@NULL copy of array on heap. */
		auto_pointer<array> clone() const
			{ return auto_pointer<array>(new array(*this)); }


		/**
		 * Copy or move assign of @bref{other} array.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty() source.
		 */
		//@{
		array& operator =(const array& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			array offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		array& operator =(array&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			array offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		/**
		 * Reserve storage so that at least @bref{request_capacity} entries
		 * fit into the array without later dynamic allocation.
		 *
		 * Provides strong exception safety.
		 */
		void reserve(int64 request_capacity)
		{
			if (request_capacity < 0)
				throw invalid_parameter();
			if (request_capacity > container_limits<T>::max_size)
				throw bounds_exceeded();

	
			// Early out if we have enough storage.
			if (request_capacity <= capacity_)
				return;


			// Extrapolate future capacity requirements based on current capacity.
			// This avoids frequent reallocations on many push_back() operations.
			// Additionally clamp extrapolation to container size limits.
			int64 grow_capacity =
				capacity_ / 2; // Never overflows.

			int64 extrapolate_capacity =
				(!integer_checks::is_add_safe(capacity_, grow_capacity) ||
				 capacity_ + grow_capacity > container_limits<T>::max_size) ?
					container_limits<T>::max_size :
					(capacity_ + grow_capacity);
				

			// Directly use capacity request if it exceeds the extrapolation,
			// and use a fixed lower capacity limit to avoid dense reallocations
			// on the first few entries.
			int64 new_capacity =
				max(32, max(extrapolate_capacity, request_capacity));


			// Allocate new storage and copy over objects.
			// We must ensure strong exception safety here.			
			T* new_data =
				memory::alloc_typed<T>(new_capacity);

			SCOPE_GUARD
				(new_data_guard, (&memory::free, new_data));

			int64 copied_objects = 0;
			SCOPE_GUARD
				(new_objects_guard,
					(&destructor_helper,
					 new_data,
					 guard_by_reference(copied_objects)));

			for (int64 i = 0; i < size_; ++i)
			{
				new(new_data + i) T(data_[i]);
				++copied_objects;
			}


			// Copy finished successfully.
			// Delete old objects, then switch to new storage.
			// Exception safety: NOTHROW from here.
			new_objects_guard.dismiss();
			new_data_guard.dismiss();

			destructor_helper(data_, size_);
			memory::free(data_);

			data_ = new_data;
			capacity_ = new_capacity;			
		}


		/**
		 * Add default-constructed entries or destroy entries
		 * at the end of the array up to the given @bref{new_size}.
		 *
		 * Does not throw on its own if at least @bref{new_size} entries
		 * have already been reserve()d successfully.
		 *
		 * @attent T::T() can throw even on a properly reserve()d array.
		 */
		void resize(int64 new_size)
		{
			if (new_size < 0)
				throw invalid_parameter();
			if (new_size > container_limits<T>::max_size)
				throw bounds_exceeded();


			// Early out if array already has requested size.
			if (new_size == size_)
				return;


			// Shrink array.
			// Entirely NOTHROW in this case.
			if (new_size < size_)
			{
				destructor_helper
					(data_ + new_size, size_ - new_size);

				size_ = new_size;
				return;
			}


			// Here, new_size > size_, so create new elements.
			// Call destructors on any exception, but keep capacity.
			reserve(new_size);

			int64 created_objects = 0;
			SCOPE_GUARD
				(new_objects_guard,
					(&destructor_helper,
					 data_ + size_,
					 guard_by_reference(created_objects)));

			for (int64 i = size_; i < new_size; ++i)
			{
				new(data_ + i) T();
				++created_objects;
			}


			// NOTHROW from here.
			new_objects_guard.dismiss();
			size_ = new_size;
		}

	
		/**
		 * Insert or destroy a single entry at the end of the array.
		 * A call to pop_back() is ignored if the array is empty().
		 */
		//@{
		void push_back(const T& value)
		{
			reserve(size_ + 1);
			new(data_ + size_) T(value);
			++size_;
		}

		void push_back(T&& value)
		{
			reserve(size_ + 1);
			new(data_ + size_) T(std::move(value));
			++size_;
		}

		void pop_back() NOTHROW
		{
			if (size_ <= 0) return;

			// No resize() here to avoid path
			// to default constructor T().
			data_[size_ - 1].~T();
			--size_;
		}
		//@}


		/**
		 * Replace array contents by copying @bref{count} entries
		 * from the provided input @bref{data} array.
		 *
		 * @attent In order to realize strong exception safety,
		 * this call discards any previously reserve()d memory.
		 * This implies that copy_data() may always throw.
		 */
		void copy_data(const T* data, int64 count)
		{
			array offhand_copy(data, count);
			swap(*this, offhand_copy);
		}


		/**
		 * Pointer to contained data,
		 * or an undefined value if the array is empty.
		 */
		//@{
		const T* data() const NOTHROW { return data_; }
		T* data() NOTHROW { return data_; }
		//@}


		/** Checked entry access, throws on invalid @bref{index}. */
		//@{
		const T& at(int64 index) const
		{
			if (index < 0 || index >= size_) throw invalid_parameter();
			return data_[index];
		}
		T& at(int64 index)
		{
			if (index < 0 || index >= size_) throw invalid_parameter();
			return data_[index];
		}
		//@}


		/** Unchecked entry access. */
		//@{
		const T& operator[](int64 index) const NOTHROW { return data_[index]; }
		T& operator[](int64 index) NOTHROW { return data_[index]; }	
		//@}


		int64 size() const NOTHROW { return size_; }
		bool empty() const NOTHROW { return size_ == 0; }
		int64 capacity() const NOTHROW { return capacity_; }


		/**
		 * Iterators for limited STL compatibility.
		 * @attent See @ref container_module for limitations.
		 */
		//@{
		T* begin() NOTHROW { return data_; }
		T* end() NOTHROW { return data_ + size_; }
		const T* begin() const NOTHROW { return data_; }
		const T* end() const NOTHROW { return data_ + size_; }
		//@}


		void swap_with(array& other) NOTHROW
		{
			swap(data_, other.data_);
			swap(size_, other.size_);
			swap(capacity_, other.capacity_);
		}


		/** Destroy array contents, but keep any dynamic memory. */
		void clear() NOTHROW
		{
			destructor_helper(data_, size_);
			size_ = 0;
		}


		/** Destroy array contents and release any dynamic memory. */
		void free() NOTHROW
		{
			destructor_helper(data_, size_);
			size_ = 0;

			memory::free(data_);
			data_ = NULL;
			capacity_ = 0;
		}


	private:

		static void destructor_helper
			(T* objects, int64 object_count)
		{
			/**
			 * Call destructor on @bref{object_count} @bref{objects}
			 * in inverse order but do not free array memory.
			 */

			for (int64 i = object_count - 1; i >= 0; --i)
				objects[i].~T();
		}


		T* data_;
		int64 size_;
		int64 capacity_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__ARRAY_HPP) */
