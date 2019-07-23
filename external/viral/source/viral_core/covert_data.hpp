/**
 *************************************************************************
 *
 * @file covert_data.hpp
 *
 * Client-side data for hidden objects.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COVERT_DATA_HPP)
#define INCLUDED__VIRAL_CORE__COVERT_DATA_HPP


#include "build_config.hpp"


#include "string.hpp"
#include "cache.hpp"
#include "tree_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class covert_data
 * @ingroup covert_module
 *
 * Stores a client-side representation of some covert_object.
 *
 * Usage notes:
 * - Unlike most subsystems, covert_data is not threadsafe.
 *		Hence, clone() operations or auto_pointer%s are required
 *		for transferring data in between multiple threads.
 * - @attent To maintain thread safety, clients must not access
 *		any covert_data after data transfer to another thread.
 *		In particular, clients should not maintain native pointers
 *		to data that has been committed to a covert_command_queue.
 *
 * Implementation notes:
 * - Subsystems and their implementations derive from covert_data
 *		in order to support custom covert_object%s.
 * - Implementations of covert_data must have a @bref{covert_id_type}
 *		typedef which defines the associated type of @bref{covert_id}.
 *
 ************************************************************************/
class covert_data
{
	public:

		virtual ~covert_data() NOTHROW { }

		virtual auto_pointer<covert_data> clone() const = 0;
};




/**
 *************************************************************************
 *
 * @class covert_id
 * @ingroup covert_module
 *
 * An opaque identifier for a unique covert_object instance.
 *
 * Implementation notes:
 * - Clients and subsystems can share covert_id%s over multiple threads.
 *		Hence, IDs must be threadsafe for arbitrary client threads.
 * - Usage of shared_pointer%s for all covert_id%s ensures thread-safe
 *		lifetime management, even when IDs are shared over client-side
 *		and subsystem-internal references. In this context, note that
 *		subsystem-internal references can solely sustain a covert_id.
 * - Both subsystems and clients must avoid cycles in-between covert_id
 *		and covert_object references in order to avoid memory leaks
 *		(e.g. use weak_pointer%s or cycle-free dependencies).
 * - Subsystems must derive from covert_id to expose subsystem-specific
 *		covert_object types.
 *
 ************************************************************************/
class covert_id :
	private non_copyable
{
	public:

		virtual ~covert_id() NOTHROW { }

		/** Non-unique, human-readable name for debug output. */
		const string debug_name;

	protected:

		covert_id() { }
		covert_id(const string& debug_name) : debug_name(debug_name) { }
};




/**
 * Binds unique names to covert_data instances.
 * @ingroup covert_module
 */
typedef owning_cache<covert_data> covert_data_cache;


/**
 * Binds unique names to covert_id%s.
 * @ingroup covert_module
 */
typedef shared_cache<covert_id> covert_id_cache;




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COVERT_DATA_HPP) */