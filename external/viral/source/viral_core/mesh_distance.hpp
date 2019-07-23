/**
 *************************************************************************
 *
 * @file mesh_distance.hpp
 *
 * Distance between meshes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_DISTANCE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_DISTANCE_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Common bounding_distance_result%s for mesh_distance.
 * @ingroup mesh_module
 */
//@{
typedef bounding_distance_result
			<sphere_tree_triangle, sphere_tree_triangle,
			 bounding_distance_no_extended_result>
					mesh_distance_result_sphere_tree_triangle;
typedef bounding_distance_result
			<box_tree_triangle, box_tree_triangle,
			 bounding_distance_no_extended_result>
					mesh_distance_result_box_tree_triangle;

typedef bounding_distance_result
			<sphere_tree_index, sphere_tree_index,
			 bounding_distance_no_extended_result>
					mesh_distance_result_sphere_tree_index;
typedef bounding_distance_result
			<box_tree_index, box_tree_index,
			 bounding_distance_no_extended_result>
					mesh_distance_result_box_tree_index;
//@}




/**
 *************************************************************************
 *
 * @class mesh_distance_metric_triangle
 * @ingroup mesh_module
 *
 * A bounding_distance metric for explicit triangle objects.
 *
 ************************************************************************/
class mesh_distance_metric_triangle :
	private non_copyable
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:

		mesh_distance_metric_triangle
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool try_node_distance_squared
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const sphere_tree_triangle& tb,
			 const sphere_tree_triangle_const_node_iterator& nb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;
		bool try_node_distance_squared
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const box_tree_triangle& tb,
			 const box_tree_triangle_const_node_iterator& nb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const sphere_tree_triangle& tb,
			 const sphere_tree_triangle_const_node_iterator& nb,
			 vector& closest_point_ta, vector& closest_point_tb,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;
		bool try_leaf_distance_squared
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const box_tree_triangle& tb,
			 const box_tree_triangle_const_node_iterator& nb,
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
 * @class mesh_distance_metric_index
 * @ingroup mesh_module
 *
 * A bounding_distance metric for triangle indices into a @ref mesh.
 *
 ************************************************************************/
class mesh_distance_metric_index :
	private non_copyable
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:
	
		mesh_distance_metric_index
			(const mesh& mesh_ta,
			 const si_transform& transform_ta,
			 const mesh& mesh_tb,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool try_node_distance_squared
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const sphere_tree_index& tb,
			 const sphere_tree_index_const_node_iterator& nb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;
		bool try_node_distance_squared
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const box_tree_index& tb,
			 const box_tree_index_const_node_iterator& nb,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const sphere_tree_index& tb,
			 const sphere_tree_index_const_node_iterator& nb,
			 vector& closest_point_ta, vector& closest_point_tb,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;
		bool try_leaf_distance_squared
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const box_tree_index& tb,
			 const box_tree_index_const_node_iterator& nb,
			 vector& closest_point_ta, vector& closest_point_tb,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;


		const mesh& mesh_ta_; // Don't copy, expensive.
		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const mesh& mesh_tb_; // Don't copy, expensive.
		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class mesh_distance
 * @ingroup mesh_module
 *
 * Static utilities that calculate the distance between meshes, 
 * ideally under use of bounding_tree%s.
 *
 * Implementation notes:
 * - Current distance utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_distance :
	private non_constructable
{

	public:

		/**
		 * Perform a closest-distance test for two bounding_tree%s
		 * under arbitrary si_transform%s.
		 *
		 * Usage notes:
		 * - Mesh distance tests support explicit and indexed trees.
		 *		In the latter case, clients must supply suitable
		 *		target meshes for the tree indices.
		 * - On any error, mesh_distance tests throw an exception.
		 * - See geo_distance and bounding_distance for generic notes,
		 *		notably for details on @bref{distance_squared_bound}
		 *		and @bref{distance_squared_tolerance}.
		 */
		//@{
		static mesh_distance_result_sphere_tree_triangle
			distance
				(const sphere_tree_triangle& ta, const si_transform& transform_ta,
				 const sphere_tree_triangle& tb, const si_transform& transform_tb,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		static mesh_distance_result_box_tree_triangle
			distance
				(const box_tree_triangle& ta, const si_transform& transform_ta,
				 const box_tree_triangle& tb, const si_transform& transform_tb,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);

		static mesh_distance_result_sphere_tree_index
			distance
				(const sphere_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const sphere_tree_index& tb, const mesh& mesh_tb,
				 const si_transform& transform_tb,			
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		static mesh_distance_result_box_tree_index
			distance
				(const box_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const box_tree_index& tb, const mesh& mesh_tb,
				 const si_transform& transform_tb,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		//@}


		/**
		 * Find distance between two transformed @ref mesh%es
		 * by explicitly testing each and every triangle pair.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 *
		 * On failure (e.g. a @false return or an exception),
		 * the call zeros closest points and closest distance.
		 */
		static bool try_distance_slow
			(const mesh& ma, const si_transform& transform_ma,
			 const mesh& mb, const si_transform& transform_mb,
			 vector& closest_point_ma, vector& closest_point_mb,
			 float& closest_distance);


	private:

		typedef bounding_distance
					<sphere_tree_triangle, sphere_tree_triangle,
					 mesh_distance_metric_triangle,
					 bounding_distance_no_extended_result>
							bounding_distance_sphere_triangle;
		typedef bounding_distance
					<box_tree_triangle, box_tree_triangle,
					 mesh_distance_metric_triangle,
					 bounding_distance_no_extended_result>
							bounding_distance_box_triangle;

		typedef bounding_distance
					<sphere_tree_index, sphere_tree_index,
					 mesh_distance_metric_index,
					 bounding_distance_no_extended_result>
							bounding_distance_sphere_index;
		typedef bounding_distance
					<box_tree_index, box_tree_index,
					 mesh_distance_metric_index,
					 bounding_distance_no_extended_result>
							bounding_distance_box_index;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_DISTANCE_HPP) */
