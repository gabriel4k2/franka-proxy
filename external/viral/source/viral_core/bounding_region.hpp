/**
 *************************************************************************
 *
 * @file bounding_region.hpp
 *
 * Tests for bounding volume hierarchies against regions.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_REGION_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_REGION_HPP


#include "build_config.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_region_entry
 * @ingroup bounding_module
 *
 * A single entry in a bounding_region_result.
 *
 ************************************************************************/
template <typename H, typename X>
	class bounding_region_entry
{

	public:

		/** Iterator type for nodes in the bounding volume hierarchy. */
		typedef typename H::const_node_iterator const_node_iterator;


		bounding_region_entry
			(const const_node_iterator& leaf_node,
			 const X& extended_result) NOTHROW
			:
			leaf_node(leaf_node),
			extended_result(extended_result)
		{ }


		const const_node_iterator leaf_node;
		const X extended_result;
};




/**
 *************************************************************************
 *
 * @class bounding_region_result
 * @ingroup bounding_module
 *
 * Stores @bref{entries} for leaf nodes in a bounding_region query.
 *
 ************************************************************************/
template <typename H, typename X>
	class bounding_region_result
{
	template <typename H, typename R, typename T, typename X>
		friend class bounding_region;


	public:

		typedef array<bounding_region_entry<H, X>> entry_array;
		const entry_array entries;


	private:

		bounding_region_result() { }

		bounding_region_result(const entry_array& entries)
			: entries(entries) { }
};




/**
 *************************************************************************
 *
 * @class bounding_region_no_extended_result
 * @ingroup bounding_module
 *
 * Supply to bounding_region if no extended result is required.
 *
 ************************************************************************/
class bounding_region_no_extended_result { };




/**
 *************************************************************************
 *
 * @class bounding_region
 * @ingroup bounding_module
 *
 * Collect all leaf nodes of a bounding volume @bref{H}ierarchy
 * inside a specified @bref{R}egion.
 *
 * Region tests:
 * - Clients must provide a region test @bref{T} for node boundings
 *		and geometry objects against the @bref{R}egion.
 * - Region tests may arbitrarily decide to test for either partial
 *		or full containment, but must do so consistently.
 * - bool T::try_node_region
 *			(const H& h, const const_node_iterator& n,
 *			 const R& region) const
 *		must return @true if the given @bref{region} may partially
 *		or fully contain the leaf-level geometry objects subordinate
 *		to an arbitrary node @bref{n}. False positives are allowed,
 *		for example due to bounding tests or early-out optimization.
 * - bool T::try_leaf_region
 *			(const H& h, const const_node_iterator& n,
 *			 const R& region, 
 *			 X& extended_result) const
 *		must return @true if the @bref{region} partially or fully
 *		contains the geometry object in a leaf-level node @bref{n}.
 * - Both calls may return @false to exclude geometry from region tests,
 *		for example to indicate that the region cannot contain geometry,
 *		or to explicitly exclude any hierarchy branches from testing.
 *		In this case, tests need not set any output variables.
 * - The node iterator @bref{n} is never a past-end iterator.
 *		Both region tests should avoid redundant checks.
 * - Both region tests must throw to indicate failure.
 *		The overall region test then fails as well.
 *
 * Extended results:
 * - Clients can pass back additional data from the leaf-level region test
 *		on geometry inside the region (e.g. results of a subordinate test)
 *		through an @bref{extended_result} of type @bref{X}.
 * - Type @bref{X} must be default- and copy-constructable,
 *		assignable, and @ref NOTHROW in all three operations.
 * - If no extended result is required, clients can supply
 *		bounding_region_no_extended_result as @bref{X}.
 *
 * General usage notes:
 * - See the @ref bounding_module "Bounding Module" for requirements
 *		on the bounding volume hierarchy @bref{H}.
 * - To realize transformed objects, clients can store transformations
 *		in the instance of the region @bref{test}. The region test can
 *		then apply transforms to shapes and geometry on each call.
 * -  Clients should provide a preallocated @bref{region_array}
 *		to avoid expensive array allocations per call.
 *
 ************************************************************************/
template <typename H, typename R, typename T, typename X>
	class bounding_region :
		private non_constructable
{

	public:

		/** Nodes pending for future expansion. */
		typedef array<typename H::const_node_iterator> region_array;


		/** Specific result type for the region() call. */
		//@{
		typedef bounding_region_entry<H, X> region_entry;
		typedef bounding_region_result<H, X> region_result;
		//@}


		/**
		 * Find geometry of @bref{h}ierarchy in @bref{region}.
		 * See bounding_region_result for resulting data.
		 *
		 * Algorithm outline:
		 * - Recursively expand nodes that are contained in the region.
		 * - Discard any nodes that are not contained in the region.
		 * - We need no evaluation potential, as there is no early-out.
		 */
		static region_result
			region
				(const H& h, const R& region, const T& test,
				 region_array& node_array)
		{		
			// Early-out on empty hierarchy
			// or on non-contained root.
			const_node_iterator root(h.root_node());

			if (!root)
				return region_result();

			if (!test.try_node_region(h, root, region))
				return region_result();


			// Clear array (in case it was preallocated),
			// then initialize array with a single entry
			// for the (partially) contained root.
			node_array.clear();
			node_array.push_back(root);
		

			// Iteratively put together contained leaves.
			region_result::entry_array result_entries;
			while (!node_array.empty())
			{


				// Extract next node candidate from array.
				// Use the last array entry for O(1) runtime.
				// We already know this node is in the region,
				// so check respective children nodes.
				const_node_iterator node(node_array[node_array.size() - 1]);
				node_array.pop_back();
			
				const_node_iterator expand[2] =
					{ h.left_node(node), h.right_node(node) };
				bool is_leaf =
					!expand[0] && !expand[1];

			
				// Node has no children and its bounding is in the region,
				// thus test if leaf-level geometry is in the region as well.
				// In this case, store node for result.
				if (is_leaf)
				{
					X extended_result;
					if (test.try_leaf_region
							(h, node, region, extended_result))
					{
						result_entries.push_back
							(bounding_region_entry<H, X>(node, extended_result));
					}

					continue;
				}


				// Not a leaf node, so test all children against region,
				// then store contained children for further expansion.
				if (expand[0] &&
					test.try_node_region(h, expand[0], region))
						node_array.push_back(expand[0]);			

				if (expand[1] &&
					test.try_node_region(h, expand[1], region))
						node_array.push_back(expand[1]);
			}


			// Finished, pass back result.
			return region_result(result_entries);
		}


	private:

		typedef typename H::const_node_iterator const_node_iterator;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_REGION_HPP) */
