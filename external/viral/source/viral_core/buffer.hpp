/**
 *************************************************************************
 *
 * @file buffer.hpp
 *
 * Header for dynamic buffers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BUFFER_HPP)
#define INCLUDED__VIRAL_CORE__BUFFER_HPP


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
 * @class dynamic_buffer
 * @ingroup container_module
 *
 * Stores a tightly packed sequence of an intrinsic or POD
 * data type @bref{T} in dynamically allocated memory.
 *
 * Notes:
 * - No constructors or destructors are called for stored objects.
 *		Contents of newly allocated objects initially are undefined.
 *		Likewise, all stored objects may be moved in memory without
 *		explicit calls to the assignment operator or copy constructor.
 * - By standard, the above requirements force @bref{T} to be POD or
 *		intrinsic, but on most (possibly all) compilers, even less
 *		restricted, struct-like types (i.e. with POD-only fields, but
 *		methods and constructors) work. Use an @ref viral_core::array
 *		for all types that do not meet these requirements.
 * - Even on an exception, capacity() and entry pointers may change.
 *		Apart from this, all buffer calls have strong exception safety.
 *
 ************************************************************************/
template <typename T>
	class dynamic_buffer
{
	template <typename T> friend class dynamic_buffer;


	public:

		dynamic_buffer() NOTHROW
			: data_(NULL), size_(0), capacity_(0) { }


		dynamic_buffer(const T* data, int64 count)
			: data_(NULL), size_(0), capacity_(0)
		{
			// Other validation (e.g. for maximum size)
			// is performed by reserve() and copy_block_typed().
			if (count > 0 && !data)
				throw null_pointer();


			dynamic_buffer offhand_copy;
			offhand_copy.reserve(count);
			memory::copy_block_typed
				(offhand_copy.data_, data, count);
			offhand_copy.size_ = count;

			swap(*this, offhand_copy);
		}


		dynamic_buffer(std::initializer_list<T> initializer)
			: data_(NULL), size_(0), capacity_(0)
		{
			dynamic_buffer offhand_copy;
			offhand_copy.reserve
				(integer_operations::cast_or_throw<int64>
					(initializer.size()));

			for (auto it(initializer.begin()); it != initializer.end(); ++it)
				offhand_copy.push_back(std::move(*it));

			swap(*this, offhand_copy);
		}


		/**
		 * Copy or move construct from @bref{other} dynamic_buffer.
		 * Move construction leaves a valid, empty() source.
		 */
		//@{		
		dynamic_buffer(const dynamic_buffer& other)
			: data_(NULL), size_(0), capacity_(0)
		{
			dynamic_buffer offhand_copy(other.data_, other.size_);
			swap(*this, offhand_copy);
		}

		dynamic_buffer(dynamic_buffer&& other) NOTHROW
			: data_(NULL), size_(0), capacity_(0)
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		~dynamic_buffer() NOTHROW { free(); }


		/** Create not-@NULL copy of dynamic_buffer on heap. */
		auto_pointer<dynamic_buffer> clone() const
			{ return auto_pointer<dynamic_buffer>(new dynamic_buffer(*this)); }


		/**
		 * Copy or move assign of @bref{other} buffer.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty() source.
		 */
		//@{
		dynamic_buffer& operator =(const dynamic_buffer& other)
		{		
			if (&other == this) return *this;

			// Strong exception safety.
			dynamic_buffer offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		dynamic_buffer& operator =(dynamic_buffer&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			dynamic_buffer offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		/**
		 * Reinterpret and move data to another buffer
		 * of entry type @bref{U} as by reinterpret_cast,
		 * then reset original buffer to a free()d state.		
		 *
		 * @attent
		 * Throws if contents of the buffers cannot be compatible,
		 * i.e. if the byte size of @bref{this} is not a multiple
		 * of the byte size of @bref{U}.
		 */
		template <typename U>
			dynamic_buffer<U> reinterpret_and_move()
		{
			// Implementation notes:
			// - Below size conversions have no overflow,
			//		as limited by container_limits::max_bytes.
			// - Arithmetic order is vital to avoid truncation
			//		on integer divides.

			if ((size_ * sizeof(T)) % sizeof(U) != 0)
				throw invalid_state();

			// Perform actual move.
			// NOTHROW from here.

			dynamic_buffer<U> result;

			result.data_ = reinterpret_cast<U*>(data_);
			result.size_ = (size_ * size_of<T>()) / size_of<U>();
			result.capacity_ = (capacity_ * size_of<T>()) / size_of<U>();

			data_ = NULL;		
			size_ = 0;
			capacity_ = 0;

			return std::move(result);
		}


		/**
		 * Pure binary test for buffer content equality or inequality.
		 * @attent Does not invoke the comparison operators of @bref{T}.
		 */
		//@{
		bool operator ==(const dynamic_buffer& other) NOTHROW
		{
			if (size_ != other.size_) return false;
			return memory::are_blocks_equal_typed(data_, other.data_, size_);
		}

		bool operator !=(const dynamic_buffer &other) NOTHROW
			{ return !operator ==(other); }
		//@}


		/**
		 * Reserve storage so that at least @bref{request_capacity} entries
		 * fit into the dynamic_buffer without later dynamic allocation.
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


			// Reallocate to target capacity.
			data_ =
				memory::realloc_typed(data_, new_capacity);

			capacity_ = new_capacity;
		}


		/**
		 * Add uninitialized entries or deallocate entries
		 * at the end of the buffer up to the given @bref{new_size}.
		 *
		 * Does not throw if at least @bref{new_size} entries
		 * have already been reserve()d successfully.
		 */
		void resize(int64 new_size)
		{
			reserve(new_size);
			size_ = new_size;
		}

	
		/**
		 * Insert or destroy a single entry at the end of the buffer.
		 * A call to pop_back() is ignored if the buffer is empty().
		 */
		//@{
		void push_back(const T& value)
		{
			reserve(size_ + 1);
			data_[size_] = value;
			++size_;
		}

		void pop_back() NOTHROW
		{
			if (size_ > 0)
				--size_;
		}
		//@}


		/**
		 * Remove a single entry at a potentially invalid @bref{index},
		 * either with or without maintaining entry order.
		 *
		 * Notes:
		 * - The call ignores indices not in [0, size() - 1].
		 * - When maintaining order, each entry to the right of @bref{index}
		 *		shifts left by one through the assignment operator.
		 * - When not maintaining order, the removed entry is replaced
		 *		by the last buffer entry via the assignment operator.
		 * - @attent The NOTHROW guarantee requires a NOTHROW assignment
		 *		operator on @bref{T}.
		 */
		//@{
		void remove_ordered(int64 index) NOTHROW
		{
			if (index < 0 || index >= size_) return;

			for (int64 i = index + 1; i < size_; ++i)
				data_[i - 1] = data_[i];

			--size_;
		}

		void remove_unordered(int64 index) NOTHROW
		{
			if (index < 0 || index >= size_) return;
			data_[index] = data_[size_ - 1];
			--size_;
		}
		//@}


		/**
		 * Replace buffer contents by copying @bref{count} entries
		 * from the provided input @bref{data} array.
		 *
		 * @attent In order to realize strong exception safety,
		 * this call discards any previously reserve()d memory.
		 * This implies that copy_data() may always throw.
		 */
		void copy_data(const T* data, int64 count)
		{
			dynamic_buffer offhand_copy(data, count);
			swap(*this, offhand_copy);
		}


		/**
		 * Pointer to contained data,
		 * or an undefined value if the buffer is empty.
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


		void swap_with(dynamic_buffer& other) NOTHROW
		{
			swap(data_, other.data_);
			swap(size_, other.size_);
			swap(capacity_, other.capacity_);
		}


		/** Remove all entries from buffer, but keep any dynamic memory. */
		void clear() NOTHROW { size_ = 0; }


		/** Clear buffer contents and release any dynamic memory. */
		void free() NOTHROW
		{
			size_ = 0;

			memory::free(data_);
			data_ = NULL;
			capacity_ = 0;
		}


	private:

		T* data_;
		int64 size_;
		int64 capacity_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BUFFER_HPP) */

