/**
 *************************************************************************
 *
 * @file network_stream.cpp
 *
 * Buffered network streams, implementation.
 *
 ************************************************************************/


#include "network_stream.hpp"


#include "log.hpp"
#include "scope_guard.hpp"
#include "integer_checks.hpp"
#include "network.hpp"


namespace viral_core
{


//////////////////////////////////////////////////////////////////////////
//
// network_stream
//
//////////////////////////////////////////////////////////////////////////


network_stream::network_stream
	(auto_pointer<network_connection> connection)
	:
	buffer_max_size_send(buffer_max_size_default),
	buffer_max_size_receive(buffer_max_size_default),
	connection_(std::move(connection)),
	send_buffer_(buffer_block_size_default),
	receive_buffer_(buffer_block_size_default),
	had_exception_(false)
{
	validate();
}


network_stream::network_stream
	(auto_pointer<network_connection> connection,
	 int64 buffer_block_size_send,
	 int64 buffer_max_size_send,
	 int64 buffer_block_size_receive,
	 int64 buffer_max_size_receive)
	:
	buffer_max_size_send(buffer_max_size_send),
	buffer_max_size_receive(buffer_max_size_receive),
	connection_(std::move(connection)),
	send_buffer_(buffer_block_size_send),
	receive_buffer_(buffer_block_size_receive),
	had_exception_(false)
{
	validate();
}


network_stream::~network_stream() NOTHROW
{
	// Resolve member destructors to defined types here.
}


void network_stream::send_nonblocking
	(const unsigned char* data, int64 size)
{
	MUTEX_SCOPE(stream_mutex_);


	// Handle exception flag.	
	if (had_exception_)
	{
		LOG_ERROR("Failure in preceding call. "
				  "No further transfers possible.");

		throw invalid_state();
	}

	SCOPE_GUARD
		(had_exception_guard,
			(&guard_assign<bool>,
			 guard_by_reference(had_exception_), true));


	// Copy data to sending buffer.
	// Note deque_buffer::push_back()
	// is strongly exception safe.
	if (size < 0 || (size > 0 && !data))
	{
		LOG_ERROR("Invalid size or pointer for sent data.");
		throw invalid_parameter();
	}

	if (!integer_checks::is_add_safe(size, send_buffer_.size()))
	{
		LOG_ERROR("Integer overflow on sending buffer.");
		throw integer_overflow();
	}

	if (size + send_buffer_.size() > buffer_max_size_send)
	{
		LOG_ERROR("Data overflow on sending buffer.");
		throw bounds_exceeded();
	}

	send_buffer_.push_back(data, size);


	// Success.
	// Exception safety: NOTHROW from here.
	had_exception_guard.dismiss();
}


bool network_stream::try_receive_nonblocking
	(unsigned char* data, int64 size, bool peek)
{
	MUTEX_SCOPE(stream_mutex_);

	// Exception safety:
	// This method does not set had_exception_,
	// and thus must not throw on its own.

	if (!try_peek_nonblocking(data, size, 0))
		return false;

	// Success.
	// Exception safety: NOTHROW from here.
	if (!peek)
		receive_buffer_.pop_front(size);

	return true;
}


bool network_stream::try_peek_nonblocking
	(unsigned char* data, int64 size, int64 offset)
{
	MUTEX_SCOPE(stream_mutex_);


	// Handle exception flag.	
	if (had_exception_)
	{
		LOG_ERROR("Failure in preceding call. "
				  "No further transfers possible.");

		throw invalid_state();
	}

	SCOPE_GUARD
		(had_exception_guard,
			(&guard_assign<bool>,
			 guard_by_reference(had_exception_), true));


	// Copy data from receiving buffer.
	// Note deque_buffer::fetch_front()
	// is strongly exception safe.
	if (size < 0 || (size > 0 && !data))
	{
		LOG_ERROR("Invalid size or pointer for receiving data.");
		throw invalid_parameter();
	}

	if (offset < 0)
	{
		LOG_ERROR("Invalid offset for peeking at data.");
		throw invalid_parameter();
	}

	if (!integer_checks::is_add_safe(size, offset))
	{
		LOG_ERROR("Integer overflow on peeking at offset data.");
		throw integer_overflow();
	}

	if (size + offset > buffer_max_size_receive)
	{
		LOG_ERROR("Requested more data than fits into receiving buffer.");
		throw bounds_exceeded();
	}

	if (receive_buffer_.size() < size + offset)
	{
		// Buffer underrun is not a failure.
		// Exception safety: NOTHROW in here.
		had_exception_guard.dismiss();
		return false;
	}

	receive_buffer_.fetch_front
		(data, size, offset);


	// Success.
	// Exception safety: NOTHROW from here.
	had_exception_guard.dismiss();
	return true;
}


void network_stream::discard_receive(int64 size) NOTHROW
{
	MUTEX_SCOPE(stream_mutex_);
	receive_buffer_.pop_front(size);
}


void network_stream::update()
{
	MUTEX_SCOPE(stream_mutex_);


	// Handle exception flag.	
	if (had_exception_)
	{
		LOG_ERROR("Failure in preceding call. "
				  "No further transfers possible.");

		throw invalid_state();
	}

	SCOPE_GUARD
		(had_exception_guard,
			(&guard_assign<bool>,
			 guard_by_reference(had_exception_), true));


	// Send as much data as the OS-side buffer takes.
	while (!send_buffer_.empty())
	{
		int64 sent_bytes =
			connection_->send_nonblocking
				(send_buffer_.data_first_block(),
				 send_buffer_.size_first_block());


		// Possibly success.
		// Otherwise pop any sent bytes.
		// Exception safety: NOTHROW.
		if (sent_bytes == 0) break;
		send_buffer_.pop_front(sent_bytes);
	}


	// Drain any pending data from OS-side buffers.
	// We cannot reliably determine pending data size
	// (e.g. when there are concurrent receive calls),
	// so we preallocate on block boundaries and then
	// shrink down to actually received data.
	while (true)
	{
		int64 unused_at_end =
			receive_buffer_.unused_at_end();

		int64 insert_bytes =
			unused_at_end > 0 ?
				unused_at_end : receive_buffer_.block_size;
		int64 insert_offset =
			unused_at_end > 0 ?
				receive_buffer_.size_last_block() : 0;

		insert_bytes =
			min(insert_bytes, 
				buffer_max_size_receive - receive_buffer_.size());

		if (insert_bytes == 0)
		{
			LOG_ERROR("Data overflow on receiving buffer.");
			throw bounds_exceeded();
		}

		if (!integer_checks::is_add_safe
				(receive_buffer_.size(), insert_bytes))
		{
			LOG_ERROR("Integer overflow on receiving buffer.");
			throw integer_overflow();
		}


		// Resize with guess for next boundary,
		// then fill with data from OS-side buffers.
		receive_buffer_.resize_back
			(receive_buffer_.size() + insert_bytes);

		SCOPE_GUARD
			(receive_buffer_guard,
				(receive_buffer_,
				 &deque_buffer<unsigned char>::pop_back,
				 insert_bytes));
			
		int64 received_bytes =
			connection_->receive_nonblocking
				(receive_buffer_.data_last_block() + insert_offset,
				 insert_bytes);
		

		// Remove surplus bytes, then success.
		// Exception safety: NOTHROW.
		receive_buffer_guard.dismiss();
		receive_buffer_.pop_back
			(insert_bytes - received_bytes);

		if (received_bytes == 0) break;
	}


	// Success.
	// Exception safety: NOTHROW from here.
	had_exception_guard.dismiss();
}


bool network_stream::had_exception() const NOTHROW
{
	MUTEX_SCOPE(stream_mutex_);
	return had_exception_;
}


int64 network_stream::pending_send_bytes() const NOTHROW
{
	MUTEX_SCOPE(stream_mutex_);
	return send_buffer_.size();
}

int64 network_stream::pending_receive_bytes() const NOTHROW
{
	MUTEX_SCOPE(stream_mutex_);
	return receive_buffer_.size();
}


void network_stream::clear_send() NOTHROW
{
	MUTEX_SCOPE(stream_mutex_);
	send_buffer_.clear();
}

void network_stream::clear_receive() NOTHROW
{
	MUTEX_SCOPE(stream_mutex_);
	receive_buffer_.clear();
}


void network_stream::validate()
{
	if (!connection_)
	{
		LOG_ERROR("Cannot create stream over NULL connection.");
		throw null_pointer();
	}

	if (buffer_max_size_send <= 0 ||
		buffer_max_size_receive <= 0 ||
		send_buffer_.block_size <= 0 ||		
		receive_buffer_.block_size <= 0)
	{
		LOG_ERROR("Buffer sizes must be greater than zero.");
		throw invalid_parameter();
	}

	if (send_buffer_.block_size > buffer_max_size_send ||
		receive_buffer_.block_size > buffer_max_size_receive)
	{
		LOG_ERROR("Buffer block size must not exceed maximum size.");
		throw invalid_parameter();
	}

	if (buffer_max_size_send > container_limits<unsigned char>::max_size ||
		buffer_max_size_receive > container_limits<unsigned char>::max_size)
	{
		LOG_ERROR("Maximum buffer size must not exceed overall maximum size.");
		throw invalid_parameter();
	}
}


const int64 network_stream::buffer_block_size_default = 16384;
const int64 network_stream::buffer_max_size_default = 1048576;




//////////////////////////////////////////////////////////////////////////
//
// network_stream_lock
//
//////////////////////////////////////////////////////////////////////////


network_stream_lock::network_stream_lock
	(const network_stream& stream) NOTHROW
		: stream_(stream)
{
	stream_.stream_mutex_.acquire();
}


network_stream_lock::~network_stream_lock() NOTHROW
	{ stream_.stream_mutex_.release(); }




} /* namespace viral_core */
