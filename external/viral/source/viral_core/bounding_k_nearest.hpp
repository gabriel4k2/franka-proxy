/**
 *************************************************************************
 *
 * @file bounding_k_nearest.hpp
 *
 * Nearest neighbor search on bounding hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_K_NEAREST_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_K_NEAREST_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "heap.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_k_nearest_node_candidate
 * @ingroup bounding_module
 *
 * A single candidate node in a bounding_k_nearest query
 * over a bounding volume hierarchy of type @bref{H}.
 *
 ************************************************************************/
template <typename H>
	class bounding_k_nearest_node_candidate
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		bool operator <(const bounding_k_nearest_node_candidate& other) const NOTHROW
		{
			// Sort highest-potential candidates to front of heap.
			return evaluation_potential_ > other.evaluation_potential_;
		}


		void swap_with(bounding_k_nearest_node_candidate& other) NOTHROW
		{
			swap(node_, other.node_);

			swap(min_distance_squared_lower_bound_,
				 other.min_distance_squared_lower_bound_);

			swap(evaluation_potential_, other.evaluation_potential_);
		}


	private:

		bounding_k_nearest_node_candidate
			(const typename H::const_node_iterator& node,
			 float min_distance_squared_lower_bound,
			 float evaluation_potential) NOTHROW
			:
			node_(node),
			min_distance_squared_lower_bound_(min_distance_squared_lower_bound),
			evaluation_potential_(evaluation_potential)
		{ }


		typename H::const_node_iterator node_;
		float min_distance_squared_lower_bound_;
		float evaluation_potential_;
};




/**
 *************************************************************************
 *
 * @class bounding_k_nearest_neighbor_candidate
 * @ingroup bounding_module
 *
 * A leaf-level neighbor candidate in a bounding_k_nearest query
 * over a bounding volume hierarchy of type @bref{H}.
 *
 ************************************************************************/
template <typename H, typename X>
	class bounding_k_nearest_neighbor_candidate
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		bool operator <(const bounding_k_nearest_neighbor_candidate& other) const NOTHROW
		{
			// Sort highest-distance neighbors to front of heap
			// so we can easily remove these on excess of k.
			return closest_distance_squared_ > other.closest_distance_squared_;
		}


		void swap_with(bounding_k_nearest_neighbor_candidate& other) NOTHROW
		{
			swap(leaf_node_, other.leaf_node_);
			swap(closest_distance_squared_, other.closest_distance_squared_);

			// Assignment on X must be NOTHROW,
			// see bounding_k_nearest documentation.
			swap_assign(extended_result_, other.extended_result_);
		}


	private:

		bounding_k_nearest_neighbor_candidate
			(const typename H::const_node_iterator& leaf_node,
			 float closest_distance_squared,
			 const X& extended_result) NOTHROW
			:
			leaf_node_(leaf_node),
			closest_distance_squared_(closest_distance_squared),
			extended_result_(extended_result)
		{ }


		typename H::const_node_iterator leaf_node_;
		float closest_distance_squared_;
		X extended_result_;
};




/**
 *************************************************************************
 *
 * @class bounding_k_nearest_entry
 * @ingroup bounding_module
 *
 * A single entry in a bounding_k_nearest_result.
 *
 ************************************************************************/
template <typename H, typename X>
	class bounding_k_nearest_entry
{

	public:

		/** Iterator type for nodes in the bounding volume hierarchy. */
		typedef typename H::const_node_iterator const_node_iterator;


		bounding_k_nearest_entry
			(const const_node_iterator& leaf_node,
			 float closest_distance_squared,
			 const X& extended_result) NOTHROW
			:
			leaf_node(leaf_node),
			closest_distance_squared(closest_distance_squared),
			extended_result(extended_result)
		{ }


		const const_node_iterator leaf_node;
		const float closest_distance_squared;
		const X extended_result;
};




/**
 *************************************************************************
 *
 * @class bounding_k_nearest_result
 * @ingroup bounding_module
 *
 * Stores @bref{entries} for leaf nodes in a bounding_k_nearest query,
 * sorted by ascending distance.
 *
 ************************************************************************/
template <typename H, typename X>
	class bounding_k_nearest_result
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		typedef array<bounding_k_nearest_entry<H, X>> entry_array;
		const entry_array entries;


	private:

		bounding_k_nearest_result() { }

		bounding_k_nearest_result(const entry_array& entries)
			: entries(entries) { }
};




/**
 *************************************************************************
 *
 * @class bounding_k_nearest_no_extended_result
 * @ingroup bounding_module
 *
 * Supply to bounding_k_nearest if no extended result is required.
 *
 ************************************************************************/
class bounding_k_nearest_no_extended_result { };




/**
 *************************************************************************
 *
 * @class bounding_k_nearest
 * @ingroup bounding_module
 *
 * Find nearest neighbors in a bounding volume hierarchy of type @bref{H}
 * to a geometry object of type @bref{O}.
 *
 * Distance metric:
 * - Clients must supply a distance metric @bref{M} from geometry object
 *		to node bounding shapes or node geometry objects.
 * - bool M::try_node_distance_squared
 *			(const H& h, const const_node_iterator& n, const O& o,
 *			 float& min_distance_squared_lower_bound,
 *			 float& evaluation_potential) const
 *		must find a lower bound on squared distance between
 *		and an evaluation potential for the pair of @bref{o}
 *		and leaf-level geometry in an arbitrary @bref{n}ode.
 *		The lower bound prunes entire branches of a hierarchy.
 *		The potential rates nodes for evaluation in [-INF, +INF]
 *		and may consider traversal costs or minimum probability.
 *		Distance bounding must be efficient, and thus should
 *		only consider bounding shapes, even for leaf nodes.
 * - bool M::try_leaf_distance_squared
 *			(const H& h, const const_node_iterator& n, const O& o,
  *			 float& closest_distance_squared,
 *			 X& extended_result) const
 *		must return the exact squared distance between @bref{o}
 *		and the geometry object in a leaf @bref{n}ode.
 * - Both metric methods may return @false to skip any node.
 *		This allows to suppress empty nodes or self-collisions.
 *		In this case, the metric need not set output variables.
 * - The node iterator @bref{n} is never a past-end iterator.
 *		Metrics should avoid expensive, redundant testing.
 * - Both metric methods must throw to indicate failure.
 *		The overall neighbor search then fails as well.
 *
 * Extended results:
 * - Clients can pass back data from leaf-level distance metrics
 *		on nearest neighbors (e.g. results of subordinate tests)
 *		through an @bref{extended_result} of type @bref{X}.
 * - Type @bref{X} must be default- and copy-constructable,
 *		assignable, and @ref NOTHROW in all three operations.
 * - If no extended result is required, clients can supply
 *		bounding_k_nearest_no_extended_result as @bref{X}.
 *
 * General usage notes:
 * - See the @ref bounding_module "Bounding Module" for requirements
 *		on the bounding volume hierarchy @bref{H}.
 * - To realize transformed objects, clients can store transformations
 *		in the instance of the distance @bref{metric}. The metric can
 *		then apply transforms to shapes and geometry as required.
 * -  Clients should preallocate heaps (e.g. the @bref{node_heap})
 *		to avoid expensive heap allocations per call.
 *
 ************************************************************************/
template <typename H, typename O, typename M, typename X>
	class bounding_k_nearest :
		private non_constructable
{

	public:

		/** Nodes pending for future expansion. */
		//@{
		typedef bounding_k_nearest_node_candidate<H>
			k_nearest_node_candidate;
		typedef heap<k_nearest_node_candidate>
			k_nearest_node_heap;
		//@}


		/** Neighbor candidates pending for result. */
		//@{
		typedef bounding_k_nearest_neighbor_candidate<H, X>
			k_nearest_neighbor_candidate;
		typedef heap<k_nearest_neighbor_candidate>
			k_nearest_neighbor_heap;
		//@}


		/** Specific result type for the k_nearest() call. */
		//@{
		typedef bounding_k_nearest_entry<H, X> k_nearest_entry;
		typedef bounding_k_nearest_result<H, X> k_nearest_result;
		//@}


		/**
		 * Find @bref{k} nearest neighbors to some @bref{o}bject
		 * in a bounding volume @bref{h}ierarchy.
		 *
		 * See bounding_k_nearest_result for resulting data.
		 *
		 * Usage notes:
		 * - If @bref{distance_squared_bound_valid} is @true,
		 *		then neighbor tests reject any node candidates
		 *		that cannot yield a squared distance less than
		 *		the provided @bref{distance_squared_bound}. 
		 * - Clients usually set @bref{distance_squared_bound}
		 *		to an arbitrary limit for broad-phase rejection.
		 * - Neighbor tests efficiently reject any node candidates
		 *		that cannot improve the currently worst neighbor
		 *		(or an initial, valid @bref{distance_squared_bound})
		 *		by at least the @bref{distance_squared_tolerance}.
		 *		A distance tolerance of zero gives exact results,
		 *		greater tolerances trade errors for performance.
		 *
		 * Algorithm outline:
		 * - Manage a maximum heap of currently found neighbors.
		 *		Discard the maximum on finding a better neighbor
		 *		once the heap matches the size limit @bref{k}.
		 * - Check nodes in the hierarchy against the incoming object.
		 *		Use the client-supplied evaluation potential to find
		 *		the next-best node candidate to check, but cull away
		 *		entire branches if they cannot contain any neighbor
		 *		better than found precedingly, including tolerance.
		 * - On descent, check both candidate children as above.
		 * - Determine correct and expensive distances when arriving
		 *		at leaf nodes, but still check the remaining heap
		 *		after the first leaf for yet closer neighbors.
		 *
		 * Implementation notes:
		 * - To enable skipping of nodes on @false metric returns,
		 *		we must not update our @bref{hit_distance_bound}
		 *		unless we have an actual leaf-level geometry bound.
		 *		Otherwise, we might mistakenly prune nodes due to
		 *		a distance bound on a branch with skipped leaves.
		 */
		static k_nearest_result
			k_nearest
				(const H& h, const O& o, int64 k, const M& metric,				
				 k_nearest_node_heap& node_heap,
				 k_nearest_neighbor_heap& neighbor_heap,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance)
		{		
			// Determine distance bound to the root node.
			// Abort if the root node is to be skipped.
			const_node_iterator root(h.root_node());

			if (!root)
				return k_nearest_result();

			float root_min_distance_squared_lower_bound = 0;
			float root_evaluation_potential = 0;

			if (!metric.try_node_distance_squared
					(h, root, o,
					 root_min_distance_squared_lower_bound,
					 root_evaluation_potential))
			{
				return k_nearest_result();
			}


			// Clear heaps (in case these were preallocated).
			// Also initialize node heap with a single entry
			// for the root node.
			node_heap.clear();
			node_heap.insert
				(k_nearest_node_candidate
					(root,
					 root_min_distance_squared_lower_bound,
					 root_evaluation_potential));

			neighbor_heap.clear();


			// Iteratively update neighbor candidates and distance bound
			// by extracting and testing next-best node candidates.
			while (!node_heap.empty())
			{


				// Extract next-best candidate from node heap
				// and check if it can contain a better minimum.
				// This is beneficial since distance_squared_bound
				// may have updated since inserting the candidate.
				k_nearest_node_candidate node_candidate(node_heap.minimum());
				node_heap.pop_minimum();

				if (distance_squared_bound_valid &&
					node_candidate.min_distance_squared_lower_bound_ >=
						distance_squared_bound - distance_squared_tolerance)
							continue;


				// This candidate may hold a better neighbor,
				// so fetch its children for later expansion.
				const_node_iterator expand[2] =
					{ h.left_node(node_candidate.node_),
					  h.right_node(node_candidate.node_) };

						
				// Arrived at some leaf-level candidate.
				// Unless skipped, determine squared distance to object.
				// Accept neighbor if its distance is below current bounds
				// by more than the distance tolerance.
				if (!expand[0] && !expand[1])
				{
					float leaf_closest_distance_squared = 0;
					X leaf_extended_result;

					if (metric.try_leaf_distance_squared
							(h, node_candidate.node_, o,
							 leaf_closest_distance_squared,
							 leaf_extended_result))
					{
						if (!distance_squared_bound_valid ||
							leaf_closest_distance_squared <
								distance_squared_bound - distance_squared_tolerance)
						{
							if (neighbor_heap.size() >= k)
								neighbor_heap.pop_minimum();

							neighbor_heap.insert
								(k_nearest_neighbor_candidate
									(node_candidate.node_,
									 leaf_closest_distance_squared,
									 leaf_extended_result));

							
							// If we have already found enough neighbor candidates,
							// use worst-distance candidate as an additional bound.
							// This may be:
							// - either the new neighbor candidate,
							// - or the previously worst-distance candidate,
							// - or another, existing neighbor candidate.
							if (neighbor_heap.size() >= k)
							{
								distance_squared_bound =
									neighbor_heap.minimum().closest_distance_squared_;

								distance_squared_bound_valid = true;
							}
						}
					}

					continue;
				}


				// Not at a leaf node yet, so expand further.
				// Bound distance and find potential for children.
				// Finally add unskipped children to candidate heap.
				for (int8 c = 0; c < 2; ++c)
				{
					if (!expand[c]) continue;

					float expand_min_distance_squared_lower_bound = 0;
					float expand_evaluation_potential = 0;

					if (metric.try_node_distance_squared
							(h, expand[c], o,
							 expand_min_distance_squared_lower_bound,
							 expand_evaluation_potential))
					{


						// Bound child (e.g. a sphere) by parent node,
						// as volumes outside our parent must be empty.
						// Then apply early pruning to skip heap insertion.
						expand_min_distance_squared_lower_bound =
							max(expand_min_distance_squared_lower_bound,
								node_candidate.min_distance_squared_lower_bound_);

						if (distance_squared_bound_valid &&
							expand_min_distance_squared_lower_bound >=
								distance_squared_bound - distance_squared_tolerance)
									continue;

						node_heap.insert
							(k_nearest_node_candidate
								(expand[c],
	 							 expand_min_distance_squared_lower_bound,
								 expand_evaluation_potential));
					}
				}
			}


			// Found at most k nearest neighbors.
			// Extract neighbors, then sort by increasing distance.
			k_nearest_result::entry_array entries_descending;
			entries_descending.reserve(neighbor_heap.size());
			
			while (!neighbor_heap.empty())
			{
				k_nearest_neighbor_candidate neighbor_candidate
					(neighbor_heap.minimum());

				entries_descending.push_back
					(k_nearest_entry
						(neighbor_candidate.leaf_node_,
						 neighbor_candidate.closest_distance_squared_,
						 neighbor_candidate.extended_result_));

				neighbor_heap.pop_minimum();
			}

			k_nearest_result::entry_array entries_ascending;
			entries_ascending.reserve(entries_descending.size());
			
			for (int64 i = entries_descending.size() - 1; i >= 0; --i)
				entries_ascending.push_back(entries_descending[i]);
			
			return
				k_nearest_result(entries_ascending);
		}


	private:

		typedef typename H::const_node_iterator const_node_iterator;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_K_NEAREST_HPP) */
