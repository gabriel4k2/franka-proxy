/**
 *************************************************************************
 *
 * @file hash_base.hpp
 *
 * Base header for generic hash containers
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__HASH_BASE_HPP)
#define INCLUDED__VIRAL_CORE__HASH_BASE_HPP


#include "build_config.hpp"


#include "container_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "integer_checks.hpp"
#include "scope_guard.hpp"
#include "allocator_util.hpp"
#include "container_limits.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class hash_bucket
 * @ingroup container_module
 *
 * Stores a list of nodes of type @bref{N} that correspond
 * to some given hash code.
 *
 ************************************************************************/
template <typename N, typename A>
	class hash_bucket
{
	friend class hash_base<N, A>;

	private:

		N *first_node_;
		N *last_node_;
		bool is_first_bucket_;
		bool is_last_bucket_;
};




/**
 *************************************************************************
 *
 * @class hash_base
 * @ingroup container_module
 *
 * Basic load-balancing hash functionality for nodes of type @bref{N}
 * used both by hash_map and by hash_set.
 *
 * Implementation notes:
 * - hash_base does not directly know about the content of @bref{N}.
 *		Instead, all hash calculations and tests for equal contents
 *		are delegated to the container-specific node type @bref{N}.
 *
 ************************************************************************/
template <typename N, typename A>
	class hash_base
{
	template <typename K, typename A>
		friend class hash_set;
	template <typename K, typename V, typename A>
		friend class hash_map;


	public:

		~hash_base() NOTHROW { clear(); }


		void swap_with(hash_base &other) NOTHROW
		{
			swap(allocator_, other.allocator_);
			swap(buckets_, other.buckets_);
			swap(bucket_count_, other.bucket_count_);
			swap(size_, other.size_);
			swap(max_load_hint_, other.max_load_hint_);
		}


	private:

		typedef N node_type;
		typedef A allocator_type;

		typedef hash_bucket<N, A> bucket_type;


		hash_base() NOTHROW
			:
			buckets_(NULL),
			bucket_count_(0),
			size_(0),
			max_load_hint_(0.7f)
		{ }


		explicit hash_base(const allocator_type& allocator) NOTHROW
			:
			allocator_(allocator),
			buckets_(NULL),
			bucket_count_(0),
			size_(0),
			max_load_hint_(0.7f)
		{ }


		/**
		 * Copy or move construct from @bref{other} hash.
		 * Move construction leaves a valid, empty() source.
		 */
		//@{		
		hash_base(const hash_base& other)
			:
			allocator_(other.allocator_),
			buckets_(NULL),
			bucket_count_(0),
			size_(0),
			max_load_hint_(0.7f)
		{
			// Exception safety: Initialize offhand copy.
			// Destructor of copy cleans up on exception.
			hash_base offhand_copy;

			// Prepare suitable, final bucket count
			// to avoid redundant rehash() on insert().
			offhand_copy.apply_max_load_hint(other.max_load_hint_);
			offhand_copy.rehash(other.bucket_count_);

			for (int64 i = 0; i < other.bucket_count_; ++i)
			{
				const node_type* node =
					other.buckets_[i].first_node_;

				while (node)
				{
					uint64 hash_code =
						node->hash_content();

					node_type *new_node = 
						allocator_util::allocate_and_construct<node_type>
							(allocator_, *node);		

					offhand_copy.insert
						(new_node, hash_code);

					node = node->next_;
				}
			}

			swap(*this, offhand_copy);
		}

		hash_base(hash_base&& other) NOTHROW
			:
			allocator_(other.allocator_),
			buckets_(NULL),
			bucket_count_(0),
			size_(0),
			max_load_hint_(0.7f)
		{
			// Swap ensures defaulted other.
			swap(*this, other);
		}
		//@}


		/**
		 * Copy or move assign of @bref{other} hash.
		 * Copy assignment is strongly exception safe.
		 * Move assignment leaves a valid, empty() source.
		 */
		//@{
		hash_base& operator =(const hash_base& other)
		{
			if (&other == this) return *this;

			// Strong exception safety.
			hash_base offhand_copy(other);
			swap(*this, offhand_copy);
			return *this;
		}

		hash_base& operator =(hash_base&& other) NOTHROW
		{
			if (&other == this) return *this;

			// Exception safety: NOTHROW.
			hash_base offhand_move(std::move(other));
			swap(*this, offhand_move);
			return *this;
		}
		//@}


		static int64 hash_to_index
			(uint64 hash_code, int64 bucket_count) NOTHROW
		{
			/**
			 * Convert a @bref{hash_code} to a bucket index
			 * within any hash of given @bref{bucket_count}.
			 * At least a single bucket is required.
			 *
			 * Implementation notes:
			 * - Some secondary hashing is performed to protect
			 *		against bad incoming hash codes.
			 * - There can be no overflow: We wrap the hash code
			 *		to a positive int64 bucket_count, but do so
			 *		in uint64 range for defined modulo behaviour.
			 */

			return
				static_cast<int64>
					(((hash_code * 2147483647) ^ 2531011) %
					 static_cast<uint64>(bucket_count));
		}

		bucket_type* hash_to_bucket(uint64 hash_code) const NOTHROW
			{ return buckets_ + hash_to_index(hash_code, bucket_count_); }


		template <typename U>
			node_type* find(const U& key, uint64 hash_code) const NOTHROW
		{
			/**
			 * Find node with given @bref{key} in bucket
			 * for given @bref{hash_code}, or return @NULL
			 * if the key is not contained.
			 */

			if (bucket_count_ == 0) return NULL;

			N* search = hash_to_bucket(hash_code)->first_node_;
			while (search)
			{
				if (*search == key) return search;
				search = search->next_;
			}

			return NULL;
		}


		static node_type* previous(node_type* node) NOTHROW
		{
			/**
			 * Return previous node before @bref{node},
			 * or @NULL if the input node is @NULL
			 * or the first node in the hash.
			 */

			if (!node) return NULL;
			if (node->previous_) return node->previous_;

			// Finished current bucket, so search
			// for previous bucket with contents.
			bucket_type *bucket = node->bucket_;
			while (!bucket->is_first_bucket_)
			{
				--bucket;
				if (bucket->last_node_)
					return bucket->last_node_;
			}

			return NULL;
		}

		static node_type* next(node_type* node) NOTHROW
		{
			/**
			 * Return next node after @bref{node},
			 * or @NULL if the input node is @NULL
			 * or the last node in the hash.
			 */

			if (!node) return NULL;
			if (node->next_) return node->next_;

			// Finished current bucket, so search
			// for next bucket with contents.
			bucket_type *bucket = node->bucket_;
			while (!bucket->is_last_bucket_)
			{
				++bucket;
				if (bucket->first_node_)
					return bucket->first_node_;
			}

			return NULL;
		}


		node_type* first() const NOTHROW
		{
			for (int64 i = 0; i < bucket_count_; ++i)
			{
				if (buckets_[i].first_node_)
					return buckets_[i].first_node_;
			}

			return NULL;
		}

		node_type* last() const NOTHROW
		{
			for (int64 i = bucket_count_ - 1; i >= 0; --i)
			{
				if (buckets_[i].last_node_)
					return buckets_[i].last_node_;
			}

			return NULL;
		}


		void rehash(int64 bucket_count)
		{
			/**
			 * Rehash to the given, valid @bref{bucket_count}.
			 * Ignored on hashes that already have enough buckets.
			 * Strongly exception safe.
			 */


			// Allocate any new buckets,
			// or abort if there are enough buckets.
			if (bucket_count <= bucket_count_)
				return;

			bucket_type* new_buckets =
				allocator_util::allocate<bucket_type>
					(allocator_, bucket_count);


			// Exception safety:
			// NOTHROW after here to guarantee hash consistency.


			// Initialize new buckets.
			for (int64 i = 0; i < bucket_count; ++i)
			{
				new_buckets[i].first_node_ = NULL;
				new_buckets[i].last_node_ = NULL;
				new_buckets[i].is_first_bucket_ = false;
				new_buckets[i].is_last_bucket_ = false;
			}

			new_buckets[0].is_first_bucket_ = true;
			new_buckets[bucket_count - 1].is_last_bucket_ = true;


			// Reinsert previous hash contents.
			// We can't use the existing insert() as this increases
			// the container size and only works on the member buckets.
			// We need to keep the old nodes so we do not invalidate
			// any open iterators.
			for (int64 i = 0; i < bucket_count_; ++i)
			{
				node_type* old_node = buckets_[i].first_node_;

				while (old_node)
				{
					node_type* move_node = old_node;
					old_node = old_node->next_;

					bucket_type* new_bucket =
						new_buckets +
						hash_to_index(move_node->hash_content(), bucket_count);

					if (new_bucket->last_node_)
					{
						new_bucket->last_node_->next_ = move_node;
						move_node->previous_ = new_bucket->last_node_;
					}
					else
					{
						new_bucket->first_node_ = move_node;
						move_node->previous_ = NULL;
					}
					
					move_node->next_ = NULL;
					new_bucket->last_node_ = move_node;
					move_node->bucket_ = new_bucket;
				}
			}


			// Assign new buckets and cleanup.
			allocator_.free(buckets_);
			buckets_ = new_buckets;			
			bucket_count_ = bucket_count;
		}


		void rehash_if_needed()
		{
			/**
			 * Rehash to a new, suitable bucket count,
			 * either because the load factor is exceeded,
			 * or because there currently are no buckets.
			 *
			 * Rehashing executes before insertion
			 * or after a change in maximum load.
			 * The former may cause fuzzy hash loads,
			 * as it only considers the former size.
			 */

			int64 suggested_bucket_count =
				max(bucket_count_, 32);

			while (true)
			{
				// Check if current suggestion already fits.
				float load =
					static_cast<float>(size_) /
					static_cast<float>(suggested_bucket_count);

				if (load < max_load_hint_)
					break;

			
				// Load exceeded, so grow suggestion and retry.
				// Abort on overflow or on maximum bucket count.
				int64 grow_bucket_count =
					suggested_bucket_count / 2 + 1; // Never overflows.

				if (!integer_checks::is_add_safe
						(suggested_bucket_count,
						 grow_bucket_count))
				{
					suggested_bucket_count =
						container_limits<bucket_type>::max_size;
					break;
				}

				suggested_bucket_count += grow_bucket_count;
				if (suggested_bucket_count >= 
						container_limits<bucket_type>::max_size)
				{
					suggested_bucket_count =
						container_limits<bucket_type>::max_size;
					break;
				}
			}

		
			// Rehash to suggested, valid bucket count.
			// This may be the current bucket_count_ (ignored by rehash()),
			// the initial bucket count, or an increased bucket count.
			rehash(suggested_bucket_count);
		}
		

		void insert(node_type* new_node, uint64 hash_code)
		{
			/**
			 * Insert some not-@NULL @bref{new_node} into the hash,
			 * using @bref{hash_code} to pick a suitable hash bucket.
			 * Take ownership over (and possibly delete) @bref{new_node},
			 * in particular in the case of any exception.
			 *
			 * Requirements:
			 * - The hash must not yet contain the key of the node.
			 * - The @bref{new_node} must have been allocated with
			 *		the hash_base @bref{allocator_}.
			 */


			SCOPE_GUARD
				(new_node_guard,
					(&allocator_util::destroy_and_free<node_type, allocator_type>,
					 guard_by_reference(allocator_), new_node));


			// Shared container size check
			// for all hash variants.
			if (!integer_checks::is_add_safe(size_, 1) ||
				size_ + 1 > container_limits<N>::max_size)
					throw bounds_exceeded();


			// Possibly rehash.
			// This also ensures that there
			// is at least a single bucket.
			rehash_if_needed();


			// Insert node into hash.
			// NOTHROW from here.
			new_node_guard.dismiss();

			bucket_type* bucket =
				hash_to_bucket(hash_code);

			new_node->bucket_ = bucket;

			if (bucket->last_node_)
			{
				bucket->last_node_->next_ = new_node;
				new_node->previous_ = bucket->last_node_;
			}
			else
			{
				bucket->first_node_ = new_node;
			}

			bucket->last_node_ = new_node;

			++size_;
		}


		void remove(node_type* node) NOTHROW
		{
			if (!node) return;

			bucket_type *bucket = node->bucket_;

			if (node->next_)
				node->next_->previous_ = node->previous_;
			else
				bucket->last_node_ = node->previous_;

			if (node->previous_)
				node->previous_->next_ = node->next_;
			else
				bucket->first_node_ = node->next_;

			allocator_util::destroy_and_free
				(allocator_, node);

			--size_;
		}


		void apply_max_load_hint(float max_load_hint)
		{
			max_load_hint_ = max(0.1f, min(max_load_hint, 2.0f));
			rehash_if_needed();
		}


		float load_factor() const NOTHROW
			{ return static_cast<float>(size_) /
					 static_cast<float>(bucket_count_); }


		void clear() NOTHROW
		{
			for (int64 i = 0; i < bucket_count_; ++i)
			{
				node_type* node = buckets_[i].first_node_;
				while (node)
				{
					node_type* del_node = node;
					node = node->next_;

					allocator_util::destroy_and_free
						(allocator_, del_node);
				}
			}

			allocator_.free(buckets_);
			buckets_ = NULL;
			bucket_count_ = 0;
			size_ = 0;
		}


		void validate() const
		{
			/**
			 * Testing helper: Validate that the hash satisfies
			 * all hash properties and has correct structure.
			 */


			// Empty hash.
			if (!buckets_)
			{
				if (size_ > 0 || bucket_count_ > 0)
					throw validation_failed();
			}


			// Check each bucket.
			for (int64 i = 0; i < bucket_count_; ++i)
			{
				if (buckets_[i].is_first_bucket_ != (i == 0))
					throw validation_failed();
				if (buckets_[i].is_last_bucket_ != (i == bucket_count_ - 1))
					throw validation_failed();

				if (!buckets_[i].first_node_ && buckets_[i].last_node_)
					throw validation_failed();
				if (buckets_[i].first_node_ && !buckets_[i].last_node_)
					throw validation_failed();


				// Check each node for consistency.
				node_type* node = buckets_[i].first_node_;

				while (node)
				{
					if (node == buckets_[i].first_node_ && node->previous_)
						throw validation_failed();
					if (node == buckets_[i].last_node_ && node->next_)
						throw validation_failed();

					if (node != buckets_[i].first_node_ && !node->previous_)
						throw validation_failed();
					if (node != buckets_[i].last_node_ && !node->next_)
						throw validation_failed();

					if (node->next_ && node->next_->previous_ != node)
						throw validation_failed();
					if (node->previous_ && node->previous_->next_ != node)
						throw validation_failed();

					if ((buckets_ + i) != hash_to_bucket(node->hash_content()))
						throw validation_failed();

					node = node->next_;
				}
			}
		}

	
		allocator_type allocator_;

		bucket_type *buckets_;
		int64 bucket_count_;
		int64 size_;
		float max_load_hint_;
};




} /* namespace viral_core */


#endif /* defined(INCLUDED__VIRAL_CORE__HASH_BASE_HPP) */
