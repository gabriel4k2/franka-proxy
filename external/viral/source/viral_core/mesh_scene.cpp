/**
 *************************************************************************
 *
 * @file mesh_scene.cpp
 *
 * Mesh-based bounding volume hierarchy, implementation.
 *
 ************************************************************************/


#include "mesh_scene.hpp"


#include "mesh.hpp"
#include "bounding_tree.hpp"
#include "bounding_combiner.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_object
//
//////////////////////////////////////////////////////////////////////////


const shared_instance<::viral_core::mesh>&
	mesh_scene_object::mesh() const NOTHROW
		{ return mesh_; }
const shared_instance<mesh_scene_mesh_bounding>&
	mesh_scene_object::mesh_bounding() const NOTHROW
		{ return mesh_bounding_; }

const si_transform& mesh_scene_object::transform() const NOTHROW
	{ return transform_; }
const matrix& mesh_scene_object::to_world_matrix() const NOTHROW
	{ return to_world_matrix_; }


mesh_scene_object::mesh_scene_object
	(const shared_instance<::viral_core::mesh>& mesh,
	 const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding,
	 const si_transform& transform)
	:
	mesh_(mesh),
	mesh_bounding_(mesh_bounding),
	transform_(transform),
	to_world_matrix_(transform.build_local_to_world())
{ }


void mesh_scene_object::update_transform
	(const si_transform& transform) NOTHROW
{
	transform_ = transform;
	to_world_matrix_ = transform.build_local_to_world();
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_builder
//
//////////////////////////////////////////////////////////////////////////


box mesh_scene_builder::bounding_shape
	(const mesh_scene_object& object) const NOTHROW
{
	const box& bounding_box =
		object.mesh_().bounding_box();

	box transformed_bounding_box
		(bounding_box.transform_and_bound(object.transform_));

	return transformed_bounding_box;
}


box mesh_scene_builder::combine_shapes
	(const box& sa, const box& sb) const NOTHROW
{
	box result(sa);
	result.grow_to(sb);
	return result;
}


float mesh_scene_builder::score_insert
	(const box& shape,
	 const box& left_shape,
	 const box& right_shape) const NOTHROW
{
	box left_shape_new(left_shape);
	left_shape_new.grow_to(shape);
	float volume_increase_left =
		left_shape_new.volume() - left_shape.volume();

	box right_shape_new(right_shape);
	right_shape_new.grow_to(shape);
	float volume_increase_right =
		right_shape_new.volume() - right_shape.volume();

	// Greedy choice:
	// We want to minimize the volume of all bounding shapes,
	// so pick the side with less increase in shape volume.
	return -(volume_increase_right - volume_increase_left);
}




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene
//
//////////////////////////////////////////////////////////////////////////


mesh_scene::mesh_scene()
	:
	meshes_bounding_(builder_)
{ }


mesh_scene::object_iterator mesh_scene::insert
	(const shared_instance<::viral_core::mesh>& mesh,
	 const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding,
	 const si_transform& transform)
{
	mesh_scene_object object(mesh, mesh_bounding, transform);
	return meshes_bounding_.insert(object);
}


void mesh_scene::update
	(const object_iterator& it,
	 const si_transform& transform) NOTHROW
{
	it->update_transform(transform);
	meshes_bounding_.update(it);
}


void mesh_scene::remove
	(const object_iterator& it) NOTHROW
		{ meshes_bounding_.remove(it); }


mesh_scene::const_node_iterator
	mesh_scene::root_node() const NOTHROW
		{ return meshes_bounding_.root_node(); }

mesh_scene::const_node_iterator mesh_scene::parent_node
	(const const_node_iterator& node) const NOTHROW
		{ return meshes_bounding_.parent_node(node); }
mesh_scene::const_node_iterator mesh_scene::left_node
	(const const_node_iterator& node) const NOTHROW
		{ return meshes_bounding_.left_node(node); }
mesh_scene::const_node_iterator mesh_scene::right_node
	(const const_node_iterator& node) const NOTHROW
		{ return meshes_bounding_.right_node(node); }


const mesh_scene_mesh_bounding& mesh_scene::detail_hierarchy
	(const mesh_scene_triangles_const_node_iterator& node) const NOTHROW
		{ return node.coarse_node()->object()->mesh_bounding()(); }


mesh_scene::object_iterator mesh_scene::first_object() NOTHROW
	{ return meshes_bounding_.first_object(); }
mesh_scene::object_iterator mesh_scene::last_object() NOTHROW
	{ return meshes_bounding_.last_object(); }

mesh_scene::const_object_iterator mesh_scene::first_object() const NOTHROW
	{ return meshes_bounding_.first_object(); }
mesh_scene::const_object_iterator mesh_scene::last_object() const NOTHROW
	{ return meshes_bounding_.last_object(); }


void mesh_scene::clear() NOTHROW
	{ meshes_bounding_.clear(); }


const mesh_scene_meshes_bounding&
	mesh_scene::meshes_bounding() const NOTHROW
		{ return meshes_bounding_; }




} /* namespace viral_core */
