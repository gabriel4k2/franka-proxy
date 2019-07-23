/**
 *************************************************************************
 *
 * @file mesh_scene_triangles.cpp
 *
 * Triangle-level wrapper for mesh scenes, implementation.
 *
 ************************************************************************/


#include "mesh_scene_triangles.hpp"


#include "log.hpp"
#include "mesh_scene.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_triangles_iterator
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_triangles_iterator::mesh_scene_triangles_iterator() NOTHROW { }


mesh_scene_triangles_iterator::mesh_scene_triangles_iterator
	(const mesh_scene_triangles_iterator& other) NOTHROW
	:
	meshes_bounding_node_(other.meshes_bounding_node_),
	mesh_bounding_node_(other.mesh_bounding_node_)
{ }


mesh_scene_triangles_iterator&
	mesh_scene_triangles_iterator::operator =
		(const mesh_scene_triangles_iterator& other) NOTHROW
{
	meshes_bounding_node_ = other.meshes_bounding_node_;
	mesh_bounding_node_ = other.mesh_bounding_node_;
	return *this;
}


mesh_scene_triangles_iterator::operator bool_conversion_type() const NOTHROW
{
	// The mesh_bounding_node_ may be past-end
	// on a valid mesh_scene_triangles_iterator.
	return meshes_bounding_node_ ?
				&mesh_scene_triangles_iterator::bool_conversion_is_true :
				NULL;
}


void mesh_scene_triangles_iterator::swap_with
	(mesh_scene_triangles_iterator& other) NOTHROW
{
	swap(meshes_bounding_node_, other.meshes_bounding_node_);
	swap(mesh_bounding_node_, other.mesh_bounding_node_);
}


bool mesh_scene_triangles_iterator::operator ==
	(const mesh_scene_triangles_iterator& other) const NOTHROW
{
	return
		meshes_bounding_node_ == other.meshes_bounding_node_ &&
		mesh_bounding_node_ == other.mesh_bounding_node_;
}


bool mesh_scene_triangles_iterator::operator !=
	(const mesh_scene_triangles_iterator& other) const NOTHROW
{
	return
		meshes_bounding_node_ != other.meshes_bounding_node_ ||
		mesh_bounding_node_ != other.mesh_bounding_node_;
}


const mesh_scene_triangles_iterator::meshes_bounding_const_node_iterator&
	mesh_scene_triangles_iterator::meshes_bounding_node() const NOTHROW
		{ return meshes_bounding_node_; }
const mesh_scene_triangles_iterator::mesh_bounding_const_node_iterator&
	mesh_scene_triangles_iterator::mesh_bounding_node() const NOTHROW
		{ return mesh_bounding_node_; }


mesh_scene_triangles_iterator::mesh_scene_triangles_iterator
	(const meshes_bounding_const_node_iterator& meshes_bounding_node,
	 const mesh_bounding_const_node_iterator& mesh_bounding_node) NOTHROW
	:
	meshes_bounding_node_(meshes_bounding_node),
	mesh_bounding_node_(mesh_bounding_node)
{ }




//////////////////////////////////////////////////////////////////////////
//
// mesh_scene_triangles
//
//////////////////////////////////////////////////////////////////////////


mesh_scene_triangles::mesh_scene_triangles
	(const mesh_scene& wrapped_scene) NOTHROW
	:
	wrapped_scene(wrapped_scene)
{ }


mesh_scene_triangles_iterator
	mesh_scene_triangles::root_node() const NOTHROW
{
	return
		mesh_scene_triangles_iterator
			(wrapped_scene.root_node(),
			 mesh_scene_mesh_bounding::const_node_iterator());
}


mesh_scene_triangles_iterator mesh_scene_triangles::parent_node
	(const mesh_scene_triangles_iterator& node) const NOTHROW
{
	if (!node.meshes_bounding_node())
		return mesh_scene_triangles_iterator();


	// If the iterator points to a bounding node in a mesh instance,
	// ascend to the next higher bounding node (potentially invalid)
	// through the bounding hierarchy of that mesh.
	if (node.mesh_bounding_node())
	{
		mesh_scene::object_iterator object
			(node.meshes_bounding_node()->object());

		if (!object)
		{
			LOG_ERROR("Found broken mesh_scene leaf without a geometry object.");
			return mesh_scene_triangles_iterator(); // NOTHROW
		}

		const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding =
			object->mesh_bounding();

		return
			mesh_scene_triangles_iterator
				(node.meshes_bounding_node(),
				 mesh_bounding().parent_node(node.mesh_bounding_node()));
	}


	// The iterator points to a high-level bounding node in the mesh scene,
	// therefore ascend to the next higher bounding node in the scene.
	return
		mesh_scene_triangles_iterator
			(wrapped_scene.parent_node(node.meshes_bounding_node()),
			 mesh_scene_mesh_bounding::const_node_iterator());
}


mesh_scene_triangles_iterator mesh_scene_triangles::left_node
	(const mesh_scene_triangles_iterator& node) const NOTHROW
{
	if (!node.meshes_bounding_node())
		return mesh_scene_triangles_iterator();


	// If the iterator points to a bounding node in a mesh instance,
	// descend to the next lower, left-hand bounding node in the mesh.
	// If this node is invalid (e.g. at an outer node, possibly a leaf),
	// then return a fully invalid iterator to disable further descend.
	if (node.mesh_bounding_node())
	{
		mesh_scene::object_iterator object
			(node.meshes_bounding_node()->object());

		if (!object)
		{
			LOG_ERROR("Found broken mesh_scene leaf without a geometry object.");
			return mesh_scene_triangles_iterator(); // NOTHROW
		}

		const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding =
			object->mesh_bounding();

		mesh_scene_mesh_bounding::const_node_iterator mesh_bounding_left_node
			(mesh_bounding().left_node(node.mesh_bounding_node()));

		if (!mesh_bounding_left_node)
			return mesh_scene_triangles_iterator();

		return
			mesh_scene_triangles_iterator
				(node.meshes_bounding_node(),
				 mesh_bounding_left_node);	
	}

	
	// We are not inside a mesh yet.
	// If we are at a leaf in the mesh_scene hierarchy,
	// continue on the topmost node of the mesh hierarchy.
	// Otherwise, continue to left node in mesh_scene.
	mesh_scene_meshes_bounding::const_node_iterator meshes_bounding_left_node
		(wrapped_scene.left_node(node.meshes_bounding_node()));	

	if (!meshes_bounding_left_node)
	{
		mesh_scene::object_iterator object
			(node.meshes_bounding_node()->object());

		if (!object)
		{
			LOG_ERROR("Found broken mesh_scene leaf without a geometry object.");
			return mesh_scene_triangles_iterator(); // NOTHROW
		}

		const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding =
			object->mesh_bounding();
	
		return mesh_scene_triangles_iterator
					(node.meshes_bounding_node(),
					 mesh_bounding().root_node());
	}

	return mesh_scene_triangles_iterator
				(meshes_bounding_left_node,
				 mesh_scene_mesh_bounding::const_node_iterator());
}


mesh_scene_triangles_iterator mesh_scene_triangles::right_node
	(const mesh_scene_triangles_iterator& node) const NOTHROW
{
	if (!node.meshes_bounding_node())
		return mesh_scene_triangles_iterator();


	// If the iterator points to a bounding node in a mesh instance,
	// descend to the next lower, right-hand bounding node in the mesh.
	// If this node is invalid (e.g. at an outer node, possibly a leaf),
	// then return a fully invalid iterator to disable further descend.
	if (node.mesh_bounding_node())
	{
		mesh_scene::object_iterator object
			(node.meshes_bounding_node()->object());

		if (!object)
		{
			LOG_ERROR("Found broken mesh_scene leaf without a geometry object.");
			return mesh_scene_triangles_iterator(); // NOTHROW.
		}

		const shared_instance<mesh_scene_mesh_bounding>& mesh_bounding =
			object->mesh_bounding();

		mesh_scene_mesh_bounding::const_node_iterator mesh_bounding_right_node
			(mesh_bounding().right_node(node.mesh_bounding_node()));

		if (!mesh_bounding_right_node)
			return mesh_scene_triangles_iterator();

		return
			mesh_scene_triangles_iterator
				(node.meshes_bounding_node(),
				 mesh_bounding_right_node);	
	}


	// We are not inside a mesh yet.
	// Do not pass from leaves in the mesh_scene hierarchy to meshes,
	// because mesh_scene_triangles::left_node() already does this.
	// However, do pass to right-hand side boundings up to the meshes.
	mesh_scene_meshes_bounding::const_node_iterator meshes_bounding_right_node
		(wrapped_scene.right_node(node.meshes_bounding_node()));	

	if (!meshes_bounding_right_node)
		return mesh_scene_triangles_iterator();

	return mesh_scene_triangles_iterator
				(meshes_bounding_right_node,
				 mesh_scene_mesh_bounding::const_node_iterator());
}
 



} /* namespace viral_core */
