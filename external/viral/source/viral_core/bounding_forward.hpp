/**
 *************************************************************************
 *
 * @file bounding_forward.hpp
 *
 * Forward declarations for the @ref bounding_module "Bounding Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_FORWARD_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


namespace viral_core
{


// Bounding iterator.
template <typename N> class bounding_const_node_iterator;


// Bounding trees.
template <typename S, typename O> class bounding_tree;
template <typename S> class bounding_tree_node;
class bounding_tree_util;

typedef bounding_tree<sphere, triangle> sphere_tree_triangle;
typedef bounding_tree<sphere, vector> sphere_tree_vector;
typedef bounding_tree<sphere, int64> sphere_tree_index;

typedef bounding_tree<box, triangle> box_tree_triangle;
typedef bounding_tree<box, vector> box_tree_vector;
typedef bounding_tree<box, int64> box_tree_index;

typedef bounding_const_node_iterator<sphere_tree_triangle>
	sphere_tree_triangle_const_node_iterator;
typedef bounding_const_node_iterator<sphere_tree_vector>
	sphere_tree_vector_const_node_iterator;
typedef bounding_const_node_iterator<sphere_tree_index>
	sphere_tree_index_const_node_iterator;

typedef bounding_const_node_iterator<box_tree_triangle>
	box_tree_triangle_const_node_iterator;
typedef bounding_const_node_iterator<box_tree_vector>
	box_tree_vector_const_node_iterator;
typedef bounding_const_node_iterator<box_tree_index>
	box_tree_index_const_node_iterator;


// Implicit bounding trees.
template <typename S> class bounding_implicit;
template <typename S> class bounding_implicit_node;
class bounding_implicit_util;

typedef bounding_implicit<sphere> sphere_tree;
typedef bounding_implicit<box> box_tree;

typedef bounding_const_node_iterator<sphere_tree>
	sphere_tree_const_node_iterator;
typedef bounding_const_node_iterator<box_tree>
	box_tree_const_node_iterator;


// Bounding volume hierarchies over a single object.
template <typename S, typename O> class bounding_single;
template <typename S, typename O> class bounding_single_node;
class bounding_single_util;

typedef bounding_single<sphere, sphere> bounding_single_sphere;
typedef bounding_single<box, box> bounding_single_box;

typedef bounding_single<sphere, triangle> bounding_single_sphere_triangle;
typedef bounding_single<box, triangle> bounding_single_box_triangle;

typedef bounding_const_node_iterator<bounding_single_sphere>
	bounding_single_sphere_const_node_iterator;
typedef bounding_const_node_iterator<bounding_single_box>
	bounding_single_box_const_node_iterator;

typedef bounding_const_node_iterator<bounding_single_sphere_triangle>
	bounding_single_sphere_triangle_const_node_iterator;
typedef bounding_const_node_iterator<bounding_single_box_triangle>
	bounding_single_box_triangle_const_node_iterator;


// Dynamic scenes of bounding volumes.
template <typename S, typename O, typename B> class bounding_scene;
template <typename S, typename O> class bounding_scene_node;
class bounding_scene_util;


// Combined bounding volume hierarchies.
template <typename C, typename D> class bounding_combiner_iterator;
template <typename C, typename D> class bounding_combiner;


// Utilities for bounding volume hierarchies.
template <typename S> class bounding_builder_span;
template <typename S> class bounding_builder_split;
class bounding_builder;

template <typename A, typename B> class bounding_intersect_pair;
template <typename A, typename B, typename X>
	class bounding_intersect_result;
class bounding_intersect_no_extended_result;
class bounding_intersect_util;
template <typename A, typename B, typename T, typename X>
	class bounding_intersect;

template <typename H> class bounding_raycast_candidate;
template <typename H, typename X>
	class bounding_raycast_result;
class bounding_raycast_no_extended_result;
class bounding_raycast_util;
template <typename R, typename H, typename T, typename X>
	class bounding_raycast;

template <typename H, typename X>
	class bounding_region_entry;
template <typename H, typename X>
	class bounding_region_result;
class bounding_region_no_extended_result;
class bounding_region_util;
template <typename H, typename R, typename T, typename X>
	class bounding_region;

template <typename A, typename B> class bounding_proximate_pair;
template <typename A, typename B, typename X>
	class bounding_proximate_result;
class bounding_proximate_no_extended_result;
class bounding_proximate_util;
template <typename A, typename B, typename T, typename X>
	class bounding_proximate;

template <typename A, typename B> class bounding_distance_pair;
template <typename A, typename B, typename X>
	class bounding_distance_result;
class bounding_distance_no_extended_result;
class bounding_distance_util;
template <typename A, typename B, typename M, typename X>
	class bounding_distance;

template <typename H>
	class bounding_k_nearest_node_candidate;
template <typename H, typename X>
	class bounding_k_nearest_neighbor_candidate;
template <typename H, typename X>
	class bounding_k_nearest_entry;
template <typename H, typename X>
	class bounding_k_nearest_result;
class bounding_k_nearest_no_extended_result;
class bounding_k_nearest_util;
template <typename H, typename O, typename M, typename X>
	class bounding_k_nearest;


// Bounding utilities for implicit trees.
class bounding_implicit_intersect;
class bounding_implicit_intersect_test;

class bounding_implicit_raycast;
class bounding_implicit_raycast_test;

class bounding_implicit_proximate;
class bounding_implicit_proximate_test;

class bounding_implicit_distance;
class bounding_implicit_distance_metric;


// Bounding scenes over implicit trees.
class sphere_tree_scene;
class sphere_tree_scene_object;
class sphere_tree_scene_builder;

typedef bounding_scene<sphere, sphere_tree_scene_object,
					   sphere_tree_scene_builder>
							sphere_tree_scene_trees_bounding;

typedef bounding_combiner<sphere_tree_scene, sphere_tree>
			sphere_tree_scene_spheres;
typedef bounding_combiner_iterator<sphere_tree_scene, sphere_tree>
			sphere_tree_scene_spheres_const_node_iterator;


// Bounding utilities for scenes of implicit trees.
class sphere_tree_scene_intersect;
class sphere_tree_scene_intersect_result;
class sphere_tree_scene_intersect_test;

class sphere_tree_scene_raycast;
class sphere_tree_scene_raycast_result;
class sphere_tree_scene_raycast_test;

class sphere_tree_scene_region;
class sphere_tree_scene_region_entry;
class sphere_tree_scene_region_result;
class sphere_tree_scene_region_test;

class sphere_tree_scene_proximate;
class sphere_tree_scene_proximate_result;
class sphere_tree_scene_proximate_test;

class sphere_tree_scene_distance;
class sphere_tree_scene_distance_result;
class sphere_tree_scene_distance_metric;

class sphere_tree_scene_k_nearest;
class sphere_tree_scene_k_nearest_entry;
class sphere_tree_scene_k_nearest_result;
class sphere_tree_scene_k_nearest_metric;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_FORWARD_HPP) */