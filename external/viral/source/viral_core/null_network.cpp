/**
 *************************************************************************
 *
 * @file null_network.cpp
 *
 * Null network system, implementation.
 *
 ************************************************************************/


#include "null_network.hpp"


#include "log.hpp"
#include "auto_pointer.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// null_network_context
//
//////////////////////////////////////////////////////////////////////////


null_network_context::null_network_context
	(const string& context_name)
	:
	network_context(context_name)
{
	LOG_INFO("Using null_network_context implementation.");
}


null_network_context::~null_network_context() NOTHROW
{
	LOG_INFO("Destroying.");
}


auto_pointer<network_connection>
	null_network_context::create_connection
		(const string& remote_ip,
		 uint16 remote_port,
		 float timeout_hint_seconds)
{
	LOG_ERROR("The null_network_context cannot create connections.");
	throw unsupported_feature();
}


auto_pointer<network_connection>
	null_network_context::create_connection
		(const string& remote_ip,
		 uint16 remote_port)
{
	LOG_ERROR("The null_network_context cannot create connections.");
	throw unsupported_feature();
}


auto_pointer<network_connection>
	null_network_context::try_create_connection
		(const string& remote_ip,
		 uint16 remote_port,
		 float timeout_hint_seconds)
{
	// NULL is acceptable for try_create_connection().
	return auto_pointer<network_connection>();
}


auto_pointer<network_connection>
	null_network_context::try_create_connection
		(const string& remote_ip,
		 uint16 remote_port)
{
	// NULL is acceptable for try_create_connection().
	return auto_pointer<network_connection>();
}


auto_pointer<network_server>
	null_network_context::create_server(uint16 local_port)
{
	LOG_ERROR("The null_network_context cannot create servers.");
	throw unsupported_feature();	
}


string null_network_context::ip_from_hostname
	(const string& hostname)
{
	LOG_ERROR("The null_network_context cannot resolve hostnames.");
	throw unsupported_feature();
}


string null_network_context::hostname_from_ip(const string& ip)
{
	LOG_ERROR("The null_network_context cannot lookup IP addresses.");
	throw unsupported_feature();
}




} /* namespace viral_core */
