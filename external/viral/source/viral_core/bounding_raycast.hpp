/**
 *************************************************************************
 *
 * @file bounding_raycast.hpp
 *
 * Raycasting tests for bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_RAYCAST_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_RAYCAST_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "heap.hpp"
#include "geo_vector.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_raycast_candidate
 * @ingroup bounding_module
 *
 * A single candidate node in a bounding_raycast test
 * over a bounding volume hierarchy of type @bref{H}.
 *
 ************************************************************************/
template <typename H>
	class bounding_raycast_candidate
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;


	public:

		bool operator <(const bounding_raycast_candidate& other) const NOTHROW
		{
			// Sort highest-potential candidates to front of heap.
			return evaluation_potential_ > other.evaluation_potential_;
		}


		void swap_with(bounding_raycast_candidate& other) NOTHROW
		{
			swap(node_, other.node_);
			swap(hit_distance_lower_bound_, other.hit_distance_lower_bound_);
			swap(evaluation_potential_, other.evaluation_potential_);
		}


	private:

		bounding_raycast_candidate
			(const typename H::const_node_iterator& node,
			 float hit_distance_lower_bound,
			 float evaluation_potential) NOTHROW
			:
			node_(node),
			hit_distance_lower_bound_(hit_distance_lower_bound),
			evaluation_potential_(evaluation_potential)
		{ }


		typename H::const_node_iterator node_;
		float hit_distance_lower_bound_;
		float evaluation_potential_;
};




/**
 *************************************************************************
 *
 * @class bounding_raycast_result
 * @ingroup bounding_module
 *
 * Stores bounding_raycast results in form of the first @bref{hit_point},
 * the signed @bref{hit_distance} from the ray origin to the hit point,
 * and an iterator for the respective node in the hierarchy.
 *
 * Usage notes:
 * - If the ray is not @bref{intersecting} the geometry,
 *		then the node iterator is a past-end iterator,
 *		both the hit point and the hit distance are zero,
 *		and the extended result is default-constructed.
 * - Raycasting tests may pick any of multiple candidates for first hit,
 *		potentially causing an error of up to the distance tolerance.
 *
 ************************************************************************/
template <typename H, typename X>
	class bounding_raycast_result
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;


	public:

		/** Iterator type for nodes in the bounding volume hierarchy. */
		typedef typename H::const_node_iterator const_node_iterator;


		const bool intersecting;

		const const_node_iterator hit_node;
		const vector hit_point;
		const float hit_distance;
		const X extended_result;


	private:

		bounding_raycast_result() NOTHROW
			:
			intersecting(false),
			hit_distance(0.f)
		{ }


		bounding_raycast_result
			(const const_node_iterator& hit_node,
			 const vector& hit_point,
			 float hit_distance,
			 const X& extended_result) NOTHROW
			:
			intersecting(true),
			hit_node(hit_node),
			hit_point(hit_point),
			hit_distance(hit_distance),
			extended_result(extended_result)
		{ }
};




/**
 *************************************************************************
 *
 * @class bounding_raycast_no_extended_result
 * @ingroup bounding_module
 *
 * Supply to bounding_raycast if no extended result is required.
 *
 ************************************************************************/
class bounding_raycast_no_extended_result { };




/**
 *************************************************************************
 *
 * @class bounding_raycast
 * @ingroup bounding_module
 *
 * Finds the first hit point of a ray with type @bref{R}
 * against a bounding volume hierarchy of type @bref{H}.
 * 
 * Raycasting tests:
 * - Clients must provide a raycasting test @bref{T}
 *		for node boundings and geometry objects.
 * - bool T::try_node_raycast
 *			(const R& ray, const H& h, const const_node_iterator& n,
 *			 float& hit_distance_lower_bound,
 *			 float& evaluation_potential) const
 *		must return @true if the leaf-level geometry objects subordinate
 *		to an arbitrary node @bref{n} may intersect the given @bref{ray}.
 *		The call must also calculate a lower bound for the signed distance
 *		from the ray origin to the first potential hit point at leaf level.
 *		Finally, the call must find an evaluation potential in [-INF, +INF]
 *		that rates nodes for further evaluation, e.g. by traversal costs
 *		or intersection probability. False positive results are allowed,
 *		for instance due to bounding tests or early-out optimization.
 * - bool T::try_leaf_raycast
 *			(const R& ray, const H& h, const const_node_iterator& n,
 *			 vector& hit_point, float& hit_distance,
 *			 X& extended_result) const
 *		must return @true if the @bref{ray} intersects with geometry
 *		in a leaf-level node @bref{n}. The call then must also find
 *		the first @bref{hit_point} along the direction of @bref{ray}
 *		and the signed distance from the ray origin to the hitpoint.
 * - Both calls may return @false to skip ray-geometry intersections,
 *		for example to indicate that no such intersection is possible,
 *		or to explicitly exclude any hierarchy branches from testing.
 *		In this case, tests need not set any output variables.
 * - The node iterator @bref{n} is never a past-end iterator.
 *		Both raycasting methods should avoid redundant checks.
 * - Both raycasting methods must throw to indicate failure.
 *		The overall raycasting test then fails as well.
 *
 * Extended results:
 * - Clients can pass back additional data from the leaf-level
 *		raycasting test on the closest intersecting geometry
 *		(e.g. closest index of a subordinate raycasting test)
 *		through an @bref{extended_result} of type @bref{X}.
 * - Type @bref{X} must be default- and copy-constructable,
 *		assignable, and @ref NOTHROW in all three operations.
 * - If no extended result is required, clients can supply
 *		bounding_raycast_no_extended_result as @bref{X}.
 *
 * General usage notes:
 * - See the @ref bounding_module "Bounding Module" for requirements
 *		on the bounding volume hierarchy @bref{H}.
 * - To realize transformed objects, clients can store transformations
 *		in the instance of the raycasting @bref{test}. The test can then
 *		apply transforms to shapes and geometry before actual raycasting.
 * -  Clients should provide a preallocated @bref{node_heap}
 *		to avoid expensive heap allocations per call.
 *
 ************************************************************************/
template <typename R, typename H, typename T, typename X>
	class bounding_raycast :
		private non_constructable
{

	public:

		/** Nodes pending for future expansion. */
		//@{
		typedef bounding_raycast_candidate<H> raycast_candidate;
		typedef heap<raycast_candidate> raycast_heap;
		//@}


		/** Specific result type for raycast() call. */
		typedef bounding_raycast_result<H, X> raycast_result;


		/**
		 * Cast a @bref{ray} (e.g. a @ref line or a @ref segment)
		 * against an arbitrary bounding volume @bref{h}ierarchy.
		 * See bounding_raycast_result for resulting data.
		 *
		 * Usage notes:
		 * - If the @bref{hit_distance_bound_valid} flag is @true,
		 *		then raycasting tests reject any node candidates
		 *		that cannot yield a signed first-hit distance
		 *		less than the signed @bref{hit_distance_bound}. 
		 * - Clients can set the signed @bref{hit_distance_bound}
		 *		to an arbitrary limit for broad-phase rejection
		 *		or to the result of a preceding raycast() call.
		 *		The latter allows clients to perform raycasting
		 *		over multiple bounding volume hierarchies.
		 * - Raycasting tests efficiently reject any node candidates
		 *		that cannot reduce the current guess for hit distance
		 *		(or an initial, signed, valid @bref{hit_distance_bound})
		 *		by at least the (positive) @bref{hit_distance_tolerance}.
		 *		A distance tolerance of zero gives exact results,
		 *		greater tolerances trade errors for performance.
		 *
		 * Algorithm outline:
		 * - Iteratively expand nodes in the bounding volume hierarchy.
		 * - Sort all node candidates by their evaluation potential.
		 * - Reject any candidates without a ray-geometry intersection,
		 *		or which cannot hold a closer hit point after tolerance.
		 * - Determine correct and expensive first hits when arriving
		 *		at leaf-level geometry, but still check remaining heap
		 *		after the first intersection for a yet closer hit point.
		 *
		 * Implementation notes:
		 * - To enable skipping of nodes on @false test returns,
		 *		we must not update our @bref{hit_distance_bound}
		 *		unless we have an actual leaf-level geometry bound.
		 *		Otherwise, we might mistakenly prune nodes due to
		 *		a distance bound on a branch with skipped leaves.
		 * - We do not use squared distances, as choosing a hit point
		 *		requires signed (instead of absolute) distances.
		 */
		static raycast_result
			raycast
				(const R& ray, const H& h, const T& test,
				 raycast_heap& node_heap,
				 float hit_distance_bound,
				 bool hit_distance_bound_valid,
				 float hit_distance_tolerance)
		{
			// Determine distance bound of the root node.
			// Abort if the root is to be skipped.
			const_node_iterator root(h.root_node());

			if (!root)
				return raycast_result();

			float root_hit_distance_lower_bound = 0;
			float root_evaluation_potential = 0;

			if (!test.try_node_raycast
					(ray, h, root,
					 root_hit_distance_lower_bound,
					 root_evaluation_potential))
			{
				return raycast_result();
			}


			// Clear heap (in case it was preallocated),
			// then initialize heap with a single entry
			// for the root node.
			node_heap.clear();
			node_heap.insert
				(raycast_candidate
					(root,
					 root_hit_distance_lower_bound,
					 root_evaluation_potential));


			// Iteratively update hit point, node, and distance bound
			// by extracting next-best nodes from the candidate heap.
			const_node_iterator result_node;
			vector result_hit_point;
			X result_extended_result;
			bool result_valid = false;
		
			while (!node_heap.empty())
			{


				// Extract next-best node candidate from the node heap
				// and check if this node can contain a better hit distance.
				// This is beneficial because the overall distance_bound
				// may have updated since the insertion of the node.
				raycast_candidate candidate(node_heap.minimum());
				node_heap.pop_minimum();

				if (hit_distance_bound_valid &&
					candidate.hit_distance_lower_bound_ >=
						hit_distance_bound - hit_distance_tolerance)
							continue;


				// Pending node can contain a better minimum,
				// so fetch its children for later expansion.
				// Take note of leaves for geometry handling.
				const_node_iterator expand[2] =
					{ h.left_node(candidate.node_),
					  h.right_node(candidate.node_) };
				bool is_leaf =
					!expand[0] && !expand[1];
						

				// Arrived at some leaf node.
				// Determine hit point and signed distance to hit point.
				// Accept these if the distance improves current bounds
				// by more than the distance tolerance.
				if (is_leaf)
				{
					vector leaf_hit_point;
					float leaf_hit_distance = 0;
					X leaf_extended_result;

					if (test.try_leaf_raycast
							(ray, h, candidate.node_,
							 leaf_hit_point, leaf_hit_distance,
							 leaf_extended_result))
					{
						if (!hit_distance_bound_valid ||
							leaf_hit_distance <
								hit_distance_bound - hit_distance_tolerance)
						{
							hit_distance_bound = leaf_hit_distance;
							hit_distance_bound_valid = true;
						
							result_node = candidate.node_;
							result_hit_point = leaf_hit_point;
							result_extended_result = leaf_extended_result;
							result_valid = true;
						}
					}

					continue;
				}


				// Not a leaf node, so expand children:
				// Bound distance and find potential for these,
				// then add all unskipped, intersecting children
				// to our candidate heap.
				for (int8 c = 0; c < 2; ++c)
				{
					if (!expand[c])
						continue;

					float expand_hit_distance_lower_bound = 0;
					float expand_evaluation_potential = 0;

					if (test.try_node_raycast
							(ray, h, expand[c],
							 expand_hit_distance_lower_bound,
							 expand_evaluation_potential))
					{


						// Bound children (e.g. spheres) by parent node,
						// as volumes outside our parent must be empty.
						// Then apply early pruning to skip heap insertion.
						expand_hit_distance_lower_bound =
							max(expand_hit_distance_lower_bound,
								candidate.hit_distance_lower_bound_);

						if (hit_distance_bound_valid &&
							expand_hit_distance_lower_bound >=
								hit_distance_bound - hit_distance_tolerance)
									continue;

						node_heap.insert
							(raycast_candidate
								(expand[c],
	 							 expand_hit_distance_lower_bound,
								 expand_evaluation_potential));
					}
				}
			}


			// If no valid leaf node has been encountered
			// (e.g. due to distance bound), we abort here.
			if (!result_valid)
				return raycast_result();


			// We had at least a single valid leaf-level intersection,
			// so return respective node, hit point, and hit distance.
			return
				raycast_result
					(result_node, result_hit_point,
					 hit_distance_bound, result_extended_result);
		}


	private:

		typedef typename H::const_node_iterator const_node_iterator;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_RAYCAST_HPP) */
