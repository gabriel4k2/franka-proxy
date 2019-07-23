/**
 *************************************************************************
 *
 * @file null_network.hpp
 *
 * Null network system.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NULL_NETWORK_HPP)
#define INCLUDED__VIRAL_CORE__NULL_NETWORK_HPP


#include "build_config.hpp"


#include "network.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class null_network_context
 * @ingroup network_module
 *
 * A network_context that cannot establish servers or connections.
 *
 ************************************************************************/
class null_network_context :
	public network_context
{

	public:

		null_network_context(const string& context_name);
		~null_network_context() NOTHROW override;


		auto_pointer<network_connection>
			create_connection
				(const string& remote_ip,
				 uint16 remote_port,
				 float timeout_hint_seconds) override;

		auto_pointer<network_connection>
			create_connection
				(const string& remote_ip,
				 uint16 remote_port) override;


		auto_pointer<network_connection>
			try_create_connection
				(const string& remote_ip,
				 uint16 remote_port,
				 float timeout_hint_seconds) override;

		auto_pointer<network_connection>
			try_create_connection
				(const string& remote_ip,
				 uint16 remote_port) override;


		auto_pointer<network_server>
			create_server(uint16 local_port) override;


		string ip_from_hostname(const string& hostname) override;
		string hostname_from_ip(const string& ip) override;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NULL_NETWORK_HPP) */
