/**
 *************************************************************************
 *
 * @file bounding_intersect.hpp
 *
 * Intersection between bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_INTERSECT_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_INTERSECT_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "heap.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_intersect_pair
 * @ingroup bounding_module
 *
 * A single expansion candidate in a bounding_intersect test
 * over bounding volume hierarchy types @bref{A} and @bref{B}.
 *
 ************************************************************************/
template <typename A, typename B>
	class bounding_intersect_pair
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		bool operator <(const bounding_intersect_pair& other) const NOTHROW
		{
			// Sort highest-potential pairs to front of heap.
			return evaluation_potential_ > other.evaluation_potential_;
		}


		void swap_with(bounding_intersect_pair& other) NOTHROW
		{
			swap(node_a_, other.node_a_);
			swap(node_b_, other.node_b_);
			swap(evaluation_potential_, other.evaluation_potential_);
		}


	private:

		bounding_intersect_pair
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
 * @class bounding_intersect_result
 * @ingroup bounding_module
 *
 * Stores whether two bounding volume hierarchies intersect
 * and holds iterators to a pair of intersecting leaf nodes.
 *
 * Usage notes:
 * - If there is no intersection, then @bref{intersecting} is @false.
 *		In this case, intersecting leaf nodes are past-end iterators,
 *		and the @bref{extended_result} remains default-constructed.
 * - If there are multiple pairs of intersecting leaf nodes,
 *		then the result may reference any of these pairs.
 *
 ************************************************************************/
template <typename A, typename B, typename X>
	class bounding_intersect_result
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		/** Iterator types for nodes in the bounding volume hierarchies. */
		//@{
		typedef typename A::const_node_iterator const_node_iterator_a;
		typedef typename B::const_node_iterator const_node_iterator_b;
		//@}


		const bool intersecting;

		const const_node_iterator_a intersecting_node_a;
		const const_node_iterator_b intersecting_node_b;

		const X extended_result;


	private:

		bounding_intersect_result() NOTHROW
			:
			intersecting(false)
		{ }


		bounding_intersect_result
			(const const_node_iterator_a& intersecting_node_a,
			 const const_node_iterator_b& intersecting_node_b,
			 const X& extended_result) NOTHROW
			:
			intersecting(true),
			intersecting_node_a(intersecting_node_a),
			intersecting_node_b(intersecting_node_b),
			extended_result(extended_result)
		{ }
};




/**
 *************************************************************************
 *
 * @class bounding_intersect_no_extended_result
 * @ingroup bounding_module
 *
 * Supply to bounding_intersect if no extended result is required.
 *
 ************************************************************************/
class bounding_intersect_no_extended_result { };




/**
 *************************************************************************
 *
 * @class bounding_intersect
 * @ingroup bounding_module
 *
 * Static utilities that determine whether
 * two bounding volume hierarchies intersect.
 *
 * Intersection tests:
 * - Clients must provide an intersection test @bref{T}
 *		for node boundings and geometry objects.
 * - bool T::node_intersecting
 *			(const A& ha, const const_node_iterator_a& na,
 *			 const B& hb, const const_node_iterator_b& nb,
 *			 float& evaluation_potential) const
 *		must return @true if the leaf-level geometry objects
 *		subordinate to arbitrary nodes @bref{na} and @bref{nb}
 *		may intersect. False positive results are allowed,
 *		e.g. due to bounding overlap or early-out optimization.
 *		Additionally, the call must find an evaluation potential
 *		that rates pairs in [-INF, +INF] for further evaluation,
 *		e.g. through traversal costs or intersection probability.
 *		Node intersect tests must be efficient and thus should
 *		only consider bounding shapes, even for leaf nodes.
 * - bool T::leaf_intersecting
 *			(const A& ha, const const_node_iterator_a& na,
 *			 const B& hb, const const_node_iterator_b& nb,
 *			 X& extended_result) const
 *		must exactly determine if the geometry objects
 *		in two leaves @bref{na} and @bref{nb} intersect.
 * - Both intersection tests may return @false to skip node pairs,
 *		for example to suppress empty nodes or self-collisions.
 *		In this case, the tests need not set any output variables.
 * - Node iterators @bref{na} and @bref{nb} never are past-end.
 *		Intersection tests should avoid redundant testing.
 * - Both intersection methods must throw to indicate failure.
 *		The overall intersection test then fails as well.
 *
 * Extended results:
 * - Clients can pass back additional data from the leaf-level
 *		intersection test on the intersecting leaf-level geometry
 *		(e.g. closest indices of a subordinate intersection test)
 *		through an @bref{extended_result} of type @bref{X}.
 * - Type @bref{X} must be default- and copy-constructable,
 *		assignable, and @ref NOTHROW in all three operations.
 * - If no extended result is required, clients can supply
 *		bounding_intersect_no_extended_result as @bref{X}.
 *
 * General usage notes:
 * - See the @ref bounding_module "Bounding Module"
 *		for requirements of bounding volume hierarchies
 *		that are suited as types @bref{A} and @bref{B}.
 * - To realize transformed objects, clients can store
 *		transformations in the intersection @bref{test}.
 *		The intersection test then can apply transforms
 *		to node contents before actual testing.
 * -  Clients should provide a preallocated @bref{pair_heap}
 *		to avoid expensive heap allocations per call.
 *
 ************************************************************************/
template <typename A, typename B, typename T, typename X>
	class bounding_intersect :
		private non_constructable
{

	public:

		/** Node pairs pending for future expansion. */
		//@{
		typedef bounding_intersect_pair<A, B> intersect_pair;
		typedef heap<intersect_pair> intersect_heap;
		//@}


		/** Specific result type for intersect() call. */
		typedef bounding_intersect_result<A, B, X> intersect_result;


		/**
		 * Perform intersection test on @bref{ha} and @bref{hb}.
		 * See bounding_intersect_result for resulting data.
		 *
		 * Algorithm outline:
		 * - Recursively check all pairs of nodes for intersections,
		 *		using the evaluation potential to choose next pairs.
		 * - Expand all children pairs of all intersecting nodes,
		 *		and discard all pairs that are not intersecting.
		 * - Early out once the first intersection has been found.
		 */
		static intersect_result
			intersect
				(const A& ha, const B& hb, const T& test,
				 intersect_heap& pair_heap)
		{
			// Early-out on empty hierarchies
			// or on non-intersecting roots.
			const_node_iterator_a root_a(ha.root_node());
			const_node_iterator_b root_b(hb.root_node());

			if (!root_a || !root_b)
				return intersect_result();

			float root_evaluation_potential = 0;

			if (!test.node_intersecting
					(ha, root_a, hb, root_b,
					 root_evaluation_potential))
			{
				return intersect_result();
			}


			// Clear heap (in case it was preallocated),
			// then initialize heap with a single entry
			// for the intersecting roots.
			pair_heap.clear();
			pair_heap.insert
				(intersect_pair
					(root_a, root_b,
					 root_evaluation_potential));

		
			while (!pair_heap.empty())
			{


				// Extract next candidate pair from heap.
				// We already know this pair is intersecting,
				// so check respective children nodes.
				intersect_pair pair(pair_heap.minimum());
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

			
				// Nodes have no children and their boundings intersect,
				// thus test if leaf-level geometry intersects as well.
				// We can safely exit on any such intersection.
				if (is_leaf_a && is_leaf_b)
				{
					X extended_result;
					if (test.leaf_intersecting
							(ha, pair.node_a_,
							 hb, pair.node_b_,
							 extended_result))
					{
						return intersect_result
									(pair.node_a_, pair.node_b_,
									 extended_result);
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


				// Test all children combinations for intersection,
				// then store intersecting pairs for further expansion.
				// This avoids futile heap insertion of disjunct pairs.
				for (int8 a = 0; a < 2; ++a)
				for (int8 b = 0; b < 2; ++b)
				{
					if (!expand_a[a] || !expand_b[b])
						continue;

					float expand_evaluation_potential = 0;

					if (!test.node_intersecting
							(ha, expand_a[a], hb, expand_b[b],
							 expand_evaluation_potential))
									continue;

					pair_heap.insert
						(intersect_pair
							(expand_a[a], expand_b[b],
							 expand_evaluation_potential));			
				}
			}


			// Bounding volume hierarchies do not intersect.
			return intersect_result();
		}


	private:

		typedef typename A::const_node_iterator const_node_iterator_a;
		typedef typename B::const_node_iterator const_node_iterator_b;
};




} /* namespace viral_core */


#endif /*! defined(INCLUDED__VIRAL_CORE__BOUNDING_INTERSECT_HPP) */
