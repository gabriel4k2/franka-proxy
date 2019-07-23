/**
 *************************************************************************
 *
 * @file network.cpp
 *
 * Low-level TCP/IP networking, implementation.
 *
 ************************************************************************/


#include "network.hpp"


#include "log.hpp"
#include "auto_pointer.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// network_context
//
//////////////////////////////////////////////////////////////////////////


network_context::~network_context() NOTHROW
{
	LOG_INFO("Destroyed: "  + context_name);
}


network_context::network_context
	(const string& context_name)
	:
	context_name(context_name)
{
	LOG_INFO("Creating: "  + context_name);
}




//////////////////////////////////////////////////////////////////////////
//
// network_connection
//
//////////////////////////////////////////////////////////////////////////


network_connection::~network_connection() NOTHROW
{
	LOG_INFO("Disconnected from remote end: " + remote_ip + ":" + remote_port);
}


network_connection::network_connection
	(const string& remote_ip,
	 uint16 remote_port)
	:
	remote_ip(remote_ip),
	remote_port(remote_port)
{
	LOG_INFO("Connecting to remote end: "  + remote_ip + ":" + remote_port);
}




//////////////////////////////////////////////////////////////////////////
//
// network_server
//
//////////////////////////////////////////////////////////////////////////


network_server::~network_server() NOTHROW
{
	LOG_INFO("Stopped server on: " + local_port);
}


network_server::network_server
	(uint16 local_port)
	:
	local_port(local_port)
{
	LOG_INFO("Starting server on: " + local_port);
}




} /* namespace viral_core */

