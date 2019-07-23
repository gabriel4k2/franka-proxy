/**
 *************************************************************************
 *
 * @file network_util.cpp
 *
 * Network utilities, implementation.
 *
 ************************************************************************/


#include "network_util.hpp"


#include "network.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// network_host
//
//////////////////////////////////////////////////////////////////////////


network_host::network_host
	(const string& remote_ip,
	 const string& remote_hostname,
	 uint16 remote_port)
	:
	remote_ip(remote_ip),
	remote_hostname(remote_hostname),
	remote_port(remote_port)
{ }


network_host network_host::from_ip_and_port
	(network_context& network,
	 const string& remote_ip,
	 uint16 remote_port)
{
	return network_host
				(remote_ip,
				 network.hostname_from_ip(remote_ip),
				 remote_port);
}


network_host network_host::from_hostname_and_port
	(network_context& network,
	 const string& remote_hostname,
	 uint16 remote_port)
{
	return network_host
				(network.ip_from_hostname(remote_hostname),
				 remote_hostname,
				 remote_port);
}




} /* namespace viral_core */
