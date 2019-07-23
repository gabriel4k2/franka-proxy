/**
 *************************************************************************
 *
 * @file bounding_implicit_proximate.hpp
 *
 * Proximity for implicit bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_PROXIMATE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_PROXIMATE_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Various bounding_proximate_result%s for common bounding_implicit%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_proximate_result
			<sphere_tree, sphere_tree,
			 bounding_proximate_no_extended_result>
					bounding_implicit_proximate_result_sphere_tree;

typedef bounding_proximate_result
			<box_tree, box_tree,
			 bounding_proximate_no_extended_result>
					bounding_implicit_proximate_result_box_tree;
//@}




/**
 *************************************************************************
 *
 * @class bounding_implicit_proximate_test
 * @ingroup bounding_module
 *
 * A bounding_proximate test for bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_proximate_test :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:

		bounding_implicit_proximate_test
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_proximate
			(const sphere_tree& ta,
			 const sphere_tree_const_node_iterator& na,
			 const sphere_tree& tb,
			 const sphere_tree_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;
		bool node_proximate
			(const box_tree& ta,
			 const box_tree_const_node_iterator& na,
			 const box_tree& tb,
			 const box_tree_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;

		bool leaf_proximate
			(const sphere_tree& ta,
			 const sphere_tree_const_node_iterator& na,
			 const sphere_tree& tb,
			 const sphere_tree_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;
		bool leaf_proximate
			(const box_tree& ta,
			 const box_tree_const_node_iterator& na,
			 const box_tree& tb,
			 const box_tree_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;


		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class bounding_implicit_proximate
 * @ingroup bounding_module
 *
 * Static utilities that test if two transformed bounding_implicit%s
 * are in proximity to each other.
 *
 ************************************************************************/
class bounding_implicit_proximate :
	private non_constructable
{

	public:

		/**
		 * Perform a proximity test for two bounding_implicit%s
		 * under arbitrary si_transform%s.
		 *
		 * Usage notes:
		 * - On any error, proximity tests throw an exception.
		 * - See geo_proximity and bounding_proximity for generic notes,
		 *		notably on the use of @bref{distance_squared_upper_bound}.
		 */
		//@{
		static bounding_implicit_proximate_result_sphere_tree
			proximate
				(const sphere_tree& ta, const si_transform& transform_ta,
				 const sphere_tree& tb, const si_transform& transform_tb,
				 float distance_squared_upper_bound);

		static bounding_implicit_proximate_result_box_tree
			proximate
				(const box_tree& ta, const si_transform& transform_ta,
				 const box_tree& tb, const si_transform& transform_tb,
				 float distance_squared_upper_bound);
		//@}


	private:

		typedef bounding_proximate
					<sphere_tree, sphere_tree,
					 bounding_implicit_proximate_test,
					 bounding_proximate_no_extended_result>
							bounding_proximate_sphere_tree;

		typedef bounding_proximate
					<box_tree, box_tree,
					 bounding_implicit_proximate_test,
					 bounding_proximate_no_extended_result>
							bounding_proximate_box_tree;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_PROXIMATE_HPP) */
