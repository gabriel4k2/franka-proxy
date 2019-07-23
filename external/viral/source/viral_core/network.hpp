/**
 *************************************************************************
 *
 * @file network.hpp
 *
 * Low-level TCP/IP networking.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_HPP


#include "build_config.hpp"


#include "network_forward.hpp"


#include "util.hpp"
#include "string.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @defgroup network_module Network Module
 *
 * Enables low-level communication over a TCP/IP network.
 *
 * Networking overview:
 * - Clients use network_context, network_connection, and network_server
 *		to establish connections to remote computers.
 * - Clients can directly use network_connection%s for low-level networking
 *		(e.g. for byte-wise sending and receiving) with minimal setup effort,
 *		but must then manually track the progress of transfers.
 * - Clients employ network_transfer to realize more high-level transfers
 *		over existing network_connection%s, including blocking operations
 *		and semi-automatic progress tracking.
 * - Clients can wrap network_connection%s into network_stream%s to send
 *		and receive data without blocking or explicit progress tracking.
 *		A network_task automates necessary updates of network_stream%s.
 *
 ************************************************************************/




/**
 *************************************************************************
 *
 * @class network_context
 * @ingroup network_module
 *
 * Creates network_connection%s and network_server%s.
 *
 * Socket API initialization and shutdown:
 * - If required, a network_context initializes and terminates
 *		the OS-side socket API (e.g. WSAStartup(), WSACleanup()).
 * - Multiple network_context instances are explicitly allowed
 *		and manage the OS-side API through reference counting.
 * - The last network_context shuts down the OS-side socket API
 *		on destruction, which may cause network failure on any
 *		remaining network_connection%s and network_server%s.
 *		Although this is not fatal, applications should destroy
 *		all connections and servers before any network_context.
 *
 * Timeout notes:
 * - Calls to both create_connection() and try_create_connection()
 *		exit after an implementation-specific timeout has passed.
 *		The former throws on timeout, the latter returns @NULL.
 * - Clients can explicitly specify a @bref{timeout_hint_seconds}
 *		to apply instead of the implementation-specific timeout.
 *		This hint may either be shorter or longer than the default,
 *		and need only be approximately observed.
 *
 * Thread safety:
 * - A network_context is threadsafe for arbitrary threads.
 *
 ************************************************************************/
class network_context :
	private non_copyable
{

	public:

		virtual ~network_context() NOTHROW;


		/**
		 * Connect to a network_server at @bref{remote_ip}
		 * and return the resulting not-@NULL connection.
		 * Throws on failure (e.g. no server at remote ip).
		 *
		 * See class documentation for timeout notes.
		 */
		//@{
		virtual auto_pointer<network_connection>
			create_connection
				(const string& remote_ip,
				 uint16 remote_port,
				 float timeout_hint_seconds) = 0;

		virtual auto_pointer<network_connection>
			create_connection
				(const string& remote_ip,
				 uint16 remote_port) = 0;
		//@}


		/**
		 * Connect to a network_server at @bref{remote_ip}
		 * and return a not-@NULL connection on success,
		 * or @NULL if no connection could be established.
		 * Throws on unrelated errors (e.g. out of sockets).
		 *
		 * Clients should prefer try_create_connection()
		 * for repeatedly connecting to an unreliable server
		 * to avoid exception handling and verbose log output.
		 *
		 * See class documentation for timeout notes.
		 */
		//@{
		virtual auto_pointer<network_connection>
			try_create_connection
				(const string& remote_ip,
				 uint16 remote_port,
				 float timeout_hint_seconds) = 0;
		
		virtual auto_pointer<network_connection>
			try_create_connection
				(const string& remote_ip,
				 uint16 remote_port) = 0;
		//@}


		/**
		 * Create a not-@NULL server on @bref{local_port}
		 * that listens for incoming network_connection%s.
		 * Throws on failure (e.g. port in use).
		 */
		virtual auto_pointer<network_server>
			create_server(uint16 local_port) = 0;


		/**
		 * Look up the ip address for some @bref{hostname},
		 * or return a hostname for an @bref{ip} address.
		 * Both calls throw if the lookup fails.
		 *
		 * @attent
		 * Note that both calls may exhibit OS-specific quirks.
		 * For example, an empty @bref{hostname} might throw
		 * or might return an external ip of the local machine.
		 */
		//@{
		virtual string ip_from_hostname(const string& hostname) = 0;
		virtual string hostname_from_ip(const string& ip) = 0;
		//@}


		const string context_name;


	protected:

		network_context(const string& context_name);
};




/**
 *************************************************************************
 *
 * @class network_connection
 * @ingroup network_module
 *
 * Sends and receives data through a TCP/IP stream.
 *
 * Usage notes:
 * - Applications use either a network_context or a network_server
 *		to create new network_connection%s.
 * - Although TCP/IP realizes in-order delivery and error checking,
 *		applications should still sanitize incoming transfers.
 * - Clients can employ either network_transfer or network_stream%s
 *		to perform advanced (e.g. progressive or blocking) transfers
 *		over a network_connection.
 *
 * Error handling:
 * - Both the send_nonblocking() and the receive_nonblocking() call
 *		indicate network failure (e.g. timeout or remote disconnect)
 *		by an exception (usually a network_exception). In this case,
 *		partial data may have been sent or received.
 * - After failure, the network_connection may remain in a dead state
 *		where all further network transfers fail.
 *
 * Thread safety:
 * - A network_connection is threadsafe for arbitrary threads.
 * - Concurrent transfers with per-call mutexing on a single socket 
 *		forego TCP/IP data order and thus are discouraged.
 *
 * Implementation notes:
 * - To prohibit dependencies on quirky OS-side data buffering,
 *		network_connection does not implement peek operations.
 *		Clients should instead use network_stream for peeking.
 *
 ************************************************************************/
class network_connection :
	private non_copyable
{

	public:

		virtual ~network_connection() NOTHROW;


		/** 
		 * Send at most @bref{size} bytes of not-@NULL @bref{data}
		 * over the network, but do not wait for the transfer to end.
		 * Returns the number of bytes actually sent to the network.
		 *
		 * See network_connection documentation for error handling.
		 */
		virtual int64 send_nonblocking
			(const unsigned char* data, int64 size) = 0;


		/** 
		 * Fetch a maximum of @bref{size} bytes pending network data
		 * and store this data to a not-@NULL @bref{data} pointer.
		 * Returns the number of bytes actually read from the network.
		 *
		 * Applications must ensure that the @bref{data} pointer
		 * has enough space for @bref{size} bytes of data.
		 *
		 * See network_connection documentation for error handling.
		 */
		virtual int64 receive_nonblocking
			(unsigned char* data, int64 size) = 0;


		/** Remote end of the connection. */
		//@{
		const string remote_ip;
		const uint16 remote_port;
		//@}


	protected:

		network_connection
			(const string& remote_ip,
			 uint16 remote_port);
};




/**
 *************************************************************************
 *
 * @class network_server
 * @ingroup network_module
 *
 * Wraps established TCP/IP connections into network_connection%s.
 *
 * Thread safety:
 * - A network_server is threadsafe for arbitrary threads.
 *
 ************************************************************************/
class network_server :
	private non_copyable
{

	public:

		/**
		 * Stop listening for incoming TCP/IP connections.
		 *
		 * @attent Any created network_connection%s remain open
		 * and are not destroyed or disconnected automatically.
		 */
		virtual ~network_server() NOTHROW;


		/**
		 * Wrap an established TCP/IP connection into a network_connection,
		 * or return @NULL if no unwrapped TCP/IP connections remain.
		 *
		 * Notes:
		 * - The implementation must be able to establish TCP/IP connections
		 *		even if an application does not call try_accept_connection().
		 *		Notably, network_context::create_connection() must succeed
		 *		even without a corresponding call to try_accept_connection()
		 *		if there is a network_server listening on the remote end.
		 *		Network APIs (e.g. WinSock) usually use concurrent handshakes
		 *		and implicitly realize this behaviour.
		 * - Established TCP/IP connections wait in queue for later accepting.
		 *		Regular try_accept_connection() calls avoid queue overflows,
		 *		which otherwise cause rejected TCP/IP connections. 
		 */
		virtual auto_pointer<network_connection>
			try_accept_connection() = 0;


		/** Port that the server listens on. */
		const uint16 local_port;


	protected:

		network_server(uint16 local_port);
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_HPP) */
