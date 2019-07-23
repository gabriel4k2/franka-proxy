/**
 *************************************************************************
 *
 * @file mesh_intersect_single.hpp
 *
 * Intersection between a mesh and a single shape.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_INTERSECT_SINGLE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_INTERSECT_SINGLE_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Common bounding_intersect_result%s for mesh_intersect_single.
 * @ingroup mesh_module
 */
//@{
typedef bounding_intersect_result
			<sphere_tree_triangle, bounding_single_sphere,
			 bounding_intersect_no_extended_result>
					mesh_intersect_single_result_sphere_tree_triangle_sphere;
typedef bounding_intersect_result
			<box_tree_triangle, bounding_single_box,
			 bounding_intersect_no_extended_result>
					mesh_intersect_single_result_box_tree_triangle_box;
typedef bounding_intersect_result
			<box_tree_triangle, bounding_single_box_triangle,
			 bounding_intersect_no_extended_result>
					mesh_intersect_single_result_box_tree_triangle_box_triangle;

typedef bounding_intersect_result
			<sphere_tree_index, bounding_single_sphere,
			 bounding_intersect_no_extended_result>
					mesh_intersect_single_result_sphere_tree_index_sphere;
typedef bounding_intersect_result
			<box_tree_index, bounding_single_box,
			 bounding_intersect_no_extended_result>
					mesh_intersect_single_result_box_tree_index_box;
typedef bounding_intersect_result
			<box_tree_index, bounding_single_box_triangle,
			 bounding_intersect_no_extended_result>
					mesh_intersect_single_result_box_tree_index_box_triangle;
//@}




/**
 *************************************************************************
 *
 * @class mesh_intersect_single_test_triangle
 * @ingroup mesh_module
 *
 * A bounding_intersect test for mesh triangles against single objects.
 *
 ************************************************************************/
class mesh_intersect_single_test_triangle :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		mesh_intersect_single_test_triangle
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_intersecting
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const bounding_single_sphere& tb,
			 const bounding_single_sphere_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const bounding_single_box& tb,
			 const bounding_single_box_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool node_intersecting
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const bounding_single_sphere_triangle& tb,
			 const bounding_single_sphere_triangle_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const bounding_single_box_triangle& tb,
			 const bounding_single_box_triangle_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const bounding_single_sphere& tb,
			 const bounding_single_sphere_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const bounding_single_box& tb,
			 const bounding_single_box_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;

		bool leaf_intersecting
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const bounding_single_sphere_triangle& tb,
			 const bounding_single_sphere_triangle_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const bounding_single_box_triangle& tb,
			 const bounding_single_box_triangle_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;


		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class mesh_intersect_single_test_index
 * @ingroup mesh_module
 *
 * A bounding_intersect test for triangle indices in a @ref mesh
 * against single objects.
 *
 ************************************************************************/
class mesh_intersect_single_test_index :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:
	
		mesh_intersect_single_test_index
			(const mesh& mesh_ta,
			 const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_intersecting
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const bounding_single_sphere& tb,
			 const bounding_single_sphere_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const bounding_single_box& tb,
			 const bounding_single_box_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool node_intersecting
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const bounding_single_sphere_triangle& tb,
			 const bounding_single_sphere_triangle_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const bounding_single_box_triangle& tb,
			 const bounding_single_box_triangle_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const bounding_single_sphere& tb,
			 const bounding_single_sphere_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const bounding_single_box& tb,
			 const bounding_single_box_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;

		bool leaf_intersecting
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const bounding_single_sphere_triangle& tb,
			 const bounding_single_sphere_triangle_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const bounding_single_box_triangle& tb,
			 const bounding_single_box_triangle_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;


		const mesh& mesh_ta_; // Don't copy, expensive.
		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class mesh_intersect_single
 * @ingroup mesh_module
 *
 * Static utilities that check a mesh for intersection with a single object,
 * ideally under the use of a bounding_tree.
 *
 * Implementation notes:
 * - Current intersection utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_intersect_single :
	private non_constructable
{

	public:

		/**
		 * Test a si_transform%ed bounding_tree for intersection
		 * with a single, si_transform%ed object.
		 *
		 * Usage notes:
		 * - Intersection tests support explicit and indexed trees.
		 *		In the latter case, clients must supply a suitable
		 *		target mesh for the tree indices.
		 * - On any error, mesh_intersect_single tests throw an exception.
		 * - See geo_intersect and bounding_intersect for generic notes.
		 */
		//@{
		static mesh_intersect_single_result_sphere_tree_triangle_sphere
			intersect
				(const sphere_tree_triangle& ta, const si_transform& transform_ta,
				 const bounding_single_sphere& tb, const si_transform& transform_tb);
		static mesh_intersect_single_result_box_tree_triangle_box
			intersect
				(const box_tree_triangle& ta, const si_transform& transform_ta,
				 const bounding_single_box& tb, const si_transform& transform_tb);
		static mesh_intersect_single_result_box_tree_triangle_box_triangle
			intersect
				(const box_tree_triangle& ta, const si_transform& transform_ta,
				 const bounding_single_box_triangle& tb, const si_transform& transform_tb);

		static mesh_intersect_single_result_sphere_tree_index_sphere
			intersect
				(const sphere_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const bounding_single_sphere& tb,
				 const si_transform& transform_tb);
		static mesh_intersect_single_result_box_tree_index_box
			intersect
				(const box_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const bounding_single_box& tb,
				 const si_transform& transform_tb);
		static mesh_intersect_single_result_box_tree_index_box_triangle
			intersect
				(const box_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const bounding_single_box_triangle& tb,
				 const si_transform& transform_tb);
		//@}


		/**
		 * Find intersection between a mesh and a single object
		 * by explicitly testing each and every possible pairing.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 */
		//@{
		static bool intersecting_slow
			(const mesh& m, const si_transform& transform_m,
			 const sphere& s, const si_transform& transform_s);
		static bool intersecting_slow
			(const mesh& m, const si_transform& transform_m,
			 const box& b, const si_transform& transform_b);
		static bool intersecting_slow
			(const mesh& m, const si_transform& transform_m,
			 const triangle& t, const si_transform& transform_t);
		//@}


	private:

		typedef bounding_intersect
					<sphere_tree_triangle, bounding_single_sphere,
					 mesh_intersect_single_test_triangle,
					 bounding_intersect_no_extended_result>
							bounding_intersect_sphere_triangle_sphere;
		typedef bounding_intersect
					<box_tree_triangle, bounding_single_box,
					 mesh_intersect_single_test_triangle,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_triangle_box;
		typedef bounding_intersect
					<box_tree_triangle, bounding_single_box_triangle,
					 mesh_intersect_single_test_triangle,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_triangle_box_triangle;

		typedef bounding_intersect
					<sphere_tree_index, bounding_single_sphere,
					 mesh_intersect_single_test_index,
					 bounding_intersect_no_extended_result>
							bounding_intersect_sphere_index_sphere;
		typedef bounding_intersect
					<box_tree_index, bounding_single_box,
					 mesh_intersect_single_test_index,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_index_box;
		typedef bounding_intersect
					<box_tree_index, bounding_single_box_triangle,
					 mesh_intersect_single_test_index,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_index_box_triangle;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_INTERSECT_SINGLE_HPP) */
