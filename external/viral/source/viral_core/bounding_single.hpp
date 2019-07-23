/**
 *************************************************************************
 *
 * @file bounding_single.hpp
 *
 * Single-node bounding volume hierarchy.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_SINGLE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_SINGLE_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "auto_pointer.hpp"
#include "bounding_iterator.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_single_util
 * @ingroup bounding_module
 *
 * Internal utilities for bounding_single.
 * 
 ************************************************************************/
class bounding_single_util :
	private non_constructable
{
	template <typename S, typename O>
		friend class bounding_single;

	private:

		static void log_shape_past_end() NOTHROW;
		static void log_shape_broken_iterator() NOTHROW;

		static void log_object_past_end() NOTHROW;
		static void log_object_broken_iterator() NOTHROW;
};




/**
 *************************************************************************
 *
 * @class bounding_single_node
 * @ingroup bounding_module
 *
 * The one node in a bounding_single.
 *
 ************************************************************************/
template <typename S, typename O>
	class bounding_single_node
{

	public:

		bounding_single_node(const S& shape, const O& object)
			: shape(shape), object(object) { }

		const S shape;
		const O object;
};




/**
 *************************************************************************
 *
 * @class bounding_single
 * @ingroup bounding_module
 *
 * A bounding volume hierarchy with a single shape of type @bref{S}
 * and a single object of type @bref{O}.
 *
 * Usage notes:
 * - A bounding_single allows to use just one shape and geometry object
 *		in certain bounding volume utilities (e.g. bounding_intersect)
 *		without the overhead of memory allocation in bounding_tree. 
 * - The @ref bounding_module "Bounding Module" lists requirements
 *		for nodes, shapes and objects in all bounding hierarchies.
 *
 ************************************************************************/
template <typename S, typename O>
	class bounding_single
{

	public:

		typedef S shape_type;
		typedef O object_type;

		typedef bounding_single_node<S, O> node_type;
		typedef bounding_const_node_iterator<bounding_single> const_node_iterator;


		bounding_single() { }
		bounding_single(const S& shape, const O& object)
			: node_(shape, object) { }


		auto_pointer<bounding_single> clone() const
		{
			return auto_pointer<bounding_single>
						(new bounding_single(*this));
		}


		int64 node_count() const NOTHROW { return 1; }
		int64 object_count() const NOTHROW { return 1; }


		/** No-op hierarchy traversal. */
		//@		
		const_node_iterator root_node() const NOTHROW
			{ return const_node_iterator(&node_); }

		const_node_iterator parent_node
			(const const_node_iterator& node) const NOTHROW
				{ return const_node_iterator(); }
		const_node_iterator left_node
			(const const_node_iterator& node) const NOTHROW
				{ return const_node_iterator(); }
		const_node_iterator right_node
			(const const_node_iterator& node) const NOTHROW
				{ return const_node_iterator(); }
		//@}


		int64 find_height() const NOTHROW { return 1; }


		/** Access to shape and object. */
		//@{
		bool has_shape(const const_node_iterator& node) const NOTHROW
			{ return node; }
		bool has_object(const const_node_iterator& node) const NOTHROW
			{ return node; }

		const S& shape(const const_node_iterator& node) const
		{
			if (!node)
			{
				bounding_single_util::log_shape_past_end();
				throw invalid_parameter();
			}

			if (&(*node) != &node_)
			{
				bounding_single_util::log_shape_broken_iterator();
				throw assert_failed();
			}

			return node_.shape;
		}

		const O& object(const const_node_iterator& node) const
		{
			if (!node)
			{
				bounding_single_util::log_object_past_end();
				throw invalid_parameter();
			}

			if (&(*node) != &node_)
			{
				bounding_single_util::log_object_broken_iterator();
				throw assert_failed();
			}

			return node_.object;
		}
		//@}


	private:

		const node_type node_;
};


/**
 * Common bounding_single%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_single<sphere, sphere> bounding_single_sphere;
typedef bounding_single<box, box> bounding_single_box;

typedef bounding_single<sphere, triangle> bounding_single_sphere_triangle;
typedef bounding_single<box, triangle> bounding_single_box_triangle;
//@}


/**
 * Iterators over common bounding_single%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_const_node_iterator<bounding_single_sphere>
	bounding_single_sphere_const_node_iterator;
typedef bounding_const_node_iterator<bounding_single_box>
	bounding_single_box_const_node_iterator;

typedef bounding_const_node_iterator<bounding_single_sphere_triangle>
	bounding_single_sphere_triangle_const_node_iterator;
typedef bounding_const_node_iterator<bounding_single_box_triangle>
	bounding_single_box_triangle_const_node_iterator;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_SINGLE_HPP) */
