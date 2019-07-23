/**
 *************************************************************************
 *
 * @file mesh_k_nearest.hpp
 *
 * Find nearest neighbor triangles in a mesh.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_K_NEAREST_HPP)
#define INCLUDED__VIRAL_CORE__MESH_K_NEAREST_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 * Common bounding_k_nearest_result%s for mesh_k_nearest.
 * @ingroup mesh_module
 */
//@{
typedef bounding_k_nearest_result
			<sphere_tree_triangle,
			 bounding_k_nearest_no_extended_result>
					mesh_k_nearest_result_sphere_tree_triangle;
typedef bounding_k_nearest_result
			<box_tree_triangle,
			 bounding_k_nearest_no_extended_result>
					mesh_k_nearest_result_box_tree_triangle;

typedef bounding_k_nearest_result
			<sphere_tree_index,
			 bounding_k_nearest_no_extended_result>
					mesh_k_nearest_result_sphere_tree_index;
typedef bounding_k_nearest_result
			<box_tree_index,
			 bounding_k_nearest_no_extended_result>
					mesh_k_nearest_result_box_tree_index;
//@}




/**
 *************************************************************************
 *
 * @class mesh_k_nearest_metric_triangle
 * @ingroup mesh_module
 *
 * A bounding_k_nearest metric for explicit triangle objects.
 *
 ************************************************************************/
class mesh_k_nearest_metric_triangle :
	private non_copyable
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		mesh_k_nearest_metric_triangle() NOTHROW;


	private:

		bool try_node_distance_squared
			(const sphere_tree_triangle& t,
			 const sphere_tree_triangle_const_node_iterator& n,
			 const vector& o,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;
		bool try_node_distance_squared
			(const box_tree_triangle& t,
			 const box_tree_triangle_const_node_iterator& n,
			 const vector& o,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree_triangle& t,
			 const sphere_tree_triangle_const_node_iterator& n,
			 const vector& o,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;
		bool try_leaf_distance_squared
			(const box_tree_triangle& t,
			 const box_tree_triangle_const_node_iterator& n,
			 const vector& o,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class mesh_k_nearest_metric_index
 * @ingroup mesh_module
 *
 * A bounding_k_nearest metric for triangle indices into a @ref mesh.
 *
 ************************************************************************/
class mesh_k_nearest_metric_index :
	private non_copyable
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		mesh_k_nearest_metric_index
			(const mesh& mesh_t) NOTHROW;


	private:

		bool try_node_distance_squared
			(const sphere_tree_index& t,
			 const sphere_tree_index_const_node_iterator& n,
			 const vector& o,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;
		bool try_node_distance_squared
			(const box_tree_index& t,
			 const box_tree_index_const_node_iterator& n,
			 const vector& o,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const sphere_tree_index& t,
			 const sphere_tree_index_const_node_iterator& n,
			 const vector& o,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;
		bool try_leaf_distance_squared
			(const box_tree_index& t,
			 const box_tree_index_const_node_iterator& n,
			 const vector& o,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;


		const mesh& mesh_t_; // Don't copy, expensive.
};




/**
 *************************************************************************
 *
 * @class mesh_k_nearest
 * @ingroup mesh_module
 *
 * Query triangles of a mesh nearest to another object,
 * ideally under the use of bounding_tree%s.
 *
 ************************************************************************/
class mesh_k_nearest :
	private non_constructable
{

	public:

		/**
		 * Find @bref{k} nearest triangles in a si_transform%ed mesh
		 * to the given @bref{point}.
		 *
		 * Usage notes:
		 * - Neighbor queries support explicit and indexed trees.
		 *		In the latter case, clients must supply suitable
		 *		target meshes for the tree indices.
		 * - On any error, mesh_k_nearest queries throw an exception.
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
		static mesh_k_nearest_result_sphere_tree_triangle
			k_nearest
				(const sphere_tree_triangle& t, const si_transform& transform_t,
				 const vector& point,
				 int64 k,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		static mesh_k_nearest_result_box_tree_triangle
			k_nearest
				(const box_tree_triangle& t, const si_transform& transform_t,
				 const vector& point,
				 int64 k,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);

		static mesh_k_nearest_result_sphere_tree_index
			k_nearest
				(const sphere_tree_index& t, const mesh& mesh_t,
				 const si_transform& transform_t,
				 const vector& point,
				 int64 k,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		static mesh_k_nearest_result_box_tree_index
			k_nearest
				(const box_tree_index& t, const mesh& mesh_t,
				 const si_transform& transform_t,
				 const vector& point,
				 int64 k,
				 float distance_squared_bound,
				 bool distance_squared_bound_valid,
				 float distance_squared_tolerance);
		//@}


		/**
		 * Find @bref{k} nearest triangles in a @bref{m}esh
		 * to given @bref{point} by testing each of the triangles.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 */
		static dynamic_buffer<int64>
			try_k_nearest_slow
				(const mesh& m, const si_transform& transform_m,
				 const vector& point, int64 k);


	private:

		typedef bounding_k_nearest
					<sphere_tree_triangle, vector,
					 mesh_k_nearest_metric_triangle,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_sphere_triangle;
		typedef bounding_k_nearest
					<box_tree_triangle, vector,
					 mesh_k_nearest_metric_triangle,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_box_triangle;

		typedef bounding_k_nearest
					<sphere_tree_index, vector,
					 mesh_k_nearest_metric_index,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_sphere_index;
		typedef bounding_k_nearest
					<box_tree_index, vector,
					 mesh_k_nearest_metric_index,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_box_index;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_K_NEAREST_HPP) */
