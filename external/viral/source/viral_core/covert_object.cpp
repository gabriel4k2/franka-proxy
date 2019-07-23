/**
 *************************************************************************
 *
 * @file covert_object.cpp
 *
 * Subsystem-internal hidden objects, implementation.
 *
 ************************************************************************/


#include "covert_object.hpp"


#include "log.hpp"
#include "covert_data.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// covert_object_manager
//
//////////////////////////////////////////////////////////////////////////


covert_object_manager::~covert_object_manager()
{
	if (!id_to_object_.empty())
	{
		// We have a major problem:
		// The covert object manager cannot acquire mutexes
		// or other resources required by object destructors.

		LOG_CRITICAL
			("Some objects have not been released\r\n"
			 "before destruction of the covert_object_manager.\r\n"			 
			 "Implementations must call clear() on destruction.\r\n"			 
			 "Threadsafe object destruction is not guaranteed,\r\n"
			 "and the application will potentially crash.");
	}
}


void covert_object_manager::insert
	(const shared_pointer<covert_id>& id,
	 auto_pointer<covert_object> object)
{
	if (!id)
	{
		LOG_ERROR("Cannot store object for NULL ID.");
		throw null_pointer();
	}

	if (!object)
	{
		LOG_ERROR("Cannot insert NULL object.");
		throw null_pointer();
	}

	id_to_object_.insert
		(id, std::move(object));
}


covert_object* covert_object_manager::find_object
	(const shared_pointer<covert_id>& id) const
{
	if (!id)
	{
		LOG_ERROR("Cannot fetch object for NULL ID.");
		throw null_pointer();
	}

	id_to_object_map::const_iterator find(id_to_object_.find(id));
	return find ? find->value : NULL;
}


void covert_object_manager::purge() NOTHROW
{
	/**
	 * Implementation notes:
	 * - Purge must only delete unused objects, i.e. objects
	 *		whose covert_id was already deleted.
	 * - Weak pointers to IDs are required in order to allow
	 *		for a valid id_to_object_map even if original IDs
	 *		have been destroyed and respective addresses
	 *		have been reused.
	 * - Multiple passes are required to resolve dependency
	 *		chains in-between covert_objects.
	 */
	bool any_deleted;

	do
	{
		any_deleted = false;

		for (id_to_object_map::iterator it(id_to_object_.first()); it; )
		{
			id_to_object_map::iterator test_id(it);
			++it;

			if (test_id->key.lock())
				continue;

			// ID was deleted and can never be lock()ed again.
			id_to_object_.remove(test_id);
			any_deleted = true;
		}
	}
	while (any_deleted);
}


void covert_object_manager::clear() NOTHROW
	{ id_to_object_.clear(); }


void covert_object_manager::throw_null_id()
{
	LOG_ERROR("Cannot query object for NULL id.");
	throw null_pointer();
}

void covert_object_manager::throw_no_object
	(const shared_pointer<covert_id>& id)
{
	LOG_ERROR("No object is associated with id: " + id->debug_name);
	throw not_found();
}

void covert_object_manager::throw_type_mismatch
	(const shared_pointer<covert_id>& id)
{
	LOG_ERROR("Object requested for id: " + id->debug_name + " has mismatching type.");
	throw type_mismatch();
}




} /* namespace viral_core */