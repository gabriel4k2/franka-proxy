/**
 *************************************************************************
 *
 * @file network_stream.hpp
 *
 * Buffered network streams.
 *
 ************************************************************************/


#if !defined(INCLUDED__VIRAL_CORE__NETWORK_STREAM_HPP)
#define INCLUDED__VIRAL_CORE__NETWORK_STREAM_HPP


#include "build_config.hpp"


#include "network_forward.hpp"


#include "util.hpp"
#include "deque_buffer.hpp"
#include "auto_pointer.hpp"
#include "thread_synch.hpp"


namespace viral_core
{


/**
 *************************************************************************
 *
 * @class network_stream
 * @ingroup network_module
 *
 * Provides custom deque_buffer%ing over a network_connection.
 *
 * Usage notes:
 * - A network_stream stores incoming and outgoing data in two buffers.
 *		This enables client threads to send or receive without blocking
 *		and without explicitly tracking the current transfer progress.
 *		Finally, custom buffering circumvents limits to OS-side buffers
 *		in naive network_connection%s.
 * - Clients must regularly call update() to actually send buffered data
 *		and to receive new data into the receiving buffer. A network_task
 *		can automate these update()s on multiple network_stream%s.
 * - For one-shot transfers, explicitly blocking or segmented operations
 *		through network_transfer may be a better alternative.
 * - Clients may use the network_serialize utility to wrap basic types
 *		into a network_stream.
 * - @attent Clients must not use the network_connection separately
 *		after stream wrapping to avoid internal race conditions.
 *
 * Error handling:
 * - On any exceptions (e.g. network_connection exceptions in update()
 *		or buffer overflow in send_nonblocking()), transfer consistency
 *		cannot be guaranteed and the network_connection might be dead.
 *		In this case, network_stream raises had_exception() and throws
 *		on future update(), send_nonblocking() and receive_nonblocking().
 * - @attent Due to asynchronous transfers, some errors only register
 *		at a later update() call (e.g. failed network_connection%s).
 * - @attent If clients regularly call update() (e.g. via a network_task)
 *		without removing pending data (e.g. by try_receive_nonblocking()),
 *		then update()s may fail due to overflow on the receiving buffer.
 *
 * Thread safety:
 * - A network_stream is threadsafe for arbitrary threads.
 * - When receiving on a network_stream, be aware that default locking
 *		occurs per call. Thus, multiple calls may cause race conditions
 *		(e.g. try_receive_nonblocking() for all pending_receive_bytes()
 *		may fail under multi-threaded receiving on a single stream).
 * - To perform multiple calls as one thread-safe and atomic operation,
 *		clients must explicitly use a network_stream_lock.
 *
 ************************************************************************/
class network_stream :
	private non_copyable
{
	friend class network_stream_lock;


	public:

		/**
		 * Create stream either with default buffer settings
		 * or with custom settings (e.g. for large transfers
		 * or for infrequent update()).
		 */
		//@{
		network_stream
			(auto_pointer<network_connection> connection);

		network_stream
			(auto_pointer<network_connection> connection,
			 int64 buffer_block_size_send,
			 int64 buffer_max_size_send,
			 int64 buffer_block_size_receive,
			 int64 buffer_max_size_receive);
		//@}


		~network_stream() NOTHROW;


		/**
		 * Asynchronously send @bref{size} bytes of @bref{data}.
		 * As all @bref{data} is copied, clients can immediately
		 * reuse the data array after send_nonblocking() returns.
		 * The call throws on any failure (e.g. buffer overflow).
		 * In this case, @bref{data} is not copied.
		 */
		void send_nonblocking
			(const unsigned char* data, int64 size);


		/**
		 * If there are at least @bref{size} pending_receive_bytes(),
		 * fetch the @bref{size} bytes to @bref{data} and return @true.
		 * Otherwise, return @false and do not change @bref{data}.
		 *
		 * By default, this call removes all resulting @bref{data}
		 * from the receiving deque_buffer of the stream. Optionally,
		 * clients can @bref{peek} without removing buffered data.
		 *
		 * The call throws on any failure (e.g. if had_exception()).
		 * Neither @bref{data} nor the receiving buffer change then.
		 * Receiving more than @bref{buffer_max_size_receive} bytes
		 * throws as well, because this request can never succeed.
		 */	
		bool try_receive_nonblocking
			(unsigned char* data, int64 size, bool peek);


		/**
		 * Fetch @bref{size} bytes at @bref{offset} in receiving buffer
		 * to @bref{data} if there are sufficient pending_receive_bytes().
		 * Otherwise, return @false and do not change @bref{data}.
		 *
		 * This call does not allow to remove resulting @bref{data},
		 * as the backing deque_buffer can only remove at either end.
		 * 
		 * Peeking throws on any failure (e.g. if had_exception()),
		 * in which case @bref{data} remains unchanged.
		 *
		 * Notably, peeking throws if @bref{size} plus @bref{offset}
		 * exceeds @bref{buffer_max_size_receive}, because this request
		 * can never succeed.
		 */
		bool try_peek_nonblocking
			(unsigned char* data, int64 size, int64 offset);


		/**
		 * Discard at most @bref{size} bytes of pending data
		 * from the receiving buffer. Any errors are logged and
		 * ignored for @ref NOTHROW.
		 *
		 * @attent When discarding more bytes than in the buffer,
		 * the remainder does not carry over to future transfers.
		 * Thus, do not discard more than pending_receive_bytes().
		 */
		void discard_receive(int64 size) NOTHROW;


		/**
		 * Add pending data from network_connection to receiving buffer
		 * and send pending data to network_connection.
		 *
		 * Throws on errors (e.g. connection failure or buffer overflow).
		 * In this case, partial data may already have been transferred.
		 */
		void update();


		/**
		 * Whether an exception occurred on preceding transfers.
		 * As explained in the network_stream class documentation,
		 * this causes certain future calls to fail as well.
		 */
		bool had_exception() const NOTHROW;


		/** Fetch current deque_buffer states. */
		//@{
		int64 pending_send_bytes() const NOTHROW;
		int64 pending_receive_bytes() const NOTHROW;
		//@}


		/** Clear deque_buffer%s. */
		//@{
		void clear_send() NOTHROW;
		void clear_receive() NOTHROW;
		//@}


		/**
		 * Default maximum of bytes per network_stream buffer
		 * and bytes per block of the backing deque_buffer.
		 */
		//@{
		static const int64 buffer_block_size_default;
		static const int64 buffer_max_size_default;
		//@}


		/** Construction-provided buffer limits. */
		//@{
		const int64 buffer_max_size_send;
		const int64 buffer_max_size_receive;
		//@}


	private:

		void validate();


		mutable mutex stream_mutex_;

		auto_pointer<network_connection> connection_;

		deque_buffer<unsigned char> send_buffer_;
		deque_buffer<unsigned char> receive_buffer_;

		bool had_exception_;
};




/**
 *************************************************************************
 *
 * @class network_stream_lock
 * @ingroup network_module
 *
 * Locks a network_stream to the current thread over multiple calls.
 *
 * Usage notes:
 * - A network_stream_lock entirely locks the provided network_stream.
 *		Notably, network_stream::update() will then block other threads
 *		(e.g. a network_task) while the stream is locked. Clients thus
 *		should not keep network_stream_lock%s for extended durations.
 * - See thread safety notes on network_stream for a use case.
 *
 ************************************************************************/
class network_stream_lock :
	private non_copyable
{

	public:

		network_stream_lock(const network_stream& stream) NOTHROW;
		~network_stream_lock() NOTHROW;


	private:

		const network_stream& stream_;
};




} /* namespace viral_core */


#endif /* !defined(INCLUDED__VIRAL_CORE__NETWORK_STREAM_HPP) */
