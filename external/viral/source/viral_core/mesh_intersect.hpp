/**
 *************************************************************************
 *
 * @file mesh_intersect.hpp
 *
 * Determine if two meshes intersect.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_INTERSECT_HPP)
#define INCLUDED__VIRAL_CORE__MESH_INTERSECT_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Common bounding_intersect_result%s for mesh_intersect.
 * @ingroup mesh_module
 */
//@{
typedef bounding_intersect_result
			<sphere_tree_triangle, sphere_tree_triangle,
			 bounding_intersect_no_extended_result>
					mesh_intersect_result_sphere_tree_triangle;
typedef bounding_intersect_result
			<box_tree_triangle, box_tree_triangle,
			 bounding_intersect_no_extended_result>
					mesh_intersect_result_box_tree_triangle;

typedef bounding_intersect_result
			<sphere_tree_index, sphere_tree_index,
			 bounding_intersect_no_extended_result>
					mesh_intersect_result_sphere_tree_index;
typedef bounding_intersect_result
			<box_tree_index, box_tree_index,
			 bounding_intersect_no_extended_result>
					mesh_intersect_result_box_tree_index;
//@}




/**
 *************************************************************************
 *
 * @class mesh_intersect_test_triangle
 * @ingroup mesh_module
 *
 * A bounding_intersect test for explicit triangle objects.
 *
 ************************************************************************/
class mesh_intersect_test_triangle :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:

		mesh_intersect_test_triangle
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_intersecting
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const sphere_tree_triangle& tb,
			 const sphere_tree_triangle_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const box_tree_triangle& tb,
			 const box_tree_triangle_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const sphere_tree_triangle& tb,
			 const sphere_tree_triangle_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const box_tree_triangle& tb,
			 const box_tree_triangle_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;


		const si_transform transform_ta_;
		const matrix to_world_matrix_ta_;

		const si_transform transform_tb_;
		const matrix to_world_matrix_tb_;
};




/**
 *************************************************************************
 *
 * @class mesh_intersect_test_index
 * @ingroup mesh_module
 *
 * A bounding_intersect test for triangle indices into a @ref mesh.
 *
 ************************************************************************/
class mesh_intersect_test_index :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_intersect;


	public:
	
		mesh_intersect_test_index
			(const mesh& mesh_ta,
			 const si_transform& transform_ta,
			 const mesh& mesh_tb,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_intersecting
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const sphere_tree_index& tb,
			 const sphere_tree_index_const_node_iterator& nb,
			 float& evaluation_potential) const;
		bool node_intersecting
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const box_tree_index& tb,
			 const box_tree_index_const_node_iterator& nb,
			 float& evaluation_potential) const;

		bool leaf_intersecting
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const sphere_tree_index& tb,
			 const sphere_tree_index_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;
		bool leaf_intersecting
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const box_tree_index& tb,
			 const box_tree_index_const_node_iterator& nb,
			 bounding_intersect_no_extended_result& extended_result) const;


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
 * @class mesh_intersect
 * @ingroup mesh_module
 *
 * Static utilities that check meshes for intersection,
 * ideally under the use of bounding_tree%s.
 *
 * Implementation notes:
 * - Current intersection utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_intersect :
	private non_constructable
{

	public:

		/**
		 * Test two si_transform%ed bounding_tree%s for intersection.
		 *
		 * Usage notes:
		 * - Mesh intersection tests support explicit and indexed trees.
		 *		In the latter case, clients must supply suitable
		 *		target meshes for the tree indices.
		 * - On any error, mesh_intersect tests throw an exception.
		 * - See geo_intersect and bounding_intersect for generic notes.
		 */
		//@{
		static mesh_intersect_result_sphere_tree_triangle
			intersect
				(const sphere_tree_triangle& ta, const si_transform& transform_ta,
				 const sphere_tree_triangle& tb, const si_transform& transform_tb);
		static mesh_intersect_result_box_tree_triangle
			intersect
				(const box_tree_triangle& ta, const si_transform& transform_ta,
				 const box_tree_triangle& tb, const si_transform& transform_tb);

		static mesh_intersect_result_sphere_tree_index
			intersect
				(const sphere_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const sphere_tree_index& tb, const mesh& mesh_tb,
				 const si_transform& transform_tb);
		static mesh_intersect_result_box_tree_index
			intersect
				(const box_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const box_tree_index& tb, const mesh& mesh_tb,
				 const si_transform& transform_tb);
		//@}


		/**
		 * Find intersection between two transformed @ref mesh%s
		 * by explicitly testing each and every triangle pair.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 */
		static bool intersecting_slow
			(const mesh& ma, const si_transform& transform_ma,
			 const mesh& mb, const si_transform& transform_mb);


	private:

		typedef bounding_intersect
					<sphere_tree_triangle, sphere_tree_triangle,
					 mesh_intersect_test_triangle,
					 bounding_intersect_no_extended_result>
							bounding_intersect_sphere_triangle;
		typedef bounding_intersect
					<box_tree_triangle, box_tree_triangle,
					 mesh_intersect_test_triangle,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_triangle;

		typedef bounding_intersect
					<sphere_tree_index, sphere_tree_index,
					 mesh_intersect_test_index,
					 bounding_intersect_no_extended_result>
							bounding_intersect_sphere_index;
		typedef bounding_intersect
					<box_tree_index, box_tree_index,
					 mesh_intersect_test_index,
					 bounding_intersect_no_extended_result>
							bounding_intersect_box_index;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_INTERSECT_HPP) */