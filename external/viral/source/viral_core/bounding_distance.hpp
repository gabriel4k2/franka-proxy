/**
 *************************************************************************
 *
 * @file bounding_distance.hpp
 *
 * Distance between bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_DISTANCE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_DISTANCE_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "heap.hpp"
#include "geo_vector.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_distance_pair
 * @ingroup bounding_module
 *
 * A single pair of candidate nodes in a bounding_distance test
 * over bounding volume hierarchy types @bref{A} and @bref{B}.
 *
 ************************************************************************/
template <typename A, typename B>
	class bounding_distance_pair
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:

		bool operator <(const bounding_distance_pair& other) const NOTHROW
		{
			// Sort highest-potential pairs to front of heap.
			return evaluation_potential_ > other.evaluation_potential_;
		}


		void swap_with(bounding_distance_pair& other) NOTHROW
		{
			swap(node_a_, other.node_a_);
			swap(node_b_, other.node_b_);

			swap(min_distance_squared_lower_bound_,
				 other.min_distance_squared_lower_bound_);
			swap(evaluation_potential_,
				 other.evaluation_potential_);
		}


	private:

		bounding_distance_pair
			(const typename A::const_node_iterator& node_a,
			 const typename B::const_node_iterator& node_b,
			 float min_distance_squared_lower_bound,
			 float evaluation_potential) NOTHROW
			:
			node_a_(node_a),
			node_b_(node_b),
			min_distance_squared_lower_bound_(min_distance_squared_lower_bound),
			evaluation_potential_(evaluation_potential)
		{ }


		typename A::const_node_iterator node_a_;
		typename B::const_node_iterator node_b_;
		float min_distance_squared_lower_bound_;
		float evaluation_potential_;
};




/**
 *************************************************************************
 *
 * @class bounding_distance_result
 * @ingroup bounding_module
 *
 * Stores bounding_distance results in form of a pair of closest points,
 * the squared distance of this point pair, and associated node iterators
 * into the bounding volume hierarchies.
 *
 * Usage notes:
 * - If there are no closest points (e.g. because either hierarchy is empty),
 *		then @bref{valid} is @false. In this case, closest nodes are set to
 *		past-end iterators, closest points and squared distance are zero,
 *		and the @bref{extended_result} remains default-constructed.
 * - Distance tests may pick any of multiple candidates for closest points,
 *		potentially causing an error of up to the distance tolerance.
 *
 ************************************************************************/
template <typename A, typename B, typename X>
	class bounding_distance_result
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:

		/** Iterator types for nodes in the bounding volume hierarchies. */
		//@{
		typedef typename A::const_node_iterator const_node_iterator_a;
		typedef typename B::const_node_iterator const_node_iterator_b;
		//@}


		const bool valid;

		const const_node_iterator_a closest_node_a;
		const const_node_iterator_b closest_node_b;

		const vector closest_point_a;
		const vector closest_point_b;

		const float closest_distance_squared;

		const X extended_result;


	private:

		bounding_distance_result() NOTHROW
			:
			valid(false),
			closest_distance_squared(0.f)
		{ }


		bounding_distance_result
			(const const_node_iterator_a& closest_node_a,
			 const const_node_iterator_b& closest_node_b,
			 const vector& closest_point_a,
			 const vector& closest_point_b,
			 float closest_distance_squared,
			 const X& extended_result) NOTHROW
			:
			valid(true),
			closest_node_a(closest_node_a),
			closest_node_b(closest_node_b),
			closest_point_a(closest_point_a),
			closest_point_b(closest_point_b),
			closest_distance_squared(closest_distance_squared),
			extended_result(extended_result)
		{ }
};




/**
 *************************************************************************
 *
 * @class bounding_distance_no_extended_result
 * @ingroup bounding_module
 *
 * Supply to bounding_distance if no extended result is required.
 *
 ************************************************************************/
class bounding_distance_no_extended_result { };




/**
 *************************************************************************
 *
 * @class bounding_distance
 * @ingroup bounding_module
 *
 * Find closest points and distances for bounding volume hierarchies
 * of types @bref{A} and @bref{B}.
 *
 * Distance metrics:
 * - Clients must provide a metric @bref{M} that calculates
 *		bounded distances, exact distances, and closest points
 *		for nodes of the input hierarchies.
 * - bool M::try_node_distance_squared
 *			(const A& ha, const const_node_iterator_a& na,
 *			 const B& hb, const const_node_iterator_b& nb,
 *			 float& min_distance_squared_lower_bound,
 *			 float& evaluation_potential) const
 *		must find a lower bound on the squared distance between
 *		and an evaluation potential for the leaf-level geometry
 *		subordinate to arbitrary nodes @bref{na} and @bref{nb}.
 *		The lower bound prunes entire branches of a hierarchy.
 *		The potential rates pairs for evaluation in [-INF, +INF]
 *		and may consider traversal costs or minimum probability.
 *		Distance bounding must be efficient, and thus should
 *		only consider bounding shapes, even for leaf nodes.
 * - bool M::try_leaf_distance_squared
 *			(const A& ha, const const_node_iterator_a& na,
 *			 const B& hb, const const_node_iterator_b& nb,
 *			 vector& closest_point_a, vector& closest_point_b,
 *			 float& closest_distance_squared,
 *			 X& extended_result) const
 *		must return the exact squared distance and closest points
 *		on the geometry objects in leaves @bref{na} and @bref{nb}.
 * - Both metric methods may return @false to skip any node pair,
 *		for example to suppress empty nodes or self-collision tests.
 *		In this case, the metric need not set any output variables.
 * - Node iterators @bref{na} and @bref{nb} never are past-end.
 *		Metrics should avoid expensive, redundant testing.
 * - Both metric methods must throw to indicate failure.
 *		The overall distance calculation then fails as well.
 *
 * Extended results:
 * - Clients can pass back additional data from the leaf-level
 *		distance test on the closest leaf-level geometry pair
 *		(e.g. closest indices of a subordinate distance test)
 *		through an @bref{extended_result} of type @bref{X}.
 * - Type @bref{X} must be default- and copy-constructable,
 *		assignable, and @ref NOTHROW in all three operations.
 * - If no extended result is required, clients can supply
 *		bounding_distance_no_extended_result as @bref{X}.
 *
 * General usage notes:
 * - See the @ref bounding_module "Bounding Module"
 *		for requirements of bounding volume hierarchies
 *		that are suited as types @bref{A} and @bref{B}.
 * - To realize transformed objects, clients can store
 *		transformations in the @bref{metric} instance.
 *		The metric then can apply transforms to nodes
 *		before calculating any distances.
 * -  Clients should provide a preallocated @bref{pair_heap}
 *		to avoid expensive heap allocations per call.
 *
 ************************************************************************/
template <typename A, typename B, typename M, typename X>
	class bounding_distance :
		private non_constructable
{

	public:

		/** Node pairs pending for future expansion. */
		//@{
		typedef bounding_distance_pair<A, B> distance_pair;
		typedef heap<distance_pair> distance_heap;
		//@}


		/** Specific result type for distance() call. */
		typedef bounding_distance_result<A, B, X> distance_result;


		/**
		 * Perform a closest-distance test on @bref{ha} and @bref{hb}.
		 * See bounding_distance_result for resulting data.
		 *
		 * Usage notes:
		 * - If @bref{distance_squared_bound_valid} is @true,
		 *		then distance tests reject any node candidates
		 *		that cannot yield a squared distance less than
		 *		the provided @bref{distance_squared_bound}. 
		 * - Clients can either set @bref{distance_squared_bound}
		 *		to an arbitrary limit for broad-phase rejection,
		 *		or to the result of a preceding distance() call.
		 *		The latter allows clients to perform distance tests
		 *		over multiple pairs of bounding volume hierarchies.
		 * - Distance tests efficiently reject any node candidates
		 *		that cannot improve the current closest-point guess
		 *		(or an initial, valid @bref{distance_squared_bound})
		 *		by at least the @bref{distance_squared_tolerance}.
		 *		A distance tolerance of zero gives exact results,
		 *		greater tolerances trade errors for performance.
		 *
		 * Algorithm outline:
		 * - Check nodes in one hierarchy against nodes in the other.
		 *		Use the client-supplied evaluation potential to find
		 *		the next-best pair of nodes to check, but cull away
		 *		entire branches if they cannot contain a minimum
		 *		better than found precedingly, including tolerance.
		 * - On descent in both hierarchies, expand a pair of nodes
		 *		to several pairs of children nodes. We need to test
		 *		each child of one parent with each child of the other.
		 * - Determine correct and expensive distances when arriving
		 *		at leaf-leaf pairs, but still check remaining heap
		 *		after the first leaf-leaf pair for yet closer points.
		 *
		 * Implementation notes:
		 * - To enable skipping of nodes on @false metric returns,
		 *		we must not update @bref{distance_squared_bound}
		 *		unless we have an actual leaf-level geometry bound.
		 *		Otherwise, we might mistakenly prune nodes due to
		 *		a distance bound on a branch with skipped leaves.
		 * - We use squared distances instead of exact distances
		 *		because this allows metrics to avoid costy sqrtf().
		 */
		static distance_result
			distance
				(const A& ha, const B& hb, const M& metric,
				 distance_heap& pair_heap,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance)
		{
			// Determine distance bound of the root nodes.
			// Abort if the root pair is to be skipped.
			const_node_iterator_a root_a(ha.root_node());
			const_node_iterator_b root_b(hb.root_node());

			if (!root_a || !root_b)
				return distance_result();

			float root_min_distance_squared_lower_bound = 0;
			float root_evaluation_potential = 0;

			if (!metric.try_node_distance_squared
					(ha, root_a, hb, root_b,
					 root_min_distance_squared_lower_bound,
					 root_evaluation_potential))
			{
				return distance_result();
			}


			// Clear heap (in case it was preallocated),
			// then initialize heap with a single entry
			// for the pair of root nodes.
			pair_heap.clear();
			pair_heap.insert
				(distance_pair
					(root_a, root_b,
					 root_min_distance_squared_lower_bound,
					 root_evaluation_potential));


			// Iteratively update closest nodes, points, and distance bound
			// by extracting next-best pair guesses from the candidate heap.
			const_node_iterator_a result_closest_node_a;
			const_node_iterator_b result_closest_node_b;
			vector result_closest_point_a;
			vector result_closest_point_b;
			X result_extended_result;
			bool result_valid = false;
		
			while (!pair_heap.empty())
			{


				// Extract next-best candidate pair from heap
				// and check if it can contain a better minimum.
				// This is beneficial because distance_squared_bound
				// may have updated since the insertion of the pair.
				distance_pair pair(pair_heap.minimum());
				pair_heap.pop_minimum();

				if (distance_squared_bound_valid &&
					pair.min_distance_squared_lower_bound_ >=
						distance_squared_bound - distance_squared_tolerance)
							continue;


				// Pending pair can contain a better minimum,
				// so fetch its children for later expansion.
				// Take note of leaves for geometry handling.
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
						

				// Arrived at some leaf-leaf pair.
				// Unless skipped, determine closest points and distance.
				// Accept these if the distance improves current bounds
				// by more than the distance tolerance.
				if (is_leaf_a && is_leaf_b)
				{
					vector leaf_closest_point_a, leaf_closest_point_b;
					float leaf_closest_distance_squared;
					X leaf_extended_result;

					if (metric.try_leaf_distance_squared
							(ha, pair.node_a_,
							 hb, pair.node_b_,
							 leaf_closest_point_a, leaf_closest_point_b,
							 leaf_closest_distance_squared,
							 leaf_extended_result))
					{
						if (!distance_squared_bound_valid ||
							leaf_closest_distance_squared <
								distance_squared_bound - distance_squared_tolerance)
						{
							distance_squared_bound = leaf_closest_distance_squared;
							distance_squared_bound_valid = true;
						
							result_closest_node_a = pair.node_a_;
							result_closest_node_b = pair.node_b_;
							result_closest_point_a = leaf_closest_point_a;
							result_closest_point_b = leaf_closest_point_b;
							result_extended_result = leaf_extended_result;
							result_valid = true;
						}
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


				// Expand all children combinations.
				// Bound distance and find potential for these.
				// Finally add any unskipped children pairs
				// to our candidate heap.
				for (int8 a = 0; a < 2; ++a)
				for (int8 b = 0; b < 2; ++b)
				{
					if (!expand_a[a] || !expand_b[b])
						continue;

					float expand_min_distance_squared_lower_bound = 0;
					float expand_evaluation_potential = 0;

					if (metric.try_node_distance_squared
							(ha, expand_a[a], hb, expand_b[b],
							 expand_min_distance_squared_lower_bound,
							 expand_evaluation_potential))
					{


						// Bound children (e.g. spheres) by parent node,
						// as volumes outside our parent must be empty.
						// Then apply early pruning to skip heap insertion.
						expand_min_distance_squared_lower_bound =
							max(expand_min_distance_squared_lower_bound,
								pair.min_distance_squared_lower_bound_);

						if (distance_squared_bound_valid &&
							expand_min_distance_squared_lower_bound >=
								distance_squared_bound - distance_squared_tolerance)
									continue;

						pair_heap.insert
							(distance_pair
								(expand_a[a], expand_b[b],
	 							 expand_min_distance_squared_lower_bound,
								 expand_evaluation_potential));
					}
				}
			}


			// If no valid leaf-leaf pair has been encountered
			// (e.g. due to distance bound), we abort here.
			if (!result_valid)
				return distance_result();


			// We had at least a single valid leaf-leaf pair,
			// so return respective nodes, points, and distance.
			return
				distance_result
					(result_closest_node_a, result_closest_node_b,
					 result_closest_point_a, result_closest_point_b,
					 distance_squared_bound, result_extended_result);
		}


	private:

		typedef typename A::const_node_iterator const_node_iterator_a;
		typedef typename B::const_node_iterator const_node_iterator_b;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_DISTANCE_HPP) */
