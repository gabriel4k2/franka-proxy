/**
 *************************************************************************
 *
 * @file bounding_scene.hpp
 *
 * Bounding volumes for large-scale scenes.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__BOUNDING_SCENE_HPP)
#define INCLUDED__VIRAL_CORE__BOUNDING_SCENE_HPP


#include "build_config.hpp"


#include "geo_forward.hpp"


#include "util.hpp"
#include "exception.hpp"
#include "auto_pointer.hpp"
#include "list.hpp"
#include "tree_map.hpp"
#include "bounding_iterator.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_scene_util
 * @ingroup bounding_module
 *
 * Internal utilities for bounding_scene%s.
 *
 ************************************************************************/
class bounding_scene_util :
	private non_constructable
{
	template <typename S, typename O, typename B>
		friend class bounding_scene;

	private:

		static void log_update_past_end() NOTHROW;
		static void log_update_no_node() NOTHROW;

		static void log_remove_past_end() NOTHROW;
		static void log_remove_no_node() NOTHROW;

		static void log_shape_past_end() NOTHROW;
		static void log_object_past_end() NOTHROW;
		static void log_object_not_leaf() NOTHROW;

		static void log_validation_failed(const char* error) NOTHROW;
};




/**
 *************************************************************************
 *
 * @class bounding_scene_node
 * @ingroup bounding_module
 * 
 * A single node in a bounding_scene over shapes of type @bref{S}
 * with an optional object of type @bref{O}.
 *
 * Implementation notes:
 * - A bounding_scene_node owns its children nodes.
 * - Each bounding_scene_node has either two children or no children:
 *		Nodes with a single child would not consider additional boundings,
 *		and thus could directly be replaced with the respective child.
 * - See bounding_scene for considerations on the @bref{object_} iterator.
 *
 ************************************************************************/
template <typename S, typename O>
	class bounding_scene_node
{
	template <typename S, typename O, typename H>
		friend class bounding_scene;


	public:

		typedef list<O> object_list;
		typedef typename object_list::iterator object_iterator;


		const S& shape() const NOTHROW
			{ return shape_; }
		const object_iterator& object() const NOTHROW
			{ return object_; }


	private:

		bounding_scene_node
			(const S& shape, object_iterator object)
			:
			shape_(shape),
			object_(object),
			parent_(NULL),
			balance_(0)
		{ }


		S shape_;
		const object_iterator object_;
	
		bounding_scene_node* parent_;
		auto_pointer<bounding_scene_node> left_;
		auto_pointer<bounding_scene_node> right_;
		char balance_;
};




/**
 *************************************************************************
 *
 * @class bounding_scene
 * @ingroup bounding_module
 *
 * A loose bounding volume hierarchy over shapes of type @bref{S}
 * with objects of type @bref{O} and a builder utility @bref{B}.
 *
 * Usage notes:
 * - A bounding_scene can efficiently update() its bounding volumes.
 *		However, the bounding_scene has a higher traversal overhead
 *		and requires more memory than the static bounding_tree.
 * - Whenever the bounding shape of a contained object has changed,
 *		for instance through object transformation or deformation,
 *		clients must explicitly update() the scene for this object.
 *		Otherwise, later spatial queries (e.g. bounding_distance)
 *		may return wrong results.
 * - See the @ref bounding_module "Bounding Module" for requirements
 *		on the nodes, shapes and objects within a bounding_scene.
 *		The bounding shape type @bref{S} must additionally support
 *		@ref NOTHROW assignment due to @ref NOTHROW update()s.
 *
 * Object considerations:
 * - Nodes in a bounding_scene expose a non-constant object_iterator.
 *		This enables clients to modify objects in a const bounding_scene
 *		through a bounding_const_node_iterator (e.g. as by root_node()).
 * - To avoid inadverted changes to objects through the object_iterator,
 *		object types @bref{O} should realize a read-only public interface.
 *		Clients then must change objects through a bounding_scene wrapper,
 *		which in turn exposes only non-const methods for changing objects.
 * - Alternatives (e.g. const_object_iterator) would require separate
 *		const and non-const bounding utilities (e.g. bounding_distance)
 *		if clients are to modify result objects on non-const hierarchies.
 *
 * Builder requirements:
 * - The scene builder @bref{B} must implement methods
 *		to acquire, combine, and score bounding shapes.
 * - S B::bounding_shape(const O& object) NOTHROW
 *		must return a bounding shape for an @bref{object}.
 * - S B::combine_shapes(const S& sa, const S& sb) NOTHROW
 *		must combine bounding shapes @bref{sa} and @bref{sb}
 *		to another, encompassing bounding shape.
 * - float B::score_insert
 *			(const S& shape,
 *			 const S& left_shape, const S& right_shape) const NOTHROW
 *		must decide whether to insert an incoming bounding @bref{shape}
 *		into the subtree below @bref{left_shape} or @bref{right_shape}.
 *		A positive score indicates to use the @bref{right_shape} subtree,
 *		while a negative score implies the @bref{left_shape} subtree.
 *		A zero score prefers the subtree that has less leaf nodes.
 * - The construction-bound @bref{builder_} instance must outlive
 *		the bounding_scene hierarchy.
 *
 * Implementation notes:
 * - To prohibit hierarchy degeneration on pathological cases
 *		(e.g. insert()ing objects that are sorted along an axis),
 *		the bounding_scene implements an AVL-like rebalancing.
 * - Rebalancing entails an O(log(object_count())) height limit,
 *		which counteracts errors due to stack overflow on recursion.
 *
 ************************************************************************/
template <typename S, typename O, typename B>
	class bounding_scene :
		private non_copyable
{
	public:

		typedef S shape_type;
		typedef O object_type;

		typedef bounding_scene_node<S, O> node_type;
		typedef bounding_const_node_iterator<bounding_scene> const_node_iterator;
	
		typedef list<O> object_list;
		typedef typename object_list::iterator object_iterator;
		typedef typename object_list::const_iterator const_object_iterator;


		/**
		 * Create an empty bounding volume hierarchy
		 * that is built through a @bref{builder}.
		 */
		bounding_scene(B& builder) NOTHROW
			: builder_(builder) { }


		int64 node_count() const NOTHROW
		{
			// Implementation notes:
			// - Each leaf node except for the root leaf
			//		attaches through a new parent node.
			//		See bounding_scene_node and insert().
			// - No int64 overflow possible, since nodes 
			//		have been allocated successfully.
			return objects_.empty() ?
						0 : (objects_.size() * 2 - 1);
		}

		int64 object_count() const NOTHROW
			{ return objects_.size(); }


		/**
		 * Insert a new copy of some @bref{object}
		 * into the bounding_scene.
		 *
		 * Strong exception safety: Throws on failure,
		 * the bounding scene then remains unchanged.
		 */
		object_iterator insert(const O& object)
		{
			// Insert object into object list.
			object_iterator object_it
				(objects_.insert(object));
			SCOPE_GUARD
				(objects_guard, (objects_, &object_list::pop_back));


			// Create a new scene node for the object.
			// We also need a new parent node to attach
			// both the new node and an existing leaf,
			// unless the scene is currently empty.
			S shape(builder_.bounding_shape(object));
			auto_pointer<node_type> leaf_node
				(new node_type(shape, object_it));
			auto_pointer<node_type> parent_node
				(root_node_ ?
					new node_type(shape, object_iterator()) :
					NULL);


			// Insert new scene node to object lookup.
			object_to_node_.insert
				(object_it, leaf_node.get());
			SCOPE_GUARD
				(object_to_node_guard,
					(object_to_node_,
					 &object_to_node_map::remove_by_key,
					 object_it));


			// Insert new scene node into hierarchy.
			insert_leaf_node
				(std::move(leaf_node),
				 std::move(parent_node));


			// Success. NOTHROW from here.
			object_to_node_guard.dismiss();
			objects_guard.dismiss();
			return object_it;
		}

		
		/**
		 * Update existing object within the bounding_scene
		 * through given @bref{it}erator.
		 *
		 * The call ignores any errors (e.g. past-end iterators)
		 * for @ref NOTHROW behaviour.
		 *
		 * @attent For an unchanged object, update() still must
		 * build and check the bounding shape before early exit.
		 * To avoid this overhead, clients should not update()
		 * unchanged objects. 
		 */
		void update(const object_iterator& it) NOTHROW
		{
			if (!it)
			{
				bounding_scene_util::log_update_past_end();
				return; // NOTHROW
			}

			object_to_node_map::iterator find_node
				(object_to_node_.find(it));

			if (!find_node)
			{
				bounding_scene_util::log_update_no_node();
				return; // NOTHROW.
			}


			// Update the leaf-level bounding shape.
			// Early out if the bounding shape did not change.
			// This avoids perpetual tree rebalancing through
			// removing and reinserting an unchanged object.
			S new_bounding_shape
				(builder_.bounding_shape(*it));

			if (new_bounding_shape == find_node->value->shape_) return;
			find_node->value->shape_ = new_bounding_shape;


			// Instead of explicitly updating our hierarchy,
			// we detach and re-attach the leaf and its parent
			// for a simplified, allocation-less update process.
			auto_pointer<node_type> detached_leaf_node;
			auto_pointer<node_type> detached_parent_node;

			detach_leaf_node
				(find_node->value,
				 detached_leaf_node,
				 detached_parent_node);

			insert_leaf_node
				(std::move(detached_leaf_node),
				 std::move(detached_parent_node));
		}


		/**
		 * Remove an existing object from the bounding_scene
		 * through given @bref{it}erator.
		 *
		 * The call ignores any errors (e.g. past-end iterators)
		 * for @ref NOTHROW behaviour.
		 */
		void remove(const object_iterator& it) NOTHROW
		{
			if (!it)
			{
				bounding_scene_util::log_remove_past_end();
				return; // NOTHROW
			}

			object_to_node_map::iterator find_node
				(object_to_node_.find(it));

			if (!find_node)
			{
				bounding_scene_util::log_remove_no_node();
				return; // NOTHROW.
			}


			// Remove leaf node and its parent
			// from the bounding hierarchy.
			auto_pointer<node_type> delete_detached_leaf_node;
			auto_pointer<node_type> delete_detached_parent_node;
			detach_leaf_node
				(find_node->value,
				 delete_detached_leaf_node,
				 delete_detached_parent_node);


			// Remove object from list and lookup.
			object_to_node_.remove_by_iterator(find_node);
			objects_.remove(it);
		}


		/** Traversal of the bounding volume hierarchy. */
		//@{
		const_node_iterator root_node() const NOTHROW
			{ return const_node_iterator(root_node_.get()); }

		const_node_iterator parent_node
			(const const_node_iterator& node) const NOTHROW
				{ return const_node_iterator(node ? node->parent_ : NULL); }

		const_node_iterator left_node
			(const const_node_iterator& node) const NOTHROW
				{ return const_node_iterator(node ? node->left_.get() : NULL); }

		const_node_iterator right_node
			(const const_node_iterator& node) const NOTHROW
				{ return const_node_iterator(node ? node->right_.get() : NULL); }
		//@}


		/**
		 * Determine overall hierarchy height, 0 for empty scenes,
		 * and 1 for scenes with exactly one (root) object.
		 * @attent Height is not cached and expensive to find.
		 */
		int64 find_height() const NOTHROW
			{ return find_height(root_node_.get(), 0); }


		/** Access to the encapsulated object list. */
		//@{
		object_iterator first_object() NOTHROW 
			{ return objects_.first(); }
		object_iterator last_object() NOTHROW 
			{ return objects_.last(); }

		const_object_iterator first_object() const NOTHROW 
			{ return objects_.first(); }
		const_object_iterator last_object() const NOTHROW 
			{ return objects_.last(); }
		//@}


		/** Clear the entire bounding scene. */
		void clear() NOTHROW
		{
			objects_.clear();
			object_to_node_.clear();
			root_node_.reset();
		}


		/**
		 * Validate scene structure for testing purposes.
		 * Clients never have to call this method.
		 */
		void validate() const
		{
			// Validate object structures:
			// Each object lookup must reference the respective node
			// within the bounding volume hierarchy.
			if (objects_.size() != object_to_node_.size())
			{
				bounding_scene_util::log_validation_failed
					("Mismatch between object count and object node lookup.");

				throw validation_failed();
			}

			for (const_object_iterator it(objects_.first()); it; ++it)
			{
				object_to_node_map::const_iterator find
					(object_to_node_.find(it));

				if (!find || !find->value || find->value->object_ != it)
				{
					bounding_scene_util::log_validation_failed
						("Object not found in object to node lookup.");

					throw validation_failed();
				}
			}


			// Validate node hierarchy.
			int64 count_nodes = 0;
			int64 count_leaves = 0;
			int64 height = 0;
			validate(root_node_.get(), NULL,
					 count_nodes, count_leaves, height);


			// Check counters against expected values.
			if (count_leaves == 0 && count_nodes != 0 ||
				count_leaves != 0 && count_nodes != count_leaves * 2 - 1 ||
				count_nodes != node_count())
			{
				bounding_scene_util::log_validation_failed("Inconsistent node and leaf count.");
				throw validation_failed();
			}

			if (count_leaves != objects_.size() ||
				count_leaves != object_count())
			{
				bounding_scene_util::log_validation_failed("Inconsistent object and leaf count.");
				throw validation_failed();
			}

			if (height != find_height())
			{
				bounding_scene_util::log_validation_failed("Inconsistent tree height.");
				throw validation_failed();
			}
		}


	private:


		void update_hierarchy_rotate_right
			(node_type *update_node) NOTHROW
		{
			/**
			 * Perform a right rotation to fix imbalance
			 * of a not-@NULL @bref{update_node}.
			 */

			auto_pointer<node_type>& pivot_spot =
				(!update_node->parent_) ? root_node_ :
				(update_node->parent_->left_.get() == update_node) ?
					update_node->parent_->left_ :
					update_node->parent_->right_;


			// Detach update node, then perform rotation:
			// Adjust hierarchy structure and balance factors.
			// In particular, raise the larger of both subtrees,
			// which works due to order-independent children.
			auto_pointer<node_type> detached_update_node
				(std::move(pivot_spot));

			pivot_spot =
				std::move(detached_update_node->left_);

			if (pivot_spot->balance_ == -1)
			{
				detached_update_node->left_ = std::move(pivot_spot->right_);
				pivot_spot->right_ = std::move(detached_update_node);

				pivot_spot->balance_ = 0;
				update_node->balance_ = 0;
			}
			else if (pivot_spot->balance_ == 0)
			{
				detached_update_node->left_ = std::move(pivot_spot->right_);
				pivot_spot->right_ = std::move(detached_update_node);

				pivot_spot->balance_ = 1;
				update_node->balance_ = -1;
			}
			else if (pivot_spot->balance_ == 1)
			{
				detached_update_node->left_ = std::move(pivot_spot->left_);
				pivot_spot->left_ = std::move(detached_update_node);

				pivot_spot->balance_ = 0;
				update_node->balance_ = 0;
			}	


			// Update parent pointers.
			pivot_spot->parent_ = update_node->parent_;
			update_node->parent_ = pivot_spot.get();
			update_node->left_->parent_ = update_node;
		}


		void update_hierarchy_rotate_left
			(node_type *update_node) NOTHROW
		{
			/**
			 * Perform a left rotation to fix imbalance
			 * of a not-@NULL @bref{update_node}.
			 */

			auto_pointer<node_type>& pivot_spot =
				(!update_node->parent_) ? root_node_ :
				(update_node->parent_->left_.get() == update_node) ?
					update_node->parent_->left_ :
					update_node->parent_->right_;


			// Detach update node, then perform rotation:
			// Adjust hierarchy structure and balance factors.
			// In particular, raise the larger of both subtrees,
			// which works due to order-independent children.
			auto_pointer<node_type> detached_update_node
				(std::move(pivot_spot));

			pivot_spot =
				std::move(detached_update_node->right_);

			if (pivot_spot->balance_ == -1)
			{
				detached_update_node->right_ = std::move(pivot_spot->right_);
				pivot_spot->right_ = std::move(detached_update_node);

				pivot_spot->balance_ = 0;
				update_node->balance_ = 0;
			}
			else if (pivot_spot->balance_ == 0)
			{
				detached_update_node->right_ = std::move(pivot_spot->right_);
				pivot_spot->right_ = std::move(detached_update_node);

				pivot_spot->balance_ = -1;
				update_node->balance_ = 1;
			}
			else if (pivot_spot->balance_ == 1)
			{
				detached_update_node->right_ = std::move(pivot_spot->left_);
				pivot_spot->left_ = std::move(detached_update_node);

				pivot_spot->balance_ = 0;
				update_node->balance_ = 0;
			}	


			// Update parent pointers.
			pivot_spot->parent_ = update_node->parent_;
			update_node->parent_ = pivot_spot.get();
			update_node->right_->parent_ = update_node;
		}


		void update_hierarchy
			(node_type* origin_node, bool remove) NOTHROW
		{
			/**
			 * Rebalance the hierarchy and rebuild bounding shapes
			 * because a leaf was either @bref{remove}d or inserted
			 * directly below a not-@NULL @bref{origin_node}.
			 */

			char origin_height_dif =
				remove ? -1 : 1;

			while (true)
			{
				node_type* update_node = origin_node->parent_;
				if (!update_node) break;


				// Possibly update balance factor.
				// Note we cannot exit on a zero height difference,
				// as we still need to update bounding shapes.
				update_node->balance_ +=
					(origin_node == update_node->left_.get()) ?
						-origin_height_dif : origin_height_dif;


				// Possibly rebalance through AVL-like rotation.
				// Note we need not maintain left_/right_ order,
				// so AVL double rotations are not necessary.
				if (update_node->balance_ == -2 ||
					update_node->balance_ == 2)
				{
					if (update_node->balance_ == -2)
						update_hierarchy_rotate_right(update_node);
					else
						update_hierarchy_rotate_left(update_node);


					// Update shapes on both rotated nodes.
					update_node->shape_ =
						builder_.combine_shapes
							(update_node->left_->shape_,
							 update_node->right_->shape_);	

					update_node->parent_->shape_ =
						builder_.combine_shapes
							(update_node->parent_->left_->shape_,
							 update_node->parent_->right_->shape_);	


					// Since the parent balance was updated as well,
					// skip both rotated nodes in future iterations.
					origin_node = update_node->parent_;
				}


				// No rotation was necessary, so just update shapes
				// and continue on next hierarchy node.
				else
				{
					update_node->shape_ =
						builder_.combine_shapes
							(update_node->left_->shape_,
							 update_node->right_->shape_);	

					origin_node = update_node;
				}


				// If reducing the height of a subtree causes balance,
				// then the superordinate tree shrinks in height as well.
				// Likewise, the superordinate tree increases in height
				// if a height increase in a subtree leads to imbalance.
				// This also holds true for the pivot node of a rotation,
				// albeit with a slightly more intricate rationale.
				origin_height_dif =
					(origin_node->balance_ == 0 && origin_height_dif == -1) ? -1 :
					(origin_node->balance_ != 0 && origin_height_dif == 1) ? 1 : 0;
			}
		}


		void insert_leaf_node
			(auto_pointer<node_type> leaf_node,
			 auto_pointer<node_type> parent_node) NOTHROW
		{
			/**
			 * Insert a not-@NULL @bref{leaf_node} into the hierarchy.
			 *
			 * If the hierarchy is not empty, the call also requires
			 * a not-@NULL @bref{parent_node} for the leaf node.
			 */


			// Insert root node on empty hierarchies.
			if (!root_node_)
			{
				root_node_ = std::move(leaf_node);
				root_node_->parent_ = NULL;
				return;
			}


			// Find an insertion spot below which
			// to insert the incoming parent node.
			auto_pointer<node_type>* insert_spot = &root_node_;
			while ((*insert_spot)->left_)
			{
				float score =
					builder_.score_insert
						(leaf_node->shape_,
						 (*insert_spot)->left_->shape_,
						 (*insert_spot)->right_->shape_);

				insert_spot =
					(score < 0) ?
						&(*insert_spot)->left_ :
						&(*insert_spot)->right_;
			}


			// Patch in the incoming nodes. 
			parent_node->parent_ = (*insert_spot)->parent_;
			parent_node->left_ = std::move(*insert_spot);
			parent_node->left_->parent_ = parent_node.get();
			parent_node->right_ = std::move(leaf_node);
			parent_node->right_->parent_ = parent_node.get();
			*insert_spot = std::move(parent_node);

			
			// Rebuild parent shape from leaf shapes.
			(*insert_spot)->shape_ =
				builder_.combine_shapes
					((*insert_spot)->left_->shape_,
					 (*insert_spot)->right_->shape_);


			// Reset balance factor of parent node,
			// which may be non-zero on re-insert.
			// Leaf nodes always have zero balance
			// and need not be reset here.
			(*insert_spot)->balance_ = 0;


			// Finally update remaining hierarchy,
			// including shapes and balance.
			update_hierarchy
				(insert_spot->get(), false);
		}


		void detach_leaf_node
			(node_type* leaf_node_to_detach,
			 auto_pointer<node_type>& detached_leaf_node,
			 auto_pointer<node_type>& detached_parent_node) NOTHROW
		{
			/**
			 * Detach a not-@NULL leaf node and its parent from the scene,
			 * then hand back ownership over both nodes.
			 *
			 * The parent node may be @NULL (i.e. if the leaf node is root),
			 * in which case @bref{detached_parent_node} becomes @NULL as well.
			 */


			node_type* parent_node_to_detach =
				leaf_node_to_detach->parent_;


			// Early out when detaching the root node.
			if (!parent_node_to_detach)			
			{
				detached_leaf_node = std::move(root_node_);
				detached_parent_node = auto_pointer<node_type>();
				return;
			}


			// Detach leaf node from its parent.
			detached_leaf_node =
				(parent_node_to_detach->left_.get() == leaf_node_to_detach) ?
					std::move(parent_node_to_detach->left_) :
					std::move(parent_node_to_detach->right_);


			// Detach parent and replace with its remaining child.
			// This works because all interior nodes have two children.
			auto_pointer<node_type>& detach_spot =
				(!parent_node_to_detach->parent_) ? root_node_ :
				(parent_node_to_detach->parent_->left_.get() == parent_node_to_detach) ?
					parent_node_to_detach->parent_->left_ :
					parent_node_to_detach->parent_->right_;

			detached_parent_node = std::move(detach_spot);
			detach_spot =
				detached_parent_node->left_ ?
					std::move(detached_parent_node->left_) :
					std::move(detached_parent_node->right_);


			// Update parent back-pointers.
			detach_spot->parent_ = detached_parent_node->parent_;
			detached_leaf_node->parent_ = NULL;
			detached_parent_node->parent_ = NULL;


			// Finally, update remaining hierarchy,
			// including shapes and balance.
			update_hierarchy
				(detach_spot.get(), true);
		}


		int64 find_height
			(const node_type* node, int64 parent_height) const NOTHROW
		{
			if (!node) return parent_height;

			return max(find_height(node->left_.get(), parent_height + 1),
					   find_height(node->right_.get(), parent_height + 1));
		}


		void validate
			(const node_type* node,
			 const node_type* parent_node,
			 int64& count_nodes, int64& count_leaves,
			 int64& height) const
		{
			if (!node) return;

			if (node->parent_ != parent_node)
			{
				bounding_scene_util::log_validation_failed("Inconsistent parent pointers.");
				throw validation_failed();
			}

			if ((node->left_ && !node->right_) ||
				(!node->left_ && node->right_))
			{
				bounding_scene_util::log_validation_failed("Node has but one child.");
				throw validation_failed();
			}

			if ((node->left_ && node->object_) ||
				(!node->left_ && !node->object_))
			{
				bounding_scene_util::log_validation_failed
					("Inner node with object or outer node without object.");

				throw validation_failed();
			}

			int64 height_left = 0;
			validate(node->left_.get(), node,
					 count_nodes, count_leaves,
					 height_left);

			int64 height_right = 0;
			validate(node->right_.get(), node,
					 count_nodes, count_leaves,
					 height_right);

			if (node->balance_ != height_right - height_left ||
				node->balance_ < -1 || node->balance_ > 1)
			{
				bounding_scene_util::log_validation_failed("Invalid node balance.");
				throw validation_failed();
			}

			++count_nodes;
			if (!node->left_) ++count_leaves;
			height = max(height_left, height_right) + 1;
		}


		B& builder_;

		list<O> objects_;

		typedef tree_map<const_object_iterator, node_type*> object_to_node_map;
		object_to_node_map object_to_node_;

		auto_pointer<node_type> root_node_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__BOUNDING_SCENE_HPP) */