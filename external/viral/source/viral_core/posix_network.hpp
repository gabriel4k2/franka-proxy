/**
 *************************************************************************
 *
 * @file posix_network.hpp
 *
 * TCP/IP network on Posix.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__POSIX_NETWORK_HPP)
#define INCLUDED__VIRAL_CORE__POSIX_NETWORK_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_POSIX_NETWORK)


#include "network.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class posix_network_context
 * @ingroup network_module
 *
 * A network_context that uses Posix sockets as a backend.
 *
 ************************************************************************/
class posix_network_context :
	public network_context
{
	friend class posix_network_connection;
	friend class posix_network_server;

	
	public:

		posix_network_context
			(const string& context_name);

		~posix_network_context() NOTHROW override;


		auto_pointer<network_connection>
			create_connection
				(const string& remote_ip,
				 uint16 remote_port) override;

		auto_pointer<network_connection>
			try_create_connection
				(const string& remote_ip,
				 uint16 remote_port) override;


		auto_pointer<network_server>
			create_server(uint16 local_port) override;


		string ip_from_hostname(const string& hostname) override;
		string hostname_from_ip(const string& ip) override;


	private:

		/**
		 * Fetch error message for a Posix socket @bref{error_code}.
		 * Reentrant and thus threadsafe.
		 */
		static const char* error_message(int error_code);
};




/**
 *************************************************************************
 *
 * @class posix_network_connection
 * @ingroup network_module
 *
 * A network_connection that uses Posix sockets as a backend.
 *
 ************************************************************************/
class posix_network_connection :
	public network_connection
{
	friend class posix_network_context;
	friend class posix_network_server;


	public:

		~posix_network_connection() NOTHROW override;

		int64 send_nonblocking
			(const unsigned char* data, int64 size) override;
		int64 receive_nonblocking
			(unsigned char* data, int64 size) override;


	private:

		/**
		 * Create a new posix_network_connection over @bref{socket}.
		 * The new connection assumes ownership of the socket,
		 * sets the socket to a non-blocking transfer mode,
		 * and closes the socket on destruction.
		 *
		 * @attent If the constructor of the connection throws,
		 * callers must explicitly close the @bref{socket}.
		 */
		posix_network_connection
			(int socket,
			 const string& remote_ip,
			 uint16 remote_port);


		int socket_;
};




/**
 *************************************************************************
 *
 * @class posix_network_server
 * @ingroup network_module
 *
 * A network_server that uses Posix sockets as a backend.
 *
 ************************************************************************/
class posix_network_server :
	public network_server
{
	friend class posix_network_context;


	public:

		~posix_network_server() NOTHROW override;

		auto_pointer<network_connection>
			try_accept_connection() override;


	private:

		posix_network_server(uint16 local_port);

		int listen_socket_;
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_POSIX_NETWORK) */


#endif /* !defined(INCLUDED__VIRAL_CORE__POSIX_NETWORK_HPP) */
