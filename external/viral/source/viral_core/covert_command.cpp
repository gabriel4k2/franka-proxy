/**
 *************************************************************************
 *
 * @file covert_command.cpp
 *
 * Commands for modifying covert objects, implementation.
 *
 ************************************************************************/


#include "covert_command.hpp"


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// covert_command
//
//////////////////////////////////////////////////////////////////////////


covert_command::~covert_command() NOTHROW { }
covert_command::covert_command() NOTHROW { }




//////////////////////////////////////////////////////////////////////////
//
// covert_command_commit
//
//////////////////////////////////////////////////////////////////////////


covert_command_commit::covert_command_commit
	(auto_pointer<covert_data> data,
	 const shared_pointer<covert_id>& id)
	:
	id(id),
	data_(std::move(data))
{
	validate();
}


covert_command_commit::covert_command_commit
	(const covert_command_commit& other)
	:
	id(other.id),
	data_(other.data_ ?
			other.data_->clone() :
			auto_pointer<covert_data>())
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	covert_command_commit::clone() const
{
	return auto_pointer<covert_command>
				(new covert_command_commit(*this));
}


auto_pointer<covert_data>
	covert_command_commit::release_data() NOTHROW
		{ return std::move(data_); }


void covert_command_commit::validate()
{
	if (!data_)
	{
		LOG_ERROR("Cannot commit NULL data.");
		throw null_pointer();
	}

	if (!id)
	{
		LOG_ERROR("Cannot commit data to NULL ID.");
		throw null_pointer();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// covert_query_result
//
//////////////////////////////////////////////////////////////////////////


covert_query_result::covert_query_result()
	:
	finished_(false)
{ }


void covert_query_result::finish(auto_pointer<covert_data> data)
{
	MUTEX_SCOPE(mutex_);
	data_ = std::move(data);
	finished_ = true;
}


auto_pointer<covert_data> covert_query_result::data()
{
	MUTEX_SCOPE(mutex_);
	return std::move(data_);
}


bool covert_query_result::finished() const
{
	MUTEX_SCOPE(mutex_);
	return finished_;
}


void covert_query_result::reset()
{
	MUTEX_SCOPE(mutex_);
	data_.reset();
	finished_ = false;
}




//////////////////////////////////////////////////////////////////////////
//
// covert_command_query
//
//////////////////////////////////////////////////////////////////////////


covert_command_query::covert_command_query
	(const shared_pointer<covert_id>& id,
	 const shared_pointer<covert_query_result>& result)
	:
	id(id),
	result(result)
{
	validate();
}


covert_command_query::covert_command_query
	(const covert_command_query& other)
	:
	id(other.id),
	result(other.result)
{
	// No need to validate().
	// Original must have been valid.
}


auto_pointer<covert_command>
	covert_command_query::clone() const
{
	return auto_pointer<covert_command>
				(new covert_command_query(*this));
}


void covert_command_query::validate()
{
	if (!id)
	{
		LOG_ERROR("Cannot query NULL ID.");
		throw null_pointer();
	}

	if (!result)
	{
		LOG_ERROR("Cannot query into NULL result.");
		throw null_pointer();
	}
}




//////////////////////////////////////////////////////////////////////////
//
// covert_command_purge
//
//////////////////////////////////////////////////////////////////////////


covert_command_purge::covert_command_purge() { }


covert_command_purge::covert_command_purge
	(const covert_command_purge& other)
{ }


auto_pointer<covert_command>
	covert_command_purge::clone() const
{
	return auto_pointer<covert_command>
				(new covert_command_purge(*this));
}




//////////////////////////////////////////////////////////////////////////
//
// covert_command_queue
//
//////////////////////////////////////////////////////////////////////////


covert_command_queue::covert_command_queue() NOTHROW { }


covert_command_queue::covert_command_queue
	(const covert_command_queue& other)
	:
	commands_(other.commands_)
{ }


covert_command_queue::covert_command_queue
	(covert_command_queue&& other) NOTHROW
{
	// Swap ensures defaulted other.
	swap(*this, other);
}


covert_command_queue::~covert_command_queue() NOTHROW { }


covert_command_queue&
	covert_command_queue::operator =(const covert_command_queue& other)
{
	if (&other == this) return *this;

	// Strong exception safety.
	covert_command_queue offhand_copy(other);
	swap(*this, offhand_copy);
	return *this;
}


covert_command_queue&
	covert_command_queue::operator =(covert_command_queue&& other) NOTHROW
{
	if (&other == this) return *this;

	// Exception safety: NOTHROW.
	covert_command_queue offhand_move(std::move(other));
	swap(*this, offhand_move);
	return *this;
}


void covert_command_queue::swap_with
	(covert_command_queue& other) NOTHROW
{
	swap(commands_, other.commands_);
}


void covert_command_queue::insert
	(auto_pointer<covert_command> command)
{
	if (!command)
	{
		LOG_ERROR("Cannot insert NULL command into queue.");
		throw null_pointer();
	}

	commands_.insert
		(std::move(command));
}


void covert_command_queue::commit_untyped
	(const covert_data& data,
	 const shared_pointer<covert_id>& id)
{
	auto_pointer<covert_command> command
		(new covert_command_commit(data.clone(), id));

	insert(std::move(command));
}

void covert_command_queue::commit_untyped
	(auto_pointer<covert_data> data,
	 const shared_pointer<covert_id>& id)
{
	auto_pointer<covert_command> command
		(new covert_command_commit
			(std::move(data), id));

	insert(std::move(command));
}


void covert_command_queue::query
	(const shared_pointer<covert_id>& id,
	 const shared_pointer<covert_query_result>& result)
{
	auto_pointer<covert_command> command
		(new covert_command_query(id, result));

	insert(std::move(command));
}


void covert_command_queue::purge()
{
	auto_pointer<covert_command> command
		(new covert_command_purge());

	insert(std::move(command));
}


covert_command_queue::iterator
	covert_command_queue::first() NOTHROW
		{ return commands_.first(); }
covert_command_queue::const_iterator
	covert_command_queue::first() const NOTHROW
		{ return commands_.first(); }
covert_command_queue::iterator
	covert_command_queue::last() NOTHROW
		{ return commands_.last(); }
covert_command_queue::const_iterator
	covert_command_queue::last() const NOTHROW
		{ return commands_.last(); }


int64 covert_command_queue::size() const NOTHROW
	{ return commands_.size(); }
bool covert_command_queue::empty() const NOTHROW
	{ return commands_.empty(); }


void covert_command_queue::clear() NOTHROW
	{ commands_.clear(); }




} /* namespace viral_core */
