/**
 *************************************************************************
 *
 * @file bounding_builder.cpp
 *
 * Building of bounding volume hierarchies, implementation.
 *
 ************************************************************************/


#include "bounding_builder.hpp"


#include <algorithm>


#include "log.hpp"
#include "geo_util.hpp"
#include "geo_primitive.hpp"
#include "bounding_tree.hpp"
#include "bounding_implicit.hpp"
#include "mesh.hpp"
#include "mesh_tesselate.hpp"
#include "model.hpp"
#include "model_util.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class bounding_builder_span
 * @ingroup bounding_module
 *
 * A coherent span index interval [@bref{start}, @bref{end}]
 * that forms a node in the later bounding volume hierarchy.
 *
 * Usage notes:
 * - Due to resorting, span indices (e.g. @bref{start} and @bref{end})
 *		need not match with leaf shapes (e.g. @bref{leaf_shape_index}).
 *		See bounding_builder_split documentation for details.
 *
 ************************************************************************/
template <typename S>
	class bounding_builder_span
{
	friend class bounding_builder_split<S>;


	public:

		bounding_builder_span
			(int64 start, int64 end,
			 const S& shape,
			 int64 leaf_shape_index,
			 int64 parent)
			:
			start(start), end(end),
			shape(shape),
			leaf_shape_index(leaf_shape_index),
			parent(parent),
			left(invalid_index),
			right(invalid_index)
		{ }


		static const int64 invalid_index = -1;

		const int64 start;
		const int64 end;

		const S shape;
		const int64 leaf_shape_index;

		const int64 parent;

		// Modifiable for attaching children.
		int64 left;
		int64 right;
};




/**
 *************************************************************************
 *
 * @class bounding_builder_split
 * @ingroup bounding_module
 *
 * Utility class that builds a hierarchy of bounding_builder_span%s
 * over leaf shapes of type @bref{S} by top-down splitting.
 *
 * Algorithm overview:
 * - We work on shapes instead of geometry objects (e.g. triangles)
 *		because simpler shapes are less expensive in calculations
 *		and allow for a more generic bounding_builder_split.
 * - We start with a bounding_builder_span over all the leaf shapes,
 *		then we recursively split spans by choosing a distribution
 *		of span shapes over two children spans. In particular, we sort
 *		all span shapes into a linear array, then determine a splitter
 *		within the linear array. We apply several sorting strategies
 *		(e.g. by center coordinates) to increase splitter robustness,
 *		and we choose the sorting that gives the best splitter.
 * - To avoid perpetual resorting of leaf shapes, we instead sort on
 *		index maps spans_to_leaves_final_ and spans_to_leaves_sorted_
 *		that associate span indices with leaf shape indices.
 *
 ************************************************************************/
template <typename S>
	class bounding_builder_split
{

	public:

		typedef S shape_type;
		typedef bounding_builder_span<S> span_type;


		bounding_builder_split
			(const dynamic_buffer<shape_type>& leaf_shapes);

		const array<span_type>& spans() const NOTHROW;


	private:

		static void grow_bounding_shape
			(shape_type& shape, const shape_type& by);
		static bool compare_bounding_center
			(const shape_type& a, const shape_type& b, const vector& d);
		static float rank_splitter
			(const shape_type& left_shape, const shape_type& right_shape,
			 const shape_type& parent_shape);

		void init_spans();
		void build_spans();

		int64 find_splitter
			(const span_type& span,
			 shape_type& left_shape, shape_type& right_shape);
		int64 find_splitter_candidate
			(const span_type& span,
			 shape_type& left_shape, shape_type& right_shape,
			 float& rank);
		void resort_splitter_center
			(const span_type& span, const vector& d);


		static const int64 splitter_force_balance_count_;
		static const int64 splitter_force_balance_factor_;
		static const float splitter_balance_tolerance_constant_;
		static const float splitter_balance_tolerance_linear_;

		const dynamic_buffer<shape_type>& leaf_shapes_;
		array<span_type> spans_;

		dynamic_buffer<int64> spans_to_leaves_final_;
		dynamic_buffer<int64> spans_to_leaves_sorted_;

		dynamic_buffer<shape_type> shapes_left_;
		dynamic_buffer<shape_type> shapes_right_;
		dynamic_buffer<float> splitter_ranks_;
};


template <typename S>
	bounding_builder_split<S>::bounding_builder_split
		(const dynamic_buffer<shape_type>& leaf_shapes)
	:
	leaf_shapes_(leaf_shapes)
{
	init_spans();
	build_spans();
}


template <typename S>
	const array<bounding_builder_span<S>>&
		bounding_builder_split<S>::spans() const NOTHROW
			{ return spans_; }


template <typename S>
	void bounding_builder_split<S>::grow_bounding_shape
		(shape_type& shape, const shape_type& by)
			{ STATIC_ASSERT_USE_SPECIALIZATION_INSTEAD; }
template <typename S>
	bool bounding_builder_split<S>::compare_bounding_center
		(const shape_type& a, const shape_type& b, const vector& d)
			{ STATIC_ASSERT_USE_SPECIALIZATION_INSTEAD; }
template <typename S>
	float bounding_builder_split<S>::rank_splitter
		(const shape_type& left_shape, const shape_type& right_shape,
		 const shape_type& parent_shape)
			{ STATIC_ASSERT_USE_SPECIALIZATION_INSTEAD; }


template <>
	void bounding_builder_split<sphere>::grow_bounding_shape
		(sphere& shape, const sphere& by)
			{ shape.enclose(by); }
template <>
	void bounding_builder_split<box>::grow_bounding_shape
		(box& shape, const box& by)
			{ shape.grow_to(by); }


template <>
	bool bounding_builder_split<sphere>::compare_bounding_center
		(const sphere& a, const sphere& b, const vector& d)
			{ return a.position.dot(d) < b.position.dot(d); }
template <>
	bool bounding_builder_split<box>::compare_bounding_center
		(const box& a, const box& b, const vector& d)
			{ return (a.min + a.max).dot(d) < (b.min + b.max).dot(d); }


template <>
	float bounding_builder_split<sphere>::rank_splitter
		(const sphere& left_shape, const sphere& right_shape,
		 const sphere& parent_shape)
{
	/**
	 * Implementation notes:
	 * - Sphere pairs with lower volume are better and rank higher,
	 *		as this reduces coverage in respect to the parent sphere.
	 * - We must subtract any volume outside the parent sphere
	 *		so that we do not unnecessarily penalize border shapes.
	 */

	return -(left_shape.intersection_volume(parent_shape) +
			 right_shape.intersection_volume(parent_shape));
}


template <>
	float bounding_builder_split<box>::rank_splitter
		(const box& left_shape, const box& right_shape,
		 const box& parent_shape)
{
	/**
	 * Works like rank_splitter() for spheres, but uses boxes instead.
	 * Note that a parent box always completely contains its children.
	 */

	return -(left_shape.volume() + right_shape.volume());
}


template <typename S>
	void bounding_builder_split<S>::init_spans()
{
	/** Initialize temporary span data structures. */

	// If there is no span, then build_spans() will do nothing,
	// and we will return a hierarchy without nodes or objects.
	if (leaf_shapes_.empty()) return;


	// Allocate storage for an upper bound on required spans,
	// then initialize with a suitable root span.
	shape_type root_shape(leaf_shapes_[0]);
	for (int64 i = 1; i < leaf_shapes_.size(); ++i)
		grow_bounding_shape(root_shape, leaf_shapes_[i]);

	int64 root_leaf_shape_index =
		(leaf_shapes_.size() == 1) ?
			0 : span_type::invalid_index; // Root may be leaf.

	spans_.reserve(2 * leaf_shapes_.size());
	spans_.push_back
		(span_type
			(0, leaf_shapes_.size() - 1,
			 root_shape,
			 root_leaf_shape_index,					
			 span_type::invalid_index));


	// Preallocate final and temporary span-to-leaf maps,
	// and reserve storage for per-splitter shapes and ranks.
	spans_to_leaves_final_.resize(leaf_shapes_.size());
	spans_to_leaves_sorted_.resize(leaf_shapes_.size());
	for (int64 i = 0; i < leaf_shapes_.size(); ++i)
	{
		spans_to_leaves_final_[i] = i;
		spans_to_leaves_sorted_[i] = i;
	}

	shapes_left_.reserve(leaf_shapes_.size());
	shapes_right_.reserve(leaf_shapes_.size());
	splitter_ranks_.reserve(leaf_shapes_.size());
}


template <typename S>
	void bounding_builder_split<S>::build_spans()
{
	/**
	 * Iteratively build a hierarchy of spans,
	 * starting with the root span from init_spans().
	 */

	for (int64 i = 0; i < spans_.size(); ++i)
	{
		span_type& s = spans_[i];


		// If the span covers but a single shape,
		// we are finished here.
		if (s.start == s.end)
			continue;


		// Span has multiple shapes, so we must split.
		// First, choose a suitable splitter position.
		shape_type left_shape, right_shape;
		int64 left_splitter_end =
			find_splitter(s, left_shape, right_shape);
		int64 right_splitter_start =
			left_splitter_end + 1;


		// Fetch indices of original leaf shapes
		// (i.e. before resorting) for leaf spans.
		int64 left_leaf_shape_index =
			(left_splitter_end == s.start) ?
				spans_to_leaves_final_[s.start] :
				span_type::invalid_index;
		int64 right_leaf_shape_index =
			(s.end == right_splitter_start) ?
				spans_to_leaves_final_[s.end] :
				span_type::invalid_index;


		// Create children spans.
		spans_.push_back
			(span_type
				(s.start, left_splitter_end,
				 left_shape, left_leaf_shape_index, i));
		spans_.push_back
			(span_type
				(right_splitter_start, s.end,
				 right_shape, right_leaf_shape_index, i));


		// Attach children spans to parent.
		s.left = spans_.size() - 2;
		s.right = spans_.size() - 1;
	}
}


template <typename S>
	int64 bounding_builder_split<S>::find_splitter
		(const span_type& span,
		 shape_type& left_shape, shape_type& right_shape)
{
	/**
	 * Determine best splitter for the incoming @bref{span}.
	 * Returns the last valid element in the left child span
	 * as well as bounding shapes for both child spans.
	 */


	// Single-shape spans should be ignored in build_spans().
	if (span.end - span.start < 1)
	{
		LOG_ERROR("Cannot split up a single leaf shape any further.");
		throw assert_failed();
	}


	// Two-shape spans must always have two children
	// with a splitter in the middle.
	if (span.end - span.start == 1)
	{
		left_shape = leaf_shapes_[spans_to_leaves_final_[span.start]];
		right_shape = leaf_shapes_[spans_to_leaves_final_[span.end]];
		return span.start;
	}


	// Span covers more than two shapes,
	// so find a good splitter candidate.
	int64 best_splitter = -1;
	float best_rank = 0;

	static const int64 projection_dir_count = 7;
	vector projection_dirs[projection_dir_count] =
		{
			vector(1, 0, 0), vector(0, 1, 0), vector(0, 0, 1),
			vector(1, 1, 0), vector(1, 0, 1), vector(0, 1, 1), vector(1, 1, 1),
		};

	for (int64 dir = 0; dir < projection_dir_count; ++dir)
	{
		// Resort leaf shapes for current projection direction,
		// then determine a good splitter for this sorting.
		resort_splitter_center(span, projection_dirs[dir]);

		shape_type candidate_left_shape, candidate_right_shape;
		float candidate_rank;
		int64 candidate_splitter =
			find_splitter_candidate
				(span,
				 candidate_left_shape, candidate_right_shape,
				 candidate_rank);

		if (best_splitter != -1 && candidate_rank < best_rank)
			continue;


		// Found better splitter candidate, so apply for now.		
		best_splitter = candidate_splitter;
		best_rank = candidate_rank;
		left_shape = candidate_left_shape;
		right_shape = candidate_right_shape;

		for (int64 i = span.start; i <= span.end; ++i)
			spans_to_leaves_final_[i] = spans_to_leaves_sorted_[i];
	}


	// Copy sorting of best candidate back to
	// temporary sorting array, so that children
	// sort correct intervals later on.
	for (int64 i = span.start; i <= span.end; ++i)
		spans_to_leaves_sorted_[i] = spans_to_leaves_final_[i];


	// Finished, so return best splitter.
	// Higher-level build_spans() doesn't handle invalid results,
	// so check selected splitter here for safety.
	if (best_splitter == -1)
	{
		LOG_ERROR("Could not find any splitter position.");
		throw assert_failed();
	}

	return best_splitter;
}


template <typename S>
	int64 bounding_builder_split<S>::find_splitter_candidate
		(const span_type& span,
		 shape_type& left_shape, shape_type& right_shape,
		 float& rank)
{
	/**
	 * Within a pre-sorted @bref{spans_to_leaves_sorted_} map,
	 * find a promising splitter through ranking and balancing,
	 * and return associated left and right bounding shapes.
	 * The resulting splitter belongs to the left-hand span.
	 */

	int64 splitter_count = span.end - span.start;

	shapes_left_.resize(splitter_count); // Reserved memory in init_spans().
	shapes_right_.resize(splitter_count);
	splitter_ranks_.resize(splitter_count);


	// Build left and right bounding shapes
	// for each potential splitting point,
	// then rank all bounding shape pairs.
	shapes_left_[0] =
		leaf_shapes_[spans_to_leaves_sorted_[span.start]];
	shapes_right_[splitter_count - 1] =
		leaf_shapes_[spans_to_leaves_sorted_[span.end]];

	for (int64 i = 1; i < splitter_count; ++i)
	{
		shapes_left_[i] = shapes_left_[i - 1];
		grow_bounding_shape(shapes_left_[i],
							leaf_shapes_[spans_to_leaves_sorted_[span.start + i]]);

		shapes_right_[splitter_count - i - 1] = shapes_right_[splitter_count - i];
		grow_bounding_shape(shapes_right_[splitter_count - i - 1],
							leaf_shapes_[spans_to_leaves_sorted_[span.end - i]]);
	}

	for (int64 i = 0; i < splitter_count; ++i)
		splitter_ranks_[i] =
			rank_splitter(shapes_left_[i], shapes_right_[i], span.shape);
	

	// Locate best splitter based on ranking.
	// Only allow splitters near to center
	// on higher levels to improve balancing.
	int64 search_start =
		(splitter_count > splitter_force_balance_count_) ?
		(splitter_count / splitter_force_balance_factor_) : 0;
	int64 search_end =
		splitter_count - search_start;

	int64 best_splitter = search_start;
	for (int64 i = search_start; i < search_end; ++i)
		if (splitter_ranks_[i] > splitter_ranks_[best_splitter])
			best_splitter = i;


	// Allow a slightly worse splitter candidate
	// if it provides a better hierarchy balance.
	int64 balanced_splitter = best_splitter;
	int64 midpoint = splitter_count / 2;
	for (int64 i = search_start; i < search_end; ++i)
	{
		if (nearly_equal(splitter_ranks_[best_splitter], splitter_ranks_[i],
						 splitter_balance_tolerance_constant_,
						 splitter_balance_tolerance_linear_))
		{
			if (abs(i - midpoint) < abs(balanced_splitter - midpoint))
				balanced_splitter = i;
		}
	}


	// Return most promising splitter candidate.
	left_shape = shapes_left_[balanced_splitter];
	right_shape = shapes_right_[balanced_splitter];
	rank = splitter_ranks_[balanced_splitter];

	return span.start + balanced_splitter;
}


template <typename S>
	void bounding_builder_split<S>::resort_splitter_center
		(const span_type& span, const vector& d)
{
	/**
	 * Resort the temporary span-to-leaf map
	 * so that some @bref{s}pan enumerates its leaf shapes
	 * along a given @bref{d}irection vector.
	 */

	struct compare_coordinate
	{
		compare_coordinate
			(const vector& d, const dynamic_buffer<shape_type>& leaf_shapes) :
				d(d), leaf_shapes(leaf_shapes) { }

		bool operator ()(int64 a, int64 b)
			{ return compare_bounding_center(leaf_shapes[a], leaf_shapes[b], d); }

		const vector& d;
		const dynamic_buffer<shape_type>& leaf_shapes;
	};

	std::sort(spans_to_leaves_sorted_.data() + span.start,
			  spans_to_leaves_sorted_.data() + span.end + 1,
			  compare_coordinate(d, leaf_shapes_));			
}


template <typename S>
	const int64 bounding_builder_split<S>::splitter_force_balance_count_ = 8;
template <typename S>
	const int64 bounding_builder_split<S>::splitter_force_balance_factor_ = 6;
template <typename S>
	const float bounding_builder_split<S>::splitter_balance_tolerance_constant_ = 1e-5f;
template <typename S>
	const float bounding_builder_split<S>::splitter_balance_tolerance_linear_ = 0.1f;




//////////////////////////////////////////////////////////////////////////
//
// bounding_builder
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_index
	bounding_builder::build_sphere_tree_index(const mesh& msh)
{
	dynamic_buffer<sphere> leaf_shapes
		(build_leaf_shapes<sphere>(msh));
	
	bounding_builder_split<sphere> bounding_builder(leaf_shapes);
	array<bounding_tree_node<sphere>> nodes
		(build_bounding_tree_nodes<sphere, int64>
			(bounding_builder.spans()));

	array<int64> objects
		(build_objects_index(msh));

	return sphere_tree_index
				(std::move(nodes), std::move(objects));
}


sphere_tree_triangle
	bounding_builder::build_sphere_tree_triangle(const mesh& msh)
{
	dynamic_buffer<sphere> leaf_shapes
		(build_leaf_shapes<sphere>(msh));

	bounding_builder_split<sphere> bounding_builder(leaf_shapes);
	array<bounding_tree_node<sphere>> nodes
		(build_bounding_tree_nodes<sphere, triangle>
			(bounding_builder.spans()));

	array<triangle> objects
		(build_objects_triangle(msh));

	return sphere_tree_triangle
				(std::move(nodes), std::move(objects));
}


box_tree_index
	bounding_builder::build_box_tree_index(const mesh& msh)
{
	dynamic_buffer<box> leaf_shapes
		(build_leaf_shapes<box>(msh));

	bounding_builder_split<box> bounding_builder(leaf_shapes);
	array<bounding_tree_node<box>> nodes
		(build_bounding_tree_nodes<box, int64>
			(bounding_builder.spans()));

	array<int64> objects
		(build_objects_index(msh));

	return box_tree_index
				(std::move(nodes), std::move(objects));
}


box_tree_triangle
	bounding_builder::build_box_tree_triangle(const mesh& msh)
{
	dynamic_buffer<box> leaf_shapes
		(build_leaf_shapes<box>(msh));

	bounding_builder_split<box> bounding_builder(leaf_shapes);
	array<bounding_tree_node<box>> nodes
		(build_bounding_tree_nodes<box, triangle>
			(bounding_builder.spans()));

	array<triangle> objects
		(build_objects_triangle(msh));

	return box_tree_triangle
				(std::move(nodes), std::move(objects));
}


sphere_tree_triangle
	bounding_builder::build_sphere_tree_triangle(const model& mdl)
{
	return build_sphere_tree_triangle
				(model_util::join_model_groups(mdl));
}


box_tree_triangle
	bounding_builder::build_box_tree_triangle(const model& mdl)
{
	return build_box_tree_triangle
				(model_util::join_model_groups(mdl));
}


sphere_tree
	bounding_builder::build_sphere_tree_tesselated
		(const mesh& msh, float sphere_radius)
{
	dynamic_buffer<sphere> leaf_shapes
		(mesh_tesselate::tesselate_triangles_by_spheres
			(msh, sphere_radius));
	
	bounding_builder_split<sphere> bounding_builder(leaf_shapes);
	array<bounding_implicit_node<sphere>> nodes
		(build_bounding_implicit_nodes<sphere>
			(bounding_builder.spans()));

	return sphere_tree(std::move(nodes));
}


box_tree
	bounding_builder::build_box_tree_tesselated
		(const mesh& msh, float box_size)
{
	dynamic_buffer<box> leaf_shapes
		(mesh_tesselate::tesselate_triangles_by_boxes
			(msh, box_size));
	
	bounding_builder_split<box> bounding_builder(leaf_shapes);
	array<bounding_implicit_node<box>> nodes
		(build_bounding_implicit_nodes<box>
			(bounding_builder.spans()));

	return box_tree(std::move(nodes));
}


sphere_tree
	bounding_builder::build_sphere_tree_tesselated
		(const model& mdl, float sphere_radius)
{
	return build_sphere_tree_tesselated
				(model_util::join_model_groups(mdl),
				 sphere_radius);
}


box_tree
	bounding_builder::build_box_tree_tesselated
		(const model& mdl, float box_size)
{
	return build_box_tree_tesselated
				(model_util::join_model_groups(mdl),
				 box_size);
}


template <typename S>
	dynamic_buffer<S> bounding_builder::build_leaf_shapes
		(const mesh& msh)
{
	/**
	 * For each triangle in the input mesh,
	 * build a single bounding shape of type @bref{S}
	 * that becomes the shape of a later leaf node.
	 */

	const mesh_stream_vector& vertices = msh.vertices();
	const mesh_stream_triangle& triangles = msh.triangles();

	dynamic_buffer<S> result;
	result.reserve(triangles.size());

	for (int64 i = 0; i < triangles.size(); ++i)
	{
		triangle t
			(vertices[triangles[i].i],
			 vertices[triangles[i].j],
			 vertices[triangles[i].k]);

		result.push_back(to_leaf_shape<S>(t));
	}

	return std::move(result);
} 

template <>
	sphere bounding_builder::to_leaf_shape<sphere>
		(const triangle& t)
{
	sphere result(t.build_bounding_sphere());

	// Avoid degenerate bounding spheres for very small triangles.
	// Radius does not depend on position, so no linear epsilon.
	result.radius =
		max(result.radius, min_bounding_sphere_radius_);

	return result;
}

template <>
	box bounding_builder::to_leaf_shape<box>
		(const triangle& t)
{
	box result(t.build_bounding_box());

	// Avoid degenerate bounding boxes for sliver-like triangles.
	// We must choose epsilon as linear in regards to box position,
	// otherwise we suffer numerical issues for far-out boxes.
	for (int64 c = 0; c < 3; ++c)
	{
		float center = (result.max[c] + result.min[c]) * 0.5f;
		float linear_epsilon = fabsf(center) * min_bounding_box_size_;

		if (result.max[c] - result.min[c] < linear_epsilon)
		{
			result.min[c] = center - linear_epsilon;
			result.max[c] = center + linear_epsilon;
		}
	}

	return result;
}


template <typename S, typename O>
	static array<bounding_tree_node<S>>
		bounding_builder::build_bounding_tree_nodes
			(const array<bounding_builder_span<S>>& spans)
{
	/**
	 * Convert internal span representation
	 * to client-side bounding_tree_node%s.
	 */


	// Ensure we can pass on invalid indices
	// between different classes.
	if (bounding_tree<S, O>::invalid_index !=
		bounding_builder_span<S>::invalid_index)
	{
		LOG_ERROR("Different invalid index constant in builder and tree.");
		throw assert_failed();
	}


	// Create one result node for each span.
	// Indices of geometry objects (e.g. triangles)
	// match with indices of leaf bounding shapes.
	array<bounding_tree_node<S>> result;
	result.reserve(spans.size());

	for (int64 i = 0; i < spans.size(); ++i)
	{
		result.push_back
			(bounding_tree_node<S>
				(spans[i].shape,
				 spans[i].leaf_shape_index,
				 spans[i].parent,
				 spans[i].left,
				 spans[i].right));
	}

	return std::move(result);
}


template <typename S>
	static array<bounding_implicit_node<S>>
		bounding_builder::build_bounding_implicit_nodes
			(const array<bounding_builder_span<S>>& spans)
{
	/**
	 * Convert internal span representation
	 * to client-side bounding_implicit_node%s.
	 */


	// Ensure we can pass on invalid indices
	// between different classes.
	if (bounding_implicit<S>::invalid_index !=
		bounding_builder_span<S>::invalid_index)
	{
		LOG_ERROR("Different invalid index constant in builder and tree.");
		throw assert_failed();
	}


	// Create one result node for each span.
	// For implicit trees, we have no geometry objects,
	// so we need not pass on any leaf shape indices.
	array<bounding_implicit_node<S>> result;
	result.reserve(spans.size());

	for (int64 i = 0; i < spans.size(); ++i)
	{
		result.push_back
			(bounding_implicit_node<S>
				(spans[i].shape,
				 spans[i].parent,
				 spans[i].left,
				 spans[i].right));
	}

	return std::move(result);
}


array<int64>
	bounding_builder::build_objects_index(const mesh& msh)
{
	/** Build indices as geometry objects for bounding_tree. */

	array<int64> result;
	result.resize(msh.triangle_count());

	for (int64 i = 0; i < result.size(); ++i)
		result[i] = i;

	return std::move(result);
}


array<triangle>
	bounding_builder::build_objects_triangle(const mesh& msh)
{
	/** Build triangles as geometry objects for bounding_tree. */

	const mesh_stream_vector& vertices = msh.vertices();
	const mesh_stream_triangle& triangles = msh.triangles();

	array<triangle> result;
	result.reserve(triangles.size());

	for (int64 i = 0; i < triangles.size(); ++i)
		result.push_back
			(triangle(vertices[triangles[i].i],
					  vertices[triangles[i].j],
					  vertices[triangles[i].k]));

	return std::move(result);
}


const float bounding_builder::min_bounding_sphere_radius_ = 1e-5f;
const float bounding_builder::min_bounding_box_size_ = 1e-5f;




} /* namespace viral_core */
