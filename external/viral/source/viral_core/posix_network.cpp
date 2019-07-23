/**
 *************************************************************************
 *
 * @file posix_network.cpp
 *
 * TCP/IP network on Posix, implementation
 *
 ************************************************************************/


#include "posix_network.hpp"


#if defined(VIRAL__HAVE_POSIX_NETWORK)


#include <sys/types.h> 
#include <sys/socket.h> 


#include "log.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// posix_network_context
//
//////////////////////////////////////////////////////////////////////////


posix_network_context::posix_network_context
	(const string& context_name)
	:
	network_context(context_name)
{ }

	
posix_network_context::~posix_network_context() NOTHROW { }


auto_pointer<network_connection>
	posix_network_context::create_connection
		(const string& remote_ip,
		 uint16 remote_port)
{
	throw unsupported_feature();
}


auto_pointer<network_connection>
	posix_network_context::try_create_connection
		(const string& remote_ip,
		 uint16 remote_port)
{
	throw unsupported_feature();
}


auto_pointer<network_server>
	posix_network_context::create_server(uint16 local_port)
{
	return auto_pointer<network_server>
				(new posix_network_server(local_port));
}


string posix_network_context::ip_from_hostname(const string& hostname);
{
	throw unsupported_feature();
}


string posix_network_context::hostname_from_ip(const string& ip)
{
	throw unsupported_feature();
}


const char* posix_network_context::error_message(int error_code)
{
	// Attention: We cannot use strerror here
	// because of thread safety issues.

	switch (error_code)
	{
		case EADDRINUSE: return "Address or port in use.";

		case EHOSTUNREACH: return "Host unreachable.";
		case ENETUNREACH: return "Network unreachable.";

		case ECONNABORTED: return "Connection aborted.";
		case ECONNRESET: return "Connection reset by peer.";
		case ECONNREFUSED: return "Connection refused.";
		case ENOTCONN: return "Not connected.";

		case EWOULDBLOCK: return "Operation would block.";
		case ETIMEDOUT: return "Connection timed out.";
	}

	return "Unspecified error.";
}




//////////////////////////////////////////////////////////////////////////
//
// posix_network_connection
//
//////////////////////////////////////////////////////////////////////////


posix_network_connection::~posix_network_connection() NOTHROW
{
	if (socket_ != -1)
	{
		if (close(socket_) == -1)
		{
			LOG_ERROR("Failed to close network socket.")
			// No exception to satisfy NOTHROW.
		}
	}
	
	socket_ = -1;
}


int64 posix_network_connection::send_nonblocking
	(const unsigned char* data, int64 size)
{
	throw unsupported_feature();
}


int64 posix_network_connection::receive_nonblocking
	(unsigned char* data, int64 size)
{
	throw unsupported_feature();
}


posix_network_connection::posix_network_connection
	(socket_handle socket,
	 const string& remote_ip,
	 uint16 remote_port)
	:
	network_connection(remote_ip, remote_port),
	socket_(-1)
{
	if (socket == -1)
	{
		LOG_ERROR("Cannot create connection over invalid socket.")
		throw network_exception();
	}


	// Set socket to non-blocking mode.
	int socket_flags = fcntl(socket, F_GETFL, 0);
	if (socket_flags == -1)
	{
		LOG_ERROR("Failed to query current socket flags.");
		throw network_exception();
	}

	if (fcntl(socket, F_SETFL, flags | O_NONBLOCK) == -1)
	{
		LOG_ERROR("Failed to set socket to non-blocking IO mode.");
		throw network_exception();
	}


	// Take ownership of socket as last operation.
	//
	// We cannot instantly take ownership of the socket:
	// Some of our members may throw on construction,
	// which in turn prohibits the use of scope guards
	// or ~network_connection() for closing the socket.
	//
	// Exception safety: NOTHROW from here.
	socket_ = socket;
}




//////////////////////////////////////////////////////////////////////////
//
// posix_network_server
//
//////////////////////////////////////////////////////////////////////////


posix_network_server::~posix_network_server() NOTHROW
{
	if (listen_socket_ != -1)
	{
		if (close(listen_socket_) == -1)
		{
			LOG_ERROR("Failed to close listening socket.")
			// No exception to satisfy NOTHROW.
		}
	}
	
	listen_socket_ = -1;
}


auto_pointer<network_connection>
	posix_network_server::try_accept_connection()
{
	throw unsupported_feature();
}


posix_network_server::posix_network_server(uint16 local_port)
	:
	network_server(local_port),
	listen_socket_(-1)
{
	throw unsupported_feature();
}




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_POSIX_NETWORK) */
