/**
 *************************************************************************
 *
 * @file mesh_scene_k_nearest.hpp
 *
 * Nearest neighbors on a mesh_scene.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_K_NEAREST_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_K_NEAREST_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "bounding_tree.hpp"
#include "mesh_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_scene_k_nearest_metric
 * @ingroup mesh_module
 *
 * A bounding_k_nearest metric for use in mesh_scene_k_nearest.
 *
 ************************************************************************/
class mesh_scene_k_nearest_metric :
	private non_copyable
{
	template <typename H, typename O, typename M, typename X>
		friend class bounding_k_nearest;


	public:

		mesh_scene_k_nearest_metric();


	private:

		bool try_node_distance_squared
			(const mesh_scene& scene,
			 const mesh_scene::const_node_iterator& node,
			 const vector& point,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const mesh_scene& scene,
			 const mesh_scene::const_node_iterator& node,
			 const vector& point,
			 float& closest_distance_squared,
			 bounding_k_nearest_no_extended_result& extended_result) const;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_k_nearest_entry
 * @ingroup mesh_module
 *
 * A single entry in a mesh_scene_k_nearest_result.
 *
 ************************************************************************/
class mesh_scene_k_nearest_entry
{

	public:

		mesh_scene_k_nearest_entry
			(const mesh_scene::const_node_iterator& closest_scene_node,
			 const mesh_scene::object_iterator& closest_scene_object,
			 float closest_distance_squared) NOTHROW
			:
			closest_scene_node(closest_scene_node),
			closest_scene_object(closest_scene_object),
			closest_distance_squared(closest_distance_squared)
		{ }


		/** Closest node and object in mesh_scene. */
		//@{
		const mesh_scene::const_node_iterator closest_scene_node;
		const mesh_scene::object_iterator closest_scene_object;
		//@}


		/** Squared distance to mesh_scene_object. */
		const float closest_distance_squared;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_k_nearest_result
 * @ingroup mesh_module
 *
 * Stores mesh_scene_k_nearest results in form of closest distances
 * and associated node iterators in the mesh_scene.
 *
 * Usage notes:
 * - Nearest neighbor queries may pick any of multiple neigbor candidates,
 *		potentially causing an error of up to the distance tolerance.
 *
 ************************************************************************/
class mesh_scene_k_nearest_result
{
	friend class mesh_scene_k_nearest;


	public:

		typedef array<mesh_scene_k_nearest_entry> entry_array;
		const entry_array entries;


	private:

		mesh_scene_k_nearest_result() { }

		mesh_scene_k_nearest_result(const entry_array& entries)
			: entries(entries) { }
};




/**
 *************************************************************************
 *
 * @class mesh_scene_k_nearest
 * @ingroup mesh_module
 *
 * Find @bref{k} nearest neighbors to a point in a mesh_scene.
 *
 * Implementation notes:
 * - Current neighbor queries only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_scene_k_nearest :
	private non_constructable
{

	public:

		/**
		 * Find @bref{k} nearest mesh neighbors in a mesh_scene
		 * for a given @bref{p}oint.
		 *
		 * Usage notes:
		 * - Nearest neighbor queries on mesh_scene%s only return meshes.
		 *		Individual triangles of each mesh do determine distances,
		 *		but do not count individually towards @bref{k} neighbors.
		 * - On any error, mesh_scene_k_nearest queries throw an exception.
		 * - See bounding_k_nearest for generic notes, notably for details
		 *		on distance bounds and tolerances.
		 */
		static mesh_scene_k_nearest_result k_nearest
			(const mesh_scene& scene,
			 const vector& point,
			 int64 k,
			 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);


	private:

		typedef bounding_k_nearest
					<mesh_scene, vector,
					 mesh_scene_k_nearest_metric,
					 bounding_k_nearest_no_extended_result>
							bounding_k_nearest_mesh_scene_vector;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_K_NEAREST_HPP) */
