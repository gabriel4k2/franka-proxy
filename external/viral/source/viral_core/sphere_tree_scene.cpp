/**
 *************************************************************************
 *
 * @file sphere_tree_scene.cpp
 *
 * Bounding scenes over implicit sphere trees, implementation
 *
 ************************************************************************/


#include "sphere_tree_scene.hpp"


#include "geo_volume.hpp"
#include "bounding_implicit.hpp"
#include "bounding_combiner.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_object
//
//////////////////////////////////////////////////////////////////////////


const shared_instance<::viral_core::sphere_tree>&
	sphere_tree_scene_object::sphere_tree() const NOTHROW
		{ return sphere_tree_; }

const si_transform&
	sphere_tree_scene_object::transform() const NOTHROW
		{ return transform_; }
const matrix&
	sphere_tree_scene_object::to_world_matrix() const NOTHROW
		{ return to_world_matrix_; }


sphere_tree_scene_object::sphere_tree_scene_object
	(const shared_instance<::viral_core::sphere_tree>& sphere_tree,
	 const si_transform& transform)
	:
	sphere_tree_(sphere_tree),
	transform_(transform),
	to_world_matrix_(transform.build_local_to_world())
{ }


void sphere_tree_scene_object::update_transform
	(const si_transform& transform) NOTHROW
{
	transform_ = transform;
	to_world_matrix_ = transform.build_local_to_world();
}




//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene_builder
//
//////////////////////////////////////////////////////////////////////////


sphere sphere_tree_scene_builder::bounding_shape
	(const sphere_tree_scene_object& object) const NOTHROW
{
	if (!object.sphere_tree()().root_node())
		return sphere(object.transform_.position, 0.f);

	const sphere& bounding_sphere =
		object.sphere_tree()().shape
			(object.sphere_tree()().root_node());

	sphere transformed_bounding_sphere
		(bounding_sphere.transform_and_bound
			(object.transform_));

	return transformed_bounding_sphere;
}


sphere sphere_tree_scene_builder::combine_shapes
	(const sphere& sa, const sphere& sb) const NOTHROW
{
	sphere result(sa);
	result.enclose(sb);
	return result;
}


float sphere_tree_scene_builder::score_insert
	(const sphere& shape,
	 const sphere& left_shape,
	 const sphere& right_shape) const NOTHROW
{
	sphere left_shape_new(left_shape);
	left_shape_new.enclose(shape);
	float volume_increase_left =
		left_shape_new.volume() - left_shape.volume();

	sphere right_shape_new(right_shape);
	right_shape_new.enclose(shape);
	float volume_increase_right =
		right_shape_new.volume() - right_shape.volume();

	// Greedy choice:
	// We want to minimize the volume of all bounding shapes,
	// so pick the side with less increase in shape volume.
	return -(volume_increase_right - volume_increase_left);
}




//////////////////////////////////////////////////////////////////////////
//
// sphere_tree_scene
//
//////////////////////////////////////////////////////////////////////////


sphere_tree_scene::sphere_tree_scene()
	:
	trees_bounding_(builder_)
{ }


sphere_tree_scene::object_iterator sphere_tree_scene::insert
	(const shared_instance<::viral_core::sphere_tree>& sphere_tree,
	 const si_transform& transform)
{
	sphere_tree_scene_object object(sphere_tree, transform);
	return trees_bounding_.insert(object);
}


void sphere_tree_scene::update
	(const object_iterator& it,
	 const si_transform& transform) NOTHROW
{
	it->update_transform(transform);
	trees_bounding_.update(it);
}


void sphere_tree_scene::remove
	(const object_iterator& it) NOTHROW
		{ trees_bounding_.remove(it); }


sphere_tree_scene::const_node_iterator
	sphere_tree_scene::root_node() const NOTHROW
		{ return trees_bounding_.root_node(); }

sphere_tree_scene::const_node_iterator
	sphere_tree_scene::parent_node
		(const const_node_iterator& node) const NOTHROW
			{ return trees_bounding_.parent_node(node); }
sphere_tree_scene::const_node_iterator
	sphere_tree_scene::left_node
		(const const_node_iterator& node) const NOTHROW
			{ return trees_bounding_.left_node(node); }
sphere_tree_scene::const_node_iterator
	sphere_tree_scene::right_node
		(const const_node_iterator& node) const NOTHROW
			{ return trees_bounding_.right_node(node); }


const sphere_tree& sphere_tree_scene::detail_hierarchy
	(const sphere_tree_scene_spheres_const_node_iterator& node) const NOTHROW
		{ return node.coarse_node()->object()->sphere_tree()(); }
 

sphere_tree_scene::object_iterator
	sphere_tree_scene::first_object() NOTHROW
		{ return trees_bounding_.first_object(); }
sphere_tree_scene::object_iterator
	sphere_tree_scene::last_object() NOTHROW
		{ return trees_bounding_.last_object(); }

sphere_tree_scene::const_object_iterator
	sphere_tree_scene::first_object() const NOTHROW
		{ return trees_bounding_.first_object(); }
sphere_tree_scene::const_object_iterator
	sphere_tree_scene::last_object() const NOTHROW
		{ return trees_bounding_.last_object(); }


void sphere_tree_scene::clear() NOTHROW
	{ trees_bounding_.clear(); }


const sphere_tree_scene_trees_bounding&
	sphere_tree_scene::trees_bounding() const NOTHROW
		{ return trees_bounding_; }




} /* namespace viral_core */
