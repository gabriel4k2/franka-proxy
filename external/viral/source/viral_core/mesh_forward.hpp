/**
 *************************************************************************
 *
 * @file mesh_forward.hpp
 *
 * Forward declarations for the @ref mesh_module "Mesh Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MESH_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__MESH_FORWARD_HPP


#include "build_config.hpp"


#include "container_forward.hpp"
#include "image_forward.hpp"
#include "geo_forward.hpp"
#include "bounding_forward.hpp"


namespace viral_core
{


// Skeletal animation.
class skeleton;
class skeleton_joint;
typedef array<skeleton_joint> skeleton_joint_array;
typedef array<si_transform> skeleton_transform;
typedef array<matrix> skeleton_transform_matrix;

class skeleton_index;
class skeleton_weight;


// Meshes and models.
class mesh_point;
class mesh_line;
class mesh_triangle;
class mesh_stream;

template <typename T> class mesh_stream_typed;

typedef mesh_stream_typed<int32> mesh_stream_int32;
typedef mesh_stream_typed<uint32> mesh_stream_uint32;
typedef mesh_stream_typed<float> mesh_stream_float;

typedef mesh_stream_typed<color> mesh_stream_color;
typedef mesh_stream_typed<vector> mesh_stream_vector;
typedef mesh_stream_typed<plane> mesh_stream_plane;

typedef mesh_stream_typed<mesh_point> mesh_stream_point;
typedef mesh_stream_typed<mesh_line> mesh_stream_line;
typedef mesh_stream_typed<mesh_triangle> mesh_stream_triangle;

typedef mesh_stream_typed<skeleton_index> mesh_stream_skeleton_index;
typedef mesh_stream_typed<skeleton_weight> mesh_stream_skeleton_weight;

class mesh;
class model;


// Mesh utilities.
class mesh_primitive;
class mesh_util;
class mesh_optimize;
class mesh_tesselate;


// Mesh intersection tests.
class mesh_intersect;
class mesh_intersect_test_triangle;
class mesh_intersect_test_index;

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


// Mesh raycasting tests.
class mesh_raycast;
class mesh_raycast_test_triangle;
class mesh_raycast_test_index;

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


// Mesh region tests.
class mesh_region;
class mesh_region_test_triangle;
class mesh_region_test_index;

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


// Mesh proximity tests.
class mesh_proximate;
class mesh_proximate_test_triangle;
class mesh_proximate_test_index;

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


// Mesh distance tests.
class mesh_distance;
class mesh_distance_metric_triangle;
class mesh_distance_metric_index;

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


// Mesh neighbor queries.
class mesh_k_nearest;
class mesh_k_nearest_metric_triangle;
class mesh_k_nearest_metric_index;

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


// Mesh scenes.
class mesh_scene;
class mesh_scene_object;
class mesh_scene_builder;

typedef box_tree_index mesh_scene_mesh_bounding;
typedef bounding_scene<box, mesh_scene_object, mesh_scene_builder>
			mesh_scene_meshes_bounding;
typedef bounding_combiner<mesh_scene, mesh_scene_mesh_bounding>
			mesh_scene_triangles;
typedef bounding_combiner_iterator<mesh_scene, mesh_scene_mesh_bounding>
			mesh_scene_triangles_const_node_iterator;

class mesh_scene_intersect;
class mesh_scene_intersect_result;
class mesh_scene_intersect_test;

class mesh_scene_raycast;
class mesh_scene_raycast_result;
class mesh_scene_raycast_test;

class mesh_scene_region;
class mesh_scene_region_entry;
class mesh_scene_region_result;
class mesh_scene_region_test;

class mesh_scene_proximate;
class mesh_scene_proximate_result;
class mesh_scene_proximate_test;

class mesh_scene_distance;
class mesh_scene_distance_result;
class mesh_scene_distance_metric;

class mesh_scene_k_nearest;
class mesh_scene_k_nearest_entry;
class mesh_scene_k_nearest_result;
class mesh_scene_k_nearest_metric;


// Collada import.
class collada_importer;
class collada_parser;
class collada_parser_limits;
class collada_scene_parser;
class collada_geometry_parser;
class collada_skeleton_parser;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MESH_FORWARD_HPP) */