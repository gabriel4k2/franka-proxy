/**
 *************************************************************************
 *
 * @file mesh_raycast.hpp
 *
 * Raycasting through meshes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_RAYCAST_HPP)
#define INCLUDED__VIRAL_CORE__MESH_RAYCAST_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"
#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 * Common bounding_raycast_result%s for mesh_raycast.
 * @ingroup mesh_module
 */
//@{
typedef bounding_raycast_result
			<sphere_tree_triangle, bounding_raycast_no_extended_result>
					mesh_raycast_result_sphere_tree_triangle;
typedef bounding_raycast_result
			<box_tree_triangle, bounding_raycast_no_extended_result>
					mesh_raycast_result_box_tree_triangle;

typedef bounding_raycast_result
			<sphere_tree_index, bounding_raycast_no_extended_result>
					mesh_raycast_result_sphere_tree_index;
typedef bounding_raycast_result
			<box_tree_index, bounding_raycast_no_extended_result>
					mesh_raycast_result_box_tree_index;
//@}




/**
 *************************************************************************
 *
 * @class mesh_raycast_test_triangle
 * @ingroup mesh_module
 *
 * A bounding_raycast test for explicit triangle objects.
 *
 ************************************************************************/
class mesh_raycast_test_triangle :
	private non_copyable
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;

	private:

		bool try_node_raycast
			(const segment& s, const sphere_tree_triangle& t,
			 const sphere_tree_triangle_const_node_iterator& n,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;
		bool try_node_raycast
			(const segment& s, const box_tree_triangle& t,
			 const box_tree_triangle_const_node_iterator& n,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;

		bool try_leaf_raycast
			(const segment& s, const sphere_tree_triangle& t,
			 const sphere_tree_triangle_const_node_iterator& n,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
		bool try_leaf_raycast
			(const segment& s, const box_tree_triangle& t,
			 const box_tree_triangle_const_node_iterator& n,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class mesh_raycast_test_index
 * @ingroup mesh_module
 *
 * A bounding_raycast test for triangle indices into a @ref mesh.
 *
 ************************************************************************/
class mesh_raycast_test_index :
	private non_copyable
{
	template <typename R, typename H, typename T, typename X>
		friend class bounding_raycast;


	public:
	
		mesh_raycast_test_index
			(const mesh& mesh_t) NOTHROW;


	private:

		bool try_node_raycast
			(const segment& s, const sphere_tree_index& t,
			 const sphere_tree_index_const_node_iterator& n,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;
		bool try_node_raycast
			(const segment& s, const box_tree_index& t,
			 const box_tree_index_const_node_iterator& n,
			 float& hit_distance_lower_bound, float& evaluation_potential) const;

		bool try_leaf_raycast
			(const segment& s, const sphere_tree_index& t,
			 const sphere_tree_index_const_node_iterator& n,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;
		bool try_leaf_raycast
			(const segment& s, const box_tree_index& t,
			 const box_tree_index_const_node_iterator& n,
			 vector& hit_point, float& hit_distance,
			 bounding_raycast_no_extended_result& extended_result) const;


		const mesh& mesh_t_; // Don't copy, expensive.
};




/**
 *************************************************************************
 *
 * @class mesh_raycast
 * @ingroup mesh_module
 *
 * Static utilities for raycasting through the triangles of a @ref mesh,
 * ideally under the use of bounding_tree%s.
 *
 * Implementation notes:
 * - Current raycasting utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_raycast :
	private non_constructable
{

	public:

		/**
		 * Find the first point of intersection, if any,
		 * between a segment and a transformed bounding_tree.
		 *
		 * Usage notes:
		 * - Mesh raycasting tests support explicit and indexed trees.
		 *		In the latter case, clients must supply suitable
		 *		target meshes for the tree indices.
		 * - On any error, mesh_raycast tests throw an exception.
		 * - See geo_raycast and bounding_raycast for generic notes,
		 *		notably for details on @bref{hit_distance_bound}
		 *		and @bref{hit_distance_tolerance}.
		 *
		 * Implementation notes:
		 * - To avoid expensive transforms per shape and object,
		 *		we instead transform the incoming @bref{s}egment
		 *		by the inverse of @bref{transform_t}.
		 */
		//@{
		static mesh_raycast_result_sphere_tree_triangle
			raycast
				(const segment& s, const sphere_tree_triangle& t,
				 const si_transform& transform_t,
		 		 float hit_distance_bound, bool hit_distance_bound_valid,
				 float hit_distance_tolerance);
		static mesh_raycast_result_box_tree_triangle
			raycast
				(const segment& s, const box_tree_triangle& t,
				 const si_transform& transform_t,
		 		 float hit_distance_bound, bool hit_distance_bound_valid,
				 float hit_distance_tolerance);

		static mesh_raycast_result_sphere_tree_index
			raycast
				(const segment& s, const sphere_tree_index& t,
				 const mesh& mesh_t, const si_transform& transform_t,
		 		 float hit_distance_bound, bool hit_distance_bound_valid,
				 float hit_distance_tolerance);
		static mesh_raycast_result_box_tree_index
			raycast
				(const segment& s, const box_tree_index& t,
				 const mesh& mesh_t, const si_transform& transform_t,
				 float hit_distance_bound, bool hit_distance_bound_valid,
				 float hit_distance_tolerance);
		//@}


		/**
		 * Find first hit point between a segment and a mesh
		 * by explicitly testing each and every triangle.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 *
		 * On failure (e.g. a @false return or an exception),
		 * the call zeros all output variables.
		 */
		static bool try_raycast_slow
			(const segment& s,
			 const mesh& m, const si_transform& transform_m,
			 vector& hit_point, float& hit_distance);


	private:

		typedef bounding_raycast
					<segment, sphere_tree_triangle,
					 mesh_raycast_test_triangle,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_sphere_triangle;
		typedef bounding_raycast
					<segment, box_tree_triangle,
					 mesh_raycast_test_triangle,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_box_triangle;

		typedef bounding_raycast
					<segment, sphere_tree_index,
					 mesh_raycast_test_index,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_sphere_index;
		typedef bounding_raycast
					<segment, box_tree_index,
					 mesh_raycast_test_index,
					 bounding_raycast_no_extended_result>
							bounding_raycast_segment_box_index;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_RAYCAST_HPP) */
