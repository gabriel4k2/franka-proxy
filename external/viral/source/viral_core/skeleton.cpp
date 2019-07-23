/**
 *************************************************************************
 *
 * @file skeleton.cpp
 *
 * Low-level support for skeletal animation, implementation.
 *
 ************************************************************************/


#include "skeleton.hpp"


#include "log.hpp"
#include "integer_checks.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// skeleton_index
//
//////////////////////////////////////////////////////////////////////////


bool skeleton_index::operator ==
	(const skeleton_index& other) const NOTHROW
{
	return C[0] == other.C[0] &&
		   C[1] == other.C[1] &&
		   C[2] == other.C[2] &&
		   C[3] == other.C[3];
}


bool skeleton_index::operator !=
	(const skeleton_index& other) const NOTHROW
		{ return !(*this == other); }




//////////////////////////////////////////////////////////////////////////
//
// skeleton_weight
//
//////////////////////////////////////////////////////////////////////////


bool skeleton_weight::operator ==
	(const skeleton_weight& other) const NOTHROW
{
	return C[0] == other.C[0] &&
		   C[1] == other.C[1] &&
		   C[2] == other.C[2] &&
		   C[3] == other.C[3];
}


bool skeleton_weight::operator !=
	(const skeleton_weight& other) const NOTHROW
		{ return !(*this == other); }




//////////////////////////////////////////////////////////////////////////
//
// skeleton_joint
//
//////////////////////////////////////////////////////////////////////////


skeleton_joint::skeleton_joint
	(const string& name,
	 int64 parent, int64 first_child, int64 child_count,
	 const si_transform& joint_to_parent,
	 const si_transform& binding_to_joint)
	:
	name(name),
	parent(parent),
	first_child(first_child),
	child_count(child_count),

	joint_to_parent(joint_to_parent),
	joint_to_parent_matrix(joint_to_parent.build_local_to_world()),
	binding_to_joint(binding_to_joint),
	binding_to_joint_matrix(joint_to_parent.build_local_to_world())
{ }




//////////////////////////////////////////////////////////////////////////
//
// skeleton
//
//////////////////////////////////////////////////////////////////////////


skeleton::skeleton
	(const skeleton_joint_array& joints)
	:
	joints_(joints)
{
	init_joint_name_to_index();
	validate();
}


skeleton::skeleton
	(skeleton_joint_array&& joints)
	:
	joints_(std::move(joints))
{
	init_joint_name_to_index();
	validate();
}


const skeleton_joint_array& skeleton::joints() const NOTHROW
	{ return joints_; }


const skeleton_joint& skeleton::joint(int64 index) const
{
	if (index < 0 || index >= joints_.size())
	{
		LOG_ERROR("Joint index: " + index + " is invalid.");
		throw invalid_parameter();
	}

	return joints_[index];
}


const skeleton_joint& skeleton::joint(const string& name) const
{
	joint_name_to_index_map::const_iterator it
		(joint_name_to_index_.find(name));

	if (!it)
	{
		LOG_ERROR("There is no joint with name: " + name);
		throw not_found();
	}

	return joint(it->value);
}


void skeleton::build_absolute_pose
	(const skeleton_transform& relative_pose,
	 skeleton_transform& absolute_pose) const
{
	absolute_pose.clear();
	SCOPE_GUARD
		(clear_absolute_pose_guard,
			(absolute_pose, &skeleton_transform::clear));

	if (relative_pose.size() != joints_.size())
	{
		LOG_ERROR("Mismatch in entry count of pose transforms and joints.");
		throw invalid_parameter();
	}

	absolute_pose.resize(relative_pose.size());
	build_absolute_pose
		(relative_pose, absolute_pose, 0);

	clear_absolute_pose_guard.dismiss();
}


void skeleton::build_absolute_pose_matrix
	(const skeleton_transform& relative_pose,
	 skeleton_transform_matrix& absolute_pose_matrix) const
{
	absolute_pose_matrix.clear();
	SCOPE_GUARD
		(clear_absolute_pose_matrix_guard,
			(absolute_pose_matrix,
			 &skeleton_transform_matrix::clear));

	skeleton_transform absolute_pose;
	build_absolute_pose(relative_pose, absolute_pose);

	for (int64 i = 0; i < absolute_pose.size(); ++i)
		absolute_pose_matrix.push_back
			(absolute_pose[i].build_local_to_world());

	clear_absolute_pose_matrix_guard.dismiss();
}


void skeleton::build_absolute_normals_matrix
	(const skeleton_transform& relative_pose,
	 skeleton_transform_matrix& absolute_normals_matrix) const
{
	absolute_normals_matrix.clear();
	SCOPE_GUARD
		(clear_absolute_normals_matrix_guard,
			(absolute_normals_matrix,
			 &skeleton_transform_matrix::clear));

	skeleton_transform absolute_pose;
	build_absolute_pose(relative_pose, absolute_pose);

	for (int64 i = 0; i < absolute_pose.size(); ++i)
		absolute_normals_matrix.push_back
			(absolute_pose[i].build_normals_to_world());

	clear_absolute_normals_matrix_guard.dismiss();
}


void skeleton::validate() const
{
	int64 joints_visited = 0;
	validate(0, invalid_index, 0, joints_visited);

	if (joints_visited != joints_.size())
	{
		LOG_ERROR("Not each skeleton joint is part of the hierarchy.");
		throw validation_failed();
	}
}


void skeleton::build_absolute_pose
	(const skeleton_transform& relative_pose,
	 skeleton_transform& absolute_pose,
	 int64 joint_index) const
{
	// Constructor has validate()d the skeleton.
	// Thus, no need to check indices in here.

	const skeleton_joint& joint =
		joints_[joint_index];

	absolute_pose[joint_index] =
		((joint.parent != invalid_index) ?
			absolute_pose[joint.parent] : si_transform()) *
		joint.joint_to_parent *
		relative_pose[joint_index];

	for (int64 i = 0; i < joint.child_count; ++i)
		build_absolute_pose
			(relative_pose, absolute_pose,
			 joint.first_child + i);

	// Children joints position themselves in relation
	// to the accumulated relative poses of their parent.
	// However, children must not include their parent
	// binding_to_joint matrix.

	absolute_pose[joint_index] =
		absolute_pose[joint_index] *
		joint.binding_to_joint;
}


void skeleton::init_joint_name_to_index()
{
	for (int64 i = 0; i < joints_.size(); ++i)
	{
		const skeleton_joint& joint = joints_[i];

		if (joint_name_to_index_.contains(joint.name))
		{
			LOG_ERROR("Joint name: " + joint.name + " not unique.");
			throw invalid_parameter();
		}

		joint_name_to_index_.insert(joint.name, i);
	}
}


void skeleton::validate
	(int64 joint_index, int64 parent_index,
	 int64 parent_height, int64& joints_visited) const
{
	if (joint_index == invalid_index) return;

	++joints_visited;

	if (parent_height + 1 > maximum_height)
	{
		LOG_ERROR("Skeleton height exceeds maximum.");
		throw validation_failed();
	}
	
	if (joint_index < 0 || joint_index >= joints_.size())
	{
		LOG_ERROR("Skeleton references invalid joint index.");
		throw validation_failed();
	}

	const skeleton_joint& joint =
		joints_[joint_index];

	if (joint.parent != parent_index)
	{
		LOG_ERROR("Children joints do not reference their parent.");
		throw validation_failed();
	}

	if (joint.child_count < 0 ||
		joint.child_count > skeleton_joint::max_child_count)
	{
		LOG_ERROR("Joint has invalid child count.");
		throw validation_failed();
	}

	if (joint.child_count == 0 &&
		joint.first_child != invalid_index)
	{
		LOG_ERROR("Joint with no children has a valid first child index.");
		throw validation_failed();
	}

	if (joint.child_count != 0 &&
		joint.first_child <= joint_index)
	{
		LOG_ERROR("Joint children indices are smaller than joint index.");
		throw validation_failed();
	}

	if (!integer_checks::is_add_safe(joint.first_child, joint.child_count))
	{
		LOG_ERROR("Integer overflow on children joint indices.");
		throw validation_failed();
	}

	for (int64 i = 0; i < joint.child_count; ++i)
	{
		int64 child_index =
			joint.first_child + i;

		if (child_index < 0 || child_index >= joints_.size())
		{
			LOG_ERROR("Joint child has invalid index.");
			throw validation_failed();
		}

		validate(child_index, joint_index,
				 parent_height + 1, joints_visited);
	}
}




} /* namespace viral_core */