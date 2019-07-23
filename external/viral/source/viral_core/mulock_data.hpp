/**
 *************************************************************************
 *
 * @file mulock_data.hpp
 *
 * Data and IDs for multi-locks.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MULOCK_DATA_HPP)
#define INCLUDED__VIRAL_CORE__MULOCK_DATA_HPP


#include "build_config.hpp"


#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class mulock_id
 * @ingroup mulock_module
 *
 * Client-side handle for mulock_data.
 *
 ************************************************************************/
class mulock_id
{
	public:

		mulock_id() { }

		mulock_id(const string& debug_name)
			: debug_name(debug_name) { }


		/** Required to allow client-side derived classes. */
		virtual ~mulock_id() NOTHROW { }


		const string debug_name;
};




/**
 *************************************************************************
 *
 * @class mulock_data
 * @ingroup mulock_module
 *
 * Thread-protected data of a mulock_id.
 *
 ************************************************************************/
class mulock_data
{
	public:

		virtual ~mulock_data() NOTHROW { }
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__MULOCK_DATA_HPP) */