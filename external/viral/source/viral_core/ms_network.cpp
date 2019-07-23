/**
 *************************************************************************
 *
 * @file ms_network.cpp
 *
 * TCP/IP network on Microsoft Windows, implementation.
 *
 ************************************************************************/


#include "ms_network.hpp"


#if defined(VIRAL__HAVE_MS_NETWORK)


#include <winsock2.h>
#include <ws2tcpip.h>


#include "log.hpp"
#include "exception.hpp"
#include "scope_guard.hpp"
#include "integer_operations.hpp"
#include "auto_pointer.hpp"
#include "timer.hpp"
#include "thread_util.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// ms_network_context
//
//////////////////////////////////////////////////////////////////////////


ms_network_context::ms_network_context
	(const string& context_name)
	:
	network_context(context_name)
{
	LOG_INFO("Using ms_network_context implementation.");


	// Request version 2.2 of the WinSock API.
	WSAData wsa_data;
	ZeroMemory(&wsa_data, sizeof(wsa_data));

	int error_code =
		WSAStartup(MAKEWORD(2, 2), &wsa_data);

	if (error_code != 0)
	{
		// WSAStartup() already returns an error code,
		// so there is no need for WSAGetLastError().

		LOG_ERROR("Failed to start Winsock" +
				  ms_network_errors::format_error(error_code));
		throw network_exception();
	}

	SCOPE_GUARD(startup_guard, (&WSACleanup));


	// Check if WinSock actually has the requested version.
	// Notably, outdated systems may hand back a lower version.
	if (LOBYTE(wsa_data.wVersion) != 2 ||
		HIBYTE(wsa_data.wVersion) != 2)
	{
		LOG_ERROR("System does not support WinSock v2.2.");
		throw unsupported_feature();
	}


	// Success. NOTHROW from here.
	startup_guard.dismiss();
}


ms_network_context::~ms_network_context() NOTHROW
{
	LOG_INFO("Destroying.");

	if (WSACleanup() == SOCKET_ERROR)
	{
		LOG_CRITICAL("Failed to shut down Winsock" +
					 ms_network_errors::format_last_error());

		// No exception to ensure NOTHROW.
	}
}


auto_pointer<network_connection>
	ms_network_context::create_connection
		(const string& remote_ip,
		 uint16 remote_port,
		 float timeout_hint_seconds)
{
	MUTEX_SCOPE(network_mutex_);


	// Reuse try_create_connection(), where a NULL result
	// implies an error on connect()ing to the remote end.
	// Log this error and wrap it into a network_exception
	// to fullfill create_connection() requirements.

	auto_pointer<network_connection> connection
		(try_create_connection
			(remote_ip, remote_port, timeout_hint_seconds));

	if (!connection)
	{
		LOG_ERROR("Failed to connect socket" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}

	return connection;
}


auto_pointer<network_connection>
	ms_network_context::create_connection
		(const string& remote_ip,
		 uint16 remote_port)
{
	return create_connection
				(remote_ip, remote_port,
				 default_timeout_hint_seconds);
}


auto_pointer<network_connection>
	ms_network_context::try_create_connection
		(const string& remote_ip,
		 uint16 remote_port,
		 float timeout_hint_seconds)
{
	MUTEX_SCOPE(network_mutex_);


	// Create socket.
	SOCKET s =
		socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (s == INVALID_SOCKET)
	{
		LOG_ERROR("Failed to create socket" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}

	SCOPE_GUARD(socket_guard, (&closesocket, s));


	// Set socket to non-blocking mode for connecting.
	// We cannot use blocking mode, as this prohibits
	// any custom timeout handling.
	u_long nonblocking_mode = 1;

	if (ioctlsocket(s, FIONBIO, &nonblocking_mode) == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to set non-blocking mode for connecting" +
				  ms_network_errors::format_last_error());
		throw api_call_failed();
	}


	// Put together address structure for remote end.
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(remote_port);

	int inet_pton_result =
		inet_pton(AF_INET, remote_ip.data(), &addr.sin_addr);

	if (inet_pton_result == 0)
	{
		// This case generates no WinSock error.
		LOG_ERROR("Provided IP address is not a IPv4 address.");
		throw network_exception();
	}
	else if (inet_pton_result != 1)
	{
		LOG_ERROR("Failed to convert IP address to binary format" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}


	// Connect non-blocking socket to remote end.
	// This purportedly returns SOCKET_ERROR with WSAEWOULDBLOCK,
	// but for safety reasons, we check for any other errors.
	connect(s, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));

	int error_code = WSAGetLastError();
	if (error_code != WSAEWOULDBLOCK)
	{
		LOG_ERROR("Unexpected return from connecting socket" +
				  ms_network_errors::format_error(error_code));
		throw network_exception();
	}

	free_timer timeout_timer;
	while (timeout_timer.seconds_passed() < timeout_hint_seconds)
	{
		// Check writeability of socket to determine connection success.
		// If the socket is not writeable, sleep for some time and retry.
		// Abort with exception on any other (possibly fatal) error.
		fd_set writeable_set;
		FD_ZERO(&writeable_set);
		FD_SET(s, &writeable_set);

		TIMEVAL nonblocking_select = { 0, 0 };

		int writeable_count =
			select(1, NULL, &writeable_set, NULL, &nonblocking_select);
		
		if (writeable_count == 0)
		{
			thread_util::sleep_seconds(create_connection_sleep_seconds_);
			continue;
		}

		if (writeable_count != 1) // i.e. == SOCKET_ERROR or something else.
		{
			LOG_ERROR("Failed to wait for non-blocking connection" +
					  ms_network_errors::format_last_error());
			throw network_exception();		
		}


		// Wrap connected socket into connection object.
		//
		// Exception safety:
		// - On any exception, the scope guard closes the socket.
		// - On success, the network_connection owns the socket,
		//		and we must immediately dismiss the scope guard.
		auto_pointer<network_connection> connection
			(new ms_network_connection(s, remote_ip, remote_port));

		socket_guard.dismiss();
		return connection;
	}


	// A timeout occurred.
	// Thus, return NULL as required by try_create_connection().
	// The scope guard then automatically closes the socket.
	return auto_pointer<network_connection>();
}


auto_pointer<network_connection>
	ms_network_context::try_create_connection
		(const string& remote_ip,
		 uint16 remote_port)
{
	return try_create_connection
				(remote_ip, remote_port,
				 default_timeout_hint_seconds);
}


auto_pointer<network_server>
	ms_network_context::create_server(uint16 local_port)
{
	MUTEX_SCOPE(network_mutex_);

	return auto_pointer<network_server>
				(new ms_network_server(local_port));
}


string ms_network_context::ip_from_hostname(const string& hostname)
{
	MUTEX_SCOPE(network_mutex_);


	// Fetch list of all addresses for given hostname.
	// A scope guard frees this list on all exits.
	PADDRINFOA addr_list = NULL;
	int getaddrinfo_result =
		getaddrinfo
			(hostname.data(),
			 NULL, NULL, &addr_list);

	if (getaddrinfo_result != 0) // Does not return SOCKET_ERROR.
	{
		LOG_ERROR("Failed to lookup IP address" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}

	SCOPE_GUARD(free_addr_list, (&freeaddrinfo, addr_list));


	// Look for suitable IPv4 address in list.
	// Return the first suitable address.
	for (PADDRINFOA cur_addr = addr_list;
		 cur_addr; cur_addr = cur_addr->ai_next)
	{
		if (cur_addr->ai_family != AF_INET ||
			cur_addr->ai_addrlen != sizeof(sockaddr_in))
				continue;


		// Cast to AF_INET (IPv4) family-specific address data.
		// We already double-check for matching size above.
		sockaddr_in* cur_addr_inet =
			reinterpret_cast<sockaddr_in*>(cur_addr->ai_addr);


		// Fetch ASCII representation of binary address.
		// This requires at most 16 bytes, including an explicit \0.
		// We add another terminating \0 as a safety precaution.
		char ip_buffer[17];
		ip_buffer[16] = '\0';

		PCTSTR inet_ntop_result =
			inet_ntop(AF_INET, &cur_addr_inet->sin_addr,
					  ip_buffer, 16);

		if (inet_ntop_result == NULL)	
		{
			LOG_ERROR("Failed to convert IP address to ASCII" +
					  ms_network_errors::format_last_error());
			throw network_exception();	
		}
	

		// Reparse result up to actual terminating \0,
		// which may imply less than 16 chars of data.
		return string(ip_buffer);
	}


	// Target has no IPv4 address.
	LOG_ERROR("Hostname: " + hostname + " has no IPv4 address.");
	throw network_exception();
}


string ms_network_context::hostname_from_ip(const string& ip)
{
	MUTEX_SCOPE(network_mutex_);


	// Put together address structure for remote end.
	// We don't need a port if we don't query for a service.
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));

	addr.sin_family = AF_INET;

	int inet_pton_result =
		inet_pton(AF_INET, ip.data(), &addr.sin_addr);

	if (inet_pton_result == 0)
	{
		// This case generates no WinSock error.
		LOG_ERROR("Provided IP address is not a IPv4 address.");
		throw network_exception();
	}
	else if (inet_pton_result != 1)
	{
		LOG_ERROR("Failed to convert IP address to binary format" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}


	// Perform the actual hostname lookup.
	// NI_MAXHOST is the maximum size in bytes for a hostname,
	// including an explicit terminating \0.
	string hostname_buffer;
	hostname_buffer.resize(NI_MAXHOST);

	int getnameinfo_result =
		getnameinfo
			(reinterpret_cast<sockaddr*>(&addr),
			 sizeof(addr),
			 hostname_buffer.data(), NI_MAXHOST,
			 NULL, NULL, 0);
			
	if (getnameinfo_result != 0) // Does not return SOCKET_ERROR.
	{
		LOG_ERROR("Failed to lookup hostname" +
				  ms_network_errors::format_last_error());
		throw network_exception();	
	}


	// Reparse the resulting hostname to the very first \0 entry.
	// Safety rationale: Our string at least has its implicit \0,
	// which we did not allow getnameinfo() to overwrite.
	return string(hostname_buffer.data());
}


const float ms_network_context::default_timeout_hint_seconds = 0.1f;


/** Time to sleep before rechecking state of non-blocking connect. */
const float ms_network_context::create_connection_sleep_seconds_ = 0.01f;




//////////////////////////////////////////////////////////////////////////
//
// ms_network_connection
//
//////////////////////////////////////////////////////////////////////////


ms_network_connection::~ms_network_connection() NOTHROW
{
	if (socket_ != INVALID_SOCKET)
	{
		if (closesocket(socket_) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close network socket" +
					  ms_network_errors::format_last_error());

			// No exception to satisfy NOTHROW.
		}
	}
	
	socket_ = INVALID_SOCKET;
}


int64 ms_network_connection::send_nonblocking
	(const unsigned char* data, int64 size)
{
	MUTEX_SCOPE(connection_mutex_);


	// Check preconditions.
	if (data == NULL)
	{
		LOG_ERROR("Cannot send from NULL data pointer.");
		throw null_pointer();
	}

	if (size <= 0)
	{
		LOG_ERROR("Cannot send zero or less bytes.");
		throw invalid_parameter();
	}


	// Clamp outgoing bytes to maximum allowed range.
	// Users anyways must not expect all bytes in one call.
	// The cast should not throw for suitable max_bytes_send_.
	int bytes_to_send =
		integer_operations::cast_or_throw<int>
			(min(size, max_bytes_send_));


	// Send data over the network.
	// The sending call may cause WSAEWOULDBLOCK
	// if there is an overflow in the OS-side buffer.
	// This is no error because of non-blocking send.
	const char* next_data =
		reinterpret_cast<const char*>(data);
	int64 bytes_sent =
		send(socket_, next_data, bytes_to_send, 0);

	if (bytes_sent == SOCKET_ERROR)
	{
		int error_code = WSAGetLastError();
		if (error_code != WSAEWOULDBLOCK)
		{
			LOG_ERROR("Failed to send data" +
					  ms_network_errors::format_error(error_code));
			throw network_exception();
		}


		// There is still data pending for transmission,
		// but no data has been sent yet.
		return 0;
	}


	return bytes_sent;
}


int64 ms_network_connection::receive_nonblocking
	(unsigned char* data, int64 size)
{
	MUTEX_SCOPE(connection_mutex_);


	// Check preconditions.
	if (data == NULL)
	{
		LOG_ERROR("Cannot receive into NULL data pointer.");
		throw invalid_parameter();
	}

	if (size <= 0)
	{
		LOG_ERROR("Cannot receive zero or less bytes.");
		throw invalid_parameter();
	}


	// Clamp accepted bytes to maximum allowed range.
	// Users anyways must not expect all bytes in one call.
	// The cast should not throw for suitable max_bytes_receive_.
	int bytes_to_receive =
		integer_operations::cast_or_throw<int>
			(min(size, max_bytes_receive_));


	// Receive data from the network.
	// The receiving call may cause WSAEWOULDBLOCK
	// if there is no pending data in the OS-side buffer.
	// This is no error because of non-blocking receive.
	char* next_data =
		reinterpret_cast<char*>(data);
	int bytes_received =
		recv(socket_, next_data, bytes_to_receive, 0);

	if (bytes_received == SOCKET_ERROR)
	{
		int error_code = WSAGetLastError();
		if (error_code != WSAEWOULDBLOCK)
		{
			LOG_ERROR("Failed to receive data" +
					  ms_network_errors::format_error(error_code));
			throw network_exception();
		}


		// There are pending, further transmissions,
		// but no data has been received yet.
		return 0;
	}	


	// Instead of SOCKET_ERROR with WSAEWOULDBLOCK,
	// recv() (notably, unlike send()) returns zero
	// to indicate socket shutdown on the other end.
	// We report by exception, since the connection
	// cannot be used anymore.
	if (bytes_received == 0)
	{
		LOG_ERROR("Remote end has disconnected.");
		throw network_exception();
	}


	return bytes_received;
}


ms_network_connection::ms_network_connection
	(pointer_int socket,
	 const string& remote_ip,
	 uint16 remote_port)
	:
	network_connection(remote_ip, remote_port),
	socket_(INVALID_SOCKET)
{
	if (socket == INVALID_SOCKET)
	{
		LOG_ERROR("Cannot create connection over invalid socket.")
		throw network_exception();
	}


	// Set socket to non-blocking mode.
	// This is redundant to ms_network_context::try_create_connection(),
	// but still required for ms_network_server::try_accept_connection().
	u_long nonblocking_mode = 1;

	if (ioctlsocket(socket, FIONBIO, &nonblocking_mode) == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to set non-blocking socket operation" +
				  ms_network_errors::format_last_error());
		throw api_call_failed();
	}


	// Take ownership of socket as last operation.
	// If we do not exit normally due to an exception,
	// the caller keeps socket ownership.
	//
	// Instantly taking ownership of the socket
	// requires intricate exception safety logics
	// (e.g. when member constructors may throw),
	// and thus has not been implemented.
	//
	// Exception safety: NOTHROW from here.
	socket_ = socket;
}




//////////////////////////////////////////////////////////////////////////
//
// ms_network_server
//
//////////////////////////////////////////////////////////////////////////


ms_network_server::~ms_network_server() NOTHROW
{
	if (listen_socket_ != INVALID_SOCKET)
	{
		if (closesocket(listen_socket_) == SOCKET_ERROR)
		{
			LOG_ERROR("Failed to close listening socket" +
					  ms_network_errors::format_last_error());

			// No exception to satisfy NOTHROW.
		}
	}
	
	listen_socket_ = INVALID_SOCKET;
}


auto_pointer<network_connection>
	ms_network_server::try_accept_connection()
{
	MUTEX_SCOPE(server_mutex_);


	// Try to accept an established TCP/IP connection
	// from the connection queue of the listening socket.
	// Note that TCP/IP connections automatically finish
	// their SYN/ACK before entering the connection queue.
	// Therefore, ms_network_context::create_connection()
	// does not block for try_accept_connection().
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));
	socklen_t addr_size = sizeof(addr);

	SOCKET s =
		accept(listen_socket_,
			   reinterpret_cast<sockaddr*>(&addr),
			   &addr_size);

	if (s == INVALID_SOCKET)
	{
		int error_code = WSAGetLastError();

		if (error_code == WSAEWOULDBLOCK ||
			error_code == WSAECONNRESET)
		{
			// There are no queued TCP/IP connections (WSAEWOULDBLOCK)
			// or the other end disconnected prematurely (WSAECONNRESET).
			// Neither case is an error that warrants an exception.
			return auto_pointer<network_connection>();
		}

		LOG_ERROR("Error while accepting established TCP/IP connection" +
				  ms_network_errors::format_error(error_code))

		throw network_exception();
	}

	SCOPE_GUARD(socket_guard, (&closesocket, s));


	// Fetch textual representation of client IP.
	// Note INET_ADDRSTRLEN includes a terminating \0.
	char remote_ip[INET_ADDRSTRLEN];

	if (!inet_ntop
			(AF_INET, &addr.sin_addr,
			 remote_ip, INET_ADDRSTRLEN))
	{
		LOG_ERROR("Failed to query IP string of remote end.");
		throw network_exception();
	}

	uint16 remote_port = 
		ntohs(addr.sin_port);
	

	// Accepted an established TCP/IP connection,
	// so wrap it into a network_connection.
	//
	// Exception safety:
	// - On any exception, the scope guard closes the socket.
	// - On success, the network_connection owns the socket,
	//		and we must immediately dismiss the scope guard.
	auto_pointer<network_connection> connection
		(new ms_network_connection
			(s, remote_ip, remote_port));

	socket_guard.dismiss();
	return connection;
}


ms_network_server::ms_network_server(uint16 local_port)
	:
	network_server(local_port),
	listen_socket_(INVALID_SOCKET)
{
	// Create socket to listen on.
	listen_socket_ =
		socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (listen_socket_ == INVALID_SOCKET)
	{
		LOG_ERROR("Failed to create socket" +
				  ms_network_errors::format_last_error())
		throw network_exception();
	}

	SCOPE_GUARD
		(socket_guard, (&closesocket, listen_socket_));


	// Enable our socket to reuse an orphaned port
	// without waiting for the TCP packet timeout.
	// This implies that we might receive garbage data
	// from an incoming connection at rare occasions.
	char reuse_address = 1;

	if (setsockopt
			(listen_socket_, SOL_SOCKET, SO_REUSEADDR,
			 &reuse_address, sizeof(reuse_address)) == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to set socket options" +
				  ms_network_errors::format_last_error())
		throw network_exception();
	}


	// Bind socket to local port.
	// Allow any (of potentially multiple) IP addresses.
	sockaddr_in addr;
	ZeroMemory(&addr, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_port = htons(local_port);
	addr.sin_addr.S_un.S_addr = INADDR_ANY;

	if (bind(listen_socket_,
			 reinterpret_cast<sockaddr*>(&addr),
			 sizeof(addr)) == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to bind listening socket" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}


	// Place socket in listening state.
	if (listen(listen_socket_, SOMAXCONN) == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to place socket into listen mode" +
				  ms_network_errors::format_last_error());
		throw network_exception();
	}


	// Set socket to nonblocking mode.
	u_long nonblocking_mode = 1;

	if (ioctlsocket(listen_socket_, FIONBIO, &nonblocking_mode) == SOCKET_ERROR)
	{
		LOG_ERROR("Failed to set non-blocking socket operation" +
				  ms_network_errors::format_last_error());
		throw api_call_failed();
	}


	// Success, NOTHROW from here.
	socket_guard.dismiss();
}




//////////////////////////////////////////////////////////////////////////
//
// ms_network_errors
//
//////////////////////////////////////////////////////////////////////////


string ms_network_errors::format_last_error()
	{ return format_error(WSAGetLastError()); }


string ms_network_errors::format_error(int error_code)
{
	const char* error_message = to_error_message(error_code);

	return string() + " with error message: " + error_message +
					  " and error code: " + error_code;
}


const char* ms_network_errors::to_error_message(int error_code)
{
	switch (error_code)
	{
		// WinSock initialization
		case WSASYSNOTREADY: return "Network subsystem not ready.";
		case WSAVERNOTSUPPORTED: return "WinSock version not supported.";
		case WSANOTINITIALISED: return "WinSock was not initialized.";


		// Generic assertion-like error codes.
		case WSA_INVALID_HANDLE: return "Invalid handle.";
		case WSA_NOT_ENOUGH_MEMORY: return "Out of memory.";
		case WSA_INVALID_PARAMETER: return "Invalid parameter.";
		case WSANO_RECOVERY: return "Non-recoverable error.";

		case WSAEACCES: return "Access denied by socket permissions.";
		case WSAEFAULT: return "Faulty pointer or buffer detected.";
		case WSAEINVAL: return "Invalid argument to WinSock call.";

		case WSAETOOMANYREFS: return "Too many references to kernel object.";
		case WSAEPROCLIM: return "Too many client processes.";
		case WSASYSCALLFAILURE: return "System call failed.";


		// Socket creation and binding.
		case WSAEMFILE: return "Too many open sockets.";
		case WSAEADDRINUSE: return "Address in use.";
		case WSAEADDRNOTAVAIL: return "Address not available.";


		// Protocol management.
		case WSAENOPROTOOPT: return "Protocol unknown.";
		case WSAEPROTONOSUPPORT: return "Protocol not implemented.";
		case WSAESOCKTNOSUPPORT: return "Socket type not supported.";
		case WSAEOPNOTSUPP: return "Socket operation not supported.";
		case WSAEPFNOSUPPORT: return "Protocol family not supported.";
		case WSAEAFNOSUPPORT: return "Address family not supported.";


		// Blocking operations.
		case WSAEWOULDBLOCK: return "Operation would block.";
		case WSAEINPROGRESS: return "Concurrent operation in progress.";
		case WSAEALREADY: return "Operation already in progress.";


		// Sending and receiving.
		case WSAENOBUFS: return "No buffer space available.";
		case WSAENOTCONN: return "Socket is not connected.";
		case WSAESHUTDOWN: return "Cannot send after socket shutdown.";
		case WSAEMSGSIZE: return "Datagram message too long.";


		// Network service, connection, and host.
		case WSAENETDOWN: return "Network is down.";
		case WSAENETUNREACH: return "Network is unreachable.";
		case WSAENETRESET: return "Network connection dropped on reset.";

		case WSAECONNABORTED: return "Connection aborted.";
		case WSAECONNRESET: return "Connection reset by peer.";
		case WSAETIMEDOUT: return "Connection timed out.";
		case WSAECONNREFUSED: return "Connection refused.";

		case WSAHOST_NOT_FOUND: return "Host not found.";
		case WSATRY_AGAIN: return "Temporary error in host resolution.";
		case WSAEHOSTDOWN: return "Host is down.";
		case WSAEHOSTUNREACH: return "Host is unreachable.";
	}


	return "No explicit error message.";
}




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_NETWORK) */
