/**
 *************************************************************************
 *
 * @file bounding_tree.hpp
 *
 * Bounding volume hierarchy with explicit geometry.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_TREE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_TREE_HPP


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
 * @defgroup bounding_module Bounding Module
 * @ingroup geo_module
 *
 * Implements generic bounding volume hierarchies
 * such as bounding_tree and bounding_scene.
 *
 * Requirements for generic bounding volume hierarchies:
 * - A bounding volume hierarchy must be a binary tree with
 *		per-node bounding shapes and leaf-level geometry objects.
 * - Node shapes at any hierarchy level form a conservative bounding
 *		over the geometry objects in all subordinate leaf nodes.
 * - Node shapes need neither bound nor partition the bounding shapes
 *		of parents, siblings, or children. Notably, bounding spheres
 *		may overlap arbitrarly or may exceed parent spheres.
 * - Bounding volume hierarchies perform best if shapes at each level
 *		form a tight fit, plus epsilon, for the leaf geometry objects.
 * - Leaf-level bounding shapes and geometry objects may be identical.
 *		See bounding_implicit for an example implementation.
 *
 * Hierarchy traversal on a bounding volume hierarchy @bref{H}:
 * - typename H::const_node_iterator must be an iterator type
 *		for all nodes in the hierarchy with @ref NOTHROW behaviour,
 *		where
 *			const_node_iterator::const_node_iterator() NOTHROW
 *		must create a past-end node iterator, and where
 *			const_node_iterator::[[any bool conversion]]() const NOTHROW
 *		must test for past-end iterators (e.g. through bool_convert).
 * - const_node_iterator H::root_node() const NOTHROW,
 *   const_node_iterator H::parent_node(const const_node_iterator&) const NOTHROW,
 *   const_node_iterator H::left_node(const const_node_iterator&) const NOTHROW,
 *   const_node_iterator H::right_node(const const_node_iterator&) const NOTHROW
 *		must implement traversal of the hierarchy.
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class bounding_tree_util
 * @ingroup bounding_module
 *
 * Internal utilities for bounding_tree.
 * 
 ************************************************************************/
class bounding_tree_util :
	private non_constructable
{
	template <typename S, typename O>
		friend class bounding_tree;

	private:

		static void log_shape_past_end() NOTHROW;
		static void log_shape_broken_iterator() NOTHROW;

		static void log_object_past_end() NOTHROW;
		static void log_object_broken_iterator() NOTHROW;
		static void log_object_broken_index() NOTHROW;

		static void log_validation_failed(const char* error) NOTHROW;
};




/**
 *************************************************************************
 *
 * @class bounding_tree_node
 * @ingroup bounding_module
 *
 * A single node within a bounding_tree for shapes of type @bref{S}.
 *
 ************************************************************************/
template <typename S>
	class bounding_tree_node
{

	public:

		bounding_tree_node
			(const S& shape, int64 object_index,
			 int64 parent, int64 left, int64 right)
			:
			shape(shape), object_index(object_index),
			parent(parent), left(left), right(right)
		{ }
	

		S shape;
		int64 object_index;

		int64 parent;
		int64 left;
		int64 right; 
};




/**
 *************************************************************************
 *
 * @class bounding_tree
 * @ingroup bounding_module
 *
 * A bounding volume hierarchy with node shapes of type @bref{S}
 * and leaf-level geometry objects of type @bref{O}.
 *
 * General usage notes:
 * - The @ref bounding_module "Bounding Module" lists requirements
 *		for nodes, shapes and objects in all bounding hierarchies.
 * - Use bounding_builder utilities to create bounding_tree%s
 *		over leaf geometry for common shape and object types.
 *
 ************************************************************************/
template <typename S, typename O>
	class bounding_tree
{

	public:

		typedef S shape_type;
		typedef O object_type;

		typedef bounding_tree_node<S> node_type;
		typedef bounding_const_node_iterator<bounding_tree> const_node_iterator;

		
		/** Create an empty bounding tree. */
		bounding_tree() NOTHROW { }


		/**
		 * Create a bounding tree from given @bref{nodes} and @bref{objects}.
		 * The tree root must be the first entry in the @bref{nodes} array.
		 * Constructors automatically validate() resulting bounding trees.
		 */
		//@{
		bounding_tree
			(const array<bounding_tree_node<S>>& nodes,
			 const array<O>& objects)
			:
			nodes_(nodes),
			objects_(objects)
		{
			validate();
		}

		bounding_tree
			(array<bounding_tree_node<S>>&& nodes,
			 array<O>&& objects)
			:
			nodes_(std::move(nodes)),
			objects_(std::move(objects))
		{
			validate();
		}
		//@}


		auto_pointer<bounding_tree> clone() const
		{
			return auto_pointer<bounding_tree>
						(new bounding_tree(*this));
		}


		const array<bounding_tree_node<S>>& nodes() const NOTHROW { return nodes_; }
		int64 node_count() const NOTHROW { return nodes_.size(); }

		const array<O>& objects() const NOTHROW { return objects_; }
		int64 object_count() const NOTHROW { return objects_.size(); }


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
		 * Determine if a @bref{node} has a shape or an object.
		 * Returns @false on any past-end iterators.
		 */
		//@{
		bool has_shape(const const_node_iterator& node) const NOTHROW
			{ return node; }

		bool has_object(const const_node_iterator& node) const NOTHROW
		{
			return node &&
				   node->object_index >= 0 &&
				   node->object_index < objects_.size();
		}
		//@}


		/**
		 * Fetch shape or object for a @bref{node}.
		 * Throws if the @bref{node} iterator is past-end,
		 * or if the node has no shape or object.
		 */
		//@{
		const S& shape(const const_node_iterator& node) const
		{
			if (!node)
			{
				bounding_tree_util::log_shape_past_end();
				throw invalid_parameter();
			}

			if (nodes_.empty() ||
				&(*node) < nodes_.data() ||
				&(*node) >= nodes_.data() + nodes_.size())
			{
				bounding_tree_util::log_shape_broken_iterator();
				throw assert_failed();
			}

			return node->shape;
		}

		const O& object(const const_node_iterator& node) const
		{
			if (!node)
			{
				bounding_tree_util::log_object_past_end();
				throw invalid_parameter();
			}

			if (nodes_.empty() ||
				&(*node) < nodes_.data() ||
				&(*node) >= nodes_.data() + nodes_.size())
			{
				bounding_tree_util::log_object_broken_iterator();
				throw assert_failed();
			}		

			if (node->object_index < 0 ||
				node->object_index >= objects_.size())
			{
				bounding_tree_util::log_object_broken_index();
				throw invalid_parameter();
			}

			return objects_[node->object_index];
		}
		//@}
		

		/**
		 * Check tree for errors and throw validation_failed
		 * if any errors are found (e.g. invalid node index,
		 * invalid object index, or tree height too large).
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
			// We cannot similarly check for unused objects,
			// as multiple nodes may share object indices.
			if (nodes_visited != nodes_.size())
			{
				bounding_tree_util::log_validation_failed("There are unlinked nodes.");
				throw validation_failed(); 
			}
		}
		 

		/**
		 * Invalid index demarks unset node indices
		 * in bounding_tree_node%s.
		 */		
		static const int64 invalid_index = -1;


		/**
		 * Maximum tree height allowed by validate().
		 * Counteracts errors due to stack overflow on recursion.
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
				bounding_tree_util::log_validation_failed("Tree exceeds maximum height.");
				throw validation_failed();
			}
	
			if (node_index < 0 || node_index >= nodes_.size())
			{
				bounding_tree_util::log_validation_failed("Tree uses invalid node index.");
				throw validation_failed();
			}

			if (nodes_[node_index].parent != parent_index)
			{
				bounding_tree_util::log_validation_failed("Node has invalid parent index.");
				throw validation_failed();
			}

			if (nodes_[node_index].left == invalid_index &&
				nodes_[node_index].right == invalid_index)
			{
				if (nodes_[node_index].object_index == invalid_index)
				{
					bounding_tree_util::log_validation_failed("Leaf has no geometry object index.");
					throw validation_failed();
				}

				if (nodes_[node_index].object_index < 0 ||
					nodes_[node_index].object_index >= objects_.size())
				{
					bounding_tree_util::log_validation_failed("Object index invalid.");
					throw validation_failed();
				}
			}
			else
			{
				if (nodes_[node_index].object_index != invalid_index)
				{
					bounding_tree_util::log_validation_failed("Inner node has geometry object index.");
					throw validation_failed();
				}
			}

			validate
				(nodes_[node_index].left, node_index,
				 parent_height + 1, nodes_visited);
			validate
				(nodes_[node_index].right, node_index,
				 parent_height + 1, nodes_visited);			
		}


		array<bounding_tree_node<S>> nodes_;
		array<O> objects_;
};


/**
 * Common bounding_tree%s.
 * @ingroup bounding_module
 * 
 * Indexed variants (e.g. triangle indices in a @ref mesh)
 * require less storage space, while direct variants
 * are more efficient and can be used on their own.
 */
//@{
typedef bounding_tree<sphere, triangle> sphere_tree_triangle;
typedef bounding_tree<sphere, vector> sphere_tree_vector;
typedef bounding_tree<sphere, int64> sphere_tree_index;

typedef bounding_tree<box, triangle> box_tree_triangle;
typedef bounding_tree<box, vector> box_tree_vector;
typedef bounding_tree<box, int64> box_tree_index;
//@}


/**
 * Iterators over common bounding_tree%s.
 * @ingroup bounding_module
 */
//@{
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
//@}




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_TREE_HPP) */
