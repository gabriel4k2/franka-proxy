/**
 *************************************************************************
 *
 * @file network_util.hpp
 *
 * Network utilities.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_UTIL_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_UTIL_HPP


#include "build_config.hpp"


#include "network_forward.hpp"


#include "string.hpp"


namespace viral_core
{


/**
 ************************************************************************* 
 *
 * @class network_host
 * @ingroup network_module
 *
 * Collects and stores information about a remote host.
 *
 ************************************************************************/
class network_host
{

	public:

		network_host
			(const string& remote_ip,
			 const string& remote_hostname,
			 uint16 remote_port);


		/**
		 * Create network_host from partial data.
		 *
		 * Query missing details (e.g. @bref{remote_ip})
		 * through provided @bref{network} context.
		 */
		//@{
		static network_host from_ip_and_port
			(network_context& network,
			 const string& remote_ip,
			 uint16 remote_port);

		static network_host from_hostname_and_port
			(network_context& network,
			 const string& remote_hostname,
			 uint16 remote_port);
		//@}


		const string remote_ip;
		const string remote_hostname;
		const uint16 remote_port;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_UTIL_HPP) */
