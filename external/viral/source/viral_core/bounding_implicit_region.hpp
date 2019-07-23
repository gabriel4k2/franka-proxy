/**
 *************************************************************************
 *
 * @file bounding_implicit_region.hpp
 *
 * Region tests for implicit bounding hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_REGION_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_REGION_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Various bounding_region_result%s for common bounding_implicit%s.
 * @ingroup mesh_module
 */
//@{
typedef bounding_region_result
			<sphere_tree, bounding_region_no_extended_result>
					bounding_implicit_region_result_sphere_tree;

typedef bounding_region_result
			<box_tree, bounding_region_no_extended_result>
					bounding_implicit_region_result_box_tree;
//@}




/**
 *************************************************************************
 *
 * @class bounding_implicit_region_test
 * @ingroup bounding_module
 * 
 * A bounding_region test for bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_region_test :
	private non_copyable
{
	template <typename H, typename R, typename T, typename X>
		friend class bounding_region;


	public:

		bounding_implicit_region_test
			(const si_transform& transform_t) NOTHROW;


	private:

		bool try_node_region
			(const sphere_tree& t,
			 const sphere_tree_const_node_iterator& n,
			 const sphere& s) const;
		bool try_node_region
			(const box_tree& t,
			 const box_tree_const_node_iterator& n,
			 const box& b) const;

		bool try_leaf_region
			(const sphere_tree& t,
			 const sphere_tree_const_node_iterator& n,
			 const sphere& s,
			 bounding_region_no_extended_result& extended_result) const;
		bool try_leaf_region
			(const box_tree& t,
			 const box_tree_const_node_iterator& n,
			 const box& b,
			 bounding_region_no_extended_result& extended_result) const;


		const si_transform transform_t_;
		const matrix to_world_matrix_t_;
};




/**
 ************************************************************************* 
 *
 * @class bounding_implicit_region
 * @ingroup bounding_module
 *
 * Static utilities that query all leaf nodes of a bounding_implicit
 * inside a given region.
 *
 ************************************************************************/
class bounding_implicit_region :
	private non_constructable
{

	public:

		/**
		 * Test leaf nodes of a si_transform%ed bounding_implicit
		 * against a @bref{s}phere or a @bref{b}ox.
		 *
		 * Usage notes:
		 * - In contrast to other bounding_region queries,
		 *		queries on bounding_implicit%s return nodes
		 *		that are partially inside the given region.
		 * - On any error, all region queries throw an exception.
		 * - See geo_intersect and bounding_region for generic notes.
		 */
		//@{
		static bounding_implicit_region_result_sphere_tree
			region
				(const sphere_tree& t,
				 const si_transform& transform_t,
				 const sphere& s);

		static bounding_implicit_region_result_box_tree
			region
				(const box_tree& t, 
				 const si_transform& transform_t,
				 const box& b);
		//@}


	private:

		typedef bounding_region
					<sphere_tree, sphere,
					 bounding_implicit_region_test,
					 bounding_region_no_extended_result>
							bounding_region_sphere_tree_sphere;

		typedef bounding_region
					<box_tree, box,
					 bounding_implicit_region_test,
					 bounding_region_no_extended_result>
							bounding_region_box_tree_box;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_REGION_HPP) */