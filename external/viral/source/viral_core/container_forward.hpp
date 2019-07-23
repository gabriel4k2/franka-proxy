/**
 *************************************************************************
 *
 * @file container_forward.hpp
 *
 * Forward declarations for the @ref container_module "Container Module".
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__CONTAINER_FORWARD_HPP)
#define INCLUDED__VIRAL_CORE__CONTAINER_FORWARD_HPP


#include "build_config.hpp"


#include "allocator_forward.hpp"


namespace viral_core
{


// Utilities.
template <typename T> class container_limits;


// Smart pointers.
template <typename T> class auto_pointer;

template <typename T> class shared_pointer;
template <typename T> class weak_pointer;
template <typename T> class shared_instance;

template <typename T> class shared_pointer_seal;
template <typename T> class shared_instance_seal;


// Array containers.
template <typename T> class dynamic_buffer;
template <typename T> class array;


// Node-based containers.
template <typename C> class iterator;
template <typename C> class const_iterator;

template <typename T, typename A = allocator_heap>
	class list;
template <typename T, typename A>
	class list_node;

template <typename K, typename A = allocator_heap>
	class tree_set;
template <typename K, typename A>
	class tree_set_node;
template <typename K, typename V, typename A = allocator_heap>
	class tree_map;
template <typename K, typename V, typename A>
	class tree_map_node;
template <typename N, typename A>
	class tree_base;

template <typename T, typename A = allocator_heap>
	class hash_set;
template <typename K, typename A>
	class hash_set_node;
template <typename K, typename V, typename A = allocator_heap>
	class hash_map;
template <typename K, typename V, typename A>
	class hash_map_node;
template <typename N, typename A>
	class hash_bucket;
template <typename N, typename A>
	class hash_base;

template <typename L, typename R, typename A = allocator_heap>
	class bimap;


// Higher-level data structures.
template <typename K> class heap;

template <typename T> class deque_buffer;
template <typename T> class deque_buffer_block;


// Owning containers.
template <typename T> class owning_array;
template <typename T> class owning_list;
template <typename K, typename V> class owning_map;


// String-based caches.
template <typename T> class owning_cache;
template <typename T> class shared_cache;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__CONTAINER_FORWARD_HPP) */
