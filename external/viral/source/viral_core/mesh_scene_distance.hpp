/**
 *************************************************************************
 *
 * @file mesh_scene_distance.hpp
 *
 * Distances on mesh_scenes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_DISTANCE_HPP)
#define INCLUDED__VIRAL_CORE__MESH_SCENE_DISTANCE_HPP


#include "build_config.hpp"


#include "util.hpp"
#include "geo_volume.hpp"
#include "bounding_tree.hpp"
#include "mesh_scene.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mesh_scene_distance_metric
 * @ingroup mesh_module
 *
 * A bounding_distance metric for use in mesh_scene_distance.
 *
 ************************************************************************/
class mesh_scene_distance_metric :
	private non_copyable
{
	template <typename A, typename B, typename M, typename X>
		friend class bounding_distance;


	public:

		mesh_scene_distance_metric
			(const mesh& m, const si_transform& m_transform,
			 const mesh_scene::object_iterator& m_ignore_object);


	private:

		bool try_node_distance_squared
			(const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
			 float& min_distance_squared_lower_bound,
			 float& evaluation_potential) const;

		bool try_leaf_distance_squared
			(const mesh_scene_triangles& stris,
			 const mesh_scene_triangles_const_node_iterator& stris_node,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene_mesh_bounding::const_node_iterator& m_node,
			 vector& stris_closest_point, vector& m_closest_point,
			 float& closest_distance_squared,
			 bounding_distance_no_extended_result& extended_result) const;


		const mesh& m_; // Don't copy, expensive.
		const si_transform m_transform_;
		const matrix m_to_world_matrix_;

		const mesh_scene::object_iterator m_ignore_object_;
};




/**
 *************************************************************************
 *
 * @class mesh_scene_distance_result
 * @ingroup mesh_module
 *
 * Stores mesh_scene_distance results in form of a pair of closest points,
 * the squared distance of this point pair, and associated node iterators
 * in the bounding volume hierarchies.
 *
 * Usage notes:
 * - If there are no closest points (e.g. due to an empty scene or mesh),
 *		then @bref{valid} is @false, the indices are @bref{invalid_index},
 *		all node iterators are past-end iterators, and closest points
 *		as well as squared distance remain zero.
 * - Distance tests may pick any of multiple candidates for closest points,
 *		potentially causing an error of up to the distance tolerance.
 *
 ************************************************************************/
class mesh_scene_distance_result
{
	friend class mesh_scene_distance;


	public:

		static const int64 invalid_index = -1;


		const bool valid;


		/** Closest node and object in mesh_scene. */
		//@{
		const mesh_scene::const_node_iterator closest_scene_node;
		const mesh_scene::object_iterator closest_scene_object;
		//@}


		/** Closest node and triangle index in the the closest mesh_scene_object. */
		//@{
		const mesh_scene_mesh_bounding::const_node_iterator closest_scene_object_node;
		const int64 closest_scene_object_triangle_index;
		//@}


		/** Closest node and triangle index in the right-hand mesh. */
		//@{
		const mesh_scene_mesh_bounding::const_node_iterator closest_mesh_node;
		const int64 closest_mesh_triangle_index;
		//@}


		const vector closest_scene_point;
		const vector closest_mesh_point;

		const float closest_distance_squared;


	private:

		mesh_scene_distance_result() NOTHROW
			:
			valid(false),
			closest_scene_object_triangle_index(invalid_index),
			closest_mesh_triangle_index(invalid_index),

			closest_distance_squared(0.f)
		{ }


		mesh_scene_distance_result
			(const mesh_scene::const_node_iterator& closest_scene_node,
			 const mesh_scene::object_iterator& closest_scene_object,
			 const mesh_scene_mesh_bounding::const_node_iterator& closest_scene_object_node,
			 int64 closest_scene_object_triangle_index,
			 const mesh_scene_mesh_bounding::const_node_iterator& closest_mesh_node,
			 int64 closest_mesh_triangle_index,
			 const vector& closest_scene_point,
			 const vector& closest_mesh_point,
			 float closest_distance_squared) NOTHROW
			:
			valid(true),
			closest_scene_node(closest_scene_node),
			closest_scene_object(closest_scene_object),
			closest_scene_object_node(closest_scene_object_node),
			closest_scene_object_triangle_index(closest_scene_object_triangle_index),
			closest_mesh_node(closest_mesh_node),
			closest_mesh_triangle_index(closest_mesh_triangle_index),
			closest_scene_point(closest_scene_point),
			closest_mesh_point(closest_mesh_point),
			closest_distance_squared(closest_distance_squared)
		{ }
};




/**
 *************************************************************************
 *
 * @class mesh_scene_distance
 * @ingroup mesh_module
 *
 * Calculates distances on the mesh%es in a mesh_scene.
 *
 * Implementation notes:
 * - Current distance utilities only consider mesh_triangle%s
 *		and ignore any mesh_line%s or mesh_point%s.
 *
 ************************************************************************/
class mesh_scene_distance :
	private non_constructable
{

	public:

		/**
		 * Find the closest distance between the meshes in a mesh_scene
		 * and another, si_transform%ed @ref mesh.
		 *
		 * Usage notes:
		 * - Clients may optionally supply the @bref{m}esh and its si_transform
		 *		through an iterator to a mesh_scene_object in the @bref{scene}.
		 *		This approach suppresses self-distance results on the object.
		 * - On any error, mesh_scene_distance tests throw an exception.
		 * - See geo_distance and bounding_distance for generic notes,
		 *		notably for details on @bref{distance_squared_bound}
		 *		and @bref{distance_squared_tolerance}.
		 */
		//@{
		static mesh_scene_distance_result distance
			(const mesh_scene& scene,
			 const mesh& m, const si_transform& m_transform,
			 const mesh_scene_mesh_bounding& m_bounding,
			 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);

		static mesh_scene_distance_result distance
			(const mesh_scene& scene,
			 const mesh_scene::object_iterator& m_object,
			 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);
		//@}


	private:

		typedef bounding_distance
					<mesh_scene_triangles, mesh_scene_mesh_bounding,
					 mesh_scene_distance_metric,
					 bounding_distance_no_extended_result>
							bounding_distance_mesh_scene_mesh;


		static mesh_scene_distance_result distance_util
			(const mesh_scene& scene,
			 const mesh& m, const si_transform& m_transform,
			 const mesh_scene_mesh_bounding& m_bounding,
			 const mesh_scene::object_iterator& m_ignore_object,
		 	 float distance_squared_bound,
			 bool distance_squared_bound_valid,
			 float distance_squared_tolerance);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_SCENE_DISTANCE_HPP) */
