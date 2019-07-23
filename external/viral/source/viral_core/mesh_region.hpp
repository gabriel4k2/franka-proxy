/**
 *************************************************************************
 *
 * @file mesh_region.hpp
 *
 * Query for mesh triangles in a region.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_REGION_HPP)
#define INCLUDED__VIRAL_CORE__MESH_REGION_HPP


#include "build_config.hpp"


#include "bounding_forward.hpp"
#include "mesh_forward.hpp"


#include "util.hpp"
#include "geo_transform.hpp"


namespace viral_core
{


/**
 * Common bounding_region_result%s for mesh_region.
 * @ingroup mesh_module
 */
//@{
typedef bounding_region_result
			<sphere_tree_triangle,
			 bounding_region_no_extended_result>
					mesh_region_result_sphere_tree_triangle;
typedef bounding_region_result
			<box_tree_triangle,
			 bounding_region_no_extended_result>
					mesh_region_result_box_tree_triangle;

typedef bounding_region_result
			<sphere_tree_index,
			 bounding_region_no_extended_result>
					mesh_region_result_sphere_tree_index;
typedef bounding_region_result
			<box_tree_index,
			 bounding_region_no_extended_result>
					mesh_region_result_box_tree_index;
//@}




/**
 *************************************************************************
 *
 * @class mesh_region_test_triangle
 * @ingroup mesh_module
 *
 * A bounding_region test for explicit triangle objects.
 *
 ************************************************************************/
class mesh_region_test_triangle :
	private non_copyable
{
	template <typename H, typename R, typename T, typename X>
		friend class bounding_region;


	public:

		mesh_region_test_triangle
			(const si_transform& transform_t) NOTHROW;


	private:

		bool try_node_region
			(const sphere_tree_triangle& t,
			 const sphere_tree_triangle_const_node_iterator& n,
			 const sphere& s) const;
		bool try_node_region
			(const box_tree_triangle& t,
			 const box_tree_triangle_const_node_iterator& n,
			 const box& b) const;

		bool try_leaf_region
			(const sphere_tree_triangle& t,
			 const sphere_tree_triangle_const_node_iterator& n,
			 const sphere& s,
			 bounding_region_no_extended_result& extended_result) const;
		bool try_leaf_region
			(const box_tree_triangle& t,
			 const box_tree_triangle_const_node_iterator& n,
			 const box& b,
			 bounding_region_no_extended_result& extended_result) const;


		const si_transform transform_t_;
		const matrix to_world_matrix_t_;
};




/**
 *************************************************************************
 *
 * @class mesh_region_test_index
 * @ingroup mesh_module
 *
 * A bounding_region test for triangle indices into a @ref mesh.
 *
 ************************************************************************/
class mesh_region_test_index :
	private non_copyable
{
	template <typename H, typename R, typename T, typename X>
		friend class bounding_region;


	public:
	
		mesh_region_test_index
			(const mesh& mesh_t,
			 const si_transform& transform_t) NOTHROW;


	private:

		bool try_node_region
			(const sphere_tree_index& t,
			 const sphere_tree_index_const_node_iterator& n,
			 const sphere& s) const;
		bool try_node_region
			(const box_tree_index& t,
			 const box_tree_index_const_node_iterator& n,
			 const box& b) const;

		bool try_leaf_region
			(const sphere_tree_index& t,
			 const sphere_tree_index_const_node_iterator& n,
			 const sphere& s,
			 bounding_region_no_extended_result& extended_result) const;
		bool try_leaf_region
			(const box_tree_index& t,
			 const box_tree_index_const_node_iterator& n,
			 const box& b,
			 bounding_region_no_extended_result& extended_result) const;


		const mesh& mesh_t_; // Don't copy, expensive.
		const si_transform transform_t_;
		const matrix to_world_matrix_t_;
};




/**
 *************************************************************************
 *
 * @class mesh_region
 * @ingroup mesh_module
 *
 * Query all triangles of a mesh that completely are inside a region,
 * ideally under the use of bounding_tree%s.
 *
 ************************************************************************/
class mesh_region :
	private non_constructable
{

	public:

		/**
		 * Test triangles of a si_transform%ed bounding_tree
		 * against a @bref{s}phere or a @bref{b}ox.
		 *
		 * Usage notes:
		 * - Mesh region tests support explicit and indexed trees.
		 *		In the latter case, clients must supply suitable
		 *		target meshes for the tree indices.
		 * - On any error, mesh_region tests throw an exception.
		 * - See geo_intersect and bounding_region for generic notes.
		 */
		//@{
		static mesh_region_result_sphere_tree_triangle
			region
				(const sphere_tree_triangle& t,
				 const si_transform& transform_t,
				 const sphere& s);
		static mesh_region_result_box_tree_triangle
			region
				(const box_tree_triangle& t, 
				 const si_transform& transform_t,
				 const box& b);

		static mesh_region_result_sphere_tree_index
			region
				(const sphere_tree_index& t, const mesh& mesh_t,
				 const si_transform& transform_t,
				 const sphere& s);
		static mesh_region_result_box_tree_index
			region
				(const box_tree_index& t, const mesh& mesh_t,
				 const si_transform& transform_t,
				 const box& s);
		//@}


		/**
		 * Find all triangles of a mesh fully inside a sphere
		 * or a box region by explicitly testing each triangle.
		 * This is very slow and only useful if bounding_tree%s
		 * are too expensive to build or maintain.
		 */
		//@{
		static dynamic_buffer<int64>
			region_slow
				(const mesh& m, const si_transform& transform_m,
				 const sphere& s);
		static dynamic_buffer<int64>
			region_slow
				(const mesh& m, const si_transform& transform_m,
				 const box& b);
		//@}


	private:

		typedef bounding_region
					<sphere_tree_triangle, sphere,
					 mesh_region_test_triangle,
					 bounding_region_no_extended_result>
							bounding_region_sphere_triangle_sphere;
		typedef bounding_region
					<box_tree_triangle, box,
					 mesh_region_test_triangle,
					 bounding_region_no_extended_result>
							bounding_region_box_triangle_box;

		typedef bounding_region
					<sphere_tree_index, sphere,
					 mesh_region_test_index,
					 bounding_region_no_extended_result>
							bounding_region_sphere_index_sphere;
		typedef bounding_region
					<box_tree_index, box,
					 mesh_region_test_index,
					 bounding_region_no_extended_result>
							bounding_region_box_index_box;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_REGION_HPP) */
