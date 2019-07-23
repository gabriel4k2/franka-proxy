/**
 *************************************************************************
 *
 * @file model.cpp
 *
 * CPU-side models, implementation.
 *
 ************************************************************************/


#include "model.hpp"


#include "log.hpp"
#include "file.hpp"
#include "file_util.hpp"
#include "collada_import.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
//	model
//
//////////////////////////////////////////////////////////////////////////


model::model()
	:
	bounding_radius_(0)
{ }


model::model(const model& other)
	:
	skeleton_rig_(other.skeleton_rig_ ?
					new skeleton(*other.skeleton_rig_) : NULL),

	bounding_box_(other.bounding_box_),
	bounding_radius_(other.bounding_radius_)
{
	for (group_list::const_iterator it(other.groups_.first()); it; ++it)
	{
		auto_pointer<model_group> new_group(new model_group(**it));
		insert_group_helper(std::move(new_group));
	}
}


model::model(model&& other) NOTHROW
	:
	bounding_radius_(0)
{
	// Swap ensures defaulted other.
	swap(*this, other);
}


auto_pointer<model> model::clone() const
	{ return auto_pointer<model>(new model(*this)); }


model& model::operator =(const model& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	model offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


model& model::operator =(model&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	model offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void model::swap_with(model& other) NOTHROW
{
	swap(groups_, other.groups_);
	swap(group_lookup_, other.group_lookup_);
	swap(skeleton_rig_, other.skeleton_rig_);
	swap(bounding_box_, other.bounding_box_);
	swap(bounding_radius_, other.bounding_radius_);
}


model::group_iterator model::insert_group
	(const string& name)
{
	auto_pointer<model_group> new_group
		(new model_group(name));

	return insert_group_helper(std::move(new_group));
}

model::group_iterator model::insert_group
	(const string& name, const mesh& m)
{
	auto_pointer<model_group> new_group
		(new model_group(name, m));

	return insert_group_helper(std::move(new_group));
}

model::group_iterator model::insert_group
	(const string& name, mesh&& m)
{
	auto_pointer<model_group> new_group
		(new model_group(name, std::move(m)));

	return insert_group_helper(std::move(new_group));
}


int64 model::group_count() const
	{ return groups_.size(); }
model::group_iterator model::first_group() const
	{ return groups_.first(); }
model::group_iterator model::last_group() const
	{ return groups_.last(); }


model::group_iterator model::find_group(const string& name) const
{
	group_map::const_iterator find(group_lookup_.find(name));
	if (find) return find->value;
	return group_iterator();
}


void model::remove_group(const string& name) NOTHROW
{
	group_map::iterator find(group_lookup_.find(name));
	if (!find) return;
	groups_.remove(find->value); // Group contained, so both entries exist.
	group_lookup_.remove(name);
}


void model::set_skeleton_rig
	(auto_pointer<skeleton> skeleton_rig) NOTHROW
		{ skeleton_rig_ = std::move(skeleton_rig); }

const skeleton* model::skeleton_rig() const NOTHROW
	{ return skeleton_rig_.get(); }


const box& model::bounding_box() const { return bounding_box_; }
float model::bounding_radius() const { return bounding_radius_; }

void model::rebuild_boundings()
{
	if (groups_.empty())
	{
		bounding_box_ = box();
		bounding_radius_ = 0;
		return;
	}

	group_list::iterator it(groups_.first());
	
	(*it)->mesh.rebuild_boundings();
	bounding_box_ = (*it)->mesh.bounding_box();
	bounding_radius_ = (*it)->mesh.bounding_radius();
	++it;

	for (; it; ++it)
	{
		(*it)->mesh.rebuild_boundings();

		bounding_box_.grow_to((*it)->mesh.bounding_box());
		bounding_radius_ =
			max(bounding_radius_, (*it)->mesh.bounding_radius());
	}
}


void model::optimize()
{
	for (group_list::iterator it(groups_.first()); it; ++it)
		(*it)->mesh.optimize();
}


void model::validate() const
{
	for (group_list::const_iterator it(groups_.first()); it; ++it)
		(*it)->mesh.validate(skeleton_rig());
}


model model::load
	(const string& path, file_context& files, model_usage_hint usage)
{
	string ext
		(virtual_path::extract_extension(path).to_lower());

	auto_pointer<file> input
		(files.open_file(path, file::read_only));

	if (ext == "dae")
		return load_dae(*input, usage);
	else if (ext == "vmsh")
		return load_vmsh(*input, usage);
	else
	{
		LOG_ERROR("Model file extension unknown: " + path);
		throw invalid_parameter();
	}
}

model model::load_dae(file& input, model_usage_hint usage)
{
	return
		collada_importer::import_model(input, usage);
}

model model::load_vmsh(file& input, model_usage_hint usage)
{
	LOG_WARN("Not implemented yet.");
	throw unsupported_feature();
}


void model::clear() NOTHROW
{
	groups_.clear();
	group_lookup_.clear();

	skeleton_rig_.reset();

	bounding_box_ = box();
	bounding_radius_ = 0;	
}


model::group_iterator model::insert_group_helper
	(auto_pointer<model_group> group)
{
	if (!group)
	{
		LOG_ERROR("Cannot insert NULL group into model.");
		throw null_pointer();
	}

	if (group_lookup_.contains(group->name))
	{
		LOG_ERROR("Model already contains a group: " + group->name);
		throw object_registration_failed();
	}

	// Insert group into model.
	// Exception safety / invariant:
	// Ensure that lookup and list entries are consistent.

	group_list::iterator entry
		(groups_.push_back(std::move(group)));
	SCOPE_GUARD
		(list_guard, (groups_, &group_list::pop_back));

	group_lookup_.insert((*entry)->name, entry);

	list_guard.dismiss();

	return entry;
}




} /* namespace viral_core */
