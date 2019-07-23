/**
 *************************************************************************
 *
 * @file bounding_implicit_k_nearest.hpp
 *
 * Nearest neighbors in implicit bounding hierarchies.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_K_NEAREST_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_K_NEAREST_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "bounding_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 * Various bounding_k_nearest_result%s for common bounding_implicit%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_k_nearest_result
			<sphere_tree, bounding_k_nearest_no_extended_result>
					bounding_implicit_k_nearest_result_sphere_tree;

typedef bounding_k_nearest_result
			<box_tree, bounding_k_nearest_no_extended_result>
					bounding_implicit_k_nearest_result_box_tree;
//@}




/**
 *************************************************************************
 *
 * @class bounding_implicit_k_nearest_metric
 * @ingroup bounding_module
 *
 * A bounding_k_nearest metric for bounding_implicit%s.
 *
 ************************************************************************/
class bounding_implicit_k_nearest_metric :
	private non_copyable
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		bounding_implicit_k_nearest_metric() NOTHROW;


	private:

		bool try_node_distance_squared
			(const sphere_tree& t,
			 const sphere_tree_const_node_iterator& n,
			 const vector& o,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;
		bool try_node_distance_squared
			(const box_tree& t,
			 const box_tree_const_node_iterator& n,
			 const vector& o,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree& t,
			 const sphere_tree_const_node_iterator& n,
			 const vector& o,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;
		bool try_leaf_distance_squared
			(const box_tree& t,
			 const box_tree_const_node_iterator& n,
			 const vector& o,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class bounding_implicit_k_nearest
 * @ingroup bounding_module
 *
 * Static utilities that find nearest neighbors to some object
 * in a bounding_implicit.
 *
 ************************************************************************/
class bounding_implicit_k_nearest :
	public non_constructable
{

	public:
		
		/**
		 * Find @bref{k} nearest nodes to the given @bref{point}
		 * in a si_transform%ed bounding_implicit.
		 *
		 * Usage notes:
		 * - On any error, nearest neighbor queries throw an exception.
		 * - See geo_distance and bounding_k_nearest for generic notes,
		 *		notably for details on @bref{distance_squared_bound}
		 *		and @bref{distance_squared_tolerance}.
		 *
		 * Implementation notes:
		 * - To avoid expensive transforms per shape and object,
		 *		we instead transform the incoming @bref{p}oint
		 *		by the inverse of @bref{transform_t}.
		 */
		//@{
		static bounding_implicit_k_nearest_result_sphere_tree
			k_nearest
				(const sphere_tree& t, const si_transform& transform_t,
				 const vector& point,
				 int64 k,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);

		static bounding_implicit_k_nearest_result_box_tree
			k_nearest
				(const box_tree& t, const si_transform& transform_t,
				 const vector& point,
				 int64 k,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		//@}


	private:

		typedef bounding_k_nearest
					<sphere_tree, vector,
					 bounding_implicit_k_nearest_metric,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_sphere_tree;

		typedef bounding_k_nearest
					<box_tree, vector,
					 bounding_implicit_k_nearest_metric,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_box_tree;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_K_NEAREST_HPP) */