/**
 *************************************************************************
 *
 * @file deque_buffer.hpp
 *
 * Deques based on list%s of dynamic_buffer%s.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__DEQUE_BUFFER_HPP)
#define INCLUDED__VIRAL_CORE__DEQUE_BUFFER_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "memory.hpp"
#include "exception.hpp"
#include "integer_checks.hpp"
#include "buffer.hpp"
#include "list.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class deque_buffer_block
 * @ingroup container_module
 *
 * A single data block in a deque_buffer.
 * 
 * Implementation notes:
 * - Whereas @bref{start_} is the index of the first valid entry
 *		in @bref{buffer_}, @bref{end_} is the index of the first
 *		invalid entry (i.e. size is @bref{end_} - @bref{start_}).
 *
 ************************************************************************/
template <typename T>
	class deque_buffer_block
{
	friend class deque_buffer<T>;


	private:

		deque_buffer_block
			(int64 block_size,
			 int64 start, int64 end)
			:
			start_(start),
			end_(end)
		{
			buffer_.resize(block_size);
		}


		dynamic_buffer<T> buffer_;
		int64 start_;
		int64 end_;				
};




/**
 *************************************************************************
 *
 * @class deque_buffer
 * @ingroup container_module
 *
 * A double-ended queue of an intrinsic or PoD type @bref{T}.
 *
 * Usage notes:
 * - In contrast to dynamic_buffer, inserting and removing an entry
 *		at the front of a deque_buffer requires but constant time
 *		and does not move existing buffer entries.
 * - The deque_buffer uses a list of dynamic_buffer%s as storage.
 *		Therefore, all limitations of dynamic_buffer%s in regards
 *		to intrinsic or PoD types apply for deque_buffer%s as well,
 *		but deque_buffer entries need not be continuous in memory.
 *
 * Block sizes:
 * - Clients should select a suitable @bref{block_size} based on the size
 *		of common push_front() and push_back() operations. When in doubt,
 *		a larger block size is preferrable: Both too small and too large
 *		block sizes cause memory overhead, but small blocks additionally
 *		imply frequent splitting of data, which costs performance.
 * - If no explicit @bref{block_size} is given, the deque_buffer uses
 *		the @bref{default_block_size}.
 * - Block sizes are not validated on construction due to @ref NOTHROW,
 *		but instead are checked when adding elements to the deque_buffer.
 *
 * Implementation notes:
 * - Const @bref{block_size} prohibits assignment of deque buffers.
 *		If assignment is required, clients should wrap deque buffers
 *		(e.g. into auto_pointer%s or shared_instance%s).
 *		
 ************************************************************************/
template <typename T>
	class deque_buffer 
{

	public:

		deque_buffer() NOTHROW
			:
			block_size(default_block_size),
			size_(0)
		{ }
		

		deque_buffer(int64 block_size) NOTHROW
			:
			block_size(block_size),
			size_(0)
		{ }

	
		deque_buffer
			(const T* data, int64 count,
			 int64 block_size)
			:
			block_size(block_size),
			size_(0)
		{
			// Strong exception safety in push_back().
			push_back(data, count);
		}


		/**
		 * Copy or move construct from @bref{other} deque_buffer.
		 * Move construction leaves a valid, empty() source.
		 */
		//@{
		deque_buffer(const deque_buffer& other)
			:
			block_size(other.block_size),
			blocks_(other.blocks_),
			size_(other.size_)
		{ }


		deque_buffer(deque_buffer&& other) NOTHROW
			:
			block_size(other.block_size),
			blocks_(std::move(other.blocks_)),
			size_(other.size_)
		{
			// Ensure empty(), but not defaulted other.
			// We cannot swap due to const block_size.
			other.size_ = 0;
		}
		//@}


		/** Create not-@NULL copy of deque_buffer on heap. */
		auto_pointer<deque_buffer> clone() const
			{ return auto_pointer<deque_buffer>(new deque_buffer(*this)); }


		/**
		 * Change the size() of the deque_buffer to a @bref{new_size}
		 * by adding or removing entries on either side of the deque.
		 * The deque_buffer remains unchanged on any exceptions.
		 *
		 * @attent New entries have random contents.
		 */
		//@{
		void resize_front(int64 new_size)
		{
			if (new_size < 0)
				throw invalid_parameter();
			if (new_size > container_limits<T>::max_size)
				throw bounds_exceeded();


			// Early out if size does not change.
			if (new_size == size_) return;


			// Early out on shrinking the deque buffer.
			if (new_size < size_)
			{
				pop_front(size_ - new_size);
				return;
			}


			// Check block size here for NOTHROW constructor.
			if (block_size <= 0 || block_size > max_block_size)
				throw invalid_state(); 


			// Buffer must grow, i.e. new_size > size_.
			// Determine how much of the new data to place
			// in the existing start block and in new blocks.
			block_list::iterator
				existing_first_block(blocks_.first());

			int64 data_count_allocate =
				new_size - size_;
			int64 data_count_to_existing =
				existing_first_block ?
					min(data_count_allocate,
						existing_first_block->start_) : 0;
			int64 data_count_overflow =
				data_count_allocate - data_count_to_existing;


			// Allocate extra blocks for any overflow.
			// Both block allocation and splicing potentially might throw,
			// so rely on offhand copy and strongly exception safe splice.
			block_list new_blocks;

			while (data_count_overflow > 0)
			{
				int64 data_count_to_new_block =
					(data_count_overflow > block_size) ?
						block_size : data_count_overflow;

				new_blocks.push_front
					(deque_buffer_block<T>
						(block_size,
						 block_size - data_count_to_new_block,
						 block_size));

				data_count_overflow -= data_count_to_new_block;
			}

			blocks_.splice
				(block_list::iterator(), std::move(new_blocks));


			// Success.
			// Adjust start of former first block and total size.
			// Exception safety: NOTHROW from here.
			if (data_count_to_existing > 0)
				existing_first_block->start_ -= data_count_to_existing;

			size_ = new_size;
		}


		void resize_back(int64 new_size)
		{
			if (new_size < 0)
				throw invalid_parameter();
			if (new_size > container_limits<T>::max_size)
				throw bounds_exceeded();


			// Early out if size does not change.
			if (new_size == size_) return;


			// Early out on shrinking the deque buffer.
			if (new_size < size_)
			{
				pop_back(size_ - new_size);
				return;
			}


			// Check block size here for NOTHROW constructor.
			if (block_size <= 0 || block_size > max_block_size)
				throw invalid_state(); 


			// Buffer must grow, i.e. new_size > size_.
			// Determine how much of the new data to place
			// in the existing last block and in new blocks.
			block_list::iterator
				existing_last_block(blocks_.last());

			int64 data_count_allocate =
				new_size - size_;
			int64 data_count_to_existing =
				existing_last_block ?
					min(data_count_allocate,
						block_size - existing_last_block->end_) : 0;
			int64 data_count_overflow =
				data_count_allocate - data_count_to_existing;


			// Allocate extra blocks for any overflow.
			// Both block allocation and splicing potentially might throw,
			// so rely on offhand copy and strongly exception safe splice.
			block_list new_blocks;

			while (data_count_overflow > 0)
			{
				int64 data_count_to_block =
					(data_count_overflow > block_size) ?
						block_size : data_count_overflow;

				new_blocks.push_back
					(deque_buffer_block<T>
						(block_size, 0, data_count_to_block));

				data_count_overflow -= data_count_to_block;
			}
				
			blocks_.splice
				(blocks_.last(), std::move(new_blocks));


			// Success.
			// Adjust end of former last block and total size.
			// Exception safety: NOTHROW from here.
			if (data_count_to_existing > 0)
				existing_last_block->end_ += data_count_to_existing;

			size_ = new_size;
		}
		//@}


		/** Insert a single entry at either side of the deque. */
		//@{
		void push_front(const T& value)
			{ push_front(&value, 1); }
		void push_back(const T& value)
			{ push_back(&value, 1); }
		//@}


		/**
		 * Insert all @bref{count} entries of @bref{data}
		 * on either side of the deque.
		 */
		//@{
		void push_front(const T* data, int64 count)
		{
			if (count < 0 || (count > 0 && !data))
				throw invalid_parameter();

			if (!integer_checks::is_add_safe(size_, count) ||
				size_ + count > container_limits<T>::max_size)
					throw bounds_exceeded();


			// Create new, but uninitialized entries.
			resize_front(size_ + count);


			// Copy over data to front of deque.
			// Exception safety: NOTHROW from here.
			const T* src_data = data;
			int64 src_count = count;

			block_list::iterator dest_block(blocks_.first());

			while (src_count > 0)
			{
				int64 copy_count =
					min(src_count,
						dest_block->end_ - dest_block->start_);

				memory::copy_block_typed
					(dest_block->buffer_.data() + dest_block->start_,
					 src_data, copy_count);

				src_data += copy_count;
				src_count -= copy_count;

				++dest_block;
			}
		}


		void push_back(const T* data, int64 count)
		{
			if (count < 0 || (count > 0 && !data))
				throw invalid_parameter();

			if (!integer_checks::is_add_safe(size_, count) ||
				size_ + count > container_limits<T>::max_size)
					throw bounds_exceeded();


			// Create new, but uninitialized entries.
			resize_back(size_ + count);


			// Copy over data to back of deque.
			// Exception safety: NOTHROW from here.
			const T* src_data = data + count;
			int64 src_count = count;

			block_list::iterator dest_block(blocks_.last());

			while (src_count > 0)
			{
				int64 copy_count =
					min(src_count,
						dest_block->end_ - dest_block->start_);

				src_data -= copy_count;
				src_count -= copy_count;

				memory::copy_block_typed
					(dest_block->buffer_.data() + dest_block->end_ - copy_count,
					 src_data, copy_count);

				--dest_block;
			}
		}
		//@}


		/**
		 * Fetch a single entry from either side of the deque
		 * into the provided @bref{value}.
		 */
		//@{
		void fetch_front(T& value) const
			{ fetch_front(&value, 1, 0); }
		void fetch_back(T& value) const
			{ fetch_back(&value, 1, 0); }
		//@}

		
		/**
		 * Fetch @bref{count} entries from either side of the deque
		 * into a given @bref{data} array. The data array must have
		 * at least enough space for all requested data.
		
		 * Optionally, skip @bref{offset} entries from either side
		 * of the deque buffer before fetching. Note that skipping
		 * of large offsets may cause a search in the backing list,
		 * which leads to O(offset) time complexity.
		 *
		 * If there are less than @bref{count} plus @bref{offset} entries
		 * or if @bref{data}, @bref{count}, or @bref{offset} are invalid,
		 * then fetching throws and @bref{data} remains unchanged.
		 */
		//@{
		void fetch_front(T* data, int64 count) const
			{ fetch_front(data, count, 0); }
		void fetch_back(T* data, int64 count) const
			{ fetch_back(data, count, 0); }


		void fetch_front
			(T* data, int64 count, int64 offset) const
		{
			// Validate parameters.
			if (count < 0 || (count > 0 && !data))
				throw invalid_parameter();

			if (offset < 0)
				throw invalid_parameter();

			if (!integer_checks::is_add_safe(count, offset) ||
				count + offset > size_)
					throw bounds_exceeded();


			// Early out if not fetching anything.
			// This also is the only possible fetch for empty() deques.
			if (count == 0) return;


			// Find source block that contains our offset.
			// Due to above early out, we know src_block is never past-end.
			block_list::const_iterator src_block
				(blocks_.first());

			while (offset >= src_block->end_ - src_block->start_)
			{
				offset -= (src_block->end_ - src_block->start_);
				++src_block;
			}


			// Fetch data from source block and subsequent blocks.
			// Start at the first relevant block and at the start of the data array.
			while (count > 0)
			{
				int64 copy_count =
					min(count, src_block->end_ - src_block->start_ - offset);

				memory::copy_block_typed
					(data,
					 src_block->buffer_.data() + src_block->start_ + offset,
					 copy_count);

				data += copy_count;
				++src_block;
				offset = 0;

				count -= copy_count;
			}
		}


		void fetch_back
			(T* data, int64 count, int64 offset) const
		{
			// Validate parameters.
			if (count < 0 || (count > 0 && !data))
				throw invalid_parameter();

			if (offset < 0)
				throw invalid_parameter();

			if (!integer_checks::is_add_safe(count, offset) ||
				count + offset > size_)
					throw bounds_exceeded();


			// Early out if not fetching anything.
			// This also is the only possible fetch for empty() deques.
			if (count == 0) return;


			// Find source block that contains our offset.
			// Due to above early out, we know src_block is never past-end.
			block_list::const_iterator src_block
				(blocks_.last());

			while (offset >= src_block->end_ - src_block->start_)
			{
				offset -= (src_block->end_ - src_block->start_);
				--src_block;
			}


			// Fetch data from source block and subsequent blocks.
			// Start at the last relevant block and at the end of the data array.
			data = data + count;
			while (count > 0)
			{
				int64 copy_count = 
					min(count, src_block->end_ - src_block->start_ - offset);

				memory::copy_block_typed
					(data - copy_count,
					 src_block->buffer_.data() + src_block->end_ - copy_count - offset,
					 copy_count);

				data -= copy_count;
				--src_block;
				offset = 0;
				
				count -= copy_count;
			}
		}
		//@}
		

		/**
		 * Delete @bref{count} entries at either side of the deque.
		 * Deleting more than size() entries and invalid @bref{count}s
		 * are silently ignored to satisfy @ref NOTHROW.
		 */
		//@{
		void pop_front() NOTHROW { pop_front(1); }
		void pop_back() NOTHROW { pop_back(1); }

		void pop_front(int64 count) NOTHROW
		{
			if (count <= 0) return;

			size_ = max(0, size_ - count);

			while (count > 0)
			{
				// Abort if no block remains.
				// This happens when deleting more entries
				// than contained in the deque_buffer.
				if (blocks_.empty()) return;
			

				// Remove entries from the current front block:
				// Either remove all entries alongside the block,
				// or remove only part of the entries by moving
				// the starting index.
				deque_buffer_block<T>& block =
					*blocks_.first();
				
				int64 count_in_block =
					block.end_ - block.start_;

				if (count < count_in_block)
				{
					block.start_ += count;
					return;
				}

				blocks_.pop_front();
				count -= count_in_block;
			}
		}

		void pop_back(int64 count) NOTHROW
		{
			if (count <= 0) return;

			size_ = max(0, size_ - count);

			while (count > 0)
			{
				// Abort if no block remains.
				// This happens when deleting more entries
				// than contained in the deque_buffer.
				if (blocks_.empty()) return;
			

				// Remove entries from the current ending block:
				// Either remove all entries alongside the block,
				// or remove only part of the entries by moving
				// the ending index.
				deque_buffer_block<T>& block =
					*blocks_.last();
				
				int64 count_in_block =
					block.end_ - block.start_;

				if (count < count_in_block)
				{
					block.end_ -= count;
					return;
				}

				blocks_.pop_back();
				count -= count_in_block;
			}
		}
		//@}


		int64 size() const NOTHROW { return size_; }
		bool empty() const NOTHROW { return size_ == 0; }


		/** Number of allocated blocks. */
		int64 block_count() const NOTHROW { return blocks_.size(); }
		

		/**
		 * Fetch a pointer to the first (occupied) data entry
		 * in the first or last block, or return NULL if empty().
		 */
		//@{
		T* data_first_block() NOTHROW
		{
			if (blocks_.size() == 0) return NULL;
			deque_buffer_block<T>& first_block = *blocks_.first();
			return first_block.buffer_.data() + first_block.start_;
		}

		const T* data_first_block() const NOTHROW
		{
			if (blocks_.size() == 0) return NULL;
			const deque_buffer_block<T>& first_block = *blocks_.first();
			return first_block.buffer_.data() + first_block.start_;
		}

		T* data_last_block() NOTHROW
		{
			if (blocks_.size() == 0) return NULL;
			deque_buffer_block<T>& last_block = *blocks_.last();
			return last_block.buffer_.data() + last_block.start_;
		}

		const T* data_last_block() const NOTHROW
		{
			if (blocks_.size() == 0) return NULL;
			const deque_buffer_block<T>& last_block = *blocks_.last();
			return last_block.buffer_.data() + last_block.start_;
		}
		//@}		


		/**
		 * Return the number of entries in data_first_block()
		 * or data_last_block(), or return zero if empty().
		 */
		//@{
		int64 size_first_block() const NOTHROW
		{
			if (blocks_.size() == 0) return 0;
			const deque_buffer_block<T>& first_block = *blocks_.first();
			return first_block.end_ - first_block.start_;
		}

		int64 size_last_block() const NOTHROW
		{
			if (blocks_.size() == 0) return 0;
			const deque_buffer_block<T>& last_block = *blocks_.last();
			return last_block.end_ - last_block.start_;
		}
		//@}


		/**
		 * Return the number of unused, but allocated entries
		 * at either side of the deque_buffer, or zero if empty().
		 * Note, however, that blocks may have surplus entries
		 * at both sides (i.e. in a deque with a single block). 
		 */
		//@{
		int64 unused_at_start() const NOTHROW
		{
			if (blocks_.size() == 0) return 0;
			const deque_buffer_block<T>& first_block = *blocks_.first();
			return first_block.start_;
		}

		int64 unused_at_end() const NOTHROW
		{
			if (blocks_.size() == 0) return 0;
			const deque_buffer_block<T>& last_block = *blocks_.last();
			return block_size - last_block.end_;
		}
		//@}


		/**
		 * Search deque_buffer for a single @bref{e}ntry
		 * or a @bref{data} array of @bref{count} entries.
		 *
		 * Begin searching either at the front of the deque,
		 * or at a @bref{start_index} relative to the front.
		 * Invalid indices automatically and silently clamp
		 * to valid, but possibly past-end, indices.
		 *
		 * If the deque contains @bref{e} or @bref{data},
		 * then search returns an index to the first entry
		 * of the first match relative to the deque front.
		 *
		 * If @bref{e} or @bref{data} are not contained,
		 * search returns @bref{invalid_index}. Likewise,
		 * a @bref{count} of zero or less or a @NULL data
		 * result in @bref{invalid_index}.
		 *
		 * All variants have a O(size(deque) * size(other))
		 * worst-case runtime, but usually run in O(size(deque)).
		 *
		 * @attent For @ref NOTHROW, comparison of @bref{T}
		 * with operator != must not throw any exception.
		 */
		//@{
		int64 seek(const T& e) const NOTHROW
			{ return seek_array(&e, 1, 0); }
		int64 seek(const T& e, int64 start_index) const NOTHROW
			{ return seek_array(&e, 1, start_index); }

		int64 seek_array
			(const T* data, int64 count) const NOTHROW
				{ return seek_array(data, count, 0); }

		int64 seek_array
			(const T* data, int64 count, int64 start_index) const NOTHROW
		{
			if (!data || count <= 0)
				return invalid_index;

			if (size_ == 0 || size_ < count)
				return invalid_index;
			
			
			// Clamp starting index to nearest valid one.
			// Early-out on past-end starting index.
			int64 index_to_start =
					min(max(start_index, 0), size_);

			if (index_to_start == size_)
				return invalid_index;


			// Find starting block for first comparison pass 
			// and index to starting entry within this block.
			block_list::const_iterator pass_block(blocks_.first());
			int64 skip_in_pass_block = index_to_start;

			while (skip_in_pass_block >=
				   pass_block->end_ - pass_block->start_)
			{
				skip_in_pass_block -=
					(pass_block->end_ - pass_block->start_);

				++pass_block;
			}

			int64 index_in_pass_block =
				pass_block->start_ + skip_in_pass_block;


			// Comparison pass at each deque index i
			// that can possibly hold the search data.
			for (int64 i = index_to_start; i <= size_ - count;
				 ++i, ++index_in_pass_block)
			{


				// When the starting index of a comparison pass
				// has reached the end of a block, use next block.
				if (index_in_pass_block >= pass_block->end_)
				{
					++pass_block;
					index_in_pass_block = pass_block->start_;
				}


				// Check each entry of data against deque,
				// possibly passing multiple deque blocks.
				block_list::const_iterator cur_block(pass_block);
				int64 index_in_cur_block = index_in_pass_block;
				for (int64 j = 0; j < count;
					 ++j, ++index_in_cur_block)
				{
					if (index_in_cur_block >= cur_block->end_)
					{
						++cur_block;
						index_in_cur_block = cur_block->start_;
					}

					if (cur_block->buffer_[index_in_cur_block] != data[j])
						goto no_match;
				}

				return i;

				no_match:;
			}


			// Data not found in deque.
			return invalid_index;
		}
		//@}


		/**
		 * Not implemented: Due to const @bref{block_size},
		 * deque_buffer%s cannot be swapped.
		 */
		void swap_with(deque_buffer& other) NOTHROW;


		/** Remove all entries from deque_buffer. */
		void clear() NOTHROW
		{
			blocks_.clear();
			size_ = 0;
		}


		/** 
		 * Validate internal consistency of the deque_buffer.
		 * Clients usually need not call this.
		 */
		//@{
		void validate() const
		{
			// Check indices for block occupation.
			for (block_list::const_iterator it(blocks_.first()); it; ++it)
			{
				if (it->buffer_.size() != block_size)
					throw validation_failed();

				if (it->buffer_.size() <= 0 ||
					it->buffer_.size() > max_block_size)
						throw validation_failed();

				if (it->start_ >= it->end_)
					throw validation_failed();

				if (blocks_.size() != 1)
				{
					if (it != blocks_.first() &&
						it->start_ != 0)
							throw validation_failed();
	
					if (it != blocks_.last() &&
						it->end_ != block_size)
							throw validation_failed();
				}
			}


			// Check size over all block entries.
			int64 sum_size = 0;

			for (block_list::const_iterator it(blocks_.first()); it; ++it)
				sum_size += it->end_ - it->start_;

			if (sum_size != size_)
				throw validation_failed();

			if (sum_size < 0 || sum_size > container_limits<T>::max_size)
				throw validation_failed();
		}
		//@}


		/**
		 * Maximum number of entries allowed in a single block.
		 *
		 * @attent This limit is checked but at the first insert
		 * to allow a NOTHROW constructor.
		 */
		static const int64 max_block_size = 1073741824; 


		/** Default @bref{block_size} if not explicitly specified. */
		static const int64 default_block_size = 65536;


		/** Index for invalid deque entry. */
		static const int64 invalid_index = -1;


		/** Construction-provided block size in entries of type @bref{T}. */
		const int64 block_size;
		

	private:

		typedef list<deque_buffer_block<T>> block_list;
		block_list blocks_;
		int64 size_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__DEQUE_BUFFER_HPP) */
