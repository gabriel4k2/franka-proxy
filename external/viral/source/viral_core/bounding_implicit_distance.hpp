/**
 *************************************************************************
 *
 * @file bounding_implicit_distance.hpp
 *
 * Distances between implicit bounding volume hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_DISTANCE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_DISTANCE_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Various bounding_distance_result%s for common bounding_implicit%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_distance_result
			<sphere_tree, sphere_tree,
			 bounding_distance_no_extended_result>
					bounding_implicit_distance_result_sphere_tree;

typedef bounding_distance_result
			<box_tree, box_tree,
			 bounding_distance_no_extended_result>
					bounding_implicit_distance_result_box_tree;
//@}




/**
 *************************************************************************
 *
 * @class bounding_implicit_distance_metric
 * @ingroup bounding_module
 *
 * A bounding_distance metric for bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_distance_metric :
	private non_copyable
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:

		bounding_implicit_distance_metric
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool try_node_distance_squared
			(const sphere_tree& ta,
			 const sphere_tree_const_node_iterator& na,
			 const sphere_tree& tb,
			 const sphere_tree_const_node_iterator& nb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;
		bool try_node_distance_squared
			(const box_tree& ta,
			 const box_tree_const_node_iterator& na,
			 const box_tree& tb,
			 const box_tree_const_node_iterator& nb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree& ta,
			 const sphere_tree_const_node_iterator& na,
			 const sphere_tree& tb,
			 const sphere_tree_const_node_iterator& nb,
			 vector& closest_point_ta, vector& closest_point_tb,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;
		bool try_leaf_distance_squared
			(const box_tree& ta,
			 const box_tree_const_node_iterator& na,
			 const box_tree& tb,
			 const box_tree_const_node_iterator& nb,
			 vector& closest_point_ta, vector& closest_point_tb,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;


		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class bounding_implicit_distance
 * @ingroup bounding_module
 *
 * Static utilities that find closest points and distances
 * between two bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_distance :
	public non_constructable
{

	public:
		
		/**
		 * Perform a closest-distance test for two bounding_implicit%s
		 * under arbitrary si_transform%s.
		 *
		 * Usage notes:
		 * - On any error, distance tests throw an exception.
		 * - See geo_distance and bounding_distance for generic notes,
		 *		notably for details on @bref{distance_squared_bound}
		 *		and @bref{distance_squared_tolerance}.
		 */
		//@{
		static bounding_implicit_distance_result_sphere_tree
			distance
				(const sphere_tree& ta, const si_transform& transform_ta,
				 const sphere_tree& tb, const si_transform& transform_tb,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);

		static bounding_implicit_distance_result_box_tree
			distance
				(const box_tree& ta, const si_transform& transform_ta,
				 const box_tree& tb, const si_transform& transform_tb,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		//@}


	private:

		typedef bounding_distance
					<sphere_tree, sphere_tree,
					 bounding_implicit_distance_metric,
					 bounding_distance_no_extended_result>
							bounding_distance_sphere_tree;

		typedef bounding_distance
					<box_tree, box_tree,
					 bounding_implicit_distance_metric,
					 bounding_distance_no_extended_result>
							bounding_distance_box_tree;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_DISTANCE_HPP) */
