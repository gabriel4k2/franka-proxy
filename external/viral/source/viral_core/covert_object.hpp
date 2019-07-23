/**
 *************************************************************************
 *
 * @file covert_object.hpp
 *
 * Subsystem-internal hidden objects.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__COVERT_OBJECT_HPP)
#define INCLUDED__VIRAL_CORE__COVERT_OBJECT_HPP


#include "build_config.hpp"


#include "covert_forward.hpp"


#include "util.hpp"
#include "auto_pointer.hpp"
#include "shared_pointer.hpp"
#include "owning_map.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup covert_module Covert Object Module
 *
 * Abstracts and hides implementation-specific covert_object%s
 * from clients via a thread-safe sharing mechanism.
 *
 * Covert objects overview:
 * - Subsystems store all their internals (such as textures,
 *		models, sound effects, or scenes) in covert_object%s.
 *		For thread safety and consistency reasons, client threads
 *		never create, access, or destroy these objects directly.
 * - Clients create an opaque covert_id for each covert object.
 *		Subsystems internally create (covert_command_commit)
 *		and destroy (covert_command_purge) associated objects.
 * - Clients control subsystems and contained covert objects
 *		via a FIFO covert_command_queue of covert_command%s.
 * - In order to modify covert_objects, clients create local
 *		covert_data instances with desired object contents.
 *		Finally, a covert_command_commit updates a covert_object
 *		of given covert_id with provided covert_data.
 * - Various utility classes allow for covert_data loading
 *		(covert_loader), and by-name binding of data and IDs
 *		(covert_data_cache, covert_id_cache).
 *
 ************************************************************************/


/**
 *************************************************************************
 *
 * @class covert_object
 * @ingroup covert_module
 *
 * A subsystem-internal, hidden object, bound to a covert_id.
 *
 * Usage notes:
 * - @attent Clients must not directly work with covert_objects.
 *		Instead, clients must use covert_id%s and covert_data.
 *
 * Implementation notes:
 * - Subsystems and their implementations derive from covert_object
 *		in order to realize subsystem features.
 * - Implementations of covert_object must have a @bref{covert_id_type}
 *		typedef which specifies the associated type of @bref{covert_id}.
 * - A subsystem should employ a covert_object_manager
 *		for all objects that it shares with its clients.
 * - Internally, a subsystem can explicitly create and destroy
 *		further covert_objects without employing an object manager.
 *		For instance, objects can be members of other objects.
 * - Any covert_object is accessed only by a single thread at a time.
 *		This thread has already acquired all necessary mutexes.
 *
 ************************************************************************/
class covert_object :
	private non_copyable
{
	public:

		virtual ~covert_object() NOTHROW { }

		/** Apply or return object data. */
		//@{
		virtual void set_data(auto_pointer<covert_data> data) = 0;
		virtual auto_pointer<covert_data> data() = 0;
		//@}

	protected:

		covert_object() { }
};




/**
 *************************************************************************
 *
 * @class covert_object_manager
 * @ingroup covert_module
 *
 * Maps covert_id%s to covert_object%s and governs covert_object lifetime.
 *
 * Usage notes:
 * - See covert_object for details on threaded access.
 *
 ************************************************************************/
class covert_object_manager :
	private non_copyable
{
	public:

		covert_object_manager() { }
		~covert_object_manager() NOTHROW;


		/**
		 * Fetch covert_object exactly of type @bref{T} for given @bref{id}.
		 * Throws if no such object exists.
		 *
		 * @attent Strong ownership over @bref{id} is required
		 * to keep the resulting object reference alive.
		 */
		template <typename T>
			T& object_for_id
				(const shared_pointer<typename T::covert_id_type>& id) const
		{
			if (!id) throw_null_id();

			id_to_object_map::const_iterator find(id_to_object_.find(id));
			if (!find || !find->value) throw_no_object(id);
		
			if (typeid(*find->value) != typeid(T)) throw_type_mismatch(id);
			return static_cast<T&>(*find->value);
		}


		/** Map new @bref{object} to indicated @bref{id}. */
		void insert
			(const shared_pointer<covert_id>& id,
			 auto_pointer<covert_object> object);
		

		/** Return object for given @bref{id}, if any. */
		covert_object* find_object
			(const shared_pointer<covert_id>& id) const;


		/**
		 * Delete any covert_object%s
		 * whose covert_id has been destroyed.
		 * Invoke on covert_command_purge.
		 */
		void purge() NOTHROW;


		/**
		 * Delete all covert_object%s,
		 * even if their covert_id%s are still alive.
		 * Use this on subsystem destruction.
		 */
		void clear() NOTHROW;


	private:

		static void throw_null_id();
		static void throw_no_object(const shared_pointer<covert_id>& id);
		static void throw_type_mismatch(const shared_pointer<covert_id>& id);


		typedef owning_map<weak_pointer<covert_id>, covert_object> id_to_object_map;
		id_to_object_map id_to_object_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__COVERT_OBJECT_HPP) */