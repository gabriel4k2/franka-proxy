/**
 *************************************************************************
 *
 * @file bounding_implicit.hpp
 *
 * Bounding volume hierarchy with implicit geometry.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "auto_pointer.hpp"
#include "array.hpp"
#include "bounding_iterator.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_implicit_util
 * @ingroup bounding_module
 *
 * Internal utilities for bounding_implicit.
 * 
 ************************************************************************/
class bounding_implicit_util :
	private non_constructable
{
	template <typename S> friend class bounding_implicit;

	private:

		static void log_shape_past_end() NOTHROW;
		static void log_shape_broken_iterator() NOTHROW;

		static void log_validation_failed(const char* error) NOTHROW;
};




/**
 *************************************************************************
 *
 * @class bounding_implicit_node
 * @ingroup bounding_module
 *
 * A node within a bounding_implicit for shapes of type @bref{S}.
 *
 ************************************************************************/
template <typename S>
	class bounding_implicit_node
{

	public:

		bounding_implicit_node
			(const S& shape,
			 int64 parent, int64 left, int64 right)
			:
			shape(shape),
			parent(parent), left(left), right(right)
		{ }
	
		S shape;

		int64 parent;
		int64 left;
		int64 right; 
};




/**
 *************************************************************************
 *
 * @class bounding_implicit
 * @ingroup bounding_module
 *
 * A bounding volume hierarchy with bounding shapes of type @bref{S}
 * that implicitly define leaf-level geometry.
 *
 * General usage notes:
 * - The @ref bounding_module "Bounding Module" lists requirements
 *		for nodes, shapes and objects in all bounding hierarchies.
 * - Use bounding_builder utilities to create bounding_implicit%s
 *		over common shapes.
 *
 ************************************************************************/
template <typename S>
	class bounding_implicit
{

	public:

		typedef S shape_type;
		typedef S object_type; // Implicit objects through shapes.

		typedef bounding_implicit_node<S> node_type;
		typedef bounding_const_node_iterator<bounding_implicit> const_node_iterator;

		
		/** Create an empty bounding volume hierarchy. */
		bounding_implicit() NOTHROW { }


		/**
		 * Create an implicit bounding tree from given @bref{nodes}.
		 * The tree root must be the first entry in the @bref{nodes} array.
		 * Constructors automatically validate() resulting bounding trees.
		 */
		//@{
		bounding_implicit
			(const array<bounding_implicit_node<S>>& nodes)
			:
			nodes_(nodes)
		{
			validate();
		}

		bounding_implicit
			(array<bounding_implicit_node<S>>&& nodes)
			:
			nodes_(std::move(nodes))
		{
			validate();
		}
		//@}


		auto_pointer<bounding_implicit> clone() const
		{
			return auto_pointer<bounding_implicit>
						(new bounding_implicit(*this));
		}


		const array<bounding_implicit_node<S>>& nodes() const NOTHROW { return nodes_; }
		int64 node_count() const NOTHROW { return nodes_.size(); }


		/**
		 * Iterate through the nodes of the bounding tree.
		 * Queries return a past-end iterator to indicate
		 * that a requested node does not exist.
		 */
		//@{
		const_node_iterator root_node() const NOTHROW
			{ return const_node_iterator(nodes_.empty() ? NULL : nodes_.data()); }

		const_node_iterator parent_node
			(const const_node_iterator& node) const NOTHROW
		{
			return const_node_iterator
						((node && node->parent != invalid_index) ?
						 &nodes_[node->parent] : NULL);
		}

		const_node_iterator left_node
			(const const_node_iterator& node) const NOTHROW
		{
			return const_node_iterator
						((node && node->left != invalid_index) ?
						 &nodes_[node->left] : NULL);
		}

		const_node_iterator right_node
			(const const_node_iterator& node) const NOTHROW
		{
			return const_node_iterator
						((node && node->right != invalid_index) ?
						 &nodes_[node->right] : NULL);
		}
		//@}


		/**
		 * Determine overall tree height, 0 for empty trees,
		 * and 1 for trees with exactly one (root) node.
		 * @attent Height is not cached and expensive to find.
		 */
		int64 find_height() const NOTHROW
			{ return find_height(root_node(), 0); }


		/**
		 * Determine if a @bref{node} has a shape.
		 * Returns @false on any past-end iterators.
		 */
		bool has_shape(const const_node_iterator& node) const NOTHROW
			{ return node; }


		/**
		 * Fetch shape for given @bref{node}.
		 * Throws if the @bref{node} is past-end,
		 * or if the node has no shape.
		 */
		const S& shape(const const_node_iterator& node) const
		{
			if (!node)
			{
				bounding_implicit_util::log_shape_past_end();
				throw invalid_parameter();
			}

			if (nodes_.empty() ||
				&(*node) < nodes_.data() ||
				&(*node) >= nodes_.data() + nodes_.size())
			{
				bounding_implicit_util::log_shape_broken_iterator();
				throw assert_failed();
			}

			return node->shape;
		}


		/**
		 * Check tree for errors and throw validation_failed
		 * if any errors are found (e.g. tree too large).
		 *
		 * Validation is rather efficient and its overhead
		 * should be in the order of node initialization.
		 */
		void validate() const
		{
			int64 nodes_visited = 0;
			validate(nodes_.empty() ? invalid_index : 0,
					 invalid_index, 0, nodes_visited);

			// We neither visited nor validated unused nodes,
			// therefore we error out to be on the safe side.
			if (nodes_visited != nodes_.size())
			{
				bounding_implicit_util::log_validation_failed("There are unlinked nodes.");
				throw validation_failed(); 
			}
		}
		 

		/**
		 * Invalid index demarks unset node indices
		 * in bounding_implicit_node%s.
		 */		
		static const int64 invalid_index = -1;


		/**
		 * Maximum tree height allowed by validate().
		 * Avoids stack overflow errors on recursion.
		 */
		static const int64 maximum_height = 128;


	private:

		int64 find_height
			(const const_node_iterator& node, int64 parent_height) const NOTHROW
		{
			if (!node) return parent_height;
			return max(find_height(left_node(node), parent_height + 1),
					   find_height(right_node(node), parent_height + 1));
		}


		void validate
			(int64 node_index, int64 parent_index,
			 int64 parent_height, int64& nodes_visited) const
		{
			if (node_index == invalid_index) return;

			++nodes_visited;

			if (parent_height + 1 > maximum_height)
			{
				bounding_implicit_util::log_validation_failed("Tree exceeds maximum height.");
				throw validation_failed();
			}
	
			if (node_index < 0 || node_index >= nodes_.size())
			{
				bounding_implicit_util::log_validation_failed("Tree uses invalid node index.");
				throw validation_failed();
			}

			if (nodes_[node_index].parent != parent_index)
			{
				bounding_implicit_util::log_validation_failed("Node has invalid parent index.");
				throw validation_failed();
			}

			validate
				(nodes_[node_index].left, node_index,
				 parent_height + 1, nodes_visited);
			validate
				(nodes_[node_index].right, node_index,
				 parent_height + 1, nodes_visited);			
		}


		array<bounding_implicit_node<S>> nodes_;
};


/**
 * Common bounding_implicit%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_implicit<sphere> sphere_tree;
typedef bounding_implicit<box> box_tree;
//@}


/**
 * Iterators over common bounding_implicit%s.
 * @ingroup bounding_module
 */
//@{
typedef bounding_const_node_iterator<sphere_tree>
	sphere_tree_const_node_iterator;
typedef bounding_const_node_iterator<box_tree>
	box_tree_const_node_iterator;
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_IMPLICIT_HPP) */
