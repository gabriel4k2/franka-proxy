/**
 *************************************************************************
 *
 * @file network_transfer.hpp
 *
 * One-shot network transfers.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_TRANSFER_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_TRANSFER_HPP


#include "build_config.hpp"


#include "network_forward.hpp"


#include "util.hpp"


namespace viral_core
{


/**
 * A data buffer for network_transfer through a network_connection.
 * @ingroup network_module
 */
typedef dynamic_buffer<unsigned char> network_buffer;




/**
 *************************************************************************
 *
 * @class network_buffer_progress
 * @ingroup network_module
 *
 * Stores partial progress on the network_transfer of a network_buffer.
 *
 ************************************************************************/
class network_buffer_progress
{
	friend class network_transfer;


	public:

		/**
		 * Initialize progress to the start of a network @bref{buffer}.
		 * The @bref{buffer} must already have the correct size.
		 */
		network_buffer_progress
			(const network_buffer& buffer) NOTHROW;
	
		int64 next_byte_offset() const NOTHROW;
		int64 bytes_remaining() const NOTHROW;

		bool finished() const NOTHROW;


	private:

		int64 next_byte_offset_;
		int64 bytes_remaining_;
};




/**
 *************************************************************************
 *
 * @class network_transfer
 * @ingroup network_module
 *
 * Static utility for one-shot transfers over a network_connection.
 *
 * Usage notes:
 * - Network transfers are suitable for small, one-shot transfers
 *		in applications or threads that do not have any other tasks.
 *		In particular, this includes blocking (e.g. send_blocking())
 *		or segmented (e.g. send_partial_nonblocking()) transfers,
 *		possibly alongside checks for threaded_task::join_now().
 * - For intricate applications, network_stream and network_task
 *		offer a more verbose, but also more capable solution.
 *
 ************************************************************************/
class network_transfer :
	private non_constructable
{

	public:

		/**
		 * Transfer a @bref{buffer} object over a @bref{connection}
		 * and wait until the entire transfer has completed.
		 *
		 * Usage notes:
		 * - On receiving, the @bref{size} of the target @bref{buffer}
		 *		must be known in advance. The buffer automatically resizes
		 *		to given @bref{size}, then reads exactly that many bytes
		 *		from the network. On any exception, the buffer is cleared.
		 * - See network_connection documentation for error handling.
		 *
		 * Timeout and sleeping:
		 * - If @bref{enable_timeout} is @true, both transfer calls throw
		 *		after @bref{timeout_seconds} have passed without new data.
		 *		Otherwise, transfers run until all data has been received
		 *		or until the network_connection throws an exception.
		 * - If @bref{enable_sleep} is @true, both transfer calls sleep
		 *		for @bref{sleep_seconds} when there is no pending data.
		 *		Otherwise, transfers perform busy waiting. This enables
		 *		applications to balance between latency and CPU load.
		 * - Timeout and sleep are hints and need not be followed exactly.
		 *		For example, a connection may fail due to TCP/IP timeout
		 *		even if @bref{enable_timeout} is @false.
		 * - By default, transfers apply @bref{default_timeout_seconds}
		 *		and @bref{default_sleep_seconds}.
		 *
		 * Threading notes:
		 * - @attent Blocking loopback transfers (e.g. to 127.0.0.1)
		 *		on a single thread will deadlock each other.
		 * - @attent Concurrent blocking operations of the same type
		 *		on a single connection only lock for partial transfers
		 *		and thus may erroneously interleave.
		 */
		//@{
		static void send_blocking
			(network_connection& connection,
			 const network_buffer& buffer);
		static void receive_blocking
			(network_connection& connection,
			 network_buffer& buffer, int64 size);

		static void send_blocking
			(network_connection& connection,
			 const network_buffer& buffer,
			 bool enable_timeout, float timeout_seconds,
			 bool enable_sleep, float sleep_seconds);
		static void receive_blocking
			(network_connection& connection,
			 network_buffer& buffer, int64 size,
			 bool enable_timeout, float timeout_seconds,
			 bool enable_sleep, float sleep_seconds);
		//@}


		/**
		 * Transfer (or continue transferring) a partial @bref{buffer}
		 * while tracking the current @bref{progress} of the transfer.
		 *
		 * Usage notes:
		 * - In contrast to blocking variants, partial transfer variants
		 *		allow applications to perform other work while waiting
		 *		(e.g. to check for thread termination flags).
		 * - The @bref{buffer} must match the current @bref{progress}.
		 *		Notably, the size of the buffer needs to be the sum
		 *		of transferred and pending data in the progress.
		 * - See network_connection documentation for error handling.
		 */
		//@{
		static void send_partial_nonblocking
			(network_connection& connection,
			 const network_buffer& buffer,
			 network_buffer_progress& progress);
		static void receive_partial_nonblocking
			(network_connection& connection,
			 network_buffer& buffer,
			 network_buffer_progress& progress);
		//@}


		/** Defaults for send_blocking() and receive_blocking(). */
		//@{
		static const float default_timeout_seconds;
		static const float default_sleep_seconds;
		//@}
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_TRANSFER_HPP) */
