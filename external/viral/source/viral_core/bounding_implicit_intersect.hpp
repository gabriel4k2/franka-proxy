/**
 *************************************************************************
 *
 * @file bounding_implicit_intersect.hpp
 *
 * Intersection of implicit bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_INTERSECT_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_INTERSECT_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Various bounding_intersect_result%s for common bounding_implicit%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_intersect_result
			<sphere_tree, sphere_tree,
			 bounding_intersect_no_extended_result>
					bounding_implicit_intersect_result_sphere_tree;

typedef bounding_intersect_result
			<box_tree, box_tree,
			 bounding_intersect_no_extended_result>
					bounding_implicit_intersect_result_box_tree;
//@}




/**
 *************************************************************************
 *
 * @class bounding_implicit_intersect_test
 * @ingroup bounding_module
 *
 * A bounding_intersect test for bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_intersect_test :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		bounding_implicit_intersect_test
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_intersecting
			(const sphere_tree& ta,
			 const sphere_tree_const_node_iterator& na,
			 const sphere_tree& tb,
			 const sphere_tree_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree& ta,
			 const box_tree_const_node_iterator& na,
			 const box_tree& tb,
			 const box_tree_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const sphere_tree& ta,
			 const sphere_tree_const_node_iterator& na,
			 const sphere_tree& tb,
			 const sphere_tree_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree& ta,
			 const box_tree_const_node_iterator& na,
			 const box_tree& tb,
			 const box_tree_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;


		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class bounding_implicit_intersect
 * @ingroup bounding_module
 *
 * Static utilities that check whether two bounding_implicit%s intersect.
 *
 ************************************************************************/
class bounding_implicit_intersect :
	public non_constructable
{

	public:
		
		/**
		 * Test two si_transform%ed bounding_implicit%s for intersection.
		 *
		 * Usage notes:
		 * - On any error, intersection tests throw an exception.
		 * - See geo_intersect and bounding_intersect for generic notes.
		 */
		//@{
		static bounding_implicit_intersect_result_sphere_tree
			intersect
				(const sphere_tree& ta, const si_transform& transform_ta,
				 const sphere_tree& tb, const si_transform& transform_tb);

		static bounding_implicit_intersect_result_box_tree
			intersect
				(const box_tree& ta, const si_transform& transform_ta,
				 const box_tree& tb, const si_transform& transform_tb);
		//@}


	private:

		typedef bounding_intersect
					<sphere_tree, sphere_tree,
					 bounding_implicit_intersect_test,
					 bounding_intersect_no_extended_result>
							bounding_intersect_sphere_tree;

		typedef bounding_intersect
					<box_tree, box_tree,
					 bounding_implicit_intersect_test,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_tree;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_INTERSECT_HPP) */
