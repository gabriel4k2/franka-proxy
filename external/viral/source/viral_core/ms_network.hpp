/**
 *************************************************************************
 *
 * @file ms_network.hpp
 *
 * TCP/IP network on Microsoft Windows.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__MS_NETWORK_HPP)
#define INCLUDED__VIRAL_CORE__MS_NETWORK_HPP


#include "build_config.hpp"


#if defined(VIRAL__HAVE_MS_NETWORK)


#include "thread_synch.hpp"
#include "network.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class ms_network_context
 * @ingroup network_module
 *
 * A network_context that uses the WinSock API as backend.
 *
 * Implementation notes:
 * - The ms_network_context initializes and shuts down the WinSock API.
 *		Both operations are reference counted as well as threadsafe.
 *		In particular, reference counting enables multiple instances
 *		of ms_network_context without premature WinSock shutdown.
 * - Although WinSock is threadsafe, all network_context calls lock
 *		the @bref{network_mutex_} as an additional safety precaution.
 *		This locking is independent of locking on ms_network_server%s
 *		and ms_network_connection%s.
 *
 ************************************************************************/
class ms_network_context :
	public network_context
{
	public:

		ms_network_context
			(const string& context_name);

		~ms_network_context() NOTHROW override;


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


		/**
		 * Implementation-specific @bref{timeout_hint_seconds}
		 * for create_connection() and try_create_connection().
		 * We use this default instead of the WinSock timeout,
		 * as the WinSock timeout is unreasonably large.
		 */
		static const float default_timeout_hint_seconds;


	private:

		static const float create_connection_sleep_seconds_;

		mutable mutex network_mutex_;
};




/**
 *************************************************************************
 *
 * @class ms_network_connection
 * @ingroup network_module
 *
 * A network_connection that uses the WinSock API as backend.
 *
 ************************************************************************/
class ms_network_connection :
	public network_connection
{
	friend class ms_network_context;
	friend class ms_network_server;


	public:

		~ms_network_connection() NOTHROW override;


		int64 send_nonblocking
			(const unsigned char* data, int64 size) override;
		int64 receive_nonblocking
			(unsigned char* data, int64 size) override;


	private:

		/**
		 * Create a new network_connection over @bref{socket}.
		 * The new connection assumes ownership of the socket,
		 * sets the socket to a non-blocking transfer mode,
		 * and closes the socket on destruction.
		 *
		 * @attent If the constructor of the connection throws,
		 * callers must explicitly close the @bref{socket}.
		 */
		ms_network_connection
			(pointer_int socket,
			 const string& remote_ip,
			 uint16 remote_port);


		/**
		 * Maximum bytes processed per WinSock send() and recv() call.
		 * Avoids an integer overflow for int64 to int conversion.
		 */
		//@{
		static const int64 max_bytes_send_ = 1073741824;
		static const int64 max_bytes_receive_ = 1073741824;
		//@}


		mutable mutex connection_mutex_;
		pointer_int socket_;
};




/**
 *************************************************************************
 *
 * @class ms_network_server
 * @ingroup network_module
 *
 * A network_server that uses the WinSock API as backend.
 *
 ************************************************************************/
class ms_network_server :
	public network_server
{
	friend class ms_network_context;


	public:

		~ms_network_server() NOTHROW override;

		auto_pointer<network_connection>
			try_accept_connection() override;


	private:

		ms_network_server(uint16 local_port);

		mutable mutex server_mutex_;
		pointer_int listen_socket_;
};




/**
 *************************************************************************
 *
 * @class ms_network_errors
 * @ingroup network_module
 *
 * Internal error handling for the WinSock network backend.
 *
 * Implementation notes:
 * - Only user-relevant errors have explicit error messages.
 * - WSAGetLastError() works even if the WinSock API is not initialized.
 * - Logging macros consume any exceptions from message formatting.
 * - Error reporting calls are reentrant and thus threadsafe.
 *
 ************************************************************************/
class ms_network_errors :
	private non_constructable
{
	friend class ms_network_context;
	friend class ms_network_connection;
	friend class ms_network_server;


	private:

		static string format_last_error();
		static string format_error(int error_code);
		static const char* to_error_message(int error_code);
};




} /* namespace viral_core */


#endif /* defined(VIRAL__HAVE_MS_NETWORK) */


#endif /* !defined(INCLUDED__VIRAL_CORE__MS_NETWORK_HPP) */
