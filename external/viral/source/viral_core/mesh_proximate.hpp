/**
 *************************************************************************
 *
 * @file mesh_proximate.hpp
 *
 * Determine proximity of two meshes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_PROXIMATE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_PROXIMATE_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Common bounding_proximate_result%s for mesh_proximate.
 * @ingroup mesh_module
 */
//@{
typedef bounding_proximate_result
			<sphere_tree_triangle, sphere_tree_triangle,
			 bounding_proximate_no_extended_result>
					mesh_proximate_result_sphere_tree_triangle;
typedef bounding_proximate_result
			<box_tree_triangle, box_tree_triangle,
			 bounding_proximate_no_extended_result>
					mesh_proximate_result_box_tree_triangle;

typedef bounding_proximate_result
			<sphere_tree_index, sphere_tree_index,
			 bounding_proximate_no_extended_result>
					mesh_proximate_result_sphere_tree_index;
typedef bounding_proximate_result
			<box_tree_index, box_tree_index,
			 bounding_proximate_no_extended_result>
					mesh_proximate_result_box_tree_index;
//@}




/**
 *************************************************************************
 *
 * @class mesh_proximate_test_triangle
 * @ingroup mesh_module
 *
 * A bounding_proximate test for explicit triangle objects.
 *
 ************************************************************************/
class mesh_proximate_test_triangle :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:

		mesh_proximate_test_triangle
			(const si_transform& transform_ta,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_proximate
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const sphere_tree_triangle& tb,
			 const sphere_tree_triangle_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;
		bool node_proximate
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const box_tree_triangle& tb,
			 const box_tree_triangle_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;

		bool leaf_proximate
			(const sphere_tree_triangle& ta,
			 const sphere_tree_triangle_const_node_iterator& na,
			 const sphere_tree_triangle& tb,
			 const sphere_tree_triangle_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;
		bool leaf_proximate
			(const box_tree_triangle& ta,
			 const box_tree_triangle_const_node_iterator& na,
			 const box_tree_triangle& tb,
			 const box_tree_triangle_const_node_iterator& nb,
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
 * @class mesh_proximate_test_index
 * @ingroup mesh_module
 *
 * A bounding_proximate test for triangle indices into a @ref mesh.
 *
 ************************************************************************/
class mesh_proximate_test_index :
	private non_copyable
{
	template <typename A, typename B, typename T, typename X>
		friend class bounding_proximate;


	public:
	
		mesh_proximate_test_index
			(const mesh& mesh_ta,
			 const si_transform& transform_ta,
			 const mesh& mesh_tb,
			 const si_transform& transform_tb) NOTHROW;


	private:

		bool node_proximate
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const sphere_tree_index& tb,
			 const sphere_tree_index_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;
		bool node_proximate
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const box_tree_index& tb,
			 const box_tree_index_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 float& evaluation_potential) const;

		bool leaf_proximate
			(const sphere_tree_index& ta,
			 const sphere_tree_index_const_node_iterator& na,
			 const sphere_tree_index& tb,
			 const sphere_tree_index_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;
		bool leaf_proximate
			(const box_tree_index& ta,
			 const box_tree_index_const_node_iterator& na,
			 const box_tree_index& tb,
			 const box_tree_index_const_node_iterator& nb,
			 float distance_squared_upper_bound,
			 bounding_proximate_no_extended_result& extended_result) const;


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
 * @class mesh_proximate
 * @ingroup mesh_module
 *
 * Static utilities that determine if two meshes are in proximity, 
 * ideally under use of bounding_tree%s.
 *
 * Implementation notes:
 * - Current proximity utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_proximate :
	private non_constructable
{

	public:

		/**
		 * Perform a proximity test for two bounding_tree%s
		 * under arbitrary si_transform%s.
		 *
		 * Usage notes:
		 * - Mesh proximity tests support explicit and indexed trees.
		 *		In the latter case, clients must supply suitable
		 *		target meshes for the tree indices.
		 * - On any error, mesh_proximity tests throw an exception.
		 * - See geo_proximity and bounding_proximity for generic notes,
		 *		notably on the use of @bref{distance_squared_upper_bound}.
		 */
		//@{
		static mesh_proximate_result_sphere_tree_triangle
			proximate
				(const sphere_tree_triangle& ta, const si_transform& transform_ta,
				 const sphere_tree_triangle& tb, const si_transform& transform_tb,
				 float distance_squared_upper_bound);
		static mesh_proximate_result_box_tree_triangle
			proximate
				(const box_tree_triangle& ta, const si_transform& transform_ta,
				 const box_tree_triangle& tb, const si_transform& transform_tb,
				 float distance_squared_upper_bound);

		static mesh_proximate_result_sphere_tree_index
			proximate
				(const sphere_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const sphere_tree_index& tb, const mesh& mesh_tb,
				 const si_transform& transform_tb,			
				 float distance_squared_upper_bound);
		static mesh_proximate_result_box_tree_index
			proximate
				(const box_tree_index& ta, const mesh& mesh_ta,
				 const si_transform& transform_ta,
				 const box_tree_index& tb, const mesh& mesh_tb,
				 const si_transform& transform_tb,
				 float distance_squared_upper_bound);
		//@}


		/**
		 * Find proximity between two transformed @ref mesh%es
		 * by explicitly testing each and every triangle pair.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 */
		static bool proximate_slow
			(const mesh& ma, const si_transform& transform_ma,
			 const mesh& mb, const si_transform& transform_mb,
			 float distance_squared_upper_bound);


	private:

		typedef bounding_proximate
					<sphere_tree_triangle, sphere_tree_triangle,
					 mesh_proximate_test_triangle,
					 bounding_proximate_no_extended_result>
							bounding_proximate_sphere_triangle;
		typedef bounding_proximate
					<box_tree_triangle, box_tree_triangle,
					 mesh_proximate_test_triangle,
					 bounding_proximate_no_extended_result>
							bounding_proximate_box_triangle;

		typedef bounding_proximate
					<sphere_tree_index, sphere_tree_index,
					 mesh_proximate_test_index,
					 bounding_proximate_no_extended_result>
							bounding_proximate_sphere_index;
		typedef bounding_proximate
					<box_tree_index, box_tree_index,
					 mesh_proximate_test_index,
					 bounding_proximate_no_extended_result>
							bounding_proximate_box_index;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_PROXIMATE_HPP) */
