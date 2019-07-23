/**
 *************************************************************************
 *
 * @file bounding_proximate.hpp
 *
 * Find any proximate nodes in bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_PROXIMATE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_PROXIMATE_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "heap.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_proximate_pair
 * @ingroup bounding_module
 *
 * A single pair of candidate nodes in a bounding_proximate test
 * over bounding volume hierarchy types @bref{A} and @bref{B}.
 *
 ************************************************************************/
template <typename A, typename B>
	class bounding_proximate_pair
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:

		bool operator <(const bounding_proximate_pair& other) const NOTHROW
		{
			// Sort highest-potential pairs to front of heap.
			return evaluation_potential_ > other.evaluation_potential_;
		}


		void swap_with(bounding_proximate_pair& other) NOTHROW
		{
			swap(node_a_, other.node_a_);
			swap(node_b_, other.node_b_);
			swap(evaluation_potential_,
				 other.evaluation_potential_);
		}


	private:

		bounding_proximate_pair
			(const typename A::const_node_iterator& node_a,
			 const typename B::const_node_iterator& node_b,
			 float evaluation_potential) NOTHROW
			:
			node_a_(node_a),
			node_b_(node_b),
			evaluation_potential_(evaluation_potential)
		{ }


		typename A::const_node_iterator node_a_;
		typename B::const_node_iterator node_b_;
		float evaluation_potential_;
};




/**
 *************************************************************************
 *
 * @class bounding_proximate_result
 * @ingroup bounding_module
 *
 * Stores bounding_proximate results in form of a pair of iterators
 * to any proximate nodes of the bounding volume hierarchies.
 *
 * Usage notes:
 * - If there are no proximate nodes (e.g. as either hierarchy is empty),
 *		then @bref{valid} is @false. In this case, proximate nodes are set
 *		to past-end iterators and @bref{extended_result} remains defaulted.
 * - The result may contain any of multiple candidates for proximate nodes,
 *		potentially causing an error of up to the distance bound.
 *
 ************************************************************************/
template <typename A, typename B, typename X>
	class bounding_proximate_result
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:

		/** Iterator types for nodes in the bounding volume hierarchies. */
		//@{
		typedef typename A::const_node_iterator const_node_iterator_a;
		typedef typename B::const_node_iterator const_node_iterator_b;
		//@}


		const bool valid;

		const const_node_iterator_a proximate_node_a;
		const const_node_iterator_b proximate_node_b;

		const X extended_result;


	private:

		bounding_proximate_result() NOTHROW
			:
			valid(false)
		{ }


		bounding_proximate_result
			(const const_node_iterator_a& proximate_node_a,
			 const const_node_iterator_b& proximate_node_b,			 
			 const X& extended_result) NOTHROW
			:
			valid(true),
			proximate_node_a(proximate_node_a),
			proximate_node_b(proximate_node_b),
			extended_result(extended_result)
		{ }
};




/**
 *************************************************************************
 *
 * @class bounding_proximate_no_extended_result
 * @ingroup bounding_module
 *
 * Supply to bounding_proximate if no extended result is required.
 *
 ************************************************************************/
class bounding_proximate_no_extended_result { };




/**
 *************************************************************************
 *
 * @class bounding_proximate
 * @ingroup bounding_module
 *
 * Static utilities that determine whether two bounding volume hierarchies
 * are closer together than @bref{distance_squared_upper_bound}.
 *
 * Proximity tests:
 * - Clients must provide a proximity test @bref{T}
 *		for node boundings and geometry objects.
 * - bool T::node_proximate
 *			(const A& ha, const const_node_iterator_a& na,
 *			 const B& hb, const const_node_iterator_b& nb,
 *			 float distance_squared_upper_bound,
 *			 float& evaluation_potential) const
 *		must return @true if the leaf-level geometry objects
 *		subordinate to arbitrary nodes @bref{na} and @bref{nb}
 *		may be closer than @bref{distance_squared_upper_bound}.
 *		False-positive results are allowed, e.g. due to tests
 *		on node bounding shapes instead of leaf-level geometry.
 *		The potential rates pairs for evaluation in [-INF, +INF]
 *		and may consider traversal costs or minimum probability.
 *		Node proximity tests must be efficient and thus should
 *		only consider bounding shapes, even for leaf nodes.
 * - bool T::leaf_proximate
 *			(const A& ha, const const_node_iterator_a& na,
 *			 const B& hb, const const_node_iterator_b& nb,
 *			 float distance_squared_upper_bound
 *			 X& extended_result) const
 *		must exactly determine if the geometry objects
 *		in leaf nodes @bref{na} and @bref{nb} are closer
 *		than @bref{distance_squared_upper_bound}.
 * - A distance of exactly @bref{distance_squared_upper_bound}
 *		may or may not be proximate (even case-by-case),
 *		depending on consistency in the proximity tests.
 * - Both proximity tests may return @false to skip any node pair,
 *		which allows to suppress empty nodes or self-collisions.
 *		In this case, tests need not set any output variables.
 * - Node iterators @bref{na} and @bref{nb} never are past-end.
 *		Proximity tests should avoid redundant testing.
 * - Both proximity tests must throw to indicate failure.
 *		The overall proximity test then fails as well.
 *
 * Extended results:
 * - Clients can pass back additional data from the leaf-level
 *		proximity test on any proximate leaf-level geometry pair
 *		(e.g. actual distances of a subordinate distance test)
 *		through an @bref{extended_result} of type @bref{X}.
 * - Type @bref{X} must be default- and copy-constructable,
 *		assignable, and @ref NOTHROW in all three operations.
 * - If no extended result is required, clients can supply
 *		bounding_proximate_no_extended_result as @bref{X}.
 *
 * General usage notes:
 * - See the @ref bounding_module "Bounding Module"
 *		for requirements of bounding volume hierarchies
 *		that are suited as types @bref{A} and @bref{B}.
 * - To realize transformed objects, clients can store
 *		transformations in the proximity @bref{test}.
 *		The proximity test then can apply transforms
 *		to node contents before actual testing.
 * -  Clients should provide a preallocated @bref{pair_heap}
 *		to avoid expensive heap allocations per call.
 *
 ************************************************************************/
template <typename A, typename B, typename T, typename X>
	class bounding_proximate :
		private non_constructable
{

	public:

		/** Node pairs pending for future expansion. */
		//@{
		typedef bounding_proximate_pair<A, B> proximate_pair;
		typedef heap<proximate_pair> proximate_heap;
		//@}


		/** Specific result type for proximate() call. */
		typedef bounding_proximate_result<A, B, X> proximate_result;


		/**
		 * Perform a proximity test on @bref{ha} and @bref{hb}.
		 * See bounding_proximity_result for resulting data.
		 *
		 * Algorithm outline:
		 * - Recursively expand node pairs and test these for proximity.
		 * - Use evaluation potential to select next-best pair of nodes.
		 * - Early-out on the first proximate node pair.
		 * - Discard any node pairs that are not proximate.
		 *
		 * Implementation notes:
		 * - We use squared distances instead of exact distances
		 *		because this allows tests to avoid costy sqrtf().
		 */
		static proximate_result
			proximate
				(const A& ha, const B& hb, const T& test,
				 proximate_heap& pair_heap,
				 float distance_squared_upper_bound)
		{
			// Early-out on empty hierarchies
			// or on distant or skipped roots.
			const_node_iterator_a root_a(ha.root_node());
			const_node_iterator_b root_b(hb.root_node());

			if (!root_a || !root_b)
				return proximate_result();

			float root_evaluation_potential = 0;

			if (!test.node_proximate
					(ha, root_a, hb, root_b,
					 distance_squared_upper_bound,
					 root_evaluation_potential))
			{
				return proximate_result();
			}


			// Clear heap (in case it was preallocated),
			// then initialize heap with a single entry
			// for the proximate roots.
			pair_heap.clear();
			pair_heap.insert
				(proximate_pair
					(root_a, root_b,
					 root_evaluation_potential));

		
			while (!pair_heap.empty())
			{


				// Fetch next candidate pair from heap.
				// We already know this pair is proximate,
				// so check respective children nodes.
				proximate_pair pair(pair_heap.minimum());
				pair_heap.pop_minimum();
			
				const_node_iterator_a expand_a[2] =
					{ ha.left_node(pair.node_a_),
					  ha.right_node(pair.node_a_) };
				bool is_leaf_a =
					!expand_a[0] && !expand_a[1];

				const_node_iterator_b expand_b[2] =
					{ hb.left_node(pair.node_b_),
					  hb.right_node(pair.node_b_) };
				bool is_leaf_b =
					!expand_b[0] && !expand_b[1];

			
				// Nodes have no children and boundings are proximate,
				// thus test if leaf-level geometry is proximate as well.
				// We can safely exit on the first proximate geometry.
				if (is_leaf_a && is_leaf_b)
				{
					X leaf_extended_result;

					if (test.leaf_proximate
							(ha, pair.node_a_,
							 hb, pair.node_b_,
							 distance_squared_upper_bound,
							 leaf_extended_result))
					{
						return proximate_result
									(pair.node_a_, pair.node_b_,
									 leaf_extended_result);
					}

					continue;
				}


				// Not a leaf-leaf combination, so expand further.
				// If only one node is a leaf, directly use that node
				// for expanding the children of the other node.
				if (is_leaf_a)
				{
					expand_a[0] = pair.node_a_;
					expand_a[1] = const_node_iterator_a();
				}
				else if (is_leaf_b)
				{
					expand_b[0] = pair.node_b_;
					expand_b[1] = const_node_iterator_b();
				}


				// Test all children combinations for proximity,
				// then store proximate pairs for further expansion.
				// This avoids futile insertion of non-proximate pairs.
				for (int8 a = 0; a < 2; ++a)
				for (int8 b = 0; b < 2; ++b)
				{
					if (!expand_a[a] || !expand_b[b])
						continue;

					float expand_evaluation_potential = 0;

					if (!test.node_proximate
							(ha, expand_a[a], hb, expand_b[b],
							 distance_squared_upper_bound,
							 expand_evaluation_potential))
									continue;

					pair_heap.insert
						(proximate_pair
							(expand_a[a], expand_b[b],
							 expand_evaluation_potential));			
				}
			}


			// Bounding volume hierarchies are not proximate.
			return proximate_result();
		}


	private:

		typedef typename A::const_node_iterator const_node_iterator_a;
		typedef typename B::const_node_iterator const_node_iterator_b;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_PROXIMATE_HPP) */